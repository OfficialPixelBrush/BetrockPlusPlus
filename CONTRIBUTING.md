# Contributing
Simply fork the [`main` branch](https://github.com/OfficialPixelBrush/BetrockPlusPlus/tree/main) and commit whatever changes you want to make.

Ensure your fork is up-to-date with the `main` branch, then make a pull request.

## AI policy

Generally, we're perfectly fine with adding code that was created with the help of AI tooling to our codebase.

What we *don't* want are slop-commits that just rewrite stuff for the sake of rewriting it. We don't want or need a 505k line mega-commit that adds 26 new files, that all have duplicated functionality, or breath other parts of the code.

Keep it reasonable, don't overdo it. This is supposed to be a learning-exercise and fun project for whoever wants to get involved, and to foster a better understanding of how Beta Minecraft works. This project doesn't need to be finished overnight in a half-broken state that nobody understands.

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
- `clangd` (ideally `clangd-22` for Doxygen comment integration)
- CMake Tools
- Doxygen Documentation Generator

## Style Guide
- Use types that guarantee a known bit-width (i.e. `int32_t`, `int16_t` or `int8_t`)
- If possible, do not pass values separately. Make use of structs that combine them
    - i.e. instead of `int32_t posX, int32_t posY, int32_t posZ`, just use `Int3 pos`
- Run `clang-format -i` over the files you changed
- Do not use raw block IDs. Make use of the Blocks enum
- Enums **must** have a type
- Provide a short description what a file is for the relevant file, why it exists and what its used for
- PLEASE include the copyright notice at the top. This also serves as a nice way for people to get credited, if the git history is ever lost. **Any** change will get you added to there (within reason).

Use the template below if you're creating a new page.
```text
/*
 * Copyright (c) [year], [name] <temp@example.com>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/
```