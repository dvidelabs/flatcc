#!/bin/sh

# only used with new installation
# 0.44.0 is last version working with Python 3.4
INSTALL_VER=${VER:-0.44.0}
INSTALL_DIR=${INSTALL_DIR:-$(pwd)/meson}
INSTALL_URL=https://github.com/mesonbuild/meson.git

echoerr() { printf "%s\n" "$*" >&2; }
require() {
    if !(command -v $1 > /dev/null 2>&1); then
        echoerr "aborting, please install $1"
        exit 1
    fi
}
detect() {
    command -v $1 2>/dev/null && exit 0
}

require ninja
detect $INSTALL_DIR/meson.py
#detect meson
#detect meson.py

echoerr "meson not installed, cloning meson-$INSTALL_VER into $INSTALL_DIR"

require python3
require git

# git it too noisy, even with -q
git clone -q --depth=1 $INSTALL_URL -b $INSTALL_VER $INSTALL_DIR > /dev/null 2>&1

detect $INSTALL_DIR/meson.py

echoerr "meson installation failed, please check that $INSTALL_DIR is not corrupted"
exit 1
