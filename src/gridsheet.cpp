/*
 * gridsheet.cpp - GtkGridSheet widget for Gtk+.
 *
 * Copyright 2021 Fatih Iskap
 * Author: Fatih Iskap <fiskap@protonmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include "utils.h"
#include "sqlite.h"
#include "gridsheet.h"
#include "window_gui.h"


static gint         get_sum_row_diffs (GGRID *grid, gint below)
{
below = grid->NUM_ROWS;
	gint diff = 0;

	for(auto x : grid->diff_row_heights)
		if(x.first < below)
			diff += x.second;

	return diff;
}

static gint         get_sum_col_widths (GGRID *grid)
{
	gint width = grid->widest_rowbtn + grid->margin_bottom;

	for(auto x : grid->column_widths)
		width += x;

	return width;
}

static void         delete_row (GGRID *grid, gint position)
{
	/// destroy all cells in row
	for(guint i = 0; i < grid->NUM_COLS; i++)
		gtk_widget_destroy(GTK_WIDGET(gtk_grid_get_child_at (GTK_GRID(grid->sheetgrid), i, position)));

	/// destroy row button
	gtk_widget_destroy(GTK_WIDGET(gtk_grid_get_child_at (GTK_GRID(grid->row_buttons_grid), 0, position)));

	/// remove from respective grids
	gtk_grid_remove_row(GTK_GRID(grid->sheetgrid), position);
	gtk_grid_remove_row(GTK_GRID(grid->row_buttons_grid), position);
}

static gboolean     window_configure_event (GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{

	GGRID     *grid;
	GtkLayout *layout;
	GtkWidget *hdrButtonsGrid;
	GtkWidget *rowButtonsGrid;

	grid = (GGRID *)user_data;
	layout = GTK_LAYOUT(grid->layout);
	hdrButtonsGrid = grid->column_buttons_grid;
	rowButtonsGrid = grid->row_buttons_grid;


	gtk_widget_show_all(GTK_WIDGET(window));
	gint window_total_height = gdk_window_get_height (gtk_widget_get_window(GTK_WIDGET(window)));

	grid->GRID_MAXRROWS = (window_total_height / grid->STDRROWHEIGHT) + grid->extra_rows;
	if((gint)grid->NUM_ROWS < grid->GRID_MAXRROWS )
		grid->GRID_MAXRROWS = grid->NUM_ROWS;
	grid->GRID_TOT_HEIGHT = (grid->NUM_ROWS * grid->STDRROWHEIGHT) + grid->margin_bottom + grid->STDRROWHEIGHT ;

	gtk_layout_set_size (layout, get_sum_col_widths(grid), grid->GRID_TOT_HEIGHT + get_sum_row_diffs(grid, grid->NUM_ROWS));
	gtk_widget_set_size_request (rowButtonsGrid, grid->widest_rowbtn, -1);
	gtk_widget_set_size_request (hdrButtonsGrid, get_sum_col_widths(grid), grid->STDRROWHEIGHT);

	return FALSE;

}



/// vertical scroll
static void         jump_scroll (GGRID *grid, gdouble scroll_y)
{
	/// delete all rows
	for (gint i = grid->rows.size()-1; i > -1; i--)
		{
			delete_row(grid, grid->rows.size() - 1);
			grid->rows.pop_back();
		}

	/// ascertain new starting position
	guint row_no =  (scroll_y / (grid->GRID_TOT_HEIGHT + get_sum_row_diffs(grid, grid->NUM_ROWS))) * grid->DATAS->result.size();
	if(row_no <= 5)
		{
			grid->last_row_index = 0;
			grid->first_row_index = 0;
			grid->new_margintop = 0;
		}
	else if(row_no > 5 && row_no <= grid->NUM_ROWS - grid->GRID_MAXRROWS)
		{
			grid->last_row_index = row_no - 5;
			grid->first_row_index = row_no - 5;

			grid->new_margintop = (row_no - 5) * grid->STDRROWHEIGHT ;
			grid->new_margintop += get_sum_row_diffs(grid, row_no - 5);
		}
	else
		{
			grid->last_row_index = grid->NUM_ROWS - grid->GRID_MAXRROWS ;
			grid->first_row_index = grid->NUM_ROWS - grid->GRID_MAXRROWS;

			grid->new_margintop =  grid->first_row_index * grid->STDRROWHEIGHT;
			grid->new_margintop += get_sum_row_diffs(grid, grid->NUM_ROWS - grid->GRID_MAXRROWS);
		}

	/// reposition
	gtk_layout_move ((GtkLayout *)grid->layout, grid->sheetgrid, 0, grid->new_margintop );
	gtk_layout_move ((GtkLayout *)grid->layout, grid->row_buttons_grid, grid->current_scroll_position_x, grid->new_margintop );

	/// add rows
	for(gint i = 0; i < grid->GRID_MAXRROWS; i++)
		{
			if( grid->last_row_index == (gint)grid->NUM_ROWS)
				return;

			grid->add_one_row_to_end(i);
		}


	GtkWidget *rButtonsGrid = grid->row_buttons_grid;
	GtkLayout *layout = GTK_LAYOUT(grid->layout);

	gint diff = get_sum_row_diffs (grid, grid->NUM_ROWS);
	gtk_widget_set_size_request (rButtonsGrid, grid->widest_rowbtn, grid->GRID_TOT_HEIGHT + diff);
	gtk_layout_set_size (layout, get_sum_col_widths(grid), grid->GRID_TOT_HEIGHT + diff);
}

static void         on_scroll_down (GGRID *grid, gdouble scroll_y)
{
		gint cutoff;
		gint rowbottom;

		cutoff = grid->STDRROWHEIGHT * (grid->extra_rows/2);
		rowbottom = 0;

		for(gint i = 0; i < 5; i++)
			{
				rowbottom += grid->rows[i]->height;
			}

		while(scroll_y - cutoff - grid->new_margintop > rowbottom)
			{
				if(grid->last_row_index == (gint)grid->NUM_ROWS) break;

				delete_row (grid, 0);
				grid->new_margintop += grid->rows[0]->height;

				grid->rows.pop_front();
				grid->first_row_index++;

				/// decrement
				rowbottom = 0;
				for(gint i = 0; i < 5; i++)
					rowbottom += grid->rows[i]->height;
			}

		while((gint)grid->rows.size() < grid->GRID_MAXRROWS)
			{
				/// add one row to bottom
				if(grid->last_row_index == (gint)grid->NUM_ROWS)
					break;

				grid->add_one_row_to_end (grid->rows.size());
			}

		gtk_layout_move ((GtkLayout *)grid->layout, grid->sheetgrid, 0, grid->new_margintop );
		gtk_layout_move ((GtkLayout *)grid->layout, grid->column_buttons_grid, 0, scroll_y);
		gtk_layout_move ((GtkLayout *)grid->layout, grid->row_buttons_grid, grid->current_scroll_position_x, grid->new_margintop);
}

static void         on_scroll_up (GGRID *grid, gdouble scroll_y, gdouble page_size)
{
		gint cutoff;
		gint rowtop;

		cutoff = scroll_y + page_size + grid->STDRROWHEIGHT * (grid->extra_rows/2);


		auto find_top_of_last_row = [&]() -> gint{
			rowtop = 0;
			for(guint i = 0; i < grid->rows.size() -1; i++)
				{
					rowtop += grid->rows[i]->height;
				}
			return rowtop;
		};


		rowtop = find_top_of_last_row();



		while( rowtop + grid->new_margintop > cutoff )
			{
				if( grid->last_row_index < grid->GRID_MAXRROWS ) break;

				/// remove one row at end
				gint position = grid->rows.size() - 1;
				delete_row( grid, position);

				grid->rows.pop_back();
				grid->last_row_index--;

				/// decrement
				rowtop = find_top_of_last_row();
			}

		/// add rows
		while((gint)grid->rows.size() < grid->GRID_MAXRROWS)
			{
				if (grid->first_row_index == 0)
					break;

				grid->add_one_row_at_front(0);
				grid->new_margintop -= grid->rows[0]->height;
			}

		gtk_layout_move ((GtkLayout *)grid->layout, grid->sheetgrid, 0, grid->new_margintop);
		gtk_layout_move ((GtkLayout *)grid->layout, grid->column_buttons_grid, 0, scroll_y);
		gtk_layout_move ((GtkLayout *)grid->layout, grid->row_buttons_grid, grid->current_scroll_position_x, grid->new_margintop);
}

static gboolean     on_explicit_vscrollbar_action (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{

	/// Explicitly on the scrollbar - runs just once, and runs first - emits a single value for click
	/// For drag and scroll, it emits a series of values

	/// useful to emit a changed value once instead of a zillion times (scroll or drag notwithstanding)

	gtk_range_set_value (range, value);

	return TRUE;
}

static void         on_vadjustment_value_changed (GtkAdjustment *adjustment, gpointer user_data)
{

	/// on scroll up or down
	/// NB: above 'on_explicit_vscrollbar_action' must be in place to prevent multiple scroll-event emissions

	GGRID *grid = (GGRID *)user_data;
	gdouble scroll_y = gtk_adjustment_get_value (adjustment);

	/// set y position of column buttons grid
	gtk_layout_move ((GtkLayout *)grid->layout, grid->column_buttons_grid, 0, scroll_y);

	/// If rows are out of view by more than top_edge or bottom_edge (jumped)
	gint top_edge = grid->new_margintop;
	gint bottom_edge = grid->new_margintop;
	for(guint i = 0; i < grid->rows.size(); i++)
		{
			bottom_edge += grid->rows[i]->height;
		}


	if(scroll_y > bottom_edge || scroll_y < top_edge)
		{
			grid->current_scroll_position_y = scroll_y;
			jump_scroll(grid, scroll_y);
			return;
		}

	/// scroll_down
	if(scroll_y > grid->current_scroll_position_y)
		{
			if(scroll_y >= gtk_adjustment_get_upper (adjustment) - gtk_adjustment_get_page_size (adjustment)) return;
			grid->current_scroll_position_y = scroll_y;
			on_scroll_down (grid, scroll_y);
			return;
		}

	/// scroll_up
	if(scroll_y < grid->current_scroll_position_y)
		{
			grid->current_scroll_position_y = scroll_y;
			gdouble page_size = gtk_adjustment_get_page_size (adjustment);
			on_scroll_up (grid, scroll_y, page_size);
			return;
		}



	return;
}

/// horizontal scroll
static gboolean     on_explicit_hscrollbar_action (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{

	/// Explicitly on the scrollbar - runs just once, and runs first - emits a single value for click
	/// For drag and scroll, it emits a series of values

	/// useful to emit a changed value once instead of a zillion times (scroll or drag notwithstanding)

	gtk_range_set_value (range, value);

	return FALSE;
}

static void         on_hadjustment_value_changed (GtkAdjustment *adjustment, gpointer user_data)
{

	GGRID *grid = (GGRID *)user_data;
	gdouble scroll_x = gtk_adjustment_get_value (adjustment);

	grid->current_scroll_position_x = scroll_x;

	gtk_layout_move ((GtkLayout *)grid->layout, grid->row_buttons_grid, scroll_x, grid->new_margintop );
}



/// button actions - col
static gboolean     col_button_motion (GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{

	/// Drag resize col width.

	GGRID *grid = (GGRID *)user_data;

	/// traverse glist to manually find which col button was clicked
	GList *gl = gtk_container_get_children (GTK_CONTAINER (grid->column_buttons_grid));
	gl = g_list_reverse (gl);
	gint col = g_list_index (gl, (gconstpointer) widget);


	gint width = gdk_window_get_width (event->window);
	GdkModifierType state;
	if(event->x > width - 10  || (gdk_event_get_state ((GdkEvent*)event, &state) && state == GDK_BUTTON1_MASK) )
		{
			/// Set cursor to resize-handle.
			gdk_window_set_cursor (event->window, gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW));

			/// do resize
			if (gdk_event_get_state ((GdkEvent*)event, &state) && state == GDK_BUTTON1_MASK)
				{
					if(event->x >= grid->column_natural_widths[col])
						{
							/// col button resize
							gtk_widget_set_size_request(widget, event->x, grid->STDRROWHEIGHT);

							/// set width of all cells in column
							for(auto row : grid->rows)
								gtk_widget_set_size_request(row->rowcells[col], event->x, -1);

							/// update column_widths vector
							grid->column_widths[col] = event->x;

							grid->COL_widths[col] = event->x;
						}
				}

			if(grid->sum_colwidths != get_sum_col_widths(grid))
				{
					/// a column has been resized, so resize the grid and colbuttons_grid widths accordingly
					gint newwidth = get_sum_col_widths(grid);
					grid->sum_colwidths = newwidth;
					gtk_layout_set_size (GTK_LAYOUT(grid->layout), newwidth, grid->GRID_TOT_HEIGHT);
					gtk_widget_set_size_request(grid->column_buttons_grid, newwidth, grid->STDRROWHEIGHT);
				}

		}
	else
		{
			gdk_window_set_cursor (event->window, NULL);
		}

	return FALSE;
}

static gboolean     on_col_button_press (GtkWidget *widget, GdkEventButton *event, gpointer user_data){

	/// on press col button, that column is sorted ascending or descending


	GGRID *grid = (GGRID *)user_data;

	/// Resize edge ?
	gint width = gdk_window_get_width (event->window);
	if(event->x > width - 10)
		return FALSE;

	/// traverse glist to manually find which col button was clicked
	GList *gl = gtk_container_get_children (GTK_CONTAINER (grid->column_buttons_grid));
	gl = g_list_reverse (gl);
	gint col = g_list_index (gl, (gconstpointer) widget);

	/// Lamda sorter functions
	auto sorterASC = [&](const sqliteROW &a, const sqliteROW &b) {

		/// both 0
		if(a[col] == "0"  && b[col] == "0")
			return (a[col] < b[col]);

		/// determine if passed in string, all of it, *can* be converted to a number
		gchar *posA = NULL;
		gchar *posB = NULL;
		if(   ((a[col] != "0"  &&  std::strtol(a[col].c_str(), &posA, 10) == 0)  ||  g_strcmp0(posA, "") != 0 )  ||
		      ((b[col] != "0"  &&  std::strtol(b[col].c_str(), &posB, 10) == 0)  ||  g_strcmp0(posB, "") != 0 )  ){

			/// trying to catch decimal numbers
			gdouble aval = g_ascii_strtod(a[col].c_str(), &posA);
			gdouble bval = g_ascii_strtod(b[col].c_str(), &posB);
			if((posA != NULL && posB != NULL)  && (strlen(posA) == 0 && strlen(posB) == 0))
				return aval < bval;

			/// not completely numeric
			return (a[col] < b[col]);
		}
		else {
			/// integer numeric
			return (std::stoi(a[col]) < std::stoi(b[col]));
		}
	};

	auto sorterDESC = [&](const sqliteROW &a, const sqliteROW &b) {

		/// both 0
		if(a[col] == "0"  && b[col] == "0")
			return (a[col] > b[col]);

		/// determine if passed in string, all of it, *can* be converted to a number
		gchar *posA = NULL;
		gchar *posB = NULL;
		if(   ((a[col] != "0"  &&  std::strtol(a[col].c_str(), &posA, 10) == 0)  ||  g_strcmp0(posA, "") != 0 )  ||
		      ((b[col] != "0"  &&  std::strtol(b[col].c_str(), &posB, 10) == 0)  ||  g_strcmp0(posB, "") != 0 )  ){

			/// trying to catch decimal numbers
			gdouble aval = g_ascii_strtod(a[col].c_str(), &posA);
			gdouble bval = g_ascii_strtod(b[col].c_str(), &posB);
			if((posA != NULL && posB != NULL)  && (strlen(posA) == 0 && strlen(posB) == 0))
				return aval > bval;

			/// not completely numeric
			return (a[col] > b[col]);
		}
		else {
			/// integer numeric
			return (std::stoi(a[col]) > std::stoi(b[col]));
		}
	};




	/// what is current sort
	if(grid->sorted.first == -1){
		std::stable_sort(grid->DATAS->result.begin(), grid->DATAS->result.end(), sorterASC);
		grid->sorted.first = col;
		grid->sorted.second = SORT_UP;
	}
	else {
		if(col == grid->sorted.first){
			if(grid->sorted.second == SORT_UN){
				std::stable_sort(grid->DATAS->result.begin(), grid->DATAS->result.end(), sorterASC);
				grid->sorted.second = SORT_UP;
			} else if (grid->sorted.second == SORT_UP){
				std::stable_sort(grid->DATAS->result.begin(), grid->DATAS->result.end(), sorterDESC);
				grid->sorted.second = SORT_DOWN;
			} else {
				std::stable_sort(grid->DATAS->result.begin(), grid->DATAS->result.end(), sorterASC);
				grid->sorted.second = SORT_UP;
			}
		}
		else {
			std::stable_sort(grid->DATAS->result.begin(), grid->DATAS->result.end(), sorterASC);
			grid->sorted.first = col;
			grid->sorted.second = SORT_UP;
		}
	}




	/// delete all rows
	for (gint i = grid->rows.size()-1; i > -1; i--){
		delete_row(grid, grid->rows.size() - 1);
		grid->rows.pop_back();
	}


	/// reset to 0
	grid->last_row_index = 0;
	grid->first_row_index = 0;
	grid->new_margintop = 0;


	/// FIXME: needs fix - sorting, when combined with row resizing buggers up the grid height...
	grid->GRID_TOT_HEIGHT = (grid->NUM_ROWS * grid->STDRROWHEIGHT) + grid->margin_bottom + grid->STDRROWHEIGHT;


	/// reposition
	gtk_layout_move ((GtkLayout *)grid->layout, grid->sheetgrid, 0, grid->new_margintop );
	gtk_layout_move ((GtkLayout *)grid->layout, grid->row_buttons_grid, grid->current_scroll_position_x, grid->new_margintop );


	/// add rows
	for(gint i = 0; i < grid->GRID_MAXRROWS; i++)
		if( grid->last_row_index == (gint)grid->NUM_ROWS)
			return FALSE;
		else
			grid->add_one_row_to_end(i);


	/// reset scrollbar position to top
	GtkWidget * vsw = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(grid->grid_scrollwin));
	gtk_range_set_value (GTK_RANGE(vsw), grid->new_margintop);


	return FALSE;
}


/// button actions - row
static gboolean     row_button_motion (GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
	/// Drag resize row height

	RROW *row = (RROW *)user_data;
	GGRID *grid = row->grid;
	//gint row_no = row->id;

	GdkModifierType state;
	gint height = gdk_window_get_height (event->window);

	if(event->y > height - 10  || (gdk_event_get_state ((GdkEvent*)event, &state) && state == GDK_BUTTON1_MASK) ){

		/// Set cursor to resize-handle.
		gdk_window_set_cursor (event->window, gdk_cursor_new (GDK_SB_V_DOUBLE_ARROW));

		/// resize the button and grid row cells
		if (gdk_event_get_state ((GdkEvent*)event, &state) && state == GDK_BUTTON1_MASK){

			if(event->y >= row->original_height){

				/// setting button height
				gtk_widget_set_size_request(widget, -1, event->y);

				/// setting height for all cells in grid row
				for(guint i = 0; i < grid->NUM_COLS; i++){
					GtkWidget *cell0 = row->rowcells[i];
					gtk_widget_set_size_request(cell0, -1, -1);
					gtk_widget_set_size_request(cell0, -1, event->y);
				}

				/// on resize row add diff for row to std::map
				/// NB: not changing row->height, leave that as standard height to minimise how much can smallen row height
				if(event->y != grid->STDRROWHEIGHT) {
					gint diff = event->y - grid->STDRROWHEIGHT;
					grid->diff_row_heights[row->row_original_index] = diff;
					row->height = event->y;
				}
			}
		}

		/// if there has been a change in height
		if(row->height != height){
			/// adjust grid total height according to new changed height of row
			gint diff = get_sum_row_diffs(grid, grid->NUM_ROWS);
			gtk_widget_set_size_request(grid->row_buttons_grid, grid->widest_rowbtn, grid->GRID_TOT_HEIGHT + diff);
			gtk_layout_set_size (GTK_LAYOUT(grid->layout), get_sum_col_widths(grid), grid->GRID_TOT_HEIGHT + diff);
		}

	}
	else {
		/// on button release, unset the resize cursor back to normal cursor
		gdk_window_set_cursor (event->window, NULL);
	}

	return FALSE;
}

static gboolean     on_row_button_press (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	/// Resize edge ?
	gint height = gdk_window_get_height (event->window);
	if(event->y > height - 10)
		return FALSE;

	RROW *row = (RROW *)user_data;
	gint row_no = row->id;

	std::cout << "button press row: " << row->id << "    "<< row_no    << std::endl;

	return FALSE;
}



void                GGRID::add_one_row_to_end (gint position)
{
		RROW *r = new RROW((gpointer)this, this->last_row_index);
		r->row_original_index = std::stoi(this->DATAS->result[this->last_row_index][this->NUM_COLS]);
		this->rows.push_back(r);

		gtk_grid_insert_row (GTK_GRID(this->sheetgrid), position);

		/// row button
		{
			add_css_class_to_widget(r->rowbutton, "rowHeader");
			gtk_grid_insert_row (GTK_GRID(this->row_buttons_grid), position);
			gtk_grid_attach (GTK_GRID(this->row_buttons_grid), r->rowbutton, 0, position, 1, 1);
			gtk_widget_add_events(r->rowbutton, GDK_POINTER_MOTION_MASK);
			g_signal_connect(r->rowbutton, "button-press-event", G_CALLBACK(on_row_button_press), (gpointer)r);
			g_signal_connect(r->rowbutton, "motion-notify-event", G_CALLBACK(row_button_motion), (gpointer)r);
		}


		/// row cells
		gint tallest = this->STDRROWHEIGHT;
		gint nat_height, nat_width;
		for(guint j = 0; j < this->NUM_COLS; j++)
			{
				gint widest;

				/// put max width of all cells per col into rowcells vector
				if(r->rowcells.size() < j +1)
					r->rowcells.push_back(NULL);

				/// start width to compare against is col button width
				widest = this->COL_widths[j];

				GtkWidget *cell = gtk_label_new (this->DATAS->result[this->last_row_index][j].c_str());
				gtk_label_set_selectable (GTK_LABEL(cell), TRUE);
				add_css_class_to_widget(cell, "gridcell");
				gtk_widget_show(cell);
				gtk_grid_attach (GTK_GRID(this->sheetgrid), cell, j, position, 1, 1);
				r->rowcells[j] = cell;

				/// left align label content  // deprecated from 3.16, use gtk_label_set_xalign (GTK_LABEL(cell), 0.0)
				gtk_misc_set_alignment (GTK_MISC (cell), 0.0, 0.5);

				/// wrap long lines
				if(DATAS->result[position][j].size() > 50)
					{
						gtk_label_set_max_width_chars (GTK_LABEL(cell), 55);
						gtk_label_set_line_wrap (GTK_LABEL(cell), TRUE);
					}

				/// get width and height of each cell
				gtk_widget_get_preferred_height (cell, NULL, &nat_height);
				gtk_widget_get_preferred_width (cell, NULL, &nat_width);

				/// has an irregular height (just the diff) been recorded for this row
		    auto search = this->diff_row_heights.find(r->row_original_index);
		    if (search != this->diff_row_heights.end())
					nat_height = this->STDRROWHEIGHT + search->second;

				r->original_height = nat_height - search->second;

				tallest = (nat_height > tallest) ? nat_height : tallest;
				widest = (nat_width > widest) ? nat_width : widest;

				if(nat_width > this->column_natural_widths[j])
					{
						this->column_natural_widths[j] = nat_width;
					}

				if(widest >= this->column_widths[j])
					{
						this->column_widths[j] = widest;
						this->COL_widths[j] = widest;
					}

				if(nat_width >= widest)
					{
						widest = nat_width;
						this->COL_widths[j] = widest;
						this->sum_colwidths = get_sum_col_widths(this);
					}

				/// set width for col button and col cell
				gtk_widget_set_size_request(gtk_grid_get_child_at (GTK_GRID(this->column_buttons_grid), j, 0), widest, this->STDRROWHEIGHT);
				gtk_widget_set_size_request(cell, widest, -1);  ////   tallest
			}

		gtk_widget_set_size_request(r->rowbutton, this->widest_rowbtn, tallest);

		r->height = tallest;

		/// set height of first cell in row - the others will follow
		gtk_widget_set_size_request( gtk_grid_get_child_at (GTK_GRID(this->sheetgrid), 0, position),
		                             this->column_widths[0],
		                             tallest
		                           );

		/// add irregular height row to std::map
		if(tallest != this->STDRROWHEIGHT)
			{
				gint diff = tallest - this->STDRROWHEIGHT;
				this->diff_row_heights[r->row_original_index] = diff;
			}

		this->last_row_index++;
}

void                GGRID::add_one_row_at_front (gint position)
{
		RROW *r = new RROW((gpointer)this, this->first_row_index-1);

		this->first_row_index--;
		this->rows.push_front(r);
		r->row_original_index = std::stoi(this->DATAS->result[this->first_row_index][this->NUM_COLS]);

		gtk_grid_insert_row (GTK_GRID(this->sheetgrid), position);

		/// row button
		add_css_class_to_widget(r->rowbutton, "rowHeader");
		gtk_grid_insert_row (GTK_GRID(this->row_buttons_grid), position);
		gtk_grid_attach (GTK_GRID(this->row_buttons_grid), r->rowbutton, 0, 0, 1, 1);
		gtk_widget_add_events(r->rowbutton, GDK_POINTER_MOTION_MASK);
		g_signal_connect(r->rowbutton, "button-press-event", G_CALLBACK(on_row_button_press), (gpointer)r);
		g_signal_connect(r->rowbutton, "motion-notify-event", G_CALLBACK(row_button_motion), (gpointer)r);


		/// row cells
		gint tallest = this->STDRROWHEIGHT;
		gint nat_height, nat_width;



		for(guint j = 0; j < this->NUM_COLS; j++)
			{
				gint widest;

				/// put max width of all cells per col into rowcells vector
				if(r->rowcells.size() < j + 1)
					r->rowcells.push_back(NULL);

				/// start width to compare against is col button width
				widest = this->COL_widths[j];

				GtkWidget *cell;
				cell = gtk_label_new (this->DATAS->result[this->first_row_index][j].c_str());
				gtk_label_set_selectable (GTK_LABEL(cell), TRUE);
				add_css_class_to_widget(cell, "gridcell");
				gtk_widget_show(cell);
				gtk_grid_attach (GTK_GRID(this->sheetgrid), cell, j, 0, 1, 1);
				r->rowcells[j] = cell;

				/// Left align label content  // deprecated from 3.16, use gtk_label_set_xalign (GTK_LABEL(cell), 0.0)
				gtk_misc_set_alignment (GTK_MISC (cell), 0.0, 0.5);

				/// cell widths and heights
				gtk_widget_get_preferred_height (cell, NULL, &nat_height);
				gtk_widget_get_preferred_width (cell, NULL, &nat_width);


				auto search = this->diff_row_heights.find(r->row_original_index);
				if (search != this->diff_row_heights.end())
					nat_height = this->STDRROWHEIGHT + search->second;
				r->original_height = nat_height - search->second;

				tallest = (nat_height > (gint)tallest) ? nat_height : tallest;
				widest = (nat_width > widest) ? nat_width : widest;
				if(nat_width > this->column_natural_widths[j])
					this->column_natural_widths[j] = nat_width;

				if(widest >= this->column_widths[j])
					this->column_widths[j] = widest;


				if(nat_width >= widest)
					{
						widest = nat_width;
						this->COL_widths[j] = widest;
						this->sum_colwidths = get_sum_col_widths(this);
					}

				gtk_widget_set_size_request(gtk_grid_get_child_at (GTK_GRID(this->column_buttons_grid), j, 0), widest, this->STDRROWHEIGHT);
				gtk_widget_set_size_request(cell, widest, -1);
			}

		gtk_widget_set_size_request(r->rowbutton, this->widest_rowbtn, tallest);

		r->height = tallest;

		gtk_widget_set_size_request(gtk_grid_get_child_at (GTK_GRID(this->sheetgrid), 0, 0), this->column_widths[0], tallest);

		/// add irregular height row to std::map
		if(tallest != this->STDRROWHEIGHT)
			{
				gint diff = tallest - this->STDRROWHEIGHT;
				this->diff_row_heights[r->row_original_index] = diff;
			}

}

void                GGRID::init_content(DATA *DATAS)
{
	/// tear down existing grid elements
	this->clear_grid();

	/// rebuild anew
	this->DATAS = DATAS;
	this->NUM_ROWS = DATAS->result.size();
	this->NUM_COLS = DATAS->column_names.size();


	gint window_total_height = gdk_window_get_height (gtk_widget_get_window(GTK_WIDGET(window)));
	this->GRID_MAXRROWS = (window_total_height / this->STDRROWHEIGHT) + this->extra_rows;
	if((gint)this->NUM_ROWS < this->GRID_MAXRROWS ) this->GRID_MAXRROWS = this->NUM_ROWS;
	this->GRID_TOT_HEIGHT = (this->NUM_ROWS * this->STDRROWHEIGHT) + this->margin_bottom + this->STDRROWHEIGHT;

	/// row buttons grid ===============================
	{
		this->row_buttons_grid = gtk_grid_new ();
		gtk_widget_set_margin_top (this->row_buttons_grid, this->STDRROWHEIGHT );

		// buttons are added per row in add_one_row_to_end()

		/// get width of last row button in dataset (highest number, therefore widest)
		GtkWidget *rowHeader = gtk_button_new_with_label (std::to_string(this->NUM_ROWS - 1).c_str());
		gtk_widget_show(rowHeader);
		gtk_widget_get_preferred_width (rowHeader, NULL, &this->widest_rowbtn);
	}

	/// col buttons grid ================================
	{
		this->column_buttons_grid = gtk_grid_new ();
		gtk_widget_set_margin_start (this->column_buttons_grid, this->widest_rowbtn );

		gint nat_width_colbtn;
		for(gint i = 0; i < (gint)this->NUM_COLS; i++)
			{
				GtkWidget *colHeader = gtk_button_new_with_label (this->DATAS->column_names[i].c_str());
				gtk_button_set_relief (GTK_BUTTON (colHeader), GTK_RELIEF_NONE);
				add_css_class_to_widget(colHeader, "colHeader");
				gtk_widget_show(colHeader);

				gtk_grid_attach (GTK_GRID(this->column_buttons_grid), colHeader, i, 0, 1, 1);

				gtk_widget_get_preferred_width (colHeader, NULL, &nat_width_colbtn);

				if(this->column_widths.size() < guint(i + 1))
					{
						this->column_widths.push_back (nat_width_colbtn);
						this->column_natural_widths.push_back (nat_width_colbtn);

						this->COL_widths.push_back (nat_width_colbtn);
					}

				gtk_widget_set_size_request(colHeader, nat_width_colbtn, this->STDRROWHEIGHT);

				gtk_widget_add_events(colHeader, GDK_POINTER_MOTION_MASK);
				g_signal_connect(colHeader, "button-press-event", G_CALLBACK(on_col_button_press), (gpointer)this);
				g_signal_connect(colHeader, "motion-notify-event", G_CALLBACK(col_button_motion), (gpointer)this);
			}
	}

	/// sheetgrid =================================
	{
		this->sheetgrid = gtk_grid_new ();
		gtk_widget_set_margin_start (this->sheetgrid, this->widest_rowbtn );
	  gtk_widget_set_margin_top (this->sheetgrid, this->STDRROWHEIGHT );

		for(gint i = 0; i < this->GRID_MAXRROWS; i++)
				this->add_one_row_to_end(i);
	}

	/// global button ===============================
	{
		this->global_button = gtk_button_new_with_label ("");
		gtk_button_set_relief (GTK_BUTTON (this->global_button), GTK_RELIEF_NONE);
		add_css_class_to_widget(this->global_button, "rowHeader");
		gtk_overlay_add_overlay (GTK_OVERLAY(this->grid), this->global_button);
	  gtk_widget_set_hexpand(this->global_button, FALSE);
	  gtk_widget_set_vexpand(this->global_button, FALSE);
	  gtk_widget_set_halign(this->global_button, GTK_ALIGN_START);
	  gtk_widget_set_valign(this->global_button, GTK_ALIGN_START);
		gtk_widget_set_size_request(this->global_button, this->widest_rowbtn, this->STDRROWHEIGHT);
	}

	gtk_layout_put (GTK_LAYOUT(this->layout), this->sheetgrid, 0, 0);
	gtk_layout_put (GTK_LAYOUT(this->layout), this->column_buttons_grid, 0, 0);
	gtk_layout_put (GTK_LAYOUT(this->layout), this->row_buttons_grid, 0, 0);


	gint diff = get_sum_row_diffs(this, this->NUM_ROWS);
	gtk_widget_set_size_request(this->column_buttons_grid, get_sum_col_widths(this), this->STDRROWHEIGHT);
	gtk_widget_set_size_request(this->row_buttons_grid, this->widest_rowbtn, this->GRID_TOT_HEIGHT + diff);
	gtk_layout_set_size (GTK_LAYOUT(this->layout), get_sum_col_widths(this), this->GRID_TOT_HEIGHT + diff);

	GtkAdjustment * vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (this->grid_scrollwin));
	GtkAdjustment * hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (this->grid_scrollwin));
	gtk_adjustment_set_value (vadj, 0);
	gtk_adjustment_set_value (hadj, 0);

	gtk_layout_move (GTK_LAYOUT(this->layout), this->row_buttons_grid, 0, 0 );
	gtk_layout_move (GTK_LAYOUT(this->layout), this->column_buttons_grid, 0, 0 );

	this->sum_colwidths = get_sum_col_widths(this);

}

void                GGRID::clear_grid()
{
	/// tear down existing grid elements

	/// remove all rows
	while(this->rows.size() > 0)
		{
			delete_row(this, 0);
			this->rows.pop_front();
		}

	this->rows.clear();
	this->rows = {};
	this->diff_row_heights.clear();

	this->last_row_index = 0;
	this->first_row_index = 0;
	this->column_widths = {};
	this->column_natural_widths = {};
	this->sum_colwidths = 0;
	this->current_scroll_position_x = 0;
	this->current_scroll_position_y = 0;
	this->new_margintop = 0;
	this->COL_widths = {};



	//	this->rows.~deque(); /// This calls allocator_traits::destroy on each of the contained elements, and deallocates all the storage capacity allocated by the vector using its allocator. --- but above while.. is still necessary. ?

	/// destroy elements
	if(this->global_button){
		gtk_widget_destroy(this->global_button);
		gtk_widget_destroy(this->column_buttons_grid);
		gtk_widget_destroy(this->row_buttons_grid);
		gtk_widget_destroy(this->sheetgrid);
	}
}

GGRID::GGRID (DATA *DATAS)
{

	this->DATAS = DATAS;
	this->NUM_ROWS = DATAS->result.size();
	this->NUM_COLS = DATAS->column_names.size();

	gtk_widget_show_all(GTK_WIDGET(window));
	gint window_total_height = gdk_window_get_height (gtk_widget_get_window(GTK_WIDGET(window)));

	this->GRID_MAXRROWS = (window_total_height / this->STDRROWHEIGHT) + this->extra_rows;
	if((gint)this->NUM_ROWS < this->GRID_MAXRROWS ) this->GRID_MAXRROWS = this->NUM_ROWS;
	this->GRID_TOT_HEIGHT = (this->NUM_ROWS * this->STDRROWHEIGHT) + this->margin_bottom + this->STDRROWHEIGHT;


	/// Overlay
	this->grid = gtk_overlay_new ();

	/// ScrolledWindow (on overlay)
	this->grid_scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(this->grid_scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(this->grid_scrollwin), GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(this->grid), this->grid_scrollwin);

	/// Layout (in scrolled window)
	this->layout = gtk_layout_new (NULL, NULL);
	gtk_widget_set_margin_start (this->layout, 0);
	gtk_widget_set_vexpand(this->layout, TRUE);
	gtk_widget_set_hexpand(this->layout, TRUE);
  gtk_container_add(GTK_CONTAINER(this->grid_scrollwin), this->layout);



  /// scrollbars
  GtkWidget * hsw = gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(this->grid_scrollwin));
  GtkAdjustment * hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (this->grid_scrollwin));

  GtkWidget * vsw = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(this->grid_scrollwin));
  GtkAdjustment * vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (this->grid_scrollwin));


	/// scroll events
	g_signal_connect(vsw, "change-value", G_CALLBACK(on_explicit_vscrollbar_action), NULL);
  g_signal_connect(vadj, "value-changed", G_CALLBACK(on_vadjustment_value_changed), (gpointer)this);
	g_signal_connect(hsw, "change-value", G_CALLBACK(on_explicit_hscrollbar_action), NULL);
  g_signal_connect(hadj, "value-changed", G_CALLBACK(on_hadjustment_value_changed), (gpointer)this);

	/// Redo grid sizes on any window size change
	g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(window_configure_event), this);
}




