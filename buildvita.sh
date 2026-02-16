#!/bin/sh

rm -rf ./vitaout/
rm -rf ./build/

mkdir ./vitaout/

echo "Running CMake"
cmake -B build \
    -DCMAKE_POSITION_INDEPENDENT_CODE=OFF \
    -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake \
    -DSRB2_CONFIG_SYSTEM_LIBRARIES=OFF \
    -DSRB2_CONFIG_HWRENDER=OFF

echo "Running Make (with -j $(nproc))"
make -C build -j $(nproc)

echo "Creating Vita elf file"
vita-elf-create ./build/bin/srb2_2.2.13-archive ./vitaout/srb2-vita-2.2.13.self

echo "Creating eboot file"
vita-make-fself -c ./vitaout/srb2-vita-2.2.13.self ./vitaout/eboot.bin

echo "Creating param.sfo file"
vita-mksfoex -s TITLE_ID=SRB20000 -d MEMSIZE=2 "Sonic Robo Blast 2" ./vitaout/param.sfo

echo "Creating VPK"
vita-pack-vpk -s ./vitaout/param.sfo -b ./vitaout/eboot.bin ./vitaout/srb2vita.vpk

echo "Output in ./vitaout/"
