#!/bin/sh

# autogen.sh - generate makefiles and configure.ac.

# usage:
# run these commands from a terminal:
#    ./autogen.sh [arguments]    eg. ./autogen.sh --prefix=/usr
#    make
#    make install









srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.


PROGNAME="sqlite-browser"
PROGVER="0.2"
MAINTAINER="fiskap@protonmail.com"


datadir="\$(datadir)"
DESTDIR="\$(DESTDIR)"
gtk_update_icon_cache="\$(gtk_update_icon_cache)"
TAB="$(printf '\t')"
iconsdir="\$(iconsdir)"
desktop_DATA="\$(desktop_in_data:.desktop.in=.desktop)"


declare -a SOURCES=()
for s in src/*.c;   do [ -f "$s" ] && SOURCES+=($(basename "$s")); done
for s in src/*.cpp; do [ -f "$s" ] && SOURCES+=($(basename "$s")); done
for s in src/*.h;   do [ -f "$s" ] && SOURCES+=($(basename "$s")); done


cat > configure.ac << FOE
AC_INIT([$PROGNAME], [$PROGVER], [$MAINTAINER])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX
AC_CONFIG_FILES([
 Makefile
 src/Makefile
 data/Makefile
 pixmaps/Makefile
])
dnl Test for pkg-config
PKG_PROG_PKG_CONFIG([0.22])
dnl Test for gtk
PKG_CHECK_MODULES([GTK], [gtk+-3.0])
dnl Test for sqlite
PKG_CHECK_MODULES([SQLITE], [sqlite3])
AC_OUTPUT
FOE


cat > pixmaps/Makefile.am << FOE
iconsdir = ${datadir}/icons/hicolor/scalable/apps
nobase_dist_icons_DATA =  \
   dbicon.png \
   downloadr-invalid.png \
   sqlitebrowser.png
gtk_update_icon_cache = gtk-update-icon-cache -f -t ${iconsdir}
install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
${TAB}@-if test -z "${DESTDIR}"; then echo "Updating Gtk icon cache."; ${gtk_update_icon_cache}; else echo "*** Icon cache not updated.  After (un)install, run this:"; echo "***   ${gtk_update_icon_cache}"; 	fi
FOE


cat > data/Makefile.am << FOE
# The desktop files
desktopdir = ${datadir}/applications
dist_desktop_DATA = ${PROGNAME}.desktop
mimedir = ${datadir}/mime/packages
dist_mime_DATA = ${PROGNAME}.xml
FOE


cat > src/Makefile.am << FOE
# what flags you want to pass to the C compiler & linker
AM_CXXFLAGS = -Wall -g -std=c++11  -O3 -pedantic  @GTK_CFLAGS@
AM_LDFLAGS =  @GTK_LIBS@ @SQLITE_LIBS@
bin_PROGRAMS = ${PROGNAME}
sqlite_browser_SOURCES = ${SOURCES[@]}
FOE

cat > Makefile.am << FOE
SUBDIRS = src data pixmaps
FOE




if test -z "$*" -a "$NOCONFIGURE" != 1; then
	echo "**Warning**: I am going to run \`configure' with no arguments."
	echo "If you wish to pass any to it, please specify them on the"
	echo \`$0\'" command line."
	echo
fi

echo "Processing configure.ac"


aclocal
autoconf
automake --add-missing --copy --gnu

if [ "$NOCONFIGURE" = 1 ]; then
    echo "Done. configure skipped."
    exit 0;
fi
echo "Running $srcdir/configure $@ ..."
$srcdir/configure "$@" && echo "Now type 'make' to compile." || exit 1




