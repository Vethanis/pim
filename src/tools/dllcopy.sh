#!/usr/bin/bash

EMBREE="submodules/embree/embree-3.10.0.x64.vc14.windows/bin/embree3.dll"
TBB="submodules/embree/embree-3.10.0.x64.vc14.windows/bin/tbb.dll"
OIDN="submodules/oidn/oidn-1.2.0.x64.vc14.windows/bin/OpenImageDenoise.dll"
TBB_MALLOC="submodules/oidn/oidn-1.2.0.x64.vc14.windows/bin/tbbmalloc.dll"

cd ../../

mkdir -p "proj/x64/Debug"
mkdir -p "proj/x64/Release"

cp "${EMBREE}" "proj/x64/Debug/embree3.dll"
cp "${EMBREE}" "proj/x64/Release/embree3.dll"

cp "${TBB}" "proj/x64/Debug/tbb.dll"
cp "${TBB}" "proj/x64/Release/tbb.dll"

cp "${OIDN}" "proj/x64/Debug/OpenImageDenoise.dll"
cp "${OIDN}" "proj/x64/Release/OpenImageDenoise.dll"

cp "${TBB_MALLOC}" "proj/x64/Debug/tbbmalloc.dll"
cp "${TBB_MALLOC}" "proj/x64/Release/tbbmalloc.dll"

cd src/tools
