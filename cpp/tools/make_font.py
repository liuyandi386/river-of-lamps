#!/usr/bin/env python3
"""从 Sarasa UI SC 静态字重生成子集，并嵌入 C++。

用法：
    python cpp/tools/make_font.py

来源（OFL 1.1）：
    https://github.com/be5invis/Sarasa-Gothic
    SarasaUiSC-Regular.ttf / SarasaUiSC-SemiBold.ttf / SarasaUiSC-Bold.ttf

默认从 cpp/third_party/fonts/source/ 读取源文件；也可设置环境变量
SARASA_SOURCE_DIR 指向解压后的 SarasaUiSC-*.ttf 目录。

输出：
    cpp/third_party/fonts/SarasaUiSC-Regular-subset.ttf
    cpp/third_party/fonts/SarasaUiSC-SemiBold-subset.ttf
    cpp/third_party/fonts/SarasaUiSC-Bold-subset.ttf
    cpp/src/generated/embedded_font.cpp

只在改动 UI 文案/关卡中文后需要重跑；构建本身不需要 Python/fonttools。
"""
from __future__ import annotations

import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FONT_DIR = ROOT / "cpp" / "third_party" / "fonts"
SOURCE_DIR = Path(os.environ.get("SARASA_SOURCE_DIR", FONT_DIR / "source"))
OUT_CPP = ROOT / "cpp" / "src" / "generated" / "embedded_font.cpp"

# 正文/标签用 Regular；大标题用 SemiBold；展示字“灯河”用 Bold。
SOURCES = [
    ("regular", "SarasaUiSC-Regular.ttf", "SarasaUiSC-Regular-subset.ttf"),
    ("semibold", "SarasaUiSC-SemiBold.ttf", "SarasaUiSC-SemiBold-subset.ttf"),
    ("bold", "SarasaUiSC-Bold.ttf", "SarasaUiSC-Bold-subset.ttf"),
]

TEXT_GLOBS = [
    "data/**/*.json",
    "docs/**/*.md",
    "README.md",
    "概要.txt",
    "cpp/src/**/*.cpp",
    "cpp/src/**/*.h",
    "cpp/tests/**/*.cpp",
]

EXTRA_CHARS = (
    "，。！？：；“”‘’（）【】《》、·—…～≤≥×→←↑↓"
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
    " /\\|+-=[]{}()<>:;,.!?@#$%^&*_~`'\""
)

# 24px 以上的 UI 文字：菜单标题“选择关卡”、通关标题“通过！/差一点”。
HEADING_TEXT = "选择关卡通过差一点！0123456789 "
# 76px 展示字：只用于主菜单标题。
TITLE_TEXT = "灯河"


def collect_text() -> str:
    chunks: list[str] = []
    for pattern in TEXT_GLOBS:
        for path in sorted(ROOT.glob(pattern)):
            if not path.is_file():
                continue
            try:
                chunks.append(path.read_text(encoding="utf-8"))
            except UnicodeDecodeError:
                print(f"skip non-utf8: {path}", file=sys.stderr)
    return "\n".join(chunks) + EXTRA_CHARS


def make_subset(source: Path, target: Path, text: str) -> tuple[int, int, int]:
    from fontTools import subset
    from fontTools.ttLib import TTFont

    target.parent.mkdir(parents=True, exist_ok=True)
    codepoints = {ord(ch) for ch in text}
    codepoints.update({0x20, 0x09, 0x0A, 0x0D})

    options = subset.Options()
    options.layout_features = ["*"]
    options.name_IDs = ["*"]
    options.name_legacy = True
    options.notdef_outline = True
    options.recalc_bounds = True
    options.recalc_timestamp = False

    subset_font = subset.load_font(str(source), options)
    subsetter = subset.Subsetter(options=options)
    subsetter.populate(unicodes=codepoints)
    subsetter.subset(subset_font)
    subset.save_font(subset_font, str(target), options)
    subset_font.close()

    final = TTFont(target)
    glyph_count = len(final.getGlyphOrder())
    final.close()
    return len(codepoints), glyph_count, target.stat().st_size


def emit_cpp(entries: list[tuple[str, Path]]) -> None:
    names = {
        "regular": ("kFontRegularData", "kFontRegularSize"),
        "semibold": ("kFontHeadingData", "kFontHeadingSize"),
        "bold": ("kFontDisplayData", "kFontDisplaySize"),
    }
    lines = [
        "// 由 cpp/tools/make_font.py 生成，请勿手改。",
        "// Font: Sarasa UI SC (subset), SIL Open Font License 1.1.",
        '#include "generated/embedded_assets.h"',
        "",
        "namespace river::embedded {",
        "",
    ]
    for key, path in entries:
        data = path.read_bytes()
        array_name, size_name = names[key]
        lines.append(f"const unsigned char {array_name}[] = {{")
        for i in range(0, len(data), 16):
            chunk = data[i : i + 16]
            lines.append("    " + ", ".join(f"0x{byte:02x}" for byte in chunk) + ",")
        lines += [
            "};",
            f"const std::size_t {size_name} = {len(data)};",
            "",
        ]
    lines += ["}  // namespace river::embedded", ""]
    OUT_CPP.parent.mkdir(parents=True, exist_ok=True)
    OUT_CPP.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    missing = [name for _, name, _ in SOURCES if not (SOURCE_DIR / name).exists()]
    if missing:
        print(f"missing Sarasa UI SC source in {SOURCE_DIR}: {missing}", file=sys.stderr)
        print("download: https://github.com/be5invis/Sarasa-Gothic/releases", file=sys.stderr)
        return 1

    text = collect_text()
    print(f"corpus: {len(text)} chars, {len(set(text))} unique")
    entries: list[tuple[str, Path]] = []
    for key, source_name, subset_name in SOURCES:
        source = SOURCE_DIR / source_name
        target = FONT_DIR / subset_name
        subset_text = text if key == "regular" else (
            HEADING_TEXT if key == "semibold" else TITLE_TEXT)
        count, glyphs, size = make_subset(source, target, subset_text)
        print(f"{key}: {size} bytes, {glyphs} glyphs, {count} codepoints -> {target.name}")
        entries.append((key, target))
    emit_cpp(entries)
    print(f"wrote {OUT_CPP.relative_to(ROOT)} ({OUT_CPP.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
