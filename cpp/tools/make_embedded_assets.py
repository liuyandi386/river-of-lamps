#!/usr/bin/env python3
"""把现有 JSON 关卡/元件数据嵌入 C++，让 exe 可以单文件运行。

用法（在项目根目录或任意目录）：
    python cpp/tools/make_embedded_assets.py

生成：
    cpp/src/generated/embedded_json.cpp
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LEVELS = ROOT / "data" / "levels"
ELEMENTS = ROOT / "data" / "levels" / "elements.json"
OUT = ROOT / "cpp" / "src" / "generated" / "embedded_json.cpp"

DELIM = "RIVERJSON"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def raw_literal(text: str, name: str) -> str:
    if f"){DELIM}\"" in text:
        raise SystemExit(f"{name} contains the raw-string delimiter, choose another")
    return f'R"{DELIM}({text}){DELIM}"'


def main() -> int:
    index_path = LEVELS / "index.json"
    if not index_path.exists():
        print(f"missing {index_path}", file=sys.stderr)
        return 1
    index = json.loads(read(index_path))
    ids = [entry["id"] for entry in index["levels"]]
    missing = [i for i in ids if not (LEVELS / f"{i}.json").exists()]
    if missing:
        print(f"missing level files: {missing}", file=sys.stderr)
        return 1

    parts = [
        "// 由 cpp/tools/make_embedded_assets.py 生成，请勿手改。",
        '#include "generated/embedded_assets.h"',
        "",
        "namespace river::embedded {",
        "",
        f"const char* const kIndexJson = {raw_literal(read(index_path), 'index')};",
        f"const char* const kElementsJson = {raw_literal(read(ELEMENTS), 'elements')};",
        "",
    ]

    level_names = []
    for i, level_id in enumerate(ids, start=1):
        cpp_name = f"kLevel{i:02d}Json"
        level_names.append((level_id, cpp_name))
        parts.append(f"const char* const {cpp_name} = {raw_literal(read(LEVELS / (level_id + '.json')), level_id)};")

    parts += [
        "",
        "const LevelBlob kLevels[] = {",
    ]
    for level_id, cpp_name in level_names:
        parts.append(f'    {{"{level_id}", {cpp_name}}},')
    parts += [
        "};",
        "const std::size_t kLevelCount = sizeof(kLevels) / sizeof(kLevels[0]);",
        "",
        "}  // namespace river::embedded",
        "",
    ]
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(parts), encoding="utf-8")
    print(f"wrote {OUT.relative_to(ROOT)} ({OUT.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
