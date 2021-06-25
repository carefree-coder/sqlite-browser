/*
 * window_gui.cpp - SQLite Browser
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
#include "window_gui.h"
#include "main.h"
#include "sqlite.h"
#include "gridsheet.h"

GtkWidget *window;
GtkWidget *headerbar;
GtkWidget *statusbar;
GtkWidget *revealer;
GtkTreeStore *treestore;
GtkWidget *treeview;
GtkWidget *treeview_scrollwin;
GtkWidget *rightpane_vbox;
GtkWidget *globalhbox;


DATA *DATAS;
GGRID *GRID;




struct DBNode {
    /// DBNode for treeview db item

    std::string DBPATH;
    std::string dbname;

    std::vector <std::string> tables;
    std::vector <std::string> views;
    std::vector <std::string> triggers;
    std::vector <std::string> indexs;

    /// constructor
    DBNode(std::string DBPATH): DBPATH(DBPATH) {

			this->dbname = (std::string)g_path_get_basename (DBPATH.c_str());

			gboolean valid = verify_db_is_valid( g_file_new_for_path (DBPATH.c_str()) );
			if(! valid)
				return;

			/// Fetch tables' and other names from supplied db
			DATA *d = DATAS; /// global DATAS

			for(auto x : d->result){

				if(x[0] == "table")
					this->tables.push_back(x[1]);
				if(x[0] == "view")
					this->views.push_back(x[1]);
				if(x[0] == "trigger")
					this->triggers.push_back(x[1]);
				if(x[0] == "index")
					this->indexs.push_back(x[1]);
			}
		}
};

std::vector<DBNode *> DBNodes;


/// ////////
/// get data
/// ////////
gboolean            verify_db_is_valid_unquiet (GFile *file){

	/// Unquiet -> show the error messages.

	GError *error = NULL;
	GFileInfo *file_info = g_file_query_info (file, "standard::*", (GFileQueryInfoFlags)0, NULL, &error);
	if(! G_IS_FILE_INFO (file_info)) {
		std::cout   <<   "Not a valid file"   <<    std::endl;
		return FALSE;
	}
	const char *content_type = g_file_info_get_content_type (file_info); // application/vnd.sqlite3
	char *desc = g_content_type_get_description (content_type); // SQLite3 database

	/// Not a sqlite3 db
	if( g_strcmp0(content_type, "application/vnd.sqlite3") != 0 &&
      g_strcmp0(content_type, "application/x-sqlite3") != 0 &&
	    g_strcmp0(desc, "SQLite3 database") != 0
	    ){

		std::cout << "g_file_info_get_content_type: "<< content_type << std::endl;
		std::cout << "g_content_type_get_description: "<< desc << std::endl;
		std::cout << "File is not a sqlite db, using default db " << std::endl;

		return FALSE;
	}

	return TRUE;
}

gboolean            verify_db_is_valid (GFile *file){

	GError *error = NULL;
	GFileInfo *file_info = g_file_query_info (file, "standard::*", (GFileQueryInfoFlags)0, NULL, &error);
	if(! G_IS_FILE_INFO (file_info))
		return FALSE;

	const char *content_type = g_file_info_get_content_type (file_info); // application/vnd.sqlite3
	char *desc = g_content_type_get_description (content_type); // SQLite3 database

	/// Not a sqlite3 db
	if( g_strcmp0(content_type, "application/vnd.sqlite3") != 0 &&
      g_strcmp0(content_type, "application/x-sqlite3") != 0 &&
	    g_strcmp0(desc, "SQLite3 database") != 0
	    )
		return FALSE;

	return TRUE;
}

void                init_datas(){
	DATAS = new DATA();
}

void                get_data00(int i){

	/// If db is already running, open treeview to that

	gtk_tree_view_collapse_all (GTK_TREE_VIEW(treeview));
	std::string nn = std::to_string(i) + ":0";
	gtk_tree_view_expand_to_path (GTK_TREE_VIEW(treeview), gtk_tree_path_new_from_string (nn.c_str()));


		/// new grid

		GtkTreeIter iter;
		gchar *dbfullpath;
		GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
		gtk_tree_model_get_iter_from_string (model, &iter, std::to_string(i).c_str());
		gtk_tree_model_get (model, &iter,  0, &dbfullpath,  -1);



		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_unselect_all (selection);
		GtkTreePath * path = gtk_tree_path_new_from_indices (i, -1);
		gtk_tree_selection_select_path (selection, path);


		/// DATAS is implicitly set with new values
		std::string DBPATH = dbfullpath;
		std::string SQL = "SELECT * FROM sqlite_master";

		/// Populate struct DATA with results of query/table
		sqlite_select((char *)DBPATH.c_str(), (char *)SQL.c_str(), DATAS);

		/// SQLite error
		if(g_strcmp0(DATAS->global_sqlite_errmsg.c_str(), "") != 0){
			std::cout   <<   "SQLITE ERROR: "   << DATAS->global_sqlite_errmsg <<  std::endl;
			return;
		}

		/// Set statusbar text(s)
		std::string n_records = std::to_string(DATAS->result.size()) + (std::string)" record(s)";
		gtk_statusbar_push (GTK_STATUSBAR( statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR( statusbar), ""), n_records.c_str());

		/// headerbar texts
		gtk_header_bar_set_title ((GtkHeaderBar *)headerbar, g_path_get_basename (DBPATH.c_str()));
		gtk_header_bar_set_subtitle ((GtkHeaderBar *)headerbar, SQL.c_str());

		/// Make the sheet with new data
		make_grid();

}

void                get_data(){

	/// Populate struct DATA with results of query/table

	/// Verify db is valid
	GFile * gf = g_file_new_for_path (APP_STATE->DBPATH.c_str());
	if( ! verify_db_is_valid(gf) ){

		DBNode *n = new DBNode(APP_STATE->DBPATH);
		DBNodes.push_back(n);
		APP_STATE->current_dbnodes_count = DBNodes.size();
		APP_STATE->dbpaths.push_back(APP_STATE->DBPATH);

		return;
	}


	/// DATAS is implicitly set with new values in sqlite_select query
	std::string DBPATH = APP_STATE->DBPATH;
	std::string SQL = "SELECT * FROM sqlite_master";
	sqlite_select((char *)DBPATH.c_str(), (char *)SQL.c_str(), DATAS); /// populates DATAS

	DBNode *n = new DBNode(DBPATH);

	/// If there was an sqlite error
	if(g_strcmp0(DATAS->global_sqlite_errmsg.c_str(), "") != 0){
		std::cout   <<   "SQLITE ERROR: "   << DATAS->global_sqlite_errmsg <<  std::endl;
		return;
	}


	int res = std::stoi(DBSQLITE->db_select("SELECT COUNT(id) FROM databases")[0][0]);
	if(res != 0){
		/// Set statusbar text(s)
		std::string n_records = std::to_string(DATAS->result.size()) + (std::string)" record(s)";
		gtk_statusbar_push (GTK_STATUSBAR( statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR( statusbar), ""), n_records.c_str());

		/// headerbar texts
		gtk_header_bar_set_title ((GtkHeaderBar *)headerbar, g_path_get_basename (DBPATH.c_str()));
		gtk_header_bar_set_subtitle ((GtkHeaderBar *)headerbar, SQL.c_str());
	} else {
		/// headerbar texts
		gtk_header_bar_set_title ((GtkHeaderBar *)headerbar, g_path_get_basename (DBPATH.c_str()));
		gtk_header_bar_set_subtitle ((GtkHeaderBar *)headerbar, "No databases in Master.sqlite");
	}

	DBNodes.push_back(n);
	APP_STATE->current_dbnodes_count = DBNodes.size();
	APP_STATE->dbpaths.push_back(DBPATH);
}

static void         open_file_chooser (GtkWidget *button, gpointer data){

	/// Open file chooser dialogue

	GtkWidget *toplevel;
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkFileFilter *filter;
	gint res;
	gchar *dbname = NULL;

	toplevel = gtk_widget_get_toplevel (button);
	dialog = gtk_file_chooser_dialog_new ("Choose an SQLite database",
	                   GTK_WINDOW(toplevel),
										 GTK_FILE_CHOOSER_ACTION_OPEN,
	                   "OK", GTK_RESPONSE_ACCEPT,
	                   "Cancel", GTK_RESPONSE_CANCEL,
	                   NULL);

	chooser = GTK_FILE_CHOOSER (dialog);
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, "SQLite Databases");
	gtk_file_filter_add_pattern (filter, "*.sqlite");
	gtk_file_filter_add_pattern (filter, "*.db");
	gtk_file_filter_add_mime_type (filter, "application/x-sqlite3");
	gtk_file_filter_add_mime_type (filter, "application/vnd.sqlite3");
	gtk_file_chooser_add_filter (chooser, filter);



	res = gtk_dialog_run (GTK_DIALOG (dialog));

	if (res == GTK_RESPONSE_ACCEPT)
    dbname = gtk_file_chooser_get_filename (chooser);

	gtk_widget_destroy (dialog);


	/// if selected db already in databases, switch to it
	int c = std::stoi(DBSQLITE->db_select("SELECT COUNT(id) FROM databases WHERE database_path ='" + (std::string)dbname +"'")[0][0]);
	if(c > 0){
		APP_STATE->DBPATH = (std::string)dbname;
		get_data();
		make_treeview();
		make_grid("sqlite_master");
		return;
	}



	/// add a database to databases table
	std::string SQL = "INSERT INTO databases (\"database_path\") VALUES(\""+ (std::string)dbname +"\")";
	int n = DBSQLITE->db_insert(SQL);
	if(n != 0){
		std::cout << "error inserting into db: \n" << SQL << std::endl;

		/// TODO: some kind of display of error

		return;
	}

	/// insert ok, now switch to said db...
	APP_STATE->DBPATH = (std::string)dbname;

	get_data();
	make_treeview();

	/// Make the sheet with new data
	make_grid("sqlite_master");


	return;
}

void                get_data_emptyDB(){

	gtk_box_set_homogeneous (GTK_BOX(rightpane_vbox), FALSE);


	GtkWidget *label = gtk_label_new ("You currently have no databases in your collection");
	gtk_box_pack_start(GTK_BOX(rightpane_vbox), label, TRUE, TRUE, 0);
	gtk_widget_set_valign (label, GTK_ALIGN_END);


	GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(rightpane_vbox), hbox, FALSE, FALSE, 0);

	GtkWidget *button = gtk_button_new_from_icon_name ("document-new-symbolic", GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
	gtk_widget_set_halign (button, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_top (button, 40);
	gtk_widget_set_margin_bottom (button, 40);
	add_css_class_to_widget(button, "startBtn");
	g_signal_connect (button, "clicked", (GCallback) open_file_chooser, NULL);


	GtkWidget *label2 = gtk_label_new ("Click to select a database");
	gtk_box_pack_start(GTK_BOX(rightpane_vbox), label2, TRUE, TRUE, 0);
	gtk_widget_set_valign (label2, GTK_ALIGN_START);
}

void                get_data0(){

	/// Do all the databases in Master db

	std::string DBPATH = APP_STATE->DBPATH;
	std::string SQL = "SELECT database_path FROM databases ORDER BY id ASC";
	sqlite_select((char *)DBPATH.c_str(), (char *)SQL.c_str(), DATAS);

	std::vector<std::string> masterdbs;
	for(auto x : DATAS->result){
		masterdbs.push_back(x[0]);
	}

	for(auto dbpath : masterdbs){
			APP_STATE->DBPATH = dbpath;
			get_data();
			make_treeview ();
	}

	gtk_tree_view_collapse_all (GTK_TREE_VIEW(treeview));
	gtk_tree_view_expand_to_path (GTK_TREE_VIEW(treeview), gtk_tree_path_new_from_string ("0:0"));

	/// Reset DATAS, on initial load
	SQL = "SELECT * FROM sqlite_master";
	sqlite_select((char *)DBPATH.c_str(), (char *)SQL.c_str(), DATAS);


	int res = std::stoi(DBSQLITE->db_select("SELECT COUNT(id) FROM databases")[0][0]);
	if(res != 0){
		/// Set statusbar text(s)
		std::string n_records = std::to_string(DATAS->result.size()) + (std::string)" record(s)";
		gtk_statusbar_push (GTK_STATUSBAR( statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR( statusbar), ""), n_records.c_str());

		/// headerbar texts
		gtk_header_bar_set_title ((GtkHeaderBar *)headerbar, g_path_get_basename (DBPATH.c_str()));
		gtk_header_bar_set_subtitle ((GtkHeaderBar *)headerbar, SQL.c_str());
	} else {
		/// headerbar texts
		gtk_header_bar_set_title ((GtkHeaderBar *)headerbar, g_path_get_basename (DBPATH.c_str()));
		gtk_header_bar_set_subtitle ((GtkHeaderBar *)headerbar, "No databases in Master.sqlite");
	}

}




/// /////////////
/// make treeview
/// /////////////
static const char *menuItemsArray[] = {
	"Add this item to database",
	"Remove this item from database"
	/*, "Open containing folder", "Copy filename", "Delete this item"*/
	};

