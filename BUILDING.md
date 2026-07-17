# Building Betrock++

This pages includes instructions on how to build and compile Betrock++ for various operating systems.

### 1. Clone the Repository
Simply clone the respository with `git`.
```bash
git clone https://github.com/OfficialPixelBrush/BetrockPlusPlus.git
cd BetrockPlusPlus
```
Alternatively, download a **.zip**.

### 2. Install Dependencies

#### Docker
Docker will take care of all of this stuff automatically, so skip to the [building step](#option-4-docker).

#### Windows (10/11)
Prerequisites:
- CMake 3.16.0 (or later)
- MSVC 19.32 (or later)
- vcpkg

After those are all installed and set up, you can
install the remaining dependencies with **vcpkg**.
```powershell
# Server + Client dependencies
.\vcpkg install libdeflate
# Client-exclusive dependencies
.\vcpkg install glfw3 glm openal-soft
```

Then move onto the [building step](#3-building).

#### Linux
Betrock++ also works on Linux! Theoretically, any Distro should be supported, so long as it has the required dependencies.
We recommend anything with GCC 13 and newer. Here're the commands for acquiring those on various Distros.

##### Debian / Ubuntu / Linux Mint
```bash
# Server + Client dependencies
sudo apt install git cmake clang build-essential libdeflate-dev libasan8
# Client-exclusive dependencies
sudo apt install libglfw3-dev libglm-dev libopenal-dev libsdl3-dev libgl1-mesa-dev
```
> Note: `libsdl3-dev` is only packaged on Debian 13 (trixie) and newer, and Ubuntu 25.10 and newer. On Ubuntu 24.04/22.04 LTS it is not in `apt`, so you'll need a newer release or to build SDL3 from source.

##### RHEL / Fedora
```bash
# Server + Client dependencies
sudo dnf install git cmake clang gcc gcc-c++ make libasan libdeflate-devel
# Client-exclusive dependencies
sudo dnf install glfw-devel glm-devel openal-soft-devel SDL3-devel mesa-libGL-devel
```

##### Arch Linux / SteamOS / CachyOS
```bash
# Server + Client dependencies
sudo pacman -S git cmake clang base-devel libdeflate libasan
# Client-exclusive dependencies
sudo pacman -S glfw glm openal sdl3
```

##### openSUSE (Leap / Tumbleweed)
```bash
# Server + Client dependencies
sudo zypper install git cmake clang gcc gcc-c++ make libdeflate-devel
# Client-exclusive dependencies
sudo zypper install glfw-devel glm-devel openal-soft-devel SDL3-devel Mesa-libGL-devel libasan8
```

##### Alpine Linux
```bash
# Server + Client dependencies
sudo apk add git cmake clang gcc g++ make libdeflate-dev
# Client-exclusive dependencies
sudo apk add glfw-dev glm-dev openal-soft-dev sdl3-dev mesa-dev compiler-rt
```
> Note: `sdl3-dev` is currently only in the **edge** branch's `community` repo. Also, Alpine ships no `libasan` and GCC's AddressSanitizer is broken on musl, so for Debug builds (which use `-fsanitize=address`) compile with **clang**, which uses the ASan runtime from `compiler-rt`.

##### Void Linux
```bash
# Server + Client dependencies
sudo xbps-install -S base-devel git cmake clang libdeflate-devel
# Client-exclusive dependencies
sudo xbps-install -S glfw-devel glm libopenal-devel SDL3-devel MesaLib-devel libsanitizer-devel
```

##### Gentoo
```bash
# Server + Client dependencies
sudo emerge dev-vcs/git dev-util/cmake sys-devel/clang sys-devel/gcc sys-devel/make dev-libs/libdeflate
# Client-exclusive dependencies
sudo emerge media-libs/glfw media-libs/glm media-libs/openal media-libs/libsdl3 media-libs/mesa
```

Then move onto the [building step](#3-building).

### 3. Building

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
cmake --build . -j$(nproc)
```
After that, it's as easy as running the built application.
```bash
./BetrockPlusPlus
```

#### Option #2: Visual Studio Code
If your compiler, `cmake` and dependencies are properly set up,
Visual Studio Code (or anything based on it, like VSCodium) should just work.
Click the run or build buttons in the bar at the bottom.

#### Option #3: Visual Studio (Windows only)
**TODO**

#### Option #4: Docker
Docker makes compiling and running a server significantly easier,
since it'll take care of all the dependency hoo-haa.

Please see the [docker markdown file for more info](./DOCKER.md).

Do note that most of the information in that file was AI-generated,
and isn't updated often.