#!/bin/bash

rm -fR ./qemu/hw/misc/ls2162a_device    2> /dev/null

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

echo "source lx2162a_device/Kconfig"    >> ./qemu/hw/misc/Kconfig
echo "subdir('lx2162a_device')"         >> ./qemu/hw/misc/meson.build

mkdir ./qemu/hw/misc/lx2162a_device

cp ./lx2162a_device.h   ./qemu/hw/misc/lx2162a_device/lx2162a_device.h
cp ./lx2162a_device.c   ./qemu/hw/misc/lx2162a_device/lx2162a_device.c
cp ./Kconfig            ./qemu/hw/misc/lx2162a_device/Kconfig
cp ./meson.build        ./qemu/hw/misc/lx2162a_device/meson.build

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