void                contextmenu_item_clicked (GtkWidget * mm, gpointer item){


	gchar * key = (gchar*)item;



	/// Get tree path for row that was clicked
	GtkTreePath *path;
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	GtkTreeIter iter;
	GList * list_of_selected_rows = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION (selection), NULL);
	if( g_list_length (list_of_selected_rows)  == 1){
		path = (GtkTreePath*)list_of_selected_rows->data;
	} else {
		printf("multiple selections not supported\n");
		return;
	}

	if (! gtk_tree_model_get_iter(model, &iter, path)){
		return; // path describes a non-existing row - should not happen
	}

	gtk_tree_selection_unselect_all (selection);
	gtk_tree_selection_select_path (selection, path);
	gtk_tree_model_get_iter(model, &iter, path);
	int  group;
	gchar *dbfullpath, *type, *itemname ;
	gtk_tree_model_get (model, &iter,  0, &dbfullpath,   1, &type, 2,&itemname,  3, &group,  -1);

	std::string DBPATH = (std::string)dbfullpath;



	/// remove item from database and remove from treeview
	if((std::string)key == "Remove this item from database"){

		APP_STATE->DBPATH = APP_STATE->APP_DEFAULTDB;

		std::string sql = "DELETE FROM databases WHERE database_path = '" + DBPATH + "'";
		DBSQLITE->db_delete(sql);

		gtk_tree_store_remove (treestore, &iter);
	}

	/// Add item to db
	if((std::string)key == "Add this item to database"){

		APP_STATE->DBPATH = APP_STATE->APP_DEFAULTDB;
		std::string SQL = "INSERT INTO databases (\"database_path\") VALUES(\""+ DBPATH +"\")";
		int n = DBSQLITE->db_insert(SQL);
		if(n != 0){
			std::cout << "error inserting into db: \n" << SQL << std::endl;

			/// TODO: some kind of display of error

			return;
		}
	}
}

