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

## Sarasa UI SC (更纱黑体 UI SC, subset)

- Project: https://github.com/be5invis/Sarasa-Gothic
- License: SIL Open Font License 1.1
- Copyright (c) 2015-2025, Renzhi Li (Belleve Invis), with portions from the Inter and Source Han Sans projects.
- Full text: `licenses/SarasaGothic-OFL.txt`
- Source: `cpp/third_party/fonts/`

Three embedded subsets are generated for the project: Regular (body), SemiBold (large headings), Bold (display title). The subsets remain under the SIL OFL 1.1.

## Dear ImGui 1.89.9

- Project: https://github.com/ocornut/imgui
- License: MIT
- Copyright (c) 2014-2024 Omar Cornut
- Full text: `licenses/Dear-ImGui-MIT.txt`
- Source: `cpp/third_party/imgui/`

Dear ImGui is compiled into the executable and used for the UI draw list and font atlas.

## ImGui-SFML 2.6.1

- Project: https://github.com/SFML/imgui-sfml
- License: MIT
- Copyright (c) 2017-2024 Elias Daler
- Full text: `licenses/ImGui-SFML-MIT.txt`
- Source: `cpp/third_party/imgui-sfml/`

ImGui-SFML is compiled into the executable as the Dear ImGui backend for SFML 2.6.

## MinGW-w64 runtime

The executable is statically linked with the MinGW-w64 GCC runtime (`libstdc++`, `libgcc`). MinGW-w64 runtime components are distributed under their own permissive licenses; no MinGW DLLs are required at runtime.

## Game code and assets

The C++ game code, level JSON, and UI artwork in this repository are original project material. No separate license file is provided for the game's own code; all rights remain with the project author unless a later LICENSE file says otherwise.
