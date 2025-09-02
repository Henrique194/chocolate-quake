# BUILDING

## Windows
`tbd`

## MacOS
`tbd`

## Linux
Dependencies: <br />
Based on debian/ubuntu environment <br />
`build-essentials automake cmake make git libvorbis-dev libsdl2-dev` <br />

Following these instructions to make a `build` folder to keep the base tree clean,  <br />
you will find the compiled binary executable in `build/src`. <br />

`make` can be replaced by `cmake --build .` if preferred <br />

### GCC x64
```
export CC=gcc
export CXX=g++
```
```
git clone https://github.com/Henrique194/chocolate-quake
cd chocolate-quake
mkdir build
cd build
cmake ..
cmake --build .
```

### CLANG x64
CLANG compiler can be used instead of GCC <br />
```
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
```
```
cmake ..
cmake --build .
```

### LINUX i386
Requires: `libsdl2-dev:i386` and its dependencies <br />
```
cmake -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 ..
cmake --build .
```
