<div align="center">
  <p>
    <h1>
      <a href="#readme">
        <img src="icon.png" width="50" alt="Soundboard" />
      </a>
      <br />
      Soundux
    </h1>
    <h4>A cross-platform soundboard in QtQuick</h4>
  </p>
  <p>
    <a href="https://github.com/Soundux/Soundux/releases">
      <img src="https://img.shields.io/github/last-commit/Soundux/Soundux.svg?style=for-the-badge" alt="Last Commit" />
    </a>
    <a href="https://github.com/Soundux/Soundux/stargazers">
      <img src="https://img.shields.io/github/stars/Soundux/Soundux?style=for-the-badge" alt="Stars" />
    </a>
    <a href="https://github.com/Soundux/Soundux/blob/master/LICENSE">
      <img src="https://img.shields.io/github/license/Soundux/Soundux.svg?style=for-the-badge" alt="License" />
    </a>
    <a href="https://discord.gg/4HwSGN4Ec2">
      <img src="https://img.shields.io/discord/697348809591750706?label=Discord&style=for-the-badge" alt="Discord" />
    </a>
    <a href="https://github.com/Soundux/Soundux/actions?query=workflow%3A%22Build+on+Windows%22">
      <img src="https://img.shields.io/github/workflow/status/Soundux/Soundux/Build%20on%20Windows?label=Windows%20Build&style=for-the-badge" alt="Windows Build" />
    </a>
    <a href="https://github.com/Soundux/Soundux/actions?query=workflow%3A%22Build+on+Linux%22">
      <img src="https://img.shields.io/github/workflow/status/Soundux/Soundux/Build%20on%20Linux?label=Linux%20Build&style=for-the-badge" alt="Linux Build" />
    </a>
  </p>
  
  [![Packaging status](https://repology.org/badge/vertical-allrepos/soundux.svg)](https://repology.org/project/soundux/versions)
  
</div>

## Index
- [Introduction](#introduction)
- [Runtime Dependencies (for linux)](#runtime-dependencies-for-linux)
- [Compilation & Installation](#compilation--installation)
  - [Arch Linux and derivatives](#arch-linux-and-derivatives)
  - [Windows](#windows)
    - [Dependencies](#dependencies)
  - [Other distros](#other-distros)
    - [Dependencies](#dependencies-1)
    - [Install dependencies for Ubuntu and derivatives](#install-dependencies-for-ubuntu-and-derivatives)
    - [Build](#build)
    - [Install](#install)
- [Why _Soundux_?](#why-soundux)
- [License](#license)
- [TODO](#todo)

# Introduction
Soundux is a cross-platform soundboard that features a simple user interface.
With Soundux you can play audio to a specific application on linux and to your vb-audio-cable sink on windows.

# Runtime Dependencies (for linux)
Please refer to your distro instructions on how to install
- [pulseaudio](https://gitlab.freedesktop.org/pulseaudio/pulseaudio)
- Xorg

# Compilation & Installation

## Arch Linux and derivatives
You can install my package with your AUR helper of choice which will automatically compile and install the master branch
```sh
yay -S soundux-git
```

## Windows
*(We highly recommend you to just download the latest release for windows since it has all its dependencies packed with it)*

To compile on windows you'll have to install qt (*make sure the the important qt-paths are in your system-path!*)
### Dependencies
- [VB-Audio Cable](https://vb-audio.com/Cable/)
- [Qt](https://www.qt.io/download-thank-you?os=windows)
- MSVC
- CMake

After installing the dependencies you should be able to follow the normal build steps!

## Other distros

### Dependencies
This list may be not accurate. Contact me if you find missing dependencies that I can update this list
- [qt5-base](https://github.com/qt/qtbase)
- [qt5-tools](https://github.com/qt/qt5)
- [qt5-quickcontrols2](https://github.com/qt/qtquickcontrols2)

### Install dependencies for Ubuntu and derivatives
```sh
sudo apt install git build-essential cmake qt5-default libx11-dev libqt5x11extras5-dev qtquickcontrols2-5-dev qtdeclarative5-dev libxi-dev qml-module-qtquick2 qml-module-qtquick-controls qml-module-qtquick-controls2 qml-module-qtquick-dialogs qml-module-qtquick-layouts qml-module-qtquick-window2 qml-module-qt-labs-settings qml-module-qt-labs-folderlistmodel
```
<b>We recommend to use our Flatpak build instead! Building this from source on Ubuntu will probably not work because Ubuntu does not have Qt >=5.15 in its repos!</b>

### Build
Clone the repository
```sh
git clone https://github.com/Soundux/Soundux.git
cd Soundux
git submodule update --init --recursive
```
Create a build folder and start compilation
```sh
mkdir build
cd build
cmake ..
make
```
To start the program
```sh
./soundux
```

### Install
Automated Installation is currently not available but will be available in the future. (You may look into my [arch package](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=soundux-git) and figure it out for your distro)

# Why _Soundux_?

The project started as a **Sound**board for Lin**ux**

# License
The code is licensed under [GPLv3](LICENSE)

# TODO
- [ ] Playlist mode (play sounds one after another)
- [ ] Icons in Output application dropdown on linux
- [ ] Improve the logo
- [ ] Volume normalization
- [ ] Grid View
- [x] ~~Save window size~~
- [x] ~~Save volume states (+ Sync state)~~
- [ ] Add ability to move tabs
- [ ] Localization
- [ ] Package it as deb, Flatpak and AppImage
- [ ] Support macOS
- [ ] Fix FileDialog on windows (it's currently bugged and you have to just manually insert the path of the folder you want to add in the navigation bar)
