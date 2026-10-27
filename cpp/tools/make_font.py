#!/usr/bin/env python3
"""从 Windows 的 Noto Sans SC 变量字体生成一个很小的子集，并嵌入 C++。

用法：
    python cpp/tools/make_font.py

来源字体（OFL）：C:/Windows/Fonts/NotoSansSC-VF.ttf
输出：
    cpp/third_party/fonts/NotoSansSC-subset.ttf
    cpp/src/generated/embedded_font.cpp

只在改动 UI 文案/关卡中文后需要重跑；构建本身不需要 Python/fonttools。
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE_FONT = Path("C:/Windows/Fonts/NotoSansSC-VF.ttf")
SUBSET_FONT = ROOT / "cpp" / "third_party" / "fonts" / "NotoSansSC-subset.ttf"
OUT_CPP = ROOT / "cpp" / "src" / "generated" / "embedded_font.cpp"

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
    "，。！？：；“”‘’（）【】《》、·—…～"
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
    " /\\|+-=[]{}()<>:;,.!?@#$%^&*_~`'\""
)


def collect_text() -> str:
    chunks: list[str] = []
    for pattern in TEXT_GLOBS:
        for path in sorted(ROOT.glob(pattern)):
            if path.is_file():
                try:
                    chunks.append(path.read_text(encoding="utf-8"))
                except UnicodeDecodeError:
                    print(f"skip non-utf8: {path}", file=sys.stderr)
    return "\n".join(chunks) + EXTRA_CHARS


def make_subset(source: Path, target: Path, text: str) -> tuple[int, int, int]:
    from fontTools import subset
    from fontTools.ttLib import TTFont
    from fontTools.varLib.instancer import instantiateVariableFont

    target.parent.mkdir(parents=True, exist_ok=True)
    static_font = target.with_name("NotoSansSC-static.ttf")

    font = TTFont(source)
    try:
        instantiateVariableFont(font, {"wght": 500}, inplace=True)
        font.save(static_font)
    except Exception as exc:  # noqa: BLE001
        print(f"warning: variable instancing failed ({exc}); using source as-is", file=sys.stderr)
        static_font = source

    codepoints = {ord(ch) for ch in text}
    codepoints.update({0x20, 0x09, 0x0A, 0x0D})
    options = subset.Options()
    options.layout_features = ["*"]
    options.name_IDs = ["*"]
    options.name_legacy = True
    options.notdef_outline = True
    options.recalc_bounds = True
    options.recalc_timestamp = False

    subset_font = subset.load_font(str(static_font), options)
    subsetter = subset.Subsetter(options=options)
    subsetter.populate(unicodes=codepoints)
    subsetter.subset(subset_font)
    subset.save_font(subset_font, str(target), options)
    subset_font.close()

    if static_font != source:
        try:
            static_font.unlink(missing_ok=True)
        except OSError:
            pass

    final = TTFont(target)
    glyph_count = len(final.getGlyphOrder())
    final.close()
    size = target.stat().st_size
    return len(codepoints), glyph_count, size


def emit_cpp(font_path: Path, out_path: Path) -> None:
    data = font_path.read_bytes()
    lines = [
        "// 由 cpp/tools/make_font.py 生成，请勿手改。",
        "// Font: Noto Sans SC (subset), SIL Open Font License 1.1.",
        '#include "generated/embedded_assets.h"',
        "",
        "namespace river::embedded {",
        "",
        "const unsigned char kFontData[] = {",
    ]
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        lines.append("    " + ", ".join(f"0x{byte:02x}" for byte in chunk) + ",")
    lines += [
        "};",
        f"const std::size_t kFontSize = {len(data)};",
        "",
        "}  // namespace river::embedded",
        "",
    ]
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    if not SOURCE_FONT.exists():
        print(f"missing source font: {SOURCE_FONT}", file=sys.stderr)
        return 1
    text = collect_text()
    unique_chars = len(set(text))
    print(f"corpus: {len(text)} chars, {unique_chars} unique")
    count, glyphs, size = make_subset(SOURCE_FONT, SUBSET_FONT, text)
    print(f"subset: {size} bytes, {glyphs} glyphs, {count} codepoints")
    emit_cpp(SUBSET_FONT, OUT_CPP)
    print(f"wrote {OUT_CPP.relative_to(ROOT)} ({OUT_CPP.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
