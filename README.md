<div align="center">
  <p>
    <h1>
      <a href="#readme">
        <img src="icon.png" width="50" alt="Soundboard" />
      </a>
      <br />
      Soundux
    </h1>
    <h4>A universal soundboard in Qt for linux using pulseaudio modules</h4>
  </p>
  <p>
    <a href="https://github.com/D3S0X/Soundux/releases">
      <img src="https://img.shields.io/github/last-commit/D3S0X/Soundux.svg?style=for-the-badge" alt="Last Commit" />
    </a>
    <a href="https://github.com/D3S0X/Soundux/blob/master/LICENSE">
      <img src="https://img.shields.io/github/license/D3S0X/Soundux.svg?style=for-the-badge" alt="License" />
    </a>
    <a href="https://travis-ci.com/D3S0X/Soundux">
      <img src="https://img.shields.io/travis/com/D3S0X/Soundux?style=for-the-badge" alt="Travis" />
    </a>
  </p>
</div>

## Index
- [Introduction](#introduction)
- [Runtime Dependencies](#runtime-dependencies)
- [Compilation & Installation](#compilation--installation)
  - [Arch Linux and derivatives](#arch-linux-and-derivatives)
  - [Other distros](#other-distros)
    - [Dependencies](#dependencies)
    - [Install dependencies for Ubuntu and derivatives (Thanks to @Tibladar)](#install-dependencies-for-ubuntu-and-derivatives-thanks-to-tibladar)
    - [Build](#build)
    - [Install](#install)
- [Why _Soundux_?](#why-soundux)
- [License](#license)
- [TODO](#todo)

# Introduction
I didn't find any good soundboard application for linux so I created one. It uses pulseaudio modules to achieve a universal interface. You can select every recording stream for the audio output. The GUI is written with Qt.

It is currently in alpha because I don't know if this works for everyone.

# Runtime Dependencies
Please refer to your distro instructions on how to install
- [pulseaudio](https://gitlab.freedesktop.org/pulseaudio/pulseaudio)
- [mpg123](https://www.mpg123.de/) (optional: for playing mp3 files)

# Compilation & Installation

## Arch Linux and derivatives
You can install my package with your AUR helper of choice which will automatically compile and install the master branch
```sh
yay -S soundux-git
```

## Other distros

### Dependencies
This list may be not accurate. Contact me if you find missing dependencies that I can update this list
- [qpm](https://github.com/Cutehacks/qpm)
- [qt5-base](https://github.com/qt/qtbase)
- [qt5-tools](https://github.com/qt/qt5)

### Install dependencies for Ubuntu and derivatives (Thanks to @Tibladar)
```sh
sudo apt install git golang-go qt5-default libqt5x11extras5-dev mpg123
go get qpm.io/qpm
```

### Build
Clone the repository
```sh
git clone https://github.com/D3S0X/Soundux.git
cd Soundux
```
Install dependencies
```sh
# If you installed qpm via go
~/go/bin/qpm install
# Otherwise
qpm install
```
Create a build folder and start compilation
```sh
mkdir build
cd build
qmake ..
make
```
To start the program
```sh
./Soundux
```

### Install
Automated Installation is currently not available but will be available in the future. (You may look into my [arch package](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=soundux-git) and figure it out for your distro)

# Why _Soundux_?

**Sound**board for Lin**ux**

# License
The code is licensed under [GPLv3](LICENSE)

# TODO
- [ ] Playlist mode (play sounds one after another)
- [ ] Improve the logo
- [ ] Volume normalization
- [ ] Play sounds while another sound is playing
- [ ] Organize code
- [ ] Grid View
- [ ] Save volume states (+ Sync state)
- [ ] Option to only trigger hotkeys from current tab
- [ ] Only modify audio players started from this programm (stop, volume)
- [ ] Fix volume slider not working while playback
- [ ] Save positions when tabs moved
- [ ] Localization
- [ ] Package it as deb and flatpak and create a repology
- [ ] Change back recording streams when the program is closed (to fix a bug when the program is closed while playing a sound)
- [ ] Make it cross-platform (help on how to pass the sound to an application in Windows/macOS is greatly appreciated)
- [x] Hotkeys for folder tabs
- [x] Make UI responsive
- [x] Search sounds
- [x] Refresh button for folder tabs
- [x] Find a fancy name
- [x] CTRL+Q to quit
- [x] Automatically update files in folders (on startup?)
- [x] Separate volume sliders for me and for others (and a toggle to sync it)
- [x] Global Hotkeys
- [x] Check if dependencies are installed otherwise show a warning
- [x] Repeat button
- [x] Add support for other audio formats
- [x] Add folders and automatically create a tab for it
- [x] Create tabs to better organize your sounds
- [x] Double click items to play
- [x] Add multiple files at once
- [x] Save configuration in .config folder instead of in the same folder as the binary
- [x] Play sounds async
- [x] Implement stop feature
