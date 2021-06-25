/*
 * sqlite.h - SQLite Browser
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



typedef std::vector<std::string> sqliteROW;

struct DATA{

	std::string global_sqlite_errmsg;

	std::vector<sqliteROW> result;
	std::vector<std::string> column_names;
	std::vector<std::string> column_types;
	std::vector<GtkWidget*> column_header_buttons;

	std::vector<gfloat> column_widths;
	std::vector<gfloat> original_column_widths;

	GtkWidget *end_button;

	/// constructor
	DATA() {

		this->global_sqlite_errmsg = "";
		this->result = {};
		this->column_names = {};
		this->column_types = {};
		this->column_widths = {};
		this->original_column_widths = {};
	}
};


int sqlite_select(char *dbpath, char *sql, DATA *);
int sqlite_command(char *dbpath, char *sql);
std::vector <std::vector<std::string>>	sqlite_query(std::string dbpath, std::string sql);
