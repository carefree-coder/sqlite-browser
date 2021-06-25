/*
 * gridsheet.h - GtkGridSheet widget for Gtk+.
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


#include <map>
#include <deque>
#include <algorithm>

enum sorter {
	SORT_UN = 0,
	SORT_UP,
	SORT_DOWN
};

struct GGRID;
struct RROW;

struct RROW{

	gint id;
	gint height;
	gint original_height = -1;
	gint row_original_index = -1;

	GtkWidget *rowbutton;
	std::vector<GtkWidget *> rowcells = {};
	GGRID *grid;

	RROW(gpointer grid, gint id){
		this->grid = (GGRID *)grid;
		this->id = id;
		this->rowbutton = gtk_button_new_with_label (std::to_string(id + 1).c_str());
		gtk_button_set_relief (GTK_BUTTON (this->rowbutton), GTK_RELIEF_NONE);
		gtk_widget_show(this->rowbutton);
	}
};


struct GGRID{

	DATA *DATAS;
	guint NUM_ROWS;
	guint NUM_COLS;

	GtkWidget *grid;
	GtkWidget *grid_scrollwin;
	GtkWidget *layout;
	gint STDRROWHEIGHT = 38;
	gint GRID_PAGE_SIZE_H = 0;
	gint GRID_MAXRROWS = 0;
	gint GRID_TOT_HEIGHT = 0;
	gint extra_rows = 10;
	gint margin_bottom = 100;
	gdouble new_margintop = 0;
	GtkWidget *global_button = NULL;
	gdouble current_scroll_position_x = 0;
	gdouble current_scroll_position_y = 0;

	/// row buttons
	GtkWidget *row_buttons_grid;
	gint widest_rowbtn;
	gint sum_of_irregular_row_size_diffs = 0;
	std::map <gint, gint> diff_row_heights;

	/// column buttons
	GtkWidget *column_buttons_grid;
	std::vector<gint> column_widths = {};
	std::vector<gint> column_natural_widths = {};
	gint sum_colwidths = 0;
	std::pair<gint, gint> sorted = std::make_pair(-1, SORT_UN);

	std::vector <gint> COL_widths = {};

	/// grid
	GtkWidget *sheetgrid;
	std::deque<RROW *> rows = {};
	gint last_row_index = 0;
	gint first_row_index = 0;
	gint grid_initial_width;


	void add_one_row_at_front(gint position);
	void add_one_row_to_end(gint position);
	void clear_grid();
	void init_content(DATA *DATAS);


	/// Constructor(s)
	GGRID(DATA *DATAS);
};
