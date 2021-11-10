<div align="center">
  <p>
    Lisez la documentation dans:
    <br>
    <a href="https://github.com/Soundux/Soundux/blob/master/README.md">[Anglais]</a>
    <a href="https://github.com/Soundux/Soundux/blob/master/i18n/LISEZMOI.md">[Français]</a>
    <br><br><br>
    <img src="../assets/logo.gif" height="200"/>
    <br>
    <h6>Un soundboard Multi-Plateformes 🔊</h6>
    <br>
        <a href="https://github.com/Soundux/Soundux/releases">
      <img src="https://img.shields.io/github/release/Soundux/Soundux.svg?style=flat-square" alt="Latest Stable Release" />
    </a>
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
    <a href="https://github.com/Soundux/Soundux/blob/master/LICENSE">
      <img src="https://img.shields.io/github/license/Soundux/Soundux.svg?style=flat-square" alt="License" />
    </a>
    <a href="https://discord.gg/4HwSGN4Ec2">
      <img src="https://img.shields.io/discord/697348809591750706?label=discord&style=flat-square" alt="Discord" />
    </a>
    <a href="https://matrix.to/#/!XlIlRgKzoRavKnurkt:matrix.org">
      <img src="https://img.shields.io/badge/chat-matrix%20space-blue?style=flat-square" alt="Matrix" />
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
    <hr>
    <a href="https://discord.com/invite/4HwSGN4Ec2">
      <img src="https://invidget.switchblade.xyz/4HwSGN4Ec2" alt="Discord Invite"/>
    </a>
    <a href="https://hosted.weblate.org/engage/soundux/">
      <img src="https://hosted.weblate.org/widgets/soundux/-/frontend/multi-green.svg" alt="Translation status" />
    </a>
  </p>
</div>