static GtkWidget *  contextmenu_make (gpointer userdata) {

	GtkTreePath *path = (GtkTreePath *)userdata;
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	GtkTreeIter iter;
	gtk_tree_model_get_iter (GTK_TREE_MODEL(treestore), &iter, path);

	/// get the db path from the treepath path to determine if it is already in database or not
	gchar *dbfullpath;
	gtk_tree_model_get (model, &iter,  0, &dbfullpath, -1);

	gboolean isInDB = FALSE;
	for( auto db : DBSQLITE->db_select("SELECT database_path FROM databases")){
		if(db[0]== dbfullpath){
			isInDB = TRUE;
			break;
		}
	}



	/// Make a menu from provided elements
	GtkWidget *menu = gtk_menu_new();
	GtkWidget *menuitem;

	/// Append to the right click context menu each struct item of menuItemsArray
	for (guint i = 0; i < sizeof(menuItemsArray) / sizeof(menuItemsArray[0]); i++) {
		menuitem = gtk_menu_item_new_with_label(menuItemsArray[i]);

		if((std::string)menuItemsArray[i] == "Remove this item from database")
			gtk_widget_set_sensitive ((GtkWidget *)menuitem, (isInDB) ? TRUE : FALSE);

		if((std::string)menuItemsArray[i] == "Add this item to database")
			gtk_widget_set_sensitive ((GtkWidget *)menuitem, (isInDB) ? FALSE : TRUE);




		g_signal_connect(menuitem, "activate", (GCallback) contextmenu_item_clicked, (gpointer)menuItemsArray[i]);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}

	return menu;
}

