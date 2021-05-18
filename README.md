<div align="center">
  <p>
    <img src="assets/logo.gif" height="200"/>
    <br>
    <h6>A cross-platform soundboard 🔊</h6>
    <br>
    <a href="https://github.com/Soundux/Soundux/stargazers">
      <img src="https://img.shields.io/github/stars/Soundux/soundux?style=flat-square" alt="GitHub Repo stars">
    </a>
    <a href="https://github.com/Soundux/Soundux/issues">
      <img src="https://img.shields.io/github/issues/Soundux/soundux?style=flat-square" alt="GitHub issues">
    </a>
    <a href="https://github.com/Soundux/Soundux/pulls">
      <img src="https://img.shields.io/github/issues-pr-raw/Soundux/soundux?label=pulls&style=flat-square" alt="GitHub pull requests">
    </a>
    <br>
    <a href="https://github.com/Soundux/Soundux/releases">
      <img src="https://img.shields.io/github/release/Soundux/Soundux.svg?style=flat-square" alt="Latest Stable Release" />
    </a>
    <a href="https://discord.gg/4HwSGN4Ec2">
      <img src="https://img.shields.io/discord/697348809591750706?label=discord&style=flat-square" alt="Discord" />
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

# 👀 Preview
| ![Dark Interface](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/home-dark.png)                   | ![Light Interface](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/home-light.png)                   |
| -------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| ![Settings Dark](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/settings-dark.png)                | ![Settings Light](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/settings-light.png)                |
| ![Search Dark](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/search-dark.png)                    | ![Search Light](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/search-light.png)                    |
| ![Application Passthrough](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/pass-through-dark.png)  | ![Application Passthrough](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/pass-through-light.png)   |
| ![Seek/Pause/Stop Dark](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/multiple-playing-dark.png) | ![Seek/Pause/Stop Light](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/multiple-playing-light.png) |
| ![Grid View Dark](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/grid-view-dark.png)              | ![Grid View Light](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/grid-view-light.png)              |

# 👋 Introduction
Soundux is a cross-platform soundboard that features a simple user interface.
With Soundux you can play audio to a specific application on Linux and to your VB-CABLE sink on Windows.

# 🏃 Runtime Dependencies
These are required to run the program

