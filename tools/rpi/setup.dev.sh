#!/bin/bash

set -ex
HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

apt update

apt install -y \
  build-essential \
  git \
  golang \
  ninja-build \
  libssl-dev \
  perl \
  pkg-config \
  qtbase5-dev \
  zlib1g-dev

apt purge -y --auto-remove cmake
$HERE/../build.cmake.sh
