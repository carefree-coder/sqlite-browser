
#Browse SQLite databases

SQLite Browser is a GTK desktop application to visually browse SQLite databases.

It has the following features:

 - written using a mixture of C and C++. The intention is for it to be fast and lightweight.
 - can handle an unlimited number of rows via virtual tables, it has been tested up to 100 million rows.
 - stores multiple databases in its central Master.sqlite db.
 - can be launched standalone or on double clicking a .sqlite or .db file.

It does not (yet) have the feature to create or edit new databases. Currently, the data in individual cells can be copied.



Installation
============
<code>
git clone https://github.com/carefree-coder/sqlite-browser.git<br>
cd sqlite-browser <br>
./autogen.sh [arguments]    eg: ./autogen.sh --prefix=/usr<br>
make<br>
make install<br>
</code>

- the application should appear in your main menu.
- Or, you can type "<b>sqlite-browser</b>" from a command line.
- Or, you can double-click on any sqlite database file to launch the app.


Pre-compiled rpm and deb binaries will be made available, and also a tarball.

#Screenshots
<img src="https://github.com/carefree-coder/sqlite-browser/blob/main/screenshots/landing_page.png?raw=true "Optional Title width="480">
<img src="https://github.com/carefree-coder/sqlite-browser/blob/main/screenshots/northwind_sqlite.png?raw=true "Optional Title width="480">
<img src="https://github.com/carefree-coder/sqlite-browser/blob/main/screenshots/international_characters_.png?raw=true "Optional Title width="480">
<img src="https://github.com/carefree-coder/sqlite-browser/blob/main/screenshots/sakila_db.png?raw=true "Optional Title width="480">
<img src="https://github.com/carefree-coder/sqlite-browser/blob/main/screenshots/adwaita_light.png?raw=true "Optional Title width="480">




