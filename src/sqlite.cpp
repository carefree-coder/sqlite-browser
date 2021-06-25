/*
 * sqlite.cpp - SQLite Browser
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

#include <sqlite3.h>


DATA *localDATAS;

static int select_callback(void *NotUsed, int argc, char **argv, char **azColName){

	std::vector<std::string> row;
	localDATAS->column_names = {};

	int i;
	for(i = 0; i < argc; i++){

		localDATAS->column_names.push_back(azColName[i]);

		std::string v = argv[i] ? argv[i] : "NULL";
		row.push_back(v);
	}

	/// adding the original 'index' of row as a column to the end
	/// NB: column_names vector is NOT affected so original column processing in grid remains.
	row.push_back(std::to_string(localDATAS->result.size()));
	localDATAS->result.push_back(row);

	return 0;
}

int sqlite_select(char *dbpath, char *sql, DATA *DATAS){

	/// used to populate passed in DATAS


	localDATAS = new(DATA);
	DATAS->result = {};

	DATAS->global_sqlite_errmsg = "";
	DATAS->result = {};
	DATAS->column_names = {};
	DATAS->column_types = {};

	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;

	/// open db - This command creates the db if it dosnt exist
	rc = sqlite3_open(dbpath, &db);
	if( rc ){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}

	/// execute command
	rc = sqlite3_exec(db, sql, /*NULL*/select_callback, 0, &zErrMsg);
	if( rc != SQLITE_OK ){
		DATAS->global_sqlite_errmsg = (std::string)zErrMsg;
		sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return 1;
	}

	/// Set datas in DATAS struct
	DATAS->result = localDATAS->result;
	DATAS->column_names = localDATAS->column_names;
	DATAS->column_types = localDATAS->column_types;

	sqlite3_close(db);
	delete localDATAS;

	return 0;
}










