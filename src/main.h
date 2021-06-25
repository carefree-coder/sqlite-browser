/*
 * main.h - SQLite Browser
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


extern GtkApplication *app;

struct AAPP_STATE{
	std::string APP_DEFAULTDB;
	std::string DBPATH;
	std::vector<std::string> dbpaths;

	guint current_dbnodes_count;

	/// constructor
	AAPP_STATE(std::string defaultdb) {
		this->APP_DEFAULTDB = defaultdb;
		this->current_dbnodes_count = 0;
		this->dbpaths = {};
	}

};
extern AAPP_STATE *APP_STATE;
