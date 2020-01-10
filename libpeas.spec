%if 0%{?rhel} == 0
%global use_seed 1
%global seed_option --enable-seed
%else
%global use_seed 0
%global seed_option --disable-seed
%endif

%if 0%{?rhel} == 0
%global use_python3 1
%else
%global use_python3 0
%endif

Name:		libpeas
Version:	1.12.1
Release:	1%{?dist}
Summary:	Plug-ins implementation convenience library

Group:		System Environment/Libraries
License:	LGPLv2+
URL:		http://ftp.acc.umu.se/pub/GNOME/sources/libpeas/
Source0:	http://ftp.acc.umu.se/pub/GNOME/sources/%{name}/1.12/%{name}-%{version}.tar.xz

BuildRequires:	chrpath
BuildRequires:	gtk3-devel >= 3.0.0
BuildRequires:	pygobject3-devel
BuildRequires:	python-devel
%if %{use_python3}
BuildRequires:	python3-devel
%endif
BuildRequires:	intltool
BuildRequires:	libtool
%if %{use_seed}
BuildRequires:	seed-devel
%endif
BuildRequires:	gtk-doc
BuildRequires:	glade-devel

BuildRequires:	autoconf automake gnome-common

# For the girepository-1.0 directory
Requires:	gobject-introspection%{?_isa}

%description
libpeas is a convenience library making adding plug-ins support
to GTK+ and glib-based applications.

%package devel
Summary:	Development files for libpeas
Group:		Development/Libraries
Requires:	%{name}%{?_isa} = %{version}-%{release}

%description devel
This package contains development libraries and header files
that are needed to write applications that use libpeas.

%prep
%setup -q

%build
%configure %{seed_option}

make %{?_smp_mflags}

%install
make install DESTDIR=$RPM_BUILD_ROOT

rm $RPM_BUILD_ROOT/%{_libdir}/lib*.la	\
	$RPM_BUILD_ROOT/%{_libdir}/libpeas-1.0/loaders/lib*.la

# Remove rpath as per https://fedoraproject.org/wiki/Packaging/Guidelines#Beware_of_Rpath
chrpath --delete $RPM_BUILD_ROOT%{_bindir}/peas-demo
%if %{use_python3}
chrpath --delete $RPM_BUILD_ROOT%{_libdir}/libpeas-1.0/loaders/libpython3loader.so
%endif
chrpath --delete $RPM_BUILD_ROOT%{_libdir}/libpeas-1.0/loaders/libpythonloader.so
%if %{use_seed}
chrpath --delete $RPM_BUILD_ROOT%{_libdir}/libpeas-1.0/loaders/libseedloader.so
%endif
chrpath --delete $RPM_BUILD_ROOT%{_libdir}/libpeas-gtk-1.0.so
chrpath --delete $RPM_BUILD_ROOT%{_libdir}/peas-demo/plugins/helloworld/libhelloworld.so
chrpath --delete $RPM_BUILD_ROOT%{_libdir}/peas-demo/plugins/secondtime/libsecondtime.so

%find_lang libpeas

%post
/sbin/ldconfig
touch --no-create %{_datadir}/icons/hicolor >&/dev/null || :

%postun
/sbin/ldconfig
if [ $1 -eq 0 ]; then
  touch --no-create %{_datadir}/icons/hicolor >&/dev/null || :
  gtk-update-icon-cache %{_datadir}/icons/hicolor >&/dev/null || :
fi

%posttrans
gtk-update-icon-cache %{_datadir}/icons/hicolor >&/dev/null || :

%files -f libpeas.lang
%doc AUTHORS
%{_libdir}/libpeas*-1.0.so.*
%dir %{_libdir}/libpeas-1.0/
%dir %{_libdir}/libpeas-1.0/loaders
%{_libdir}/libpeas-1.0/loaders/libpythonloader.so
%if %{use_python3}
%{_libdir}/libpeas-1.0/loaders/libpython3loader.so
%endif
%if %{use_seed}
%{_libdir}/libpeas-1.0/loaders/libseedloader.so
%endif
%{_libdir}/girepository-1.0/*.typelib
%{_datadir}/icons/hicolor/*/actions/libpeas-plugin.*

