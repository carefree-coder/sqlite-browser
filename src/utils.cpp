/*
 * utils.cpp - SQLite Browser
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

GtkCssProvider *provider;
DB_IMPLEMENT *DBSQLITE;


void loadCSS (){

	const gchar *ccss = "  \n\
\n\
\n\
.startBtn {  \n\
	padding: 20px;  \n\
	border-radius: 10px;  \n\
	color:lightgrey;  \n\
}  \n\
\n\
GtkScrolledWindow {  \n\
	padding-left: 0px;  \n\
	padding-top: 0px;  \n\
	background-color: transparent;  \n\
}  \n\
\n\
\n\
/** gridcells **/  \n\
.gridcell {  \n\
	background-color: white;  \n\
	color: black;  \n\
	padding: 5px;  \n\
	border-right: 0.5px solid black;  \n\
	border-bottom: 0.5px solid black;  \n\
	border-left: 0.5px solid white;  \n\
	border-top: 0.5px solid white;  \n\
	font-size: 13.5px;  \n\
}  \n\
.adjacent-right {  \n\
	border-left: 0.5px solid black;  \n\
}  \n\
.adjacent-below {  \n\
	border-top: 0.5px solid black;  \n\
}  \n\
.gridcell:selected {  \n\
	background-color: @theme_selected_bg_color;  \n\
	color: @theme_selected_fg_color;  \n\
	font-weight: 100;  \n\
}  \n\
.selectedCell {  \n\
	border: 3px solid black;  \n\
	padding: 2.5px;  \n\
}  \n\
\n\
\n\
\n\
\n\
\n\
/** buttons **/  \n\
.colHeaderEndsb {  border-radius:0; opacity:0}  \n\
.colHeaderEndsbO { border-radius:0; background-color: #313636; opacity:1}  \n\
.colHeader, .rowHeader {  \n\
	background-color: #8F8F8F;  \n\
	color: black;  \n\
	padding: 5px;  \n\
	border-right: 0.5px solid black;  \n\
	border-bottom: 0.5px solid black;  \n\
	font-size: 12px;  \n\
	border-radius: 0  \n\
}  \n\
.colHeader {border-left: 0.5px solid #B0B0B0; }  \n\
.colHeader:hover,  \n\
.rowHeader:hover {  \n\
	background-color: gray;  \n\
	background-image: none;  \n\
	border-top: 0.5px solid grey;  \n\
	border-left: 0.5px solid grey;  \n\
	color: black  \n\
}  \n\
.button:hover:active {background-color: grey; background-image:none;}  \n\
\n\
\n\
\n\
.entrycell {background:transparent; color:red; border:3px solid black;}  \n\
#entrycell {background:transparent; color:red; border:3px solid black;  font-size:12px;}  \n\
\n\
\n\
";


	gchar *confdir = g_strconcat (g_get_user_config_dir(), G_DIR_SEPARATOR_S, "sqlite-browser", NULL);
	gchar *gridsheetcss = g_strconcat(confdir, G_DIR_SEPARATOR_S, "gridsheet.css", NULL);


	if(! g_file_test(gridsheetcss, G_FILE_TEST_EXISTS)){
		g_file_set_contents (gridsheetcss, ccss, -1, NULL);
	}



	/// Use CSS
	provider = gtk_css_provider_new ();
	//gtk_css_provider_load_from_data (provider, ccss, -1, NULL);

	GError *error = NULL;
  gtk_css_provider_load_from_path(provider, gridsheetcss, &error);
	if (error)
		g_warning("CSS Error %d: %s", error->code, error->message);


	gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),  GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);



	g_free(confdir);
	g_free(gridsheetcss);

}

void add_css_class_to_widget(GtkWidget *w, const gchar *classname){
  GtkStyleContext *style_context = gtk_widget_get_style_context(w);
  gtk_style_context_add_class(style_context, classname);
  gtk_style_context_add_provider (style_context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
}

void set_db_path(std::string s){
	DBSQLITE = new DB_IMPLEMENT(s);
}
