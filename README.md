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
Please read the [CONTRIBUTING](https://github.com/OfficialPixelBrush/BetrockServer/blob/main/CONTRIBUTING.md) page.

## Dependencies

Linux (Debian/Ubuntu)
```bash
sudo apt install git cmake build-essential libdeflate-dev libglfw3-dev libglm-dev libopenal-dev
```

Linux (RHEL/Fedora)
```bash
sudo dnf install git cmake gcc gcc-c++ make libdeflate-devel glfw-devel glm-devel openal-soft-devel
```

Windows (10/11)
```powershell
# Install dependencies using vcpkg
.\vcpkg install libdeflate glfw3 glm openal-soft
```

## Building

```bash
cmake -S . -B build
cd build
```

# Related projects

- Beta++ by JcbbcEnjoyer (Minecraft Beta 1.7.3 Client written in C++)
- [LibreProg](https://github.com/OfficialPixelBrush/LibreProg) (fully FOSS Minecraft Beta 1.7.3 textures, sounds, etc.)
- [Technical Beta Wiki](https://officialpixelbrush.github.io/beta-wiki) (technical protocol and implementation reference)
- [Betrock](https://github.com/OfficialPixelBrush/Betrock) (McRegion world explorer)
- [BetrockServer](https://github.com/OfficialPixelBrush/BetrockServer) (Minecraft Beta 1.7.3 Server written in C++)