# Nintendo Switch & Nintendo 3DS

Betrock++ can be cross-compiled into homebrew for the Nintendo Switch (`.nro`)
and Nintendo 3DS (`.3dsx`) using [devkitPro](https://devkitpro.org/). Only the
**server** is supported on these targets — the client needs SDL3/OpenGL,
neither of which is portable to either console yet. `CMakeLists.txt` detects
these toolchains automatically and forces `BUILD_SERVER=ON`, so passing it
explicitly (as below) is just for clarity.

The optional features that depend on vcpkg (`ONLINE_MODE_AUTHENTICATION`,
`BETACRAFT_HEARTBEAT`, `DISCORD_INTEGRATION`) and `CRASH_LOGGING` (which needs
`fork()`/`waitpid()`) are automatically disabled for these targets, since none
of their dependencies have a Switch/3DS triplet and `fork()` doesn't exist on
either console. You'll see a `WARNING` for each at configure time; that's
expected.

> [!NOTE]
> Both consoles are memory-constrained compared to a desktop (the 3DS in
> particular has only ~128MB of total RAM, less once the OS and homebrew
> loader reserve their share). Keep the render/view distance and world size
> modest, and pre-generate the area you intend to play in if possible.

## Nintendo Switch

### 1. Install devkitPro

```bash
podman run -it --rm -v $(pwd):/project:Z devkitpro/devkita64
```

Select the docker image. This image already bundles devkitA64, libnx, and
`switch-tools` (which provides `nacptool`/`elf2nro`), so nothing further needs
installing for the toolchain itself.

### 2. Build libdeflate

```bash
apt-get update && apt-get install -y git cmake

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

### 3. Build the repo

```bash
cd /project
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake -DBUILD_SERVER=ON
cd build
cmake --build . -j$(nproc)
```

### 4. Package it into a `.nro`

The build already does this for you as a post-build step (it runs
`nacptool`/`elf2nro` automatically, since both are on `PATH` in this image),
producing `build/BetrockPlusPlus.nro` alongside the raw `.elf`. If you ever
need to do it by hand instead — e.g. those tools aren't found, or you want to
re-package with different NACP metadata — run:

```bash
nacptool --create "BetrockPlusPlus" "Pixel Brush + Aidan" "0.1.0" app.nacp
elf2nro BetrockPlusPlus.elf BetrockPlusPlus.nro --nacp=app.nacp
```

### 5. Run it

Copy `BetrockPlusPlus.nro` to `/switch/BetrockPlusPlus/` on your SD card and
launch it from the Homebrew Menu (hbmenu). It reads/writes its config and
world save relative to its own folder, same as on desktop.

## Nintendo 3DS

### 1. Install devkitPro

```bash
podman run -it --rm -v $(pwd):/project:Z docker.io/devkitpro/devkitarm
```

Select the docker image. This image bundles devkitARM, libctru, and
`3ds-tools` (which provides `3dsxtool`), so nothing further needs installing
for the toolchain itself.

### 2. Build libdeflate

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

### 3. Build the repo

```bash
cd /project
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake -DBUILD_SERVER=ON
cd build
cmake --build . -j$(nproc)
```

### 4. Package it into a `.3dsx`

The build already does this for you as a post-build step (it runs `3dsxtool`
automatically, since it's on `PATH` in this image), producing
`build/BetrockPlusPlus.3dsx` alongside the raw `.elf`. If you ever need to do
it by hand instead — e.g. the tool isn't found — run:

```bash
3dsxtool BetrockPlusPlus.elf BetrockPlusPlus.3dsx
```

### 5. Run it

Copy `BetrockPlusPlus.3dsx` to `/3ds/BetrockPlusPlus/` on your SD card and
launch it from the Homebrew Launcher. As on Switch, config and world data are
read/written relative to its own folder.

## Troubleshooting

- **`find_package(libdeflate CONFIG REQUIRED)` fails to find libdeflate** —
  double check step 2 installed it under
  `$DEVKITPRO/portlibs/switch` / `$DEVKITPRO/portlibs/3ds` (matching the
  toolchain file you passed to `-DCMAKE_TOOLCHAIN_FILE`), not the host's
  default prefix.
- **A `WARNING` about `ONLINE_MODE_AUTHENTICATION`, `BETACRAFT_HEARTBEAT`,
  `DISCORD_INTEGRATION`, or `CRASH_LOGGING`** — expected; see the note above.
  These stay off unless you force them back on, and nothing guarantees they
  build if you do (their dependencies have no Switch/3DS triplet).
- **`nacptool`/`elf2nro`/`3dsxtool` not found** — these ship in the
  `devkitpro/devkita64` and `devkitpro/devkitarm` images already; if you're
  using a different image or a bare devkitPro install, install the
  `switch-tools` (Switch) or `3ds-tools` (3DS) package via `dkp-pacman`, or
  just run the manual packaging command from step 4.