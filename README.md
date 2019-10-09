<div align="center">
  <p>
    <h1>
      <a href="#readme">
        <img src="icon.jpg" width="50" alt="Soundboard" />
      </a>
      <br />
      Soundboard
    </h1>
    <h4>A universal soundboard in Qt for linux using pulseaudio modules</h4>
  </p>
  <p>
    <a href="https://github.com/D3S0X/Soundboard/releases">
      <img src="https://img.shields.io/github/release-date-pre/D3S0X/Soundboard.svg?style=flat-square&label=pre-release" alt="Latest Pre-Release" />
    </a>
    <a href="https://github.com/D3S0X/Soundboard/releases">
      <img src="https://img.shields.io/github/release/D3S0X/Soundboard.svg?style=flat-square&label=release" alt="Latest Stable Release" />
    </a>
    <a href="https://github.com/D3S0X/Soundboard/releases">
      <img src="https://img.shields.io/github/downloads/D3S0X/Soundboard/total.svg?style=flat-square" alt="Total Downloads" />
    </a>
    <a href="https://github.com/D3S0X/Soundboard/blob/master/LICENSE">
      <img src="https://img.shields.io/github/license/D3S0X/Soundboard.svg?style=flat-square" alt="License" />
    </a>
  </p>
</div>

## Index
- [Introduction](#introduction)
- [Dependencies](#dependencies)
- [Compilation](#compilation)
  - [Dependencies](#dependencies-1)
  - [Build](#build)
  - [Install](#install)
- [License](#license)
- [TODO](#todo)

# Introduction
I didn't find any good soundboard application for linux so I created one. It uses pulseaudio modules to achieve a universal interface. You can select every recording stream for the audio output. The GUI is written with Qt.

It is currently in alpha because I don't know if this works for everyone.

# Dependencies
Please refer your distro instructions for how to install them
- [pulseaudio](https://www.archlinux.org/packages/extra/x86_64/pulseaudio/)
- [mpg123](https://www.archlinux.org/packages/extra/x86_64/mpg123/) (for playing mp3 files)

# Compilation

## Dependencies
This list may be not accurate. Contact me if you find missing dependencies that I can update this list
- [ninja](https://www.archlinux.org/packages/community/x86_64/ninja/)
- [cmake](https://www.archlinux.org/packages/extra/x86_64/cmake/)
- [qt5-base](https://www.archlinux.org/packages/extra/x86_64/qt5-base/)
- [qt5-tools](https://www.archlinux.org/packages/extra/x86_64/qt5-tools/)

## Build
```sh
# Clone the repository
git clone https://github.com/D3S0X/Soundboard.git
cd Soundboard
# Create a build folder and start compilation
mkdir build
cd build
cmake ..
make
# To start the program
./Soundboard
```

## Install
Installation is currently not possible but will be available in the future

# License
The code is licensed under [GPLv3](LICENSE)

# TODO
- [ ] Find a fancy name
- [ ] Create an own logo
- [ ] Check if dependencies are installed otherwise show a warning
- [ ] Play sounds while another sound is playing
- [ ] Organize code
- [ ] Only stop mpg123 started from this programm
- [ ] Repeat button
- [ ] Save positions when tabs moved
- [ ] Change back recording streams when the program is closed (to fix a bug when the program is closed while playing a sound)
- [x] Add folders and automatically create a tab for it
- [x] Create tabs to better organize your sounds
- [x] Double click items to play
- [x] Add multiple files at once
- [x] Save configuration in .config folder instead of in the same folder as the binary
- [x] Play sounds async
- [x] Implement stop feature