%files devel
%{_bindir}/peas-demo
%{_includedir}/libpeas-1.0/
%{_libdir}/peas-demo/
%dir %{_datadir}/gtk-doc/
%dir %{_datadir}/gtk-doc/html/
%{_datadir}/gtk-doc/html/libpeas/
%{_libdir}/libpeas*-1.0.so
%{_datadir}/gir-1.0/*.gir
%{_libdir}/pkgconfig/*.pc
%{_datadir}/glade/catalogs/libpeas-gtk.xml

%changelog
* Wed Jun 24 2015 Ray Strode <rstrode@redhat.com>- 1.12.1-1
- Update to 1.12.1

* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 1.8.0-5
- Mass rebuild 2014-01-24

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 1.8.0-4
- Mass rebuild 2013-12-27

* Tue Oct  8 2013 Matthias Clasen <mclasen@redhat.com> - 1.8.0-3
- Actually apply the patch (related: #884531)

* Mon Jun 17 2013 Bastien Nocera <bnocera@redhat.com> 1.8.0-2
- Fix possible crasher (#917731)

* Tue Mar 26 2013 Ignacio Casal Quinteiro <icq@gnome.org> - 1.8.0-1
- Update to 1.8.0

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.7.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Sun Jan 06 2013 Ignacio Casal Quinteiro <icq@gnome.org> - 1.7.0-1
- Update to 1.7.0

* Wed Nov 28 2012 Kalev Lember <kalevlember@gmail.com> - 1.6.2-1
- Update to 1.6.2
- Avoid runtime deps on gtk-doc (#754495)

* Mon Nov 19 2012 Bastien Nocera <bnocera@redhat.com> 1.6.1-2
- Fix source URL

* Tue Oct 16 2012 Ignacio Casal Quinteiro <icq@gnome.org> - 1.6.1-1
- Update to 1.6.1

* Tue Sep 25 2012 Ignacio Casal Quinteiro <icq@gnome.org> - 1.6.0-1
- Update to 1.6.0

* Wed Sep 19 2012 Bastien Nocera <bnocera@redhat.com> 1.5.0-1
- Disable vala, as it was disabled upstream:
http://git.gnome.org/browse/libpeas/commit/?id=1031aaeeef282ab2bb65cb6ae48fa4abff453c4d

* Wed Jul 18 2012 Ignacio Casal Quinteiro <icq@gnome.org> - 1.5.0-1
- Update to 1.5.0

* Thu May 03 2012 Kalev Lember <kalevlember@gmail.com> - 1.4.0-2
- Re-enable the GJS loader
- Remove unwanted lib64 rpaths

* Wed Mar 28 2012 Ignacio Casal Quinteiro <icq@gnome.org> - 1.4.0-1
- Update to 1.4.0

* Fri Mar  2 2012 Matthias Clasen <mclasen@redhat.com> - 1.3.0-2
- Make seed optional for RHEL

* Sat Feb 25 2012 Ignacio Casal Quinteiro <icq@gnome.org> - 1.3.0-1
- Update to 1.3.0

* Fri Jan 13 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.2.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Wed Sep 28 2011 Ray <rstrode@redhat.com> - 1.2.0-1
- Update to 1.2.0

* Wed Sep 28 2011 Ray <rstrode@redhat.com> - 1.2.0-1
- Update to 1.2.0

* Wed Sep 28 2011 Ray <rstrode@redhat.com> - 1.2.0-1
- Update to 1.2.0

* Tue Sep 27 2011 Ray <rstrode@redhat.com> - 1.2.0-1
- Update to 1.2.0

* Tue Sep 27 2011 Ray <rstrode@redhat.com> - 1.2.0-1
- Update to 1.2.0

* Tue Sep 27 2011 Ray <rstrode@redhat.com> - 1.2.0-1
- Update to 1.2.0

* Wed Aug 31 2011 Ignacio Casal Quinteiro <icq@gnome.org> - 1.1.3-1
- Update to 1.1.3

* Wed Aug 31 2011 Ignacio Casal Quinteiro <icq@gnome.org> - 1.1.2-2
- Rebuild for latest pygobject3

* Tue Aug 23 2011 Adam Williamson <awilliam@redhat.com> - 1.1.2-1
- Update to 1.1.2
- bump BR to pygobject3-devel

* Wed Aug 03 2011 Bastien Nocera <bnocera@redhat.com> 1.1.1-3
- Another attempt at building against the latest gjs

* Wed Aug 03 2011 Bastien Nocera <bnocera@redhat.com> 1.1.1-2
- Rebuild for newer gjs

* Tue Jul 26 2011 Matthias Clasen <mclasen@redhat.com> - 1.1.1-1
- Update to 1.1.1

* Tue Jun 14 2011 Tomas Bzatek <tbzatek@redhat.com> - 1.1.0-1
- Update to 1.1.0

* Mon Apr  4 2011 Christopher Aillon <caillon@redhat.com> 1.0.0-1
- Update to 1.0.0

* Sun Mar 27 2011 Bastien Nocera <bnocera@redhat.com> 0.9.0-1
- Update to 0.9.0

* Thu Mar 10 2011 Bastien Nocera <bnocera@redhat.com> 0.7.4-1
- Update to 0.7.4

* Tue Feb 22 2011 Matthias Clasen <mclasen@redhat.com> 0.7.3-1
- Update to 0.7.3
- Drop unneeded dependencies

* Mon Feb 21 2011 Bastien Nocera <bnocera@redhat.com> 0.7.2-1
- Update to 0.7.2

* Thu Feb 10 2011 Matthias Clasen <mclasen@redhat.com> 0.7.1-7
- Rebuild

* Wed Feb 09 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.7.1-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Wed Feb  2 2011 Matthias Clasen <mclasen@redhat.com> 0.7.1-5
- Rebuild against newer gtk

* Fri Jan 28 2011 Bastien Nocera <bnocera@redhat.com> 0.7.1-4
- Update to real 0.7.1 release

* Fri Jan  7 2011 Matthias Clasen <mclasen@redhat.com> - 0.7.1-3.gita2f98e
- Rebuild against newer gtk

* Fri Dec  3 2010 Matthias Clasen <mclasen@redhat.com> - 0.7.1-2.gita2f98e
- Rebuild against newer gtk

* Thu Nov 11 2010 Dan Williams <dcbw@redhat.com> - 0.7.1-1.gita2f98e
- Update to 0.7.1
- Fix some crashes with missing introspection data

* Mon Nov  1 2010 Matthias Clasen <mclasen@redhat.com> 0.7.0-2
- Rebuild against newer gtk3

* Mon Oct 04 2010 Bastien Nocera <bnocera@redhat.com> 0.7.0-1
- Update to 0.7.0

* Tue Sep 21 2010 Matthias Clasen <mclasen@redhat.com> - 0.5.5-2
- Rebuild against newer gobject-introspection

* Thu Aug 19 2010 Matthias Clasen <mclasen@redhat.com> - 0.5.5-1
- Update to 0.5.5

* Thu Aug  5 2010 Matthias Clasen <mclasen@redhat.com> - 0.5.4-1
- Update to 0.5.4

* Tue Jul 27 2010 Mamoru Tasaka <mtasaka@ioa.s.u-tokyo.ac.jp> 0.5.3-2
- Rebuild against python 2.7

* Fri Jul 23 2010 Bastien Nocera <bnocera@redhat.com> 0.5.3-1
- Update to 0.5.3

* Thu Jul 22 2010 Bastien Nocera <bnocera@redhat.com> 0.5.2-5
- Fix post scriplet (#615021)

* Thu Jul 15 2010 Colin Walters <walters@verbum.org> - 0.5.2-4
- Rebuild with new gobject-introspection

* Tue Jul 13 2010 Matthias Clasen <mclasen@redhat.com> 0.5.2-3
- Rebuild

* Mon Jul 12 2010 Colin Walters <walters@verbum.org> - 0.5.2-2
- Rebuild against new gobject-introspection

* Mon Jul 12 2010 Matthias Clasen <mclasen@redhat.com> 0.5.2-1
- Update to 0.5.2

* Thu Jul  8 2010 Matthias Clasen <mclasen@redhat.com> 0.5.1-2
- Rebuild

* Mon Jun 28 2010 Bastien Nocera <bnocera@redhat.com> 0.5.1-1
- Update to 0.5.1

* Thu Jun 24 2010 Bastien Nocera <bnocera@redhat.com> 0.5.0-4
- Document rpath work-arounds disabling, and remove verbose build

* Fri Jun 18 2010 Bastien Nocera <bnocera@redhat.com> 0.5.0-3
- Fix a number of comments from review request

* Mon Jun 14 2010 Bastien Nocera <bnocera@redhat.com> 0.5.0-2
- Call ldconfig when installing the package

* Mon Jun 14 2010 Bastien Nocera <bnocera@redhat.com> 0.5.0-1
- First package

