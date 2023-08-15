#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

cd ${SCRIPTPATH}/deps
./download.sh
cd ${SCRIPTPATH}

mkdir -p ${SCRIPTPATH}/build && cd ${SCRIPTPATH}/build
cmake .. $1
make -j$(nproc)
