Name: soundux
Version: 0.2.4
Release: 1%{?dist}
Summary: A cross-platform soundboard

Group: Applications/System
License: GPL
URL: https://%{name}.rocks

Source0: https://github.com/Soundux/Soundux/releases/download/%{version}/%{name}-%{version}.tar.gz

BuildRequires: git webkit2gtk3-devel ninja-build gtk3-devel libwnck3-devel pulseaudio-libs-devel pipewire-devel libappindicator-gtk3-devel libXi-devel clang llvm cmake openssl-devel
Requires: webkit2gtk3 libwnck3 libappindicator-gtk3 redhat-lsb-core
Suggests: ffmpeg youtube-dl

%description
A universal soundboard that uses PulseAudio modules or PipeWire linking

%global debug_package %{nil}

%prep
%autosetup -n Soundux


%build
git submodule update --init --recursive
mkdir -p build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja


%install
cd build
DESTDIR=%{buildroot} ninja install
mkdir -p %{buildroot}/usr/bin
ln -s /opt/soundux/soundux %{buildroot}/usr/bin/soundux


%files
%license LICENSE
%doc README.md
/opt/%{name}
/usr/bin/soundux
/usr/share/applications/soundux.desktop
/usr/share/pixmaps/soundux.png
/usr/share/metainfo/io.github.Soundux.metainfo.xml


%changelog

