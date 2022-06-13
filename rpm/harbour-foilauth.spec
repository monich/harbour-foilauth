Name:           harbour-foilauth
Summary:        HMAC-Based One-Time Password generator
Version:        1.1.1
Release:        1
License:        BSD
Group:          Applications/Internet
URL:            https://github.com/monich/harbour-foilauth
Source0:        %{name}-%{version}.tar.gz

Requires:       sailfishsilica-qt5
Requires:       qt5-qtsvg-plugin-imageformat-svg
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libcrypto)
BuildRequires:  pkgconfig(sailfishapp)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  qt5-qttools-linguist

%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

%description
HMAC-Based One-Time Password generator compatible with Google
One-Time Password authentication mechanism.

%if "%{?vendor}" == "chum"
Categories:
 - Utility
Icon: https://raw.githubusercontent.com/monich/harbour-foilauth/master/icons/harbour-foilauth.svg
Screenshots:
- https://home.monich.net/chum/harbour-foilauth/screenshots/screenshot-001.png
- https://home.monich.net/chum/harbour-foilauth/screenshots/screenshot-002.png
- https://home.monich.net/chum/harbour-foilauth/screenshots/screenshot-003.png
- https://home.monich.net/chum/harbour-foilauth/screenshots/screenshot-004.png
- https://home.monich.net/chum/harbour-foilauth/screenshots/screenshot-005.png
- https://home.monich.net/chum/harbour-foilauth/screenshots/screenshot-006.png
- https://home.monich.net/chum/harbour-foilauth/screenshots/screenshot-007.png
- https://home.monich.net/chum/harbour-foilauth/screenshots/screenshot-008.png
- https://home.monich.net/chum/harbour-foilauth/screenshots/screenshot-009.png
- https://home.monich.net/chum/harbour-foilauth/screenshots/screenshot-010.png
- https://home.monich.net/chum/harbour-foilauth/screenshots/screenshot-011.png
Url:
  Homepage: https://openrepos.net/content/slava/foil-auth
%endif

%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5 CONFIG+=openrepos %{name}.pro
%qtc_make %{?_smp_mflags}

%check
make %{?_smp_mflags} -C test test

%install
rm -rf %{buildroot}
%qmake5_install

desktop-file-install --delete-original \
  --dir %{buildroot}%{_datadir}/applications \
   %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/jolla-settings/entries/%{name}.json
%{_datadir}/translations/%{name}*.qm
