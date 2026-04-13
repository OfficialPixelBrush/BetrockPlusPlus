# Contributing
Simply fork the [`main` branch](https://github.com/OfficialPixelBrush/BetrockPlusPlus/tree/main) and commit whatever changes you want to make.

Ensure your fork is up-to-date with the `main` branch, then make a pull request.

## Development
Grab the `main` branch for the most up-to-date, albeit unstable, repository.
If any of the submodules have been updated, grab their latest version.
```bash
git submodule update --recursive --remote
```

## Build
Lastly, build with the `Debug` config to make debugging easier.
We use `-fsanitize=address` to provide us with a readable debug trace.
```bash
cmake --build . --config Debug
```

## Recommended VSCode Extensions
- clangd (ideally clangd-22 for Doxygen comment integration)
- CMake Tools
- Doxygen Documentation Generator

## Style Guide
- Use types that guarantee a known bit-width (i.e. `int32_t`, `int16_t` or `int8_t`)
- If possible, do not pass values separately. Make use of structs that combine them
    - i.e. instead of `int32_t posX, int32_t posY, int32_t posZ`, just use `Int3 pos`
- Run `clang-format -i` over the files you changed
- Do not use raw block IDs. Make use of the Blocks enum