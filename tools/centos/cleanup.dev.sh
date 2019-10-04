#!/bin/bash
set -ex

yum -y update

# Let's remove our build dependencies
# and be quite aggressive about it.
yum remove -y \
  cmake \
  git \
  pkg-config \
  qtbase5-dev \
  centos-release-scl