static void         contextmenu_show (GtkWidget *treeview, GdkEventButton *event, gpointer userdata) {

	/// Shows the context menu on right click

	GtkWidget * menu = contextmenu_make (userdata);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
	(event != NULL) ? event->button : 0,	gdk_event_get_time((GdkEvent*)event));
}

static gboolean     treeview_click (GtkWidget *treeview, GdkEventButton *event, gpointer userdata) {

  /// Detect if the event was a double-click and if so, return from this function without doing anything.
	if (event->type == GDK_DOUBLE_BUTTON_PRESS)
		return TRUE;

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	GtkTreePath *path;

	/// Click on a treeview ...
	if (gtk_tree_selection_count_selected_rows(selection) <= 1) {


		/// ... area that is not a file or folder
		if ( ! (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(treeview), (gint) event->x,	(gint) event->y, &path, NULL, NULL, NULL))) {
			gtk_tree_selection_unselect_all (selection);
			return FALSE;
		}


		/// Get tree path for row that was clicked
		GtkTreeIter iter;
		GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
		gtk_tree_selection_unselect_all (selection);
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_model_get_iter(model, &iter, path);
		int  group;
		gchar *dbfullpath, *type, *itemname ;
		gtk_tree_model_get (model, &iter,  0, &dbfullpath,   1, &type, 2,&itemname,  3, &group,  -1);

		std::string DBPATH = (std::string)dbfullpath;
		std::string SQL;

		/// group = 0 : database items of treeview
		if(group == 0) {

			/// Right click on a database brings up context-menu
			if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
				/// Get path of clicked item (folder or file). If its not a folder or file, return.
				if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
					if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(treeview), (gint) event->x,	(gint) event->y, &path, NULL, NULL, NULL)) {
						/// Get tree path for row that was clicked */
						gtk_tree_selection_unselect_all (selection);
						gtk_tree_selection_select_path (selection, path);
					}  else {
						/// Click is not on a file or folder, so dont show menu */
						gtk_tree_selection_unselect_all (selection);
						return TRUE;
					}
				}

				/// we dont want to allow delete of master db
				if(dbfullpath == APP_STATE->APP_DEFAULTDB)
					return FALSE;

				/// is item already in database



				/// Show the right click context menu
				contextmenu_show (treeview, event, path);

				return TRUE; /// We handled this
			}

			return FALSE;
		}


		/// group = 1 : the header of table/view etc
		if(group == 1){

			std::string ttype;
			if(g_strcmp0(itemname, "Tables") == 0) ttype = "table";
			if(g_strcmp0(itemname, "Views") == 0) ttype = "view";
			if(g_strcmp0(itemname, "Indices") == 0) ttype = "index";
			if(g_strcmp0(itemname, "Triggers") == 0) ttype = "trigger";

			//std::cout   <<   "type: "   <<  "SELECT * FROM sqlite_master WHERE type = " << ttype <<   std::endl;

			SQL = "SELECT * FROM sqlite_master WHERE type = '" + ttype + "'";

			/// we clicked on a database - do nothing.
			return FALSE;
		}


		/// group = 2 : the actual db tables
		if(group == 2){
			if((std::string)type == "table" || (std::string)type == "view"){
				//std::cout   <<   "table: "   <<  "SELECT * FROM '"  <<  itemname  << "'" << std::endl;
				std::string suffix = (g_strcmp0(itemname, "databases") == 0) ? "ORDER BY id ASC" : "";

				SQL = "SELECT * FROM '" + (std::string)itemname + "' " + suffix;


				/// which db are we in ? - so we can check if its Master.sqlite
				if(dbfullpath == APP_STATE->APP_DEFAULTDB)
					APP_STATE->DBPATH = APP_STATE->APP_DEFAULTDB;

			}
			else{
				std::cout   <<   "its a trigger or index, handle differently"   <<    std::endl;
				return FALSE;
			}

		}




		/// Populate struct DATA with results of query/table
		sqlite_select((char *)DBPATH.c_str(), (char *)SQL.c_str(), DATAS);

		/// SQLite error
		if(g_strcmp0(DATAS->global_sqlite_errmsg.c_str(), "") != 0){
			std::cout   <<   "SQLITE ERROR: "   << DATAS->global_sqlite_errmsg <<  std::endl;
			return TRUE;
		}

		/// Set statusbar text(s)
		std::string n_records = std::to_string(DATAS->result.size()) + (std::string)" record(s)";
		gtk_statusbar_push (GTK_STATUSBAR( statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR( statusbar), ""), n_records.c_str());

		/// headerbar texts
		gtk_header_bar_set_title ((GtkHeaderBar *)headerbar, g_path_get_basename (DBPATH.c_str()));
		gtk_header_bar_set_subtitle ((GtkHeaderBar *)headerbar, SQL.c_str());

		/// Make the sheet with new data
		make_grid(itemname);

		return FALSE;
	}


	return FALSE;
}

