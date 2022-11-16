#!/bin/bash

set -ex
HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

apt update
export DEBIAN_FRONTEND=noninteractive
apt install -y \
  build-essential \
  curl \
  git \
  golang \
  ninja-build \
  libssl-dev \
  perl \
  pkg-config \
  qtbase5-dev \
  unzip \
  zlib1g-dev

apt purge -y --auto-remove cmake
$HERE/../build.cmake.sh