# 👀 Aperçu
| ![Dark Interface](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/home-dark.png)                   | ![Light Interface](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/home-light.png)                   |
| -------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| ![Settings Dark](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/settings-dark.png)                | ![Settings Light](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/settings-light.png)                |
| ![Search Dark](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/search-dark.png)                    | ![Search Light](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/search-light.png)                    |
| ![Application Passthrough](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/pass-through-dark.png)  | ![Application Passthrough](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/pass-through-light.png)   |
| ![Seek/Pause/Stop Dark](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/multiple-playing-dark.png) | ![Seek/Pause/Stop Light](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/multiple-playing-light.png) |
| ![Grid View Dark](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/grid-view-dark.png)              | ![Grid View Light](https://raw.githubusercontent.com/Soundux/screenshots/screenshots/grid-view-light.png)              |

# 👋 Introduction
Soundux est un Soundboard multi-plateformes qui possède une interface utilisateur simple  .
Avec Soundux vous pouvez jouer de l'audio à une application en particulier sous linux et pour votre VB-Cable sink pour Windows.

# Dépendances d'exécution
Ces fichiers sont nécéssaire pour le fonctionnement du programe.

## 🐧 Linux
S'il vous plait, vous référez aux instructions pour votre distribution Linux.
- [pulseaudio](https://gitlab.freedesktop.org/pulseaudio/pulseaudio) / [pipewire](https://pipewire.org/) >= 0.3.26
- Xorg
- Libwnck3 (optionnel,pour le support d'icones)
- Webkit2gtk
- libappindicator3
- [youtube-dl](https://youtube-dl.org/) & [ffmpeg](https://www.ffmpeg.org/) (optionnel, Pour utiliser la fonction de téléchargement)
## <img src="https://www.vectorlogo.zone/logos/microsoft/microsoft-icon.svg" height="20"/> Windows
- [VB-CABLE](https://vb-audio.com/Cable/) (Notre installateur installe automatiquement VB-Cable)
- [Webview2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/) (ceci est d'office dans l'installateur)
- [youtube-dl](https://youtube-dl.org/) & [ffmpeg](https://www.ffmpeg.org/) (optionnel, Pour utiliser la fonction de téléchargement)

# 📥 Installation

## 🐧 Linux

### <img src="https://www.vectorlogo.zone/logos/archlinux/archlinux-icon.svg" height="20"/> Arch Linux et dérivés.
Vous pouvez installer notre paquet avec votre AUR helper de choix ce qui vas automatiquement compiler et installer la dernière version
publiée.
```sh
yay -S soundux
```
Nous avons aussi un paquet `soundux-git` qui se compile directement de la branche maitre (master)

### <img src="https://www.vectorlogo.zone/logos/ubuntu/ubuntu-icon.svg" height="20"/> Ubuntu et dérivés
Vous pouvez installer Soundux via [pacstall](https://github.com/pacstall/pacstall)
```sh
pacstall -I soundux
```

### <img src="https://www.vectorlogo.zone/logos/getfedora/getfedora-icon.svg" height="20"> Fedora
Soundux peut être installé via [COPR repository](https://copr.fedorainfracloud.org/coprs/rivenirvana/soundux/)
```sh
sudo dnf copr enable rivenirvana/soundux
sudo dnf install soundux
```

### <img src="https://www.vectorlogo.zone/logos/linuxfoundation/linuxfoundation-icon.svg" height="20" /> Paquets Distro-agnostic 
Vous pouvez prendre directement la dernière version depuis le snapstore ou flathub

[![Get it from the Snap Store](https://snapcraft.io/static/images/badges/en/snap-store-black.svg)](https://snapcraft.io/soundux)

<a href='https://flathub.org/apps/details/io.github.Soundux'>
  <img width='240' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-en.png'/>
</a>

## <img src="https://www.vectorlogo.zone/logos/microsoft/microsoft-icon.svg" height="20"/> Windows
Télécharger notre installateur ou version portable  [de la dernière version publiée](https://github.com/Soundux/Soundux/releases/latest)

# 🔨 Compilation

## 🔗 build les dépendances

### 🐧 Linux
- Webkit2gtk
- PulseAudio development headers
- PipeWire development headers
- X11 client-side development headers
- libappindicator3 development headers
- OpenSSL development headers
- G++ >= 9
  - Certaines distributions ont toujours la G++ versions < 9 dans leur repos. Les utilisé résulterons a une erreur de construction (pour plus d'informations se réfferer à (anglais) [#71](https://github.com/Soundux/Soundux/issues/71)).

#### <img src="https://www.vectorlogo.zone/logos/debian/debian-icon.svg" height="20"/> Debian / <img src="https://www.vectorlogo.zone/logos/ubuntu/ubuntu-icon.svg" height="20"/> Ubuntu et dérivés
```sh
sudo apt install git build-essential cmake libx11-dev libxi-dev libwebkit2gtk-4.0-dev libappindicator3-dev libssl-dev libpulse-dev libpipewire-0.3-dev
```
> Si vous êtes sous Ubuntu 20.04 ou plus vieux, vous devrez peut être ajouter le PipeWire PPA:
> `sudo add-apt-repository ppa:pipewire-debian/pipewire-upstream`
#### <img src="https://www.vectorlogo.zone/logos/getfedora/getfedora-icon.svg" height="20"> Fedora et dérivés
```sh
sudo dnf install git webkit2gtk3 cmake llvm clang libXi-devel gtk3-devel webkit2gtk3-devel libappindicator-gtk3-devel pulseaudio-libs-devel pipewire-devel
```

### <img src="https://www.vectorlogo.zone/logos/microsoft/microsoft-icon.svg" height="20"/> Windows
- Nuget
- MSVC
- CMake
- OpenSSL

## 👷 Build
Clonez le repo
```sh
git clone https://github.com/Soundux/Soundux.git
cd Soundux
git submodule update --init --recursive
```
Créer un dossier de build et commencer la compilation
```sh
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
Pour lancer le programme
```sh
./soundux # .\soundux.exe on Windows
```

## 🖥️ Installation

### 🐧 Linux
```sh
sudo make install
```

# 📝 Pourquoi _Soundux_?

Le projet a démarré comme **Sound**board pour Lin**ux**

# 🗒️ License
Le code est licensé sous [GPLv3](../LICENSE)

# ✍️ Contribution
Les règles de conduite pour la contribution peuvent être trouvé [ici](../CONTRIBUTING.md), S'il vous plait, consultez les si vous prévoyez de contribuer.
# ✨ Aidants

Milles merci à tout ce beaux monde! ([emoji key](https://allcontributors.org/docs/en/emoji-key)):

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
  <tr>
    <td align="center"><a href="https://github.com/Kylianalex"><img src="https://avatars.githubusercontent.com/u/66625058?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Kylianalex</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/issues?q=author%3AKylianalex" title="Bug reports">🐛</a></td>
    <td align="center"><a href="http://gregerstoltnilsen.net/"><img src="https://avatars.githubusercontent.com/u/1364443?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Greger</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/issues?q=author%3Agregersn" title="Bug reports">🐛</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/rivenirvana"><img src="https://avatars.githubusercontent.com/u/43519644?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Arvin Verain</b></sub></a><br /><a href="#platform-rivenirvana" title="Packaging/porting to new platform">📦</a></td>
    <td align="center"><a href="http://einfacheinalex.eu/"><img src="https://avatars.githubusercontent.com/u/20642291?v=4?s=50" width="50px;" alt=""/><br /><sub><b>EinfachEinAlex</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/commits?author=EinfachEinAlex" title="Code">💻</a> <a href="#research-EinfachEinAlex" title="Research">🔬</a> <a href="https://github.com/Soundux/Soundux/commits?author=EinfachEinAlex" title="Tests">⚠️</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://discord.gg/ubmTQnuM3Z"><img src="https://avatars.githubusercontent.com/u/69876322?v=4?s=50" width="50px;" alt=""/><br /><sub><b>MeblIkea</b></sub></a><br /><a href="#translation-MeblIkea" title="Translation">🌍</a></td>
    <td align="center"><a href="https://nathanbonnemains.squill.fr/"><img src="https://avatars.githubusercontent.com/u/45366162?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Nathan Bonnemains</b></sub></a><br /><a href="#translation-NathanBnm" title="Translation">🌍</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/lucasvbeek"><img src="https://avatars.githubusercontent.com/u/29404838?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Lucas van Beek</b></sub></a><br /><a href="#translation-lucasvbeek" title="Translation">🌍</a></td>
    <td align="center"><a href="https://github.com/underhood"><img src="https://avatars.githubusercontent.com/u/6674623?v=4?s=50" width="50px;" alt=""/><br /><sub><b>Timotej S.</b></sub></a><br /><a href="https://github.com/Soundux/Soundux/issues?q=author%3Aunderhood" title="Bug reports">🐛</a> <a href="https://github.com/Soundux/Soundux/commits?author=underhood" title="Tests">⚠️</a></td>
  </tr>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

Ce projet suis les spécifications de [all-contributors](https://github.com/all-contributors/all-contributors) Les contributions de toute sortes sont bienvenue!