static void         populate_treeview (){

		guint n = APP_STATE->current_dbnodes_count - 1;

		GFile * gf = g_file_new_for_path (APP_STATE->DBPATH.c_str());
		const gchar *db = verify_db_is_valid(gf) ? "db" : "invaliddb";

		/// Database Tree item (0)
		GtkTreeIter dbIter;
		gtk_tree_store_append (treestore, &dbIter,  NULL);
		gtk_tree_store_set (treestore, &dbIter,
				0, DBNodes[n]->DBPATH.c_str(),   // 0 db fullpath
				1, db,                           // 1 class - db, table, view, index, trigger, file
				2, g_path_get_basename (DBNodes[n]->DBPATH.c_str()),
				3, 0,                            // group
				-1
				);


		/// Invalid db
		if(g_strcmp0(db, "invaliddb") == 0) {
			GtkTreeIter anonIter;
			gtk_tree_store_append (treestore, &anonIter,  &dbIter);
			gtk_tree_store_set (treestore, &anonIter,
					0, DBNodes[n]->DBPATH.c_str(),   // 0 db fullpath,
					1, "group",                      // 1 class - db, table, view, index, trigger, file
					2, "Database not found",
					3, 1,                            // group
					-1
					);
			return;
		}

		std::vector <std::string> types = {"Tables", "Views", "Indices", "Triggers"};
		for (auto type : types){
			GtkTreeIter anonIter;
			gtk_tree_store_append (treestore, &anonIter,  &dbIter);
			gtk_tree_store_set (treestore, &anonIter,
					0, DBNodes[n]->DBPATH.c_str(),   // 0 db fullpath,
					1, "group",                      // 1 class - db, table, view, index, trigger, file
					2, type.c_str(),
					3, 1,                            // group
					-1
					);

			if(type == "Tables"){
				for(auto table : DBNodes[n]->tables){
					/// Tables
					GtkTreeIter tablesIter;
					gtk_tree_store_append (treestore, &tablesIter,  &anonIter);
					gtk_tree_store_set (treestore, &tablesIter,
							0, DBNodes[n]->DBPATH.c_str(),   // 0 db fullpath,
							1, "table",                      // 1 class - db, table, view, index, trigger, file
							2, table.c_str(),                // itemname,
							3, 2,                            // group
							-1
							);
				}
			}
			if(type == "Views"){
				for(auto table : DBNodes[n]->views){
					/// Tables
					GtkTreeIter tablesIter;
					gtk_tree_store_append (treestore, &tablesIter,  &anonIter);
					gtk_tree_store_set (treestore, &tablesIter,
							0, DBNodes[n]->DBPATH.c_str(),   // 0 db fullpath,
							1, "view",                       // 1 class - db, table, view, index, trigger, file
							2, table.c_str(),                // itemname,
							3, 2,                            // group
							-1
							);
				}
			}
			if(type == "Indices"){
				for(auto table : DBNodes[n]->indexs){
					/// Tables
					GtkTreeIter tablesIter;
					gtk_tree_store_append (treestore, &tablesIter,  &anonIter);
					gtk_tree_store_set (treestore, &tablesIter,
							0, DBNodes[n]->DBPATH.c_str(),   // 0 db fullpath,
							1, "index",                      // 1 class - db, table, view, index, trigger, file
							2, table.c_str(),                // itemname,
							3, 2,                            // group
							-1
							);
				}
			}
			if(type == "Triggers"){
				for(auto table : DBNodes[n]->triggers){
					/// Tables
					GtkTreeIter tablesIter;
					gtk_tree_store_append (treestore, &tablesIter,  &anonIter);
					gtk_tree_store_set (treestore, &tablesIter,
							0, DBNodes[n]->DBPATH.c_str(),   // 0 db fullpath,
							1, "trigger",                    // 1 class - db, table, view, index, trigger, file
							2, table.c_str(),                // itemname,
							3, 2,                            // group
							-1
							);
				}
			}
		}

		/// sqlite_master table appended to end
		GtkTreeIter anonIter;
		gtk_tree_store_append (treestore, &anonIter,  &dbIter);
		gtk_tree_store_set (treestore, &anonIter,
				0, DBNodes[n]->DBPATH.c_str(),   // 0 db fullpath,
				1, "table",                      // 1 class - db, table, view, index, trigger, file
				2, "sqlite_master",
				3, 2,                            // group
				-1
				);


		/// Expand treeview at path
		std::string nn = std::to_string(n) + ":0";
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW(treeview), gtk_tree_path_new_from_string (nn.c_str()));

		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_unselect_all (selection);
		gtk_tree_selection_select_path (selection, gtk_tree_path_new_from_string (std::to_string(n).c_str()));

}

