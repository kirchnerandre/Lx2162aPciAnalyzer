#!/bin/bash

REPOSITORY_DIR=$(git rev-parse --show-toplevel)

rm -fR ./qemu/build                             2> /dev/null

if [ -d ./qemu ]
then
    pushd ./
    cd ./qemu
    git checkout hw/misc/Kconfig
    git checkout hw/misc/meson.build
    popd
fi

echo "source lx2162a_pci_device/Kconfig"        >> ./qemu/hw/misc/Kconfig
echo "subdir('lx2162a_pci_device')"             >> ./qemu/hw/misc/meson.build

ln -s $REPOSITORY_DIR/tests/lx2162a_pci_device  $REPOSITORY_DIR/tests/qemu/hw/misc

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
