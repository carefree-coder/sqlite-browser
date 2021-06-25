/*
 * window_gui.h - SQLite Browser
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


extern GtkWidget * window;
extern GtkWidget * sw;
extern GtkWidget * statusbar;

void init_datas();
gboolean verify_db_is_valid_unquiet(GFile *file);
gboolean verify_db_is_valid (GFile *file);
void get_data();
void get_data0();
void get_data00(int i);
void get_data_emptyDB();

void draw_main_window ();
void make_treeview ();
void make_grid(std::string itemname = "");