void                make_treeview () {

	/// Treeview is already initiated with content.
	if(APP_STATE->current_dbnodes_count > 1){
		gtk_tree_view_collapse_all (GTK_TREE_VIEW(treeview));
		populate_treeview ();
		return;
	}

	auto prepare_treemodel = []() {
		treestore = gtk_tree_store_new(
				4,                   // col count
				G_TYPE_STRING,       // 0 dbfullpath
				G_TYPE_STRING,       // 1 class - db, table, view, index, trigger, file
				G_TYPE_STRING,       // 2 item name
				G_TYPE_INT           // 3 group
				);
	  treeview = gtk_tree_view_new_with_model( GTK_TREE_MODEL(treestore));
	  g_object_unref(treestore);
	};

	prepare_treemodel();

	auto set_tree_cell_text = [](GtkTreeViewColumn *tc, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {

	  gchar ** text;
	  gtk_tree_model_get(model, iter, 2, &text,  -1);

		g_object_set (cell, "editable", FALSE, NULL);
	  g_object_set (cell, "text", text, NULL);

	  g_free(text);
	};

	auto set_tree_cell_pixbuf = [] (GtkTreeViewColumn *tc, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {

	  int folderNo;
	  gchar *type, *validdb;
	  gtk_tree_model_get (model, iter, 1, &validdb, 2, &type, 3, &folderNo, -1);

		const gchar *dbiconfile = "";
		const gchar *tableiconfile = "";
		const gchar * file = (folderNo < 2) ? dbiconfile : tableiconfile;

		if(folderNo == 0){
			/// DBs
			if(g_strcmp0(validdb, "db") == 0){
				file = "/usr/share/icons/hicolor/scalable/apps/dbicon.png"  ;
				//file = dbiconfile ;
			}
			if(g_strcmp0(validdb, "invaliddb") == 0){
				//file = "/usr/share/icons/hicolor/scalable/apps/sheetapp-dbinvalid.png";
				file = "/usr/share/icons/hicolor/scalable/apps/downloadr-invalid.png";
				//file = "/usr/share/icons/hicolor/scalable/apps/downloadr.png";
			}
		}
		else if(folderNo == 2){
			/// Data table
			file = tableiconfile;
		}
		else if(folderNo == 1){
			/// Groups
			if( g_strcmp0(type, "Tables") == 0 ){
				file = "/usr/share/icons/hicolor/scalable/apps/sheetapp-tables.png";
			}
			else if( g_strcmp0(type, "Views") == 0 ){
				file = "/usr/share/icons/hicolor/scalable/apps/sheetapp-views.png";
			}
			else if( g_strcmp0(type, "Triggers") == 0 ){
				file = "/usr/share/icons/hicolor/scalable/apps/sheetapp-triggers.svg";
			}
			else if( g_strcmp0(type, "Invalid Database") == 0 ){
				file = "/usr/share/icons/hicolor/scalable/apps/sheetapp.svg";
			}
			else {
				/// Indices
				file = "/usr/share/icons/hicolor/scalable/apps/sheetapp-indices.png";
			}
		}

		if( g_strcmp0(type, "sqlite_master") == 0 ){
			file = "/usr/share/icons/hicolor/scalable/apps/sheetapp-sqlitemaster.svg";
		}

		GError *error = NULL;
	  GdkPixbuf *icon = gdk_pixbuf_new_from_file_at_size (file, 24, 24, &error);
	  g_object_set (cell, "pixbuf", icon, NULL);
	};


	/// Column 1 - Pixbuf & Text combined column.
	GtkCellRenderer * pb = gtk_cell_renderer_pixbuf_new ();
	GtkCellRenderer * tx = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_set_alignment (tx, 0.0, 0.5);
	GtkTreeViewColumn * col_pbtx = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_resizable (col_pbtx, TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
	gtk_tree_view_column_set_spacing (col_pbtx, 2);
	gtk_tree_view_column_set_expand (col_pbtx, TRUE);
	//--
	gtk_tree_view_column_pack_start (col_pbtx, pb, FALSE);
	gtk_tree_view_column_pack_start (col_pbtx, tx, TRUE);
	gtk_tree_view_column_set_cell_data_func (col_pbtx, pb, set_tree_cell_pixbuf,  NULL, NULL);
	gtk_tree_view_column_set_cell_data_func (col_pbtx, tx, set_tree_cell_text,  NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col_pbtx);

	/// Column 2 - filesize.
	//GtkCellRenderer * sz = gtk_cell_renderer_text_new ();
	//GtkTreeViewColumn * col_sz = gtk_tree_view_column_new_with_attributes ("Size",  sz, "text", 3,   NULL);
	//gtk_tree_view_column_set_cell_data_func (col_sz, sz, filesize_Kib,  NULL, NULL);
	//gtk_tree_view_column_set_resizable (col_sz, TRUE);
	//gtk_cell_renderer_set_alignment (sz, 1.0, 0.5);
	//gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col_sz);

	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview)), GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_reorderable ( GTK_TREE_VIEW (treeview), TRUE);
	gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (treeview), FALSE);

	/// Events
	//g_signal_connect (GTK_TREE_MODEL(treestore), "row-inserted", G_CALLBACK(on_drag_data_inserted), NULL);
	//g_signal_connect (GTK_TREE_MODEL(treestore), "row-changed", G_CALLBACK(on_row_changed), NULL);
	//g_signal_connect (GTK_TREE_VIEW(treeview), "row-activated", G_CALLBACK(on_dblclick_row), NULL);
	g_signal_connect (GTK_TREE_VIEW(treeview), "button-press-event", (GCallback) treeview_click, NULL);

	/// Populate right click context menu
	//table = g_hash_table_new(g_str_hash, g_str_equal);
	//populate_context_menu_items();

	//gtk_tree_view_set_activate_on_single_click
	                               //(GtkTreeView *tree_view,
	                                //gboolean single);
	add_css_class_to_widget(treeview, "treeview");
	gtk_container_add(GTK_CONTAINER(treeview_scrollwin), treeview);

	populate_treeview ();
}



