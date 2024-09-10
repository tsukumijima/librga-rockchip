#!/bin/bash

SCRIPT_DIR=$(cd $(dirname ${BASH_SOURCE[0]}); pwd)
SOURCE_PATH=${SCRIPT_DIR}

TARGET_NAME="rt_thread"

# Modify to the local toolchain path.
TOOLCHAIN_PATH=${SOURCE_PATH}/toolchains/toolchain_${TARGET_NAME}.cmake
BUILD_DIR=build/build_${TARGET_NAME}
RTOS_BSP=${SOURCE_PATH}/../../rtos
BUILD_TYPE=Release

if [ ! -e ${TOOLCHAIN_PATH} ]; then
	echo "toolchain ${TOOLCHAIN_PATH} does not exist."ef
	exit 1
fi

echo "compile with C"
BUILD_SOURCE_TYPE=c

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
pushd $BUILD_DIR

cmake ../.. \
        -DCMAKE_BUILD_TARGET=rt_thread \
        -DRGA_SOURCE_CODE_TYPE=${BUILD_SOURCE_TYPE} \
        -DBUILD_TOOLCHAINS_PATH=${TOOLCHAIN_PATH} \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
		-DRTOS_BSP="${RTOS_BSP}"  \
        -DCMAKE_INSTALL_PREFIX=install \

make -j32
make install

popd
