/*
 * utils.h - SQLite Browser
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


#include <gtk/gtk.h>
#include <iostream>
#include <vector>
#include <map>
#include <string.h>
#include <utility>

#include <sqlite3.h>


extern GtkCssProvider *provider;
void loadCSS ();
void add_css_class_to_widget(GtkWidget *w, const gchar *classname);
void set_db_path(std::string s);

#ifndef UTILS_H  /// to make sure class DB_IMPLEMENT is defined only once
#define UTILS_H


/// ============================================================================
/// Class implement SQLite database functions to select, insert, update, delete.
/// ============================================================================
class DB_IMPLEMENT {

	private :
		std::string DBPATH;
		std::string global_sqlite_errmsg;
		std::vector <std::vector<std::string>> select_query_result;

		static int select_query_callback(void *NotUsed, int argc, char **argv, char **azColName){

			std::vector<std::string> row;

			for(int i = 0; i < argc; i++){
				std::string v = argv[i] ? argv[i] : "NULL";
				row.push_back(v);
			}

			std::vector <std::vector<std::string>>* select_query_result = reinterpret_cast<std::vector <std::vector<std::string>>*>(NotUsed);

			select_query_result->push_back(row);

			return 0;
		}

		/// just execute, such as INSERT, or UPDATE queries
		int sqlite_command(char *sql){

			/// Receive sql and execute it (such as INSERT) - only return success or fail

			this->global_sqlite_errmsg = "";

			sqlite3 *db;
			char *zErrMsg = 0;
			int rc;

			/// open db - This command creates the db if it dosnt exist
			rc = sqlite3_open((char*)this->DBPATH.c_str(), &db);
			if( rc ){
				fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
				sqlite3_close(db);
				return 1;
			}

			/// execute command
			rc = sqlite3_exec(db, sql, NULL/*callback*/, 0, &zErrMsg);
			if( rc != SQLITE_OK ){
				fprintf(stderr, "SQL error: %s\n", zErrMsg);
				this->global_sqlite_errmsg = (std::string)zErrMsg;
				sqlite3_free(zErrMsg);
				sqlite3_close(db);
				return 1;
			}

			sqlite3_close(db);
			return 0;
		}


	public :

		/// SELECT queries return a std::vector<std::string>>
		std::vector <std::vector<std::string>> db_select(std::string sql){

			this->select_query_result = {};

			sqlite3 *db;
			char *zErrMsg = 0;
			int rc;

			/// open db - This command creates the db if it dosnt exist
			rc = sqlite3_open((char*)this->DBPATH.c_str(), &db);
			if( rc ){
				fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
				sqlite3_close(db);
				std::vector<std::string> fail = {"Fail 1 - SQLite error msg is: ", sqlite3_errmsg(db)};
				this->select_query_result.push_back(fail);
				return this->select_query_result; // FAIL
			}

			/// execute command
			std::vector <std::vector<std::string>> *x = &this->select_query_result;
			//DB_IMPLEMENT *x = this; /// if you should want to send the whole object
			rc = sqlite3_exec(db, (char *)sql.c_str(), /*NULL*/DB_IMPLEMENT::select_query_callback, x, &zErrMsg);
			if( rc != SQLITE_OK ){
				sqlite3_free(zErrMsg);
				sqlite3_close(db);
				std::vector<std::string> fail = {"Fail 2 - SQLite error msg is: ", zErrMsg};
				this->select_query_result.push_back(fail);
				return this->select_query_result; // FAIL
			}

			sqlite3_close(db);

			return this->select_query_result;
		}

		/// just execute, such as INSERT, or UPDATE queries
		int db_insert(std::string sql){
			return this->sqlite_command((char *)sql.c_str());
		}

		int db_update(std::string sql){
			return this->sqlite_command((char *)sql.c_str());
		}

		int db_delete(std::string sql){
			return this->sqlite_command((char *)sql.c_str());
		}

		int db_command(std::string sql){
			return this->sqlite_command((char *)sql.c_str());
		}


	/// CONSTRUCTOR
	DB_IMPLEMENT(std::string DBPATH){
		this->DBPATH = DBPATH;
	}

};

extern DB_IMPLEMENT *DBSQLITE; /// defined in utils.cpp, thereby available throughout program to any 'module' that needs it
                               /// (since all modules #include "utils.h")

#endif
