#!/bin/bash

REPOSITORY_DIR=$(git rev-parse --show-toplevel)

#rm -fR ./qemu/hw/misc/ls2162a_device    2> /dev/null
#rm -fR ./qemu/build                     2> /dev/null

if [ -d ./qemu ]
then
    pushd ./
    cd ./qemu
    git checkout hw/misc/Kconfig
    git checkout hw/misc/meson.build
    popd
fi

echo "source lx2162a_device/Kconfig"    >> ./qemu/hw/misc/Kconfig
echo "subdir('lx2162a_device')"         >> ./qemu/hw/misc/meson.build

#mkdir ./qemu/hw/misc/lx2162a_device

#cp ./lx2162a_device.h   ./qemu/hw/misc/lx2162a_device/lx2162a_device.h
#cp ./lx2162a_device.c   ./qemu/hw/misc/lx2162a_device/lx2162a_device.c
#cp ./Kconfig            ./qemu/hw/misc/lx2162a_device/Kconfig
#cp ./meson.build        ./qemu/hw/misc/lx2162a_device/meson.build

ln -s $REPOSITORY_DIR/tests/lx2162a_device      $REPOSITORY_DIR/tests/qemu/hw/misc/lx2162a_device

echo "source D3Good/Kconfig"            >> ./qemu/hw/misc/Kconfig
echo "subdir('D3Good')"                 >> ./qemu/hw/misc/meson.build

echo "source lx2162a_device_2/Kconfig"  >> ./qemu/hw/misc/Kconfig
echo "subdir('lx2162a_device_2')"       >> ./qemu/hw/misc/meson.build

ln -s $REPOSITORY_DIR/tests/D3Good              $REPOSITORY_DIR/tests/qemu/hw/misc/D3Good
ln -s $REPOSITORY_DIR/tests/lx2162a_device_2    $REPOSITORY_DIR/tests/qemu/hw/misc/lx2162a_device_2

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
