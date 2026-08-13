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

- CMake 3.25 (or later)
- MSVC 19.32 (or later)
- vcpkg

After those are all installed and set up, you can
install the dependencies with **vcpkg**.

```powershell
vcpkg install
```

Then move onto the [building step](#3-building).

#### Linux

Betrock++ also works on Linux! Theoretically, any Distro should be supported, so long as it has the required dependencies.

Prerequisites:

- CMake 3.25 (or later)
- GCC 13+ or Clang 17+
- Glibc 2.35+ or Musl 1.2.4+

**GCC 13+** is recommended, though Clang should work too, same goes for **glibc** and **musl**. The install instructions below assume `gcc`/`g++` though.

##### Debian / Ubuntu / Linux Mint

```bash
# Server + Client dependencies
sudo apt install git cmake build-essential libdeflate-dev libasan8 libcurl4-openssl-dev
# Client-exclusive dependencies
sudo apt install libglm-dev libsdl3-dev libgl1-mesa-dev
```

> Note: `libsdl3-dev` is only packaged on Debian 13 (trixie) and newer, and Ubuntu 25.10 and newer. On Ubuntu 24.04/22.04 LTS it is not in `apt`, so you'll need a newer release or to build SDL3 from source.

##### RHEL / Fedora

```bash
# Server + Client dependencies
sudo dnf install git cmake gcc gcc-c++ make libasan libdeflate-devel libcurl-devel
# Client-exclusive dependencies
sudo dnf install glm-devel SDL3-devel mesa-libGL-devel
```

##### Arch Linux / SteamOS / CachyOS

```bash
# Server + Client dependencies
sudo pacman -S git cmake base-devel libdeflate libasan curl
# Client-exclusive dependencies
sudo pacman -S glm sdl3
```

##### openSUSE (Leap / Tumbleweed)

```bash
# Server + Client dependencies
sudo zypper install git cmake gcc gcc-c++ make libdeflate-devel libcurl-devel
# Client-exclusive dependencies
sudo zypper install glm-devel SDL3-devel Mesa-libGL-devel libasan8
```

##### Alpine Linux

```bash
# Server + Client dependencies
sudo apk add git cmake gcc g++ make libdeflate-dev curl-dev
# Client-exclusive dependencies
sudo apk add glm-dev sdl3-dev mesa-dev compiler-rt
```

> Note: `sdl3-dev` is currently only in the **edge** branch's `community` repo. Also, Alpine ships no `libasan` and GCC's AddressSanitizer is broken on musl, so for Debug builds (which use `-fsanitize=address`) compile with **clang**, which uses the ASan runtime from `compiler-rt`.

##### Void Linux

```bash
# Server + Client dependencies
sudo xbps-install -S base-devel git cmake libdeflate-devel libcurl-devel
# Client-exclusive dependencies
sudo xbps-install -S glm SDL3-devel MesaLib-devel libsanitizer-devel
```

##### Gentoo

```bash
# Server + Client dependencies
sudo emerge dev-vcs/git dev-build/cmake sys-devel/gcc dev-build/make app-arch/libdeflate net-misc/curl
# Client-exclusive dependencies
sudo emerge media-libs/glm media-libs/libsdl3 media-libs/mesa
```

Then move onto the [building step](#3-building).

### Optional: Reduced Terrain Precision

Reduced Terrain Precision is **off by default**. Enabling it reduces the floating-point precision of the Perlin and Simplex noise generators from 64-bit to 32-bit floats. This shouldn't do much on systems with a dedicated floating-point co-processor, such as an Intel 8087, or integrated floating-point functionality, like most x86 CPUs made after ~1987, as they use the same 80-Bit registers for 32-bit and 64-bit floating-point math, the only difference being potential memory bandwidth usage.

The main benefits are for some microcontrollers or cost-reduced x86 chips that don't have integrated floating-point support, and thus need to emulate it all in software. Examples for such include RISC-V cores that lack the F (float) and D (double) extensions (e.g. RV32I, RV64IM) or the i486SX.

Simply add `-DREDUCED_GENERATION_PRECISION=ON` to the first build command, then resume as normal.

The only major difference this option introduces is that the farlands do not generate, and they just become an infinite ocean with a bedrock floor along the X-Axis, and the same but with a grid of blocks along the Z-Axis.

### Optional: Online Mode Authentication

Online Mode Authentication is **on by default**. It allows users to be authenticated via the legacy Minecraft Login protocol. By default this goes through the Betacraft.uk proxy, since the original authentication servers got shut down long ago. It requires `libcurl`.

Simply add `-DONLINE_MODE_AUTHENTICATION=OFF` to the first build command, if you'd like to disable it. This removes all auth-related code.

### Optional: Discord Integration

Discord support is **off by default**. Enabling it pulls in [D++](https://dpp.dev/) (Gateway WebSocket bot) via vcpkg or FetchContent, and requires OpenSSL.

Simply add `-DDISCORD_INTEGRATION=ON` to the first build command, then resume as normal.

In `server.properties`:

| Key                  | Purpose                                                             |
| -------------------- | ------------------------------------------------------------------- |
| `discord-token`      | Bot token                                                           |
| `discord-channel-id` | Channel used for chat bridge + crash uploads                        |
| `discord-guild-id`   | Optional. When set, slash commands register to that guild instantly |

In the [Discord Developer Portal](https://discord.com/developers/applications), enable the **Message Content Intent**, invite the bot with `applications.commands` + `bot` scopes, and grant read/send message permissions in the bridge channel.

> **Windows note:** if installing D++ through vcpkg, use a non-static triplet (`x64-windows`, not `x64-windows-static`).

### 3. Building

#### Option #1: Command-line

First you prepare and enter the build directory.

```bash
cmake -S . -B build
cd build
```

This will make a Release Server build. If you'd like to build a client instead, use

```bash
cmake -S . -B build -DBUILD_SERVER=OFF
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
