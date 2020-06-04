#!/bin/ash

HERE="$(dirname "$0")"

set -ex

apk update

apk add libressl zlib

apk add build-base \
    curl \
    git \
    go \
    ninja \
    perl \
    pkgconfig \
    libressl-dev \
    qt5-qtbase-dev

apk del cmake
$HERE/../build.cmake.sh
