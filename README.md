# BetrockPlusPlus/Betrock++
![C++23](https://img.shields.io/badge/Language-C%2B%2B23-5E96CF)
![Issues](https://img.shields.io/github/issues/OfficialPixelBrush/BetrockPlusPlus)
![Pull requests](https://img.shields.io/github/issues-pr/OfficialPixelBrush/BetrockPlusPlus)

A from-scratch rewrite/combination of Beta++/Betrock and BetrockServer to combine their bests parts cleanly.

# Goals
A full, from-scratch reimplementation of Minecraft Beta 1.7.3.

1. Ideally BPP should be capable of acting as both a Client and Server
2. Functionality would be extended where desired or necessary, but generally compatibility and faithfulness will be prioritized
3. Cross-platform (Windows and Linux)
4. Fully open-source, anyone can fork, commit and contribute
5. Unless something requires decompiled code for the sake of accuracy, no decompiled code will be used, reimplemented or referenced. If it **is** used for anything, it'll be very clearly marked as such in the code via comments. At most it'll serve as a reference for what not to do, and what pitfalls we should avoid

## Discord
This is another project that's part of/worked on by the OpenBeta Community. We have a [Discord Server](https://discord.gg/JHTz2HSKrf)!

## Contributing
Please read the [CONTRIBUTING](./CONTRIBUTING.md) page.

## How to use
### Clone the Repository
Simply clone the respository with `git`.
```bash
git clone https://github.com/OfficialPixelBrush/BetrockPlusPlus.git
cd BetrockPlusPlus
```
Alternatively, download a **.zip**.

### Install Dependencies

#### Windows (10/11)
Prerequisites:
- CMake 3.16.0 (or later)
- MSVC 19.32 (or later)
- vcpkg

After those are all installed and set up, you can
install the remaining dependencies with **vcpkg**.
```powershell
# Install dependencies using vcpkg
.\vcpkg install libdeflate glfw3 glm openal-soft
```

#### Linux
Betrock++ also works on Linux! Theoretically, any Distro should be supported, so long as it has the required dependencies. Here're the commands for acquiring those on various Distros.

##### Debian / Ubuntu / Linux Mint
```bash
sudo apt install git cmake clang build-essential libdeflate-dev libglfw3-dev libglm-dev libopenal-dev libsdl3-dev libgl1-mesa-dev libasan8
```
> Note: `libsdl3-dev` is only packaged on Debian 13 (trixie) and newer, and Ubuntu 25.10 and newer. On Ubuntu 24.04/22.04 LTS it is not in `apt`, so you'll need a newer release or to build SDL3 from source.

##### RHEL / Fedora
```bash
sudo dnf install git cmake clang gcc gcc-c++ make libasan libdeflate-devel glfw-devel glm-devel openal-soft-devel SDL3-devel mesa-libGL-devel
```

##### Arch Linux / SteamOS / CachyOS
```bash
sudo pacman -S git cmake clang base-devel libdeflate glfw glm openal sdl3 libasan
```

##### openSUSE (Leap / Tumbleweed)
```bash
sudo zypper install git cmake clang gcc gcc-c++ make libdeflate-devel glfw-devel glm-devel openal-soft-devel SDL3-devel Mesa-libGL-devel libasan8
```

##### Alpine Linux
```bash
sudo apk add git cmake clang gcc g++ make libdeflate-dev glfw-dev glm-dev openal-soft-dev sdl3-dev mesa-dev compiler-rt
```
> Note: `sdl3-dev` is currently only in the **edge** branch's `community` repo. Also, Alpine ships no `libasan` and GCC's AddressSanitizer is broken on musl, so for Debug builds (which use `-fsanitize=address`) compile with **clang**, which uses the ASan runtime from `compiler-rt`.

##### Void Linux
```bash
sudo xbps-install -S base-devel git cmake clang libdeflate-devel glfw-devel glm libopenal-devel SDL3-devel MesaLib-devel libsanitizer-devel
```

##### Gentoo
```bash
sudo emerge dev-vcs/git dev-util/cmake sys-devel/clang sys-devel/gcc sys-devel/make dev-libs/libdeflate media-libs/glfw media-libs/glm media-libs/openal media-libs/libsdl3 media-libs/mesa
```

### Building

#### Option #1: Command-line
First you prepare and enter the build directory.
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cd build
```
This will make a Release Server build. If you'd like to build a client instead, use
```bash
cmake -S . -B build -DBUILD_SERVER=OFF -DCMAKE_BUILD_TYPE=Release
cd build
```
Then you build the project.
```bash
cmake --build .
```
After that, it's as easy as running the built application.
```bash
./BetrockPlusPlus
```

#### Option #2: Visual Studio Code
If your compiler, cmake and dependencies are properly set up, Visual Studio Code (or anything based on it, like VSCodium) should just work. Click the run or build buttons in the bar at the bottom.

#### Option #3: Visual Studio (Windows only)
**TODO**

# Related projects

- Beta++ by JcbbcEnjoyer (Minecraft Beta 1.7.3 Client written in C++)
- [LibreProg](https://github.com/OfficialPixelBrush/LibreProg) (fully FOSS Minecraft Beta 1.7.3 textures, sounds, etc.)
- [Technical Beta Wiki](https://officialpixelbrush.github.io/beta-wiki) (technical protocol and implementation reference)
- [Betrock](https://github.com/OfficialPixelBrush/Betrock) (McRegion world explorer)
- [BetrockServer](https://github.com/OfficialPixelBrush/BetrockServer) (Minecraft Beta 1.7.3 Server written in C++)

# Credits
- Barak Shoshany. Licensed under the MIT license. BS::thread_pool. https://github.com/bshoshany/thread-pool
