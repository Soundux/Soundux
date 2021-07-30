%global debug_package   %{nil}
%global repo_url        https://github.com/Soundux/Soundux

Name:           soundux
Version:        0.2.7
Release:        1%{?dist}
Summary:        A cross-platform soundboard

License:        GPLv3+
URL:            https://soundux.rocks
Source0:        %{repo_url}/releases/download/%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.1
BuildRequires:  clang llvm ninja-build
BuildRequires:  gtk3-devel
BuildRequires:  libappindicator-gtk3-devel
BuildRequires:  libwnck3-devel
BuildRequires:  libXi-devel
BuildRequires:  openssl-devel
BuildRequires:  pipewire-devel
BuildRequires:  pulseaudio-libs-devel
BuildRequires:  webkit2gtk3-devel
BuildRequires:  desktop-file-utils libappstream-glib

Requires:       libappindicator-gtk3
Requires:       libwnck3
Requires:       (pulseaudio or pipewire-pulseaudio)
Requires:       redhat-lsb-core
Requires:       webkit2gtk3
Recommends:     ffmpeg youtube-dl

%description
A universal soundboard that uses PulseAudio modules or PipeWire linking

%prep
%autosetup -n Soundux

%build
mkdir -p build
cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

%install
cd build
DESTDIR=%{buildroot} ninja install
mkdir -p %{buildroot}%{_bindir}
ln -s /opt/soundux/soundux %{buildroot}%{_bindir}/%{name}

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/*.desktop
appstream-util validate-relax --nonet %{buildroot}%{_metainfodir}/*.xml

%files
%license LICENSE
%doc README.md
/opt/%{name}
%{_bindir}/soundux
%{_datadir}/applications/*.desktop
%{_datadir}/pixmaps/*.png
%{_metainfodir}/*.xml

%changelog
* Wed May 26 2021 Arvin Verain <acverain@up.edu.ph> - 0.2.6-1
- Initial COPR package
