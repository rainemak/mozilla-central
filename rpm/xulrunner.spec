%define greversion 23.0a1

Name:       xulrunner
Summary:    XUL runner
Version:    %{greversion}
Release:    1
Group:      Applications/Internet
License:    Mozilla License
URL:        http://hg.mozilla.org/mozilla-central
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(QtCore) >= 4.6.0
BuildRequires:  pkgconfig(QtOpenGL)
BuildRequires:  pkgconfig(QtGui)
BuildRequires:  pkgconfig(xt)
BuildRequires:  pkgconfig(pango)
BuildRequires:  pkgconfig(alsa)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dbus-glib-1)
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(gstreamer-app-0.10)
BuildRequires:  pkgconfig(gstreamer-plugins-base-0.10)
BuildRequires:  pkgconfig(nspr) >= 4.9.6
BuildRequires:  pkgconfig(nss) >= 3.14.3
BuildRequires:  autoconf213
BuildRequires:  python
BuildRequires:  zip
BuildRequires:  unzip
%ifarch i586 i486 i386
BuildRequires:  yasm
%endif

%description
Mozilla XUL runner

%package devel
Group: Development/Tools/Other
Requires: xulrunner
Summary: Headers for xulrunner

%description devel
Development files for xulrunner.

%package misc
Group: Development/Tools/Other
Requires: xulrunner
Summary: Misc files for xulrunner

%description misc
Tests and misc files for xulrunner

%prep
%setup -q -n %{name}-%{version}

%build
cp -rf embedding/embedlite/config/mozconfig.merqtxulrunner mozconfig

%ifarch %arm
echo "ac_add_options --with-arm-kuser" >> mozconfig
echo "ac_add_options --with-float-abi=toolchain-default" >> mozconfig
# No need for this, this should be managed by toolchain
echo "ac_add_options --with-thumb=toolchain-default" >> mozconfig
%endif
echo "mk_add_options MOZ_MAKE_FLAGS='-j%jobs'" >> mozconfig
echo "export LD=ld.gold" >> mozconfig

export MOZCONFIG=mozconfig
%{__make} -f client.mk build_all %{?jobs:MOZ_MAKE_FLAGS="-j%jobs"}

%install
export MOZCONFIG=mozconfig
%{__make} -f client.mk install DESTDIR=%{buildroot}
for i in $(find %{buildroot}%{_libdir}/xulrunner-devel-%{greversion}/sdk/lib -name "*.so" -mindepth 1 -maxdepth 1 -type f); do
  BASENAMEF=$(basename $i)
  rm -f %{buildroot}%{_libdir}/xulrunner-%{greversion}/$BASENAMEF
  mv %{buildroot}%{_libdir}/xulrunner-devel-%{greversion}/sdk/lib/$BASENAMEF %{buildroot}%{_libdir}/xulrunner-%{greversion}/
  ln -s %{_libdir}/xulrunner-%{greversion}/$BASENAMEF %{buildroot}%{_libdir}/xulrunner-devel-%{greversion}/sdk/lib/$BASENAMEF
done
%{__chmod} -x %{buildroot}%{_libdir}/xulrunner-%{greversion}/*.so

%files
%defattr(-,root,root,-)
%attr(755,-,-) %{_bindir}/*
%dir %{_libdir}/xulrunner-%{greversion}/dictionaries
%{_libdir}/xulrunner-%{greversion}/*.so
%{_libdir}/xulrunner-%{greversion}/omni.ja
%{_libdir}/xulrunner-%{greversion}/dependentlibs.list
%{_libdir}/xulrunner-%{greversion}/dictionaries/*

%files devel
%defattr(-,root,root,-)
%{_datadir}/*
%{_libdir}/xulrunner-devel-%{greversion}
%{_libdir}/pkgconfig
%{_includedir}/*

%files misc
%defattr(-,root,root,-)
%{_libdir}/xulrunner-%{greversion}/*
%exclude %{_libdir}/xulrunner-%{greversion}/*.so
%exclude %{_libdir}/xulrunner-%{greversion}/omni.ja
%exclude %{_libdir}/xulrunner-%{greversion}/dependentlibs.list
%exclude %{_libdir}/xulrunner-%{greversion}/dictionaries/*
