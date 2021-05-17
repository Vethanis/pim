mkdir build
cd build
cmake .. -G "Visual Studio 15 Win64"
cd ..
cmake --build build --config Debug
cmake --build build --config Release
