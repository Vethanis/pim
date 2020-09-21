#!/usr/bin/bash

pushd ../../

SRCPATH="C:/Program Files (x86)/Steam/steamapps/common/Quake/Id1"
DSTPATH="data/id1"

mkdir -p "${DSTPATH}"

cp "${SRCPATH}/config.cfg" "${DSTPATH}/config.cfg"
cp "${SRCPATH}/PAK0.PAK" "${DSTPATH}/PAK0.PAK"
cp "${SRCPATH}/PAK1.PAK" "${DSTPATH}/PAK1.PAK"

popd
