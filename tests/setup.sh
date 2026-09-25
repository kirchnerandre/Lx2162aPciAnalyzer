#!/bin/bash

REPOSITORY_DIR=$(git rev-parse --show-toplevel)

rm -fR ./qemu/build                     2> /dev/null

if [ -d ./qemu ]
then
    pushd ./
    cd ./qemu
    git checkout hw/misc/Kconfig
    git checkout hw/misc/meson.build
    popd
fi

echo "source D3Good/Kconfig"            >> ./qemu/hw/misc/Kconfig
echo "subdir('D3Good')"                 >> ./qemu/hw/misc/meson.build

ln -s $REPOSITORY_DIR/tests/D3Good      $REPOSITORY_DIR/tests/qemu/hw/misc

pushd ./
cd ./qemu
./configure \
    --enable-debug \
    --enable-gtk \
    --enable-sdl \
    --enable-slirp \
    --enable-tools \
    --target-list=x86_64-softmmu
popd
