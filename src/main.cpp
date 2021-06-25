/*
 * main.cpp - SQLite Browser
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
#include "main.h"
#include "window_gui.h"


GtkApplication *app;
AAPP_STATE *APP_STATE;


static void startup (GtkApplication *app, gpointer  user_data) {

	/// create user config dir
	gchar *confdir = g_strconcat (g_get_user_config_dir(), G_DIR_SEPARATOR_S, "sqlite-browser", NULL);
	if ( ! g_file_test (confdir, G_FILE_TEST_IS_DIR)){
		g_mkdir_with_parents (confdir, 0644);
	}

	/// default db
	std::string defaultDB = g_strconcat(confdir, G_DIR_SEPARATOR_S, "Master.sqlite", NULL);
	set_db_path(defaultDB);

	/// if master db dosnt yet exist, create it
	if( ! g_file_test(defaultDB.c_str(), G_FILE_TEST_EXISTS)){
		std::string sql = "CREATE TABLE IF NOT EXISTS [databases] (\n";
		sql += "  [id] INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\n";
		sql += "  [database_path] NVARCHAR(500) NOT NULL UNIQUE\n";
		sql += ")";
		int res = DBSQLITE->db_command(sql);

		if(res != 0){
			std::cout << "error creating master db" << std::endl;
			return;
		}
	}


	APP_STATE = new AAPP_STATE(defaultDB);
	APP_STATE->DBPATH = APP_STATE->APP_DEFAULTDB;


	draw_main_window();

	init_datas();
	get_data();

	make_treeview();
	make_grid();


	/// First run, add dbs in Master table to treeview
	if(APP_STATE->current_dbnodes_count == 1 && APP_STATE->DBPATH == APP_STATE->APP_DEFAULTDB)
		get_data0();

	g_free(confdir);
}

static void activate (GtkApplication *app, gpointer  user_data) {
	GtkWindow * window = gtk_application_get_active_window (app);
	gtk_widget_show_all(GTK_WIDGET(window));
	gtk_window_present (window);
	gtk_window_activate_focus (window);
}

static void open (GApplication *app, GFile **files, gint n_files, const gchar *hint) {

	/// Application launched with arguments

	/// Verify argv[1] is indeed a sqlite3 db file
	if( ! verify_db_is_valid_unquiet(files[0])){

		/// If supplied argv[1] is not a valid file, or a valid sqlite db
		/// errors will be printed and "verify_db_is_valid_unquiet" will have returned false.

		/// Not a valid db file, check default is not already running - if it is, do nothing.
		int i = 0;
		for(auto x : APP_STATE->dbpaths){
			if(x == APP_STATE->APP_DEFAULTDB)
				return;
			i++;
		}

		/// If default db not already running, Use default db
		APP_STATE->DBPATH = APP_STATE->APP_DEFAULTDB;
	}
	else {
		/// All good, we have a validated passed in db

		/// Check that db is not already displayed
		int i = 0;
		for(auto x : APP_STATE->dbpaths){
			if(x == (std::string)g_file_get_parse_name (files[0])){
				get_data00(i); // already displayed, open that db
				gtk_window_present (GTK_WINDOW(window));
				gtk_window_activate_focus (GTK_WINDOW(window));
				return;
			}
			i++;
		}

		APP_STATE->DBPATH = (std::string)g_file_get_parse_name (files[0]);
	}

	/// Open APP_STATE->DBPATH.
	get_data();
	make_treeview ();
	make_grid();
  return;
}

int main (int argc, char **argv) {

  int status;
	app = gtk_application_new ("org.carefree.sqlite-browser", G_APPLICATION_HANDLES_OPEN);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  g_signal_connect (app, "startup", G_CALLBACK (startup), GUINT_TO_POINTER(argc));
  g_signal_connect (app, "open", G_CALLBACK (open), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
