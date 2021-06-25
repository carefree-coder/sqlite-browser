Name:           sqlite-browser
Version:        0.2
Release:        2%{dist}
Summary:        Browse SQLite databases
License:        GPLv2
URL:            http://carefree.com/gnu/%{name}
Source:         %{URL}/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:  gtk3-devel sqlite-devel
Requires:       gtk3 sqlite

%Description
SQLite Browser is a desktop application for viewing the contents of SQLite
databases. It is written in Gtk C/C++ and its only dependencies are gtk and
sqlite.

%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
%make_install


%files
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/mime/packages/%{name}.xml
%{_datadir}/icons/hicolor/scalable/apps/*.png



%post
/sbin/ldconfig
touch --no-create %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
  %{_bindir}/gtk-update-icon-cache --quiet -t %{_datadir}/icons/hicolor &>/dev/null || :
fi


echo "application/vnd.sqlite3=sqlite-browser.desktop" >> %{_datadir}/applications/defaults.list
echo "application/x-sqlite3=sqlite-browser.desktop" >> %{_datadir}/applications/defaults.list

update-mime-database %{_datadir}/mime


%postun
/sbin/ldconfig
touch --no-create %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
  %{_bindir}/gtk-update-icon-cache --quiet -t %{_datadir}/icons/hicolor &>/dev/null || :
fi



%doc

%changelog
* Thu Jun 24 2021 Fatih Iskap <fiskap@protonmail.com> - 0.2-2
- MVP release
