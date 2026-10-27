# River of Lamps — Third-Party Notices

This release is a statically linked Windows executable. The following third-party components are included in the executable and/or in the accompanying source tree. Their licenses remain with their respective authors.

## SFML 2.6.1

- Project: https://www.sfml-dev.org/
- License: zlib/png license (SFML license)
- Full text: `licenses/SFML-2.6.1.txt`
- Source: `cpp/third_party/SFML-2.6.1/`

SFML is statically linked into `RiverOfLamps.exe`.

## nlohmann/json

- Project: https://github.com/nlohmann/json
- License: MIT
- Copyright (c) 2013-2026 Niels Lohmann
- Full text: `licenses/nlohmann-json-MIT.txt`
- Source: `cpp/third_party/nlohmann/`

nlohmann/json is a header-only library compiled into the executable.

## Noto Sans SC (subset)

- Project: https://fonts.google.com/noto/specimen/Noto+Sans+SC
- License: SIL Open Font License 1.1
- Full text: `licenses/Noto-Sans-SC-OFL.txt`
- Source: `cpp/third_party/fonts/`

The embedded font is a subset generated from Noto Sans SC for this project. The subset remains under the SIL OFL 1.1.

## MinGW-w64 runtime

The executable is statically linked with the MinGW-w64 GCC runtime (`libstdc++`, `libgcc`). MinGW-w64 runtime components are distributed under their own permissive licenses; no MinGW DLLs are required at runtime.

## Game code and assets

The C++ game code, level JSON, and UI artwork in this repository are original project material. No separate license file is provided for the game's own code; all rights remain with the project author unless a later LICENSE file says otherwise.
