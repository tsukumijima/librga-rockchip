#!/bin/bash

SCRIPT_DIR=$(cd $(dirname ${BASH_SOURCE[0]}); pwd)
SOURCE_PATH=${SCRIPT_DIR}

# Modify to the local toolchain path.
TOOLCHAIN_PATH=${SOURCE_PATH}/toolchains/toolchain_linux.cmake
BUILD_DIR=build/build_linux
BUILD_TYPE=Release

if [ ${1} == 'c' ]
then
    echo "compile with C"
	BUILD_SOURCE_TYPE=c
else
    echo "compile with C++"
	BUILD_SOURCE_TYPE=cpp
fi

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
pushd $BUILD_DIR

cmake ../.. \
	-DCMAKE_BUILD_TARGET=buildroot \
	-DRGA_SOURCE_CODE_TYPE=${BUILD_SOURCE_TYPE} \
	-DBUILD_TOOLCHAINS_PATH=${TOOLCHAIN_PATH} \
	-DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
	-DCMAKE_INSTALL_PREFIX=install \

make -j32
make install

popd