/// /////////////
/// make grid
/// /////////////
void                make_grid(std::string itemname){


	if(itemname == "" &&
	   APP_STATE->DBPATH == APP_STATE->APP_DEFAULTDB &&
	   DBSQLITE->db_select("SELECT COUNT(id) FROM databases")[0][0] == "0"
	   ){

		get_data_emptyDB();
		return;
	}



	/// if db is Master.sqlite, there is only 1 db, table is not sqlite_master, and tables length = 0
	if(itemname != "sqlite_master" &&
	  // APP_STATE->current_dbnodes_count == 1 &&
	   APP_STATE->DBPATH == APP_STATE->APP_DEFAULTDB &&
	   DBSQLITE->db_select("SELECT COUNT(id) FROM databases")[0][0] == "0"
	   ){

		/// show emptyDB page

		GList *gl = gtk_container_get_children ((GtkContainer *)rightpane_vbox);
		for(guint i = 0; i < g_list_length (gl) - 1; i++)
			gtk_widget_destroy((GtkWidget*)g_list_nth_data (gl,i));

		if(GRID){
			GRID = NULL;
			delete GRID;
		}

		get_data_emptyDB();
		gtk_widget_show_all(GTK_WIDGET(window));

		return;
	}

	else {

		/// show grid table

		if(! GRID){
			GList *gl = gtk_container_get_children ((GtkContainer *)rightpane_vbox);
			for(guint i = 0; i < g_list_length (gl) - 1; i++)
				gtk_widget_destroy((GtkWidget*)g_list_nth_data (gl,i));

			GRID = new GGRID(DATAS);
			gtk_box_pack_start(GTK_BOX(rightpane_vbox), GRID->grid, TRUE, TRUE, 0);
		}


		/// change content of grid according to passed in new DATAS
		/// DATAS is set implicitly in get_data(), and on treeview item click
		GRID->init_content(DATAS);
		gtk_widget_show_all(GTK_WIDGET(GRID->grid));

		return;
	}

}




