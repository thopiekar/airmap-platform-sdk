#!/bin/bash

set -eux
HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

uname -r
sudo apt-get install qemu binfmt-support qemu-user-static

build_opts="--force-rm=true $@"

BASE_IMAGE="airmapreg.azurecr.io/platform-sdk.l4t.base:2"
IMAGE="platform-sdk.l4t"

cd "${HERE}/../.."

docker run --platform=linux/amd64 --rm --privileged multiarch/qemu-user-static --reset -p yes

# build base image if it does not yet exist
if [[ -z "$(docker images -q "${BASE_IMAGE}")" ]]; then
    docker build --file "${HERE}/../../docker/l4t/l4t.base" --tag "${BASE_IMAGE}" .
fi

# build the release image
docker build ${build_opts} --file "${HERE}/../../docker/l4t/l4t" --tag "${IMAGE}" .
