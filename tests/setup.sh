#!/bin/bash

rm -fR  ./qemu/hw/misc/D3Broken_0   2> /dev/null
rm -fR  ./qemu/hw/misc/D3Broken_1   2> /dev/null
rm -fR  ./qemu/hw/misc/D3Good       2> /dev/null

if [ -d ./qemu ]
then
    pushd ./
    cd ./qemu
    git checkout hw/misc/Kconfig
    git checkout hw/misc/meson.build
    popd
fi

if [ -d ./qemu/build ]
then
    rm -fR ./qemu/build
fi

REPOSITORY_DIR=$(git rev-parse --show-toplevel)

echo "source D3Broken_0/Kconfig"    >> ./qemu/hw/misc/Kconfig
echo "subdir('D3Broken_0')"         >> ./qemu/hw/misc/meson.build

echo "source D3Broken_1/Kconfig"    >> ./qemu/hw/misc/Kconfig
echo "subdir('D3Broken_1')"         >> ./qemu/hw/misc/meson.build

echo "source D3Good/Kconfig"        >> ./qemu/hw/misc/Kconfig
echo "subdir('D3Good')"             >> ./qemu/hw/misc/meson.build

ln -s $REPOSITORY_DIR/D3Broken_0/   $REPOSITORY_DIR/qemu/hw/misc
ln -s $REPOSITORY_DIR/D3Broken_1/   $REPOSITORY_DIR/qemu/hw/misc
ln -s $REPOSITORY_DIR/D3Good/       $REPOSITORY_DIR/qemu/hw/misc

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
