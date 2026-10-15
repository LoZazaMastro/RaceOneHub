# Race One Hub interface

FM5-only Windows WPF frontend for the `RaceOne5.exe` builder protocol. Source directory names keep historical technical identifiers; the visible product is Race One Hub.

## Backend protocol

The development backend is `tools/pc_launcher_backend.py`. The single-file release embeds a frozen backend at the internal path `launcher/backend/ForzaMastro5Backend.exe`, alongside its reviewed engine/support payload. These components are unpacked automatically to the versioned LocalAppData cache. The user does not install Python or supply a separate backend. Source development and native packaging are described in [BUILDING.md](../docs/BUILDING.md).

```text
--game fm5 --capabilities [--game-folder ABS_GAME_ROOT]
--game fm5 --settings REQUEST.json --build --output ABS_OUTPUT/RaceOne5.exe --game-folder ABS_GAME_ROOT [--desktop-shortcut] [--progress-file PROGRESS.json]
```

Requests contain exactly nine values: integer `resolution` and `frame_rate`; string `antialiasing`, `graphics`, `display`, `language`, `audio`; boolean `vsync` and `show_pre_game`. The source folder, output path and shortcut are separate command preferences. MSAA choices are off, 2x and 4x; older 8x preferences migrate to 4x. Frame-rate choices are ordered 30, 60, 120; the interface default remains 60.

Capabilities schema 1 uses `game_id: "fm5"`, `output_filename: "RaceOne5.exe"`, `ready`, `build_ready`, supported choices and detected language labels. **Build EXE** requires the real runtime and readiness checks. A missing runtime cannot be bypassed by the UI. The backend never starts a game when building.

## Verification

`--self-test REPORT` checks model, request JSON, persistence and offscreen WPF layout. `--render-preview`, `--render-video`, `--render-audio` and `--render-credits`, each followed by a PNG path, render without showing a window or launching a game. `--check-backend REPORT` reads capabilities only. Reports and generated binaries stay outside Git.

Designed by **LoZazaMastro**. The authored tyre icon belongs to the Hub; the authored orange 5 belongs to generated game executables. Donate opens [Ko-fi](https://ko-fi.com/lozazamastro) only when clicked. See `LICENSE`, `THIRD-PARTY-NOTICES.txt` and the root documentation for developer build and distribution boundaries.
