# Fonts

Current UI font: **Sarasa UI SC** (更纱黑体 UI SC), SIL Open Font License 1.1.

- `SarasaUiSC-Regular-subset.ttf` — body text
- `SarasaUiSC-SemiBold-subset.ttf` — large headings
- `SarasaUiSC-Bold-subset.ttf` — display title
- `OFL-SarasaGothic.txt` — license

The full source TTFs are not stored in the repository. To regenerate the
subsets, download `SarasaUiSC-TTF-1.0.43.7z` (or a newer release) from:

https://github.com/be5invis/Sarasa-Gothic/releases

Extract `SarasaUiSC-Regular.ttf`, `SarasaUiSC-SemiBold.ttf`,
`SarasaUiSC-Bold.ttf`, then run from the project root:

```powershell
$env:SARASA_SOURCE_DIR = "D:\path\to\SarasaUiSC-ttf"
python cpp/tools/make_font.py
```

The generated `cpp/src/generated/embedded_font.cpp` is committed, so a normal
build does not need Python or fontTools.
