mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cd ..
cmake --build build --config Debug
cmake --build build --config Release
