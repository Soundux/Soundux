# Soundboard
I didn't find any good soundboard application for linux so I created one. It uses pulseaudio modules to achieve a universal interface. You can select every recording stream for the audio output. The GUI is written with Qt.

It is currently in alpha because I don't know if this works for everyone.


# Runtime Dependencies
- [pulseaudio](https://www.archlinux.org/packages/extra/x86_64/pulseaudio/)
- [mpg123](https://www.archlinux.org/packages/extra/x86_64/mpg123/) (for playing mp3 files)

# Compilation Dependencies
- [ninja](https://www.archlinux.org/packages/community/x86_64/ninja/)
- [cmake](https://www.archlinux.org/packages/extra/x86_64/cmake/)

Please refer your distro instructions for how to install those dependencies

# Compile it yourself
```sh
git clone https://github.com/D3S0X/Soundboard.git
cd Soundboard
mkdir build
cd build
cmake ..
make
./Soundboard
```

# TODO
- [ ] Find a fancy name
- [ ] Double click items to play
- [ ] Check if dependencies are installed otherwise show a warning
- [ ] Only stop mpg123 started from this programm
- [ ] Change back recording streams when the program is closed (to fix a bug when the program is closed while playing a sound)
- [x] Save configuration in .config folder instead of in the same folder as the binary
- [x] Play sounds async
- [x] Implement stop feature
