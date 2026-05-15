# Switch
## Install devkitpro
```bash
podman run -it --rm -v $(pwd):/project:Z devkitpro/devkita64
```
Select the docker image.

## Build libdeflate 
```bash
apt-get update && apt-get install -y git

git clone https://github.com/ebiggers/libdeflate.git /tmp/libdeflate
cd /tmp/libdeflate

cmake -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DLIBDEFLATE_BUILD_SHARED_LIB=OFF \
    -DLIBDEFLATE_BUILD_GZIP=OFF \
    -DCMAKE_INSTALL_PREFIX=$DEVKITPRO/portlibs/switch

cmake --build build
cmake --install build
```

## Build the repo
```bash
cd /project
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake -DBUILD_SERVER=ON
cd build
cmake --build .
```
Then make it into a `.nro` file.
```bash
nacptool --create "BetrockPlusPlus" "Pixel Brush + Aidan" "0.1.0" app.nacp
elf2nro BetrockPlusPlus.elf BetrockPlusPlus.nro --nacp=app.nacp
```

# Nintendo 3DS
## Install devkitpro
```bash
podman run -it --rm -v $(pwd):/project:Z docker.io/devkitpro/devkitarm
```
Select the docker image.

## Build libdeflate 
```bash
apt-get update && apt-get install -y git cmake

git clone https://github.com/ebiggers/libdeflate.git /tmp/libdeflate
cd /tmp/libdeflate

cmake -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DLIBDEFLATE_BUILD_SHARED_LIB=OFF \
    -DLIBDEFLATE_BUILD_GZIP=OFF \
    -DCMAKE_INSTALL_PREFIX=$DEVKITPRO/portlibs/3ds

cmake --build build
cmake --install build
```

## Build the repo
```bash
cd /project
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake -DBUILD_SERVER=ON
cd build
cmake --build .
```
Then make it into a `.3dsx` file.
```bash
3dsxtool BetrockPlusPlus.elf BetrockPlusPlus.3dsx
```