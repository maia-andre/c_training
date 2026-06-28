# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`plotter.c` is a single-file interactive function plotter (a mini-Desmos) built on **raylib**. The user types a formula for `f(x)`, adjusts parameters `a`/`b`/`c` via on-screen sliders, and the curve redraws live. Drag to pan, scroll to zoom, `R` to reset the view. Code comments and UI strings are in Portuguese (pt-BR, mostly without accents).

## Build & run

raylib 5.5 is installed **per-user in `~/.local`** (headers in `~/.local/include`, shared lib in `~/.local/lib`) — it is **not** a system package. Debian 13 'trixie' (this machine) has no raylib in its repos; the package only exists in Debian 14 'forky'/sid. So the `pkg-config`/system-`-lraylib` commands in `plotter.c`'s header comment do **not** work here — use the local-prefix command:

```sh
cc plotter.c -o plotter -I$HOME/.local/include -L$HOME/.local/lib -Wl,-rpath,$HOME/.local/lib -lraylib -lm
./plotter
```

`-Wl,-rpath` bakes the lib path into the binary so `./plotter` runs without `LD_LIBRARY_PATH`. `-lm` is required (the parser/evaluator uses `<math.h>`). There is no build system, linter, or test suite — it's one `.c` file compiled directly.

### Rebuilding raylib itself (rarely needed)

raylib was **built from source** (not the prebuilt binary) for verifiable provenance. The source clone (pinned to tag `5.5`) lives at `~/.local/src/raylib`. To rebuild and reinstall:

```sh
make -C ~/.local/src/raylib/src PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=SHARED -j$(nproc)
cp -a ~/.local/src/raylib/src/libraylib.so* ~/.local/lib/
cp -a ~/.local/src/raylib/src/raylib.h ~/.local/src/raylib/src/raymath.h ~/.local/src/raylib/src/rlgl.h ~/.local/include/
```

Build deps (from Debian `trixie` repos, GPG-verified via apt): `libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev`.

## Architecture

The file is organized into four labeled parts that form a clear pipeline:

1. **Parser (`PART 1`)** — A recursive-descent parser turns the formula string into an AST of `Node`s *once*, rather than re-interpreting text per pixel. Grammar precedence climbs `parseExpr` (+ -) → `parseTerm` (* /) → `parseUnary` (unary ± ) → `parsePower` (right-associative `^`) → `parsePrimary` (numbers, the `x`/`a`/`b`/`c` variables, `pi`/`e` constants, and `fn(...)` calls). `ev()` walks the AST to evaluate at a given `x` plus current `a,b,c`. Parsing uses two file-static globals: `P` (the read cursor) and `parseOK` (cleared on any invalid token). Multiplication must be explicit (`2*x`, not `2x`).

2. **Camera (`PART 2`)** — World↔screen transforms. `gCx,gCy` is the world point at the plot center, `gScale` is pixels-per-world-unit, and `gPx/gPy/gPw/gPh` is the plot rectangle (everything below the header). `SX/SY` map world→screen (Y is inverted), `WX/WY` map screen→world. `niceStep()` picks 1/2/5×10^k gridline spacing.

3. **Slider (`PART 3`)** — An immediate-mode slider widget. `activeSlider` (file-static) tracks which knob is being dragged so panning is suppressed while a slider is active.

4. **Main loop (`PART 4`)** — Standard raylib loop in `main()`: handle text input into `formula[]`, reparse only when `changed`, process pan/zoom (only inside the plot and when no slider is active), then draw grid → curve → header/UI.

### Things to know before editing

- **State is file-static globals**, not passed around: the camera (`gCx,gCy,gScale,gPx..gPh`), parser cursor (`P`, `parseOK`), and `activeSlider`. Functions like `SX`/`ev`/`slider` read these directly.
- **Reparse is gated on `changed`** in the main loop — every reparse does `freeNode(ast)` then `parseFormula()`. Don't leak the old AST when adding new triggers.
- **Adding a math function** requires three coordinated spots: the `F_*` enum, the name→id mapping in `fnId()`, and the evaluation `case` in `ev()`'s `N_FUNC` switch. `ln` and `log` both map to natural log; `log10` is base-10.
- **Discontinuity handling**: the curve renderer lifts the "pen" on non-finite values (NaN/inf) and skips segments whose vertical jump exceeds `gPh * 3` to avoid drawing vertical lines through asymptotes.
- The header height is the constant `HEADER` (132px); the plot area is derived from it each frame, so the window is resizable (`FLAG_WINDOW_RESIZABLE`). `Esc` is disabled as the exit key (`SetExitKey(KEY_NULL)`) so it doesn't interfere with typing.
