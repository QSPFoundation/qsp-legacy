# QSP Legacy

This repository contains source code of the QSP Legacy game engine.

## Useful links

### Documentation

* Docs: https://github.com/QSPFoundation/qspfoundation.github.io

### Players

* Classic QSP Legacy player: https://github.com/QSPFoundation/qspgui-legacy
* Quasporium: https://play.google.com/store/apps/details?id=com.pixnpunk.quasporium

## Linux & MacOS build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Windows build

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DCMAKE_INSTALL_PREFIX=out
cmake --build build --target install --config Release
```

## Chat group

Discord: https://discord.gg/6gWVYUtUGZ | Telegram: https://t.me/QuestopiaChat