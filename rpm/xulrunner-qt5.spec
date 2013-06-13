%define greversion 24.0a1

Name:       xulrunner-qt5
Summary:    XUL runner
Version:    %{greversion}
Release:    1
Group:      Applications/Internet
License:    Mozilla License
URL:        http://hg.mozilla.org/mozilla-central
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5OpenGL)
BuildRequires:  pkgconfig(Qt5Widgets)
BuildRequires:  pkgconfig(Qt5PrintSupport)
BuildRequires:  pkgconfig(pango)
BuildRequires:  pkgconfig(alsa)
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(gstreamer-app-0.10)
BuildRequires:  pkgconfig(gstreamer-plugins-base-0.10)
BuildRequires:  pkgconfig(nspr) >= 4.9.6
BuildRequires:  pkgconfig(nss) >= 3.14.3
BuildRequires:  qt5-qttools
BuildRequires:  qt5-default
BuildRequires:  autoconf213
BuildRequires:  automake
BuildRequires:  python
BuildRequires:  python-devel
BuildRequires:  zip
BuildRequires:  unzip
BuildRequires:  hunspell-devel
BuildRequires:  libjpeg-turbo-devel
%ifarch i586 i486 i386
BuildRequires:  yasm
%endif
BuildRequires:  fdupes

%description
Mozilla XUL runner

%package devel
Group: Applications/Internet
Requires: xulrunner-qt5
Summary: Headers for xulrunner

%description devel
Development files for xulrunner.

%package misc
Group: Applications/Internet
Requires: xulrunner-qt5
Summary: Misc files for xulrunner

%description misc
Tests and misc files for xulrunner

%prep
%setup -q -n %{name}-%{version}

%build
export DONT_POPULATE_VIRTUALENV=1
export PYTHONPATH=$PWD/python:$PWD/config:$PWD/build:$PWD/xpcom/typelib/xpt/tools
for i in $(find $PWD/python $PWD/testing/mozbase -mindepth 1 -maxdepth 1 -type d); do
  export PYTHONPATH+=:$i
done
export SBOX_REDIRECT_FORCE=/usr/bin/python
# hack for when not using virtualenv
ln -sf $PWD/obj-build-mer-qt-xr/config.status $PWD/build/config.status

cp -rf embedding/embedlite/config/mozconfig.merqtxulrunner mozconfig

%ifarch %arm
echo "ac_add_options --with-arm-kuser" >> mozconfig
echo "ac_add_options --with-float-abi=toolchain-default" >> mozconfig
# No need for this, this should be managed by toolchain
echo "ac_add_options --with-thumb=toolchain-default" >> mozconfig
%endif
echo "mk_add_options MOZ_MAKE_FLAGS='-j%jobs'" >> mozconfig
echo "export CFLAGS=\"\$CFLAGS -fuse-ld=gold \"" >> mozconfig
echo "export CXXFLAGS=\"\$CXXFLAGS -fuse-ld=gold \"" >> mozconfig
echo "export LD=ld.gold" >> mozconfig
echo "ac_add_options --disable-tests" >> mozconfig
echo "ac_add_options --enable-system-hunspell" >> mozconfig
echo "ac_add_options --disable-strip" >> mozconfig
echo "ac_add_options --disable-mochitest" >> mozconfig
echo "ac_add_options --disable-installer" >> mozconfig
echo "ac_add_options --disable-javaxpcom" >> mozconfig
echo "ac_add_options --disable-crashreporter" >> mozconfig
echo "ac_add_options --without-x" >> mozconfig
export MOZCONFIG=mozconfig
%{__make} -f client.mk build STRIP="/bin/true" %{?jobs:MOZ_MAKE_FLAGS="-j%jobs"}

%install
export DONT_POPULATE_VIRTUALENV=1
export PYTHONPATH=$PWD/python:$PWD/config:$PWD/build:$PWD/xpcom/typelib/xpt/tools
for i in $(find $PWD/python $PWD/testing/mozbase -mindepth 1 -maxdepth 1 -type d); do
  export PYTHONPATH+=:$i
done
export SBOX_REDIRECT_FORCE=/usr/bin/python

export MOZCONFIG=mozconfig
%{__make} -f client.mk install DESTDIR=%{buildroot}
%{__chmod} +x %{buildroot}%{_libdir}/xulrunner-%{greversion}/*.so
%fdupes -s %{buildroot}%{_includedir}
%fdupes -s %{buildroot}%{_libdir}
chmod +x %{buildroot}%{_libdir}/xulrunner-%{greversion}/*.so
# Use the system hunspell dictionaries
%{__rm} -rf ${RPM_BUILD_ROOT}%{_libdir}/xulrunner-%{greversion}/dictionaries
ln -s %{_datadir}/myspell ${RPM_BUILD_ROOT}%{_libdir}/xulrunner-%{greversion}/dictionaries

%files
%defattr(-,root,root,-)
%attr(755,-,-) %{_bindir}/*
%{_libdir}/xulrunner-%{greversion}/*.so
%{_libdir}/xulrunner-%{greversion}/omni.ja
%{_libdir}/xulrunner-%{greversion}/dependentlibs.list
%{_libdir}/xulrunner-%{greversion}/dictionaries

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