## 🐧 Linux
Please refer to your distro instructions on how to install
- [pulseaudio](https://gitlab.freedesktop.org/pulseaudio/pulseaudio) / [pipewire](https://pipewire.org/)
- Xorg
- Libwnck3 (optional, for icon support)
- Webkit2gtk
- libappindicator3
- [youtube-dl](https://youtube-dl.org/) & [ffmpeg](https://www.ffmpeg.org/) (optional, for downloader support)
## 🪟 Windows
- [VB-CABLE](https://vb-audio.com/Cable/) (Our installer automatically installs VB-CABLE)
- [Webview2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/) (Is also shipped with the installer)
- [youtube-dl](https://youtube-dl.org/) & [ffmpeg](https://www.ffmpeg.org/) (optional, for downloader support)

# 📥 Installation

## 🐧 Linux

### ❤️ Arch Linux and derivatives
You can install our package with your AUR helper of choice which will automatically compile and install the latest release version
```sh
yay -S soundux
```
We also provide a `soundux-git` package which compiles from the master branch

### 🗃️ Ubuntu and derivatives
You can install Soundux via [pacstall](https://github.com/Henryws/pacstall)
```sh
sudo pacstall -I soundux
```

### 📜 Distro-agnostic packages
You can grab the latest release from the Snap Store or Flathub

[![Get it from the Snap Store](https://snapcraft.io/static/images/badges/en/snap-store-black.svg)](https://snapcraft.io/soundux)

<a href='https://flathub.org/apps/details/io.github.Soundux'>
  <img width='240' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-en.png'/>
</a>

## 🪟 Windows
Download our installer or portable from [the latest release](https://github.com/Soundux/Soundux/releases/latest)

# 🪛 Compilation

## 🔗 Build Dependencies

### 🐧 Linux
This list may not be accurate. Contact me if you find missing dependencies so that I can update this list
- Webkit2gtk
- PulseAudio development headers
- PipeWire development headers
- X11 client-side development headers
- libappindicator3 development headers
- OpenSSL development headers
- G++ >= 9
  - Some distros still have G++ versions < 9 in their repos, using them will result in a build failure (for more information refer to [#71](https://github.com/Soundux/Soundux/issues/71)).

#### 📜 Debian/Ubuntu and derivatives
```sh
sudo apt install git build-essential cmake libx11-dev libxi-dev libwebkit2gtk-4.0-dev libappindicator3-dev libssl-dev libpulse-dev libpipewire-0.3-dev
```
> If you're on Ubuntu 20.10 or lower you might have to add the PipeWire PPA:
> `sudo add-apt-repository ppa:pipewire-debian/pipewire-upstream`
#### 📜 Fedora and derivatives
```sh
sudo dnf install git webkit2gtk3 pulseaudio-utils cmake llvm clang libXi-devel gtk3-devel webkit2gtk3-devel libappindicator-gtk3-devel pulseaudio-libs-devel pipewire-devel
```

### 🪟 Windows
- Nuget
- MSVC
- CMake
- OpenSSL

## 👷 Build
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

## 🖥️ Install

### 🐧 Linux
```sh
sudo make install
```

# 📝 Why _Soundux_?

The project started as a **Sound**board for Lin**ux**

# 🗒️ License
The code is licensed under [GPLv3](LICENSE)

# ✨ Contributors

Thanks goes to these wonderful people ([emoji key](https://allcontributors.org/docs/en/emoji-key)):

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tr>
    <td align="center"><a href="https://github.com/Curve"><img src="https://avatars.githubusercontent.com/u/37805707?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Noah</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/issues?q=author%3ACurve" title="Bug reports">🐛</a> <a href="#business-Curve" title="Business development">💼</a> <a href="https://github.com/Soundux/Soundux/commits?author=Curve" title="Code">💻</a> <a href="#design-Curve" title="Design">🎨</a> <a href="#ideas-Curve" title="Ideas, Planning, & Feedback">🤔</a> <a href="#infra-Curve" title="Infrastructure (Hosting, Build-Tools, etc)">🚇</a> <a href="#maintenance-Curve" title="Maintenance">🚧</a> <a href="#platform-Curve" title="Packaging/porting to new platform">📦</a> <a href="#projectManagement-Curve" title="Project Management">📆</a> <a href="#question-Curve" title="Answering Questions">💬</a> <a href="https://github.com/Soundux/Soundux/pulls?q=is%3Apr+reviewed-by%3ACurve" title="Reviewed Pull Requests">👀</a> <a href="https://github.com/Soundux/Soundux/commits?author=Curve" title="Tests">⚠️</a></td>
    <td align="center"><a href="https://github.com/D3SOX"><img src="https://avatars.githubusercontent.com/u/24937357?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Nico</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/issues?q=author%3AD3SOX" title="Bug reports">🐛</a> <a href="#business-D3SOX" title="Business development">💼</a> <a href="https://github.com/Soundux/Soundux/commits?author=D3SOX" title="Code">💻</a> <a href="#design-D3SOX" title="Design">🎨</a> <a href="#ideas-D3SOX" title="Ideas, Planning, & Feedback">🤔</a> <a href="#infra-D3SOX" title="Infrastructure (Hosting, Build-Tools, etc)">🚇</a> <a href="#maintenance-D3SOX" title="Maintenance">🚧</a> <a href="#platform-D3SOX" title="Packaging/porting to new platform">📦</a> <a href="#projectManagement-D3SOX" title="Project Management">📆</a> <a href="#question-D3SOX" title="Answering Questions">💬</a> <a href="https://github.com/Soundux/Soundux/pulls?q=is%3Apr+reviewed-by%3AD3SOX" title="Reviewed Pull Requests">👀</a> <a href="https://github.com/Soundux/Soundux/commits?author=D3SOX" title="Tests">⚠️</a> <a href="#translation-D3SOX" title="Translation">🌍</a> <a href="#a11y-D3SOX" title="Accessibility">️️️️♿️</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/MrKingMichael"><img src="https://avatars.githubusercontent.com/u/30067605?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Michael</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/issues?q=author%3AMrKingMichael" title="Bug reports">🐛</a> <a href="#ideas-MrKingMichael" title="Ideas, Planning, & Feedback">🤔</a> <a href="#translation-MrKingMichael" title="Translation">🌍</a> <a href="https://github.com/Soundux/Soundux/commits?author=MrKingMichael" title="Tests">⚠️</a></td>
    <td align="center"><a href="https://github.com/BrandonKMLee"><img src="https://avatars.githubusercontent.com/u/58927531?v=4?s=50" width="50px;" alt=""/><br /><sub><b>BrandonKMLee</b></sub></a><br /><a href="#ideas-BrandonKMLee" title="Ideas, Planning, & Feedback">🤔</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/Toadfield"><img src="https://avatars.githubusercontent.com/u/68649672?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Toadfield</b></sub></a><br /><a href="#ideas-Toadfield" title="Ideas, Planning, & Feedback">🤔</a> <a href="https://github.com/Soundux/Soundux/issues?q=author%3AToadfield" title="Bug reports">🐛</a></td>
    <td align="center"><a href="https://github.com/fubka"><img src="https://avatars.githubusercontent.com/u/44064746?v=4?s=50" width="50px;" alt=""/><br /><sub><b>fubka</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/issues?q=author%3Afubka" title="Bug reports">🐛</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/TheOriginalTripleD"><img src="https://avatars.githubusercontent.com/u/6907054?v=4?s=50" width="50px;" alt=""/><br /><sub><b>TheOriginalTripleD</b></sub></a><br /><a href="#research-TheOriginalTripleD" title="Research">🔬</a></td>
    <td align="center"><a href="https://github.com/UltraBlackLinux"><img src="https://avatars.githubusercontent.com/u/62404294?v=4?s=50" width="50px;" alt=""/><br /><sub><b>UltraBlackLinux</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/issues?q=author%3AUltraBlackLinux" title="Bug reports">🐛</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://bendem.be/"><img src="https://avatars.githubusercontent.com/u/2681677?v=4?s=50" width="50px;" alt=""/><br /><sub><b>bendem</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/issues?q=author%3Abendem" title="Bug reports">🐛</a></td>
    <td align="center"><a href="https://edgar.bzh/"><img src="https://avatars.githubusercontent.com/u/46636609?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Edgar Onghena</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/issues?q=author%3Aedgarogh" title="Bug reports">🐛</a> <a href="#research-edgarogh" title="Research">🔬</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/moggesmith10"><img src="https://avatars.githubusercontent.com/u/33375517?v=4?s=50" width="50px;" alt=""/><br /><sub><b>moggesmith10</b></sub></a><br /><a href="#ideas-moggesmith10" title="Ideas, Planning, & Feedback">🤔</a></td>
    <td align="center"><a href="https://belmoussaoui.com/"><img src="https://avatars.githubusercontent.com/u/7660997?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Bilal Elmoussaoui</b></sub></a><br /><a href="#platform-bilelmoussaoui" title="Packaging/porting to new platform">📦</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/thomasfinstad"><img src="https://avatars.githubusercontent.com/u/5358752?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Thomas Finstad Larsen</b></sub></a><br /><a href="#ideas-thomasfinstad" title="Ideas, Planning, & Feedback">🤔</a></td>
    <td align="center"><a href="http://arthurmelton.me"><img src="https://avatars.githubusercontent.com/u/29708070?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Arthur Melton</b></sub></a><br /><a href="#ideas-AMTitan" title="Ideas, Planning, & Feedback">🤔</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/serkan-maker"><img src="https://avatars.githubusercontent.com/u/63740626?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Serkan ÖNDER</b></sub></a><br /><a href="#translation-serkan-maker" title="Translation">🌍</a></td>
    <td align="center"><a href="https://github.com/pizzadude"><img src="https://avatars.githubusercontent.com/u/1454420?v=4?s=50" width="50px;" alt=""/><br /><sub><b>PizzaDude</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/issues?q=author%3Apizzadude" title="Bug reports">🐛</a> <a href="#research-pizzadude" title="Research">🔬</a></td>
  </tr>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

This project follows the [all-contributors](https://github.com/all-contributors/all-contributors) specification. Contributions of any kind welcome!
