# Hardware

This page lists all hardware that Betrock++ has been successsfully compiled and run on. This is also used to determine the true minimum requirements.

| Commit                                                                                                             | Architecture | OS                                                 | CPU                                             | Memory (Physical) | Swap/Paged Memory        | Model Name                     | Submitted by |
| ------------------------------------------------------------------------------------------------------------------ | ------------ | -------------------------------------------------- | ----------------------------------------------- | ----------------- | ------------------------ | ------------------------------ | ------------ |
| [`8cb7559`](https://github.com/OfficialPixelBrush/BetrockPlusPlus/commit/8cb755915b3a9ab2d65ef61670f4d2ad02de3f83) | x86 (i686)   | Gentoo Linux i686 (Linux 6.18.38-gentoo)           | mobile AMD Athlon(tm) XP-M 2600+ (1) @ 1.99 GHz | 256 MB            | 1GB (zram) + 4GB (swap)  | Fujitsu AMILO K7600            | Pixel Brush  |
| [`c5e64f8`](https://github.com/OfficialPixelBrush/BetrockPlusPlus/commit/c5e64f8702d27b5dff891bbd32276ae36cb8f443) | aarch64      | Alpine Linux v3.24 aarch64 (Linux 6.18.35-0-rpi)   | BCM2711 (4) @ 1.50 GHz                          | 2 GB              |                          | Raspberry Pi 4 Model B Rev 1.1 | jwaxy        |
| [`7d4b516`](https://github.com/OfficialPixelBrush/BetrockPlusPlus/commit/7d4b516e5a5a08164d4e1fafd43bec417fb3c373) | aarch64      | Raspberry Pi OS aarch64 (Linux 6.12.47+rpt-rpi-v8) | BCM2711 (4) @ 1.80 GHz                          | 8 GB              | 512MB (swap)             | Raspberry Pi 4 Model B Rev 1.5 | Pixel Brush  |
| [`7d4b516`](https://github.com/OfficialPixelBrush/BetrockPlusPlus/commit/7d4b516e5a5a08164d4e1fafd43bec417fb3c373) | x86_64       | Fedora Linux 44 (Linux 7.1.3-201.fc44.x86_64)      | AMD Ryzen 9 7900X (24) @ 5.74 GHz               | 32 GB             | 8GB (zram) + 16GB (swap) | (Custom)                       | Pixel Brush  |

## Minimum Requirements

The conditions to set or lower these requirements are relatively arbitrary, and mostly down to what feels right. These are only the lowest specs that have been tested and we can 100% confirm Betrock++ can run on without trouble\*.

> As of [`8cb7559`](https://github.com/OfficialPixelBrush/BetrockPlusPlus/commit/8cb755915b3a9ab2d65ef61670f4d2ad02de3f83) the known minimum requirements are:
>
> - OS: A Linux Distro with Kernel 6.12 or later
> - Processor: At least 32-Bit capable (i686 or later) @ >1GHz
> - Memory: ~256MB of available RAM (as in, not used by the operating system)

(Working with less memory is certainly possible, but does require the use of Swap or a Page file)

\*By "without trouble" we mean that you can

- Connect to the server
- Generate terrain
- Place, break blocks
- Explore with other players
- Not have the server lag to oblivion