void                draw_main_window (){

		loadCSS ();


		/// Window
		window = gtk_application_window_new (GTK_APPLICATION (app));
	  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
		int scrWidth  = gdk_screen_get_width (gdk_screen_get_default ());
		int scrHeight = gdk_screen_get_height (gdk_screen_get_default ());
		int window_width  = scrWidth * .85;
		int window_height = scrHeight * .85;
	  gtk_window_set_default_size (GTK_WINDOW (window), window_width, window_height);
		gtk_window_set_icon_name (GTK_WINDOW (window), "sqlitebrowser");

		/// headerbar
		headerbar = gtk_header_bar_new ();
		gtk_window_set_titlebar (GTK_WINDOW(window), headerbar);
		gtk_header_bar_set_show_close_button ((GtkHeaderBar *)headerbar, TRUE);
		gtk_header_bar_set_title ((GtkHeaderBar *)headerbar, "Sqlite Browser");
		gtk_header_bar_set_subtitle ((GtkHeaderBar *)headerbar, " ");


		/// sidebar button
		auto show_sidebar = [](){
			gboolean revealed = gtk_revealer_get_child_revealed (GTK_REVEALER(revealer));
			gtk_revealer_set_reveal_child (GTK_REVEALER(revealer), ! revealed);
		};
	  GtkWidget *sidebar_btn = gtk_button_new_from_icon_name ("open-menu-symbolic", GTK_ICON_SIZE_MENU);
		gtk_button_set_relief (GTK_BUTTON(sidebar_btn), GTK_RELIEF_NONE);
		gtk_header_bar_pack_start (GTK_HEADER_BAR(headerbar), sidebar_btn);
		g_signal_connect (sidebar_btn, "clicked", G_CALLBACK(show_sidebar), NULL);


	  /// add database button
	  GtkWidget *adddatabase_btn = gtk_button_new_from_icon_name ("document-new-symbolic", GTK_ICON_SIZE_MENU);
		gtk_button_set_relief (GTK_BUTTON(adddatabase_btn), GTK_RELIEF_NONE);
		gtk_header_bar_pack_start (GTK_HEADER_BAR(headerbar), adddatabase_btn);
		g_signal_connect (adddatabase_btn, "clicked", G_CALLBACK(open_file_chooser), NULL);



	  /// HBox to split window into Treeview on left and Gridsheet on right
	  globalhbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	  gtk_container_add(GTK_CONTAINER(window), globalhbox);


		/// left pane - treeview
		revealer = gtk_revealer_new ();
		gtk_revealer_set_transition_type (GTK_REVEALER(revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
		gtk_revealer_set_reveal_child (GTK_REVEALER(revealer), TRUE);
		gtk_box_pack_start(GTK_BOX(globalhbox), revealer, FALSE, TRUE, 0);

		GtkWidget *frame1 = gtk_frame_new (NULL);
		gtk_widget_set_size_request(frame1, 260, -1);
	  gtk_container_add(GTK_CONTAINER(revealer), frame1);

	  treeview_scrollwin = gtk_scrolled_window_new (NULL, NULL);
	  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(treeview_scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	  gtk_container_add(GTK_CONTAINER(frame1), treeview_scrollwin);
	  /// ------------

		/// right pane - grid
	  rightpane_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	  gtk_box_pack_start(GTK_BOX(globalhbox), rightpane_vbox, TRUE, TRUE, 0);



	  statusbar = gtk_statusbar_new ();
	  gtk_statusbar_push (GTK_STATUSBAR( statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR( statusbar), ""), "");
	  gtk_box_pack_end(GTK_BOX(rightpane_vbox), statusbar, FALSE, TRUE, 0);


	  gtk_widget_show_all(GTK_WIDGET(window));
		gtk_application_add_window (app, GTK_WINDOW(window));
}





