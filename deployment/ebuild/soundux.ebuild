# Copyright 1999-2021 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=7

inherit optfeature cmake

DESCRIPTION="A cross-platform soundboard"
HOMEPAGE="https://soundux.rocks"
SRC_URI="https://github.com/Soundux/Soundux/releases/download/${PV}/soundux-${PV}.tar.gz -> ${P}.tar.gz"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64"

DEPEND="
    dev-libs/libappindicator:3
    dev-libs/openssl
    media-sound/pulseaudio
    media-video/pipewire
    net-libs/webkit-gtk:4
    x11-base/xorg-server
    x11-libs/gtk+:3
"
RDEPEND="${DEPEND}"

S="${WORKDIR}/Soundux"

src_configure() {
    local mycmakeargs=(
        -DCMAKE_INSTALL_PREFIX="${EPREFIX}"
        -DEMBED_PATH="OFF"
    )
    cmake_src_configure
}

pkg_postinst() {
    optfeature "Downloader support" media-video/ffmpeg net-misc/youtube-dl
    optfeature "Icon support" x11-libs/libwnck:3
}
