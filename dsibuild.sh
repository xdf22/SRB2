#!/bin/sh

# no, this will NOT work on a ds fat, ram expansion or not

# sets environmental variables and compiles a build automatically

# rm -rf ./build

source /opt/wonderful/bin/wf-env

export BLOCKSDS=/opt/wonderful/thirdparty/blocksds/core
export BLOCKSDSEXT=/opt/wonderful/thirdparty/blocksds/external

cmake -B build -DCMAKE_TOOLCHAIN_FILE=$BLOCKSDS/cmake/BlocksDSi.cmake \
-DSRB2_CONFIG_SYSTEM_LIBRARIES=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DSRB2_CONFIG_ENABLE_TESTS=OFF

make -C build -j $(nproc)