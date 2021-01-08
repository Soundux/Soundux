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
      <img src="https://img.shields.io/github/release/Soundux/Soundux.svg?style=flat-square" alt="Latest Stable Release" />
    </a>
    <a href="https://discord.gg/4HwSGN4Ec2">
      <img src="https://img.shields.io/discord/697348809591750706?label=discord&style=flat-square" alt="Discord" />
    </a>
    <a href="https://github.com/Soundux/Soundux/stargazers">
      <img src="https://img.shields.io/github/stars/Soundux/Soundux?style=flat-square" alt="Stars" />
    </a>
    <a href="https://github.com/Soundux/Soundux/blob/master/LICENSE">
      <img src="https://img.shields.io/github/license/Soundux/Soundux.svg?style=flat-square" alt="License" />
    </a>
    <br>
    <a href="https://github.com/Soundux/Soundux/actions?query=workflow%3A%22Build+on+Windows%22">
      <img src="https://img.shields.io/github/workflow/status/Soundux/Soundux/Build%20on%20Windows?label=windows%20build&style=flat-square" alt="Windows Build" />
    </a>
    <a href="https://github.com/Soundux/Soundux/actions?query=workflow%3A%22Build+on+Linux%22">
      <img src="https://img.shields.io/github/workflow/status/Soundux/Soundux/Build%20on%20Linux?label=linux%20build&style=flat-square" alt="Linux Build" />
    </a>
    <a href="https://github.com/Soundux/Soundux/actions?query=workflow%3A%22Build+Flatpak%22">
      <img src="https://img.shields.io/github/workflow/status/Soundux/Soundux/Build%20Flatpak?label=flatpak%20build&style=flat-square" alt="Flatpak Build" />
    </a>
  </p>
</div>

# Preview
![Dark Interface](https://raw.githubusercontent.com/Soundux/soundux.github.io/master/src/assets/screenshots/1.png)

# Introduction
Soundux is a cross-platform soundboard that features a simple user interface.
With Soundux you can play audio to a specific application on linux and to your vb-audio-cable sink on windows.

# Runtime Dependencies
These are required to run the program

## Linux
Please refer to your distro instructions on how to install
- [pulseaudio](https://gitlab.freedesktop.org/pulseaudio/pulseaudio)
- Xorg
## Windows
- [VB-CABLE](https://vb-audio.com/Cable/) (Our installer automatically installs VB-CABLE)

# Installation

## Linux

### Arch Linux and derivatives
You can install our package with your AUR helper of choice which will automatically compile and install the latest release version
```sh
yay -S soundux
```
We also provide a `soundux-git` package which compiles from the master branch

### Other distros
You can grab the latest release from Flathub

<a
 href='https://flathub.org/apps/details/io.github.Soundux'><img 
width='240' alt='Download on Flathub' 
src='https://flathub.org/assets/badges/flathub-badge-en.png'/></a>

## Windows
Download our installer or portable from [the latest release](https://github.com/Soundux/Soundux/releases/latest)

# Compilation

## Build Dependencies

### Linux
This list may not be accurate. Contact me if you find missing dependencies that I can update this list
- [qt5-base](https://github.com/qt/qtbase) >=5.15
- [qt5-tools](https://github.com/qt/qt5) >=5.15
- [qt5-quickcontrols2](https://github.com/qt/qtquickcontrols2) >=5.15
- X11 client-side development headers

<b>Qt >= 5.15 is strictly required!</b>

#### Ubuntu and derivatives
```sh
sudo apt install git build-essential cmake libx11-dev libqt5x11extras5-dev libxi-dev
```
Ubuntu does not have Qt 5.15 in its repositories so you need to use their [Online Installer](https://www.qt.io/download-thank-you?hsLang=en) or [compile it from source](https://doc.qt.io/qt-5/build-sources.html#linux-x11)

### Windows
*(We highly recommend you to just download the latest release for windows since it has all its dependencies packed with it)*

To compile on windows you'll have to install qt (*make sure the the important qt-paths are in your system-path!*)
- [Qt](https://www.qt.io/download-thank-you?os=windows)
- MSVC
- CMake

## Build
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
cmake --build . --config Release
```
To start the program
```sh
./soundux # .\soundux.exe on Windows
```

## Install

### Linux
```sh
sudo make install
```
Automated Installation is currently not finished and will only install the binary. It will be fully functional in the future. (You may look into my [arch package](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=soundux-git) and figure it out for your distro)

# Why _Soundux_?

The project started as a **Sound**board for Lin**ux**

# License
The code is licensed under [GPLv3](LICENSE)

# TODO
- [ ] (Un)Pause / Seek
- [ ] Playlist mode (play sounds one after another)
- [ ] Icons in Output application dropdown on linux
- [ ] Improve the logo
- [ ] Volume normalization
- [ ] Grid View
- [ ] Repeat button
- [ ] Playlist mode
- [ ] Implement hotkeys for Wayland
- [ ] Add ability to move tabs
- [ ] Localization
- [ ] App icon for Windows
- [ ] Package it as deb, ~~Flatpak~~ and AppImage
- [ ] Support macOS (Testers needed!)
- [ ] ~~Fix~~ FileDialog on windows - Its now somewhat fixed but still not truely native
- [x] ~~Block opening the program multiple times~~
- [ ] Display warning when `move-source-ouput` fails
- [ ] Use toast messages instead of modals
