# Building and required inputs

## Interface and protocol tests

Requirements: Windows x64 and the .NET 8 SDK. Build the WPF project with `dotnet build launcher/ForzaMastro.Launcher/ForzaMastro.Launcher.csproj -c Release`. Run the resulting `RaceOneHub.exe --self-test ABS_REPORT_PATH` for model, typed settings, persistence and offscreen-layout fixtures. It does not launch a game.

Python preparation tools require Python 3.11 or newer and the packages in `requirements.txt`. Use an isolated virtual environment; activate it before running the development frontend so its `python` backend command uses that environment. Installed tooling and environments are not repository content.

## User-supplied inputs

| Input | Expected identity / purpose |
| --- | --- |
| Extracted Forza Motorsport 5 | Package `Forza_1.0.0.20_x64__8wekyb3d8bbwe`, complete `Mount` and `EmbeddedXvd` directories. Game EXE SHA-256 `94dad21851d335e9a33231dc2a1de2ecd4b93eefdf5f6772085bf6886bf47db7`. |
| Original Azimuth 0.9.9 directory | `azimuth.exe` SHA-256 `579ed8d8b0b16cbf7890380cb7b925c972b837631ede367eada55eae4afc39bc`, accompanying TLS/audio/CRT dependencies and supplied license notices. Obtain it separately from its original provider. |
| Modern host XG implementation | Current private native path requires an eight-method XG implementation with SHA-256 `9e3a06d02784699c252bea271090b9e2a04ef2cb29b5b0647c6ce6e73b19db67`. This Microsoft SDK component is not supplied. The FM5 guest retains its own SDK XG libraries. A replacement/removal of the extra dependency remains future work. |
| Native build tooling | Visual Studio 2022 C++/MASM and Windows SDK. Development scripts currently name MSVC 14.44.35207 and Windows SDK 10.0.26100.0; review paths for your installation. |

`python tools/prepare_inputs.py --game-folder ABS_GAME_ROOT --azimuth-directory ABS_ENGINE_ROOT` validates explicit inputs without extracting, copying or executing them. This command does not prepare a complete runtime.

## Native source preparation gaps

The adapter source and exact metadata/patch recipes are included. Four include files deliberately stay out of the source release because they contain long original code guards or original/derived game shader data:

- `src/allocation-port/fm5_language_guards.inc`
- `src/allocation-port/fm5_resolve_original_guard.inc`
- `src/allocation-port/fm5_resolve_four_samples.inc`
- `src/allocation-port/fm5_resolve_eight_samples.inc`

`build_language_tests.py` contains user-local guard generation from pinned originals. Shader include preparation is not yet a clean-checkout automated workflow; the MSAA fixture scripts consume those generated inputs and do not replace that missing preparation step. Some development recipes additionally expect user-local proof metadata under `analysis`. Consequently `tools/build_pc_bridge.py` / `build_pc_host.py` are preserved development recipes, not a one-command supported clean-machine runtime build.

No placeholder shaders, fake backends or unconditional readiness manifests are supplied to conceal these gaps. Locally prepared binaries and generated data must remain ignored.

## Backend layout after independent preparation

The builder reads a prepared `runtime/pc/ForzaMastro5.exe`, `ForzaMastro5Bridge.dll`, `data/RaceOneEngine.ini`, runtime dependency DLLs and a truthful `launcher-capabilities.json`. The host XG dependencies live under `runtime/xg-host`. These paths are ignored and are not part of this source release. Optional user-supplied pre-game background artwork is likewise private.

The public backend stays unready when required inputs are absent. Creating a manifest without preparing and validating the native runtime is not a supported shortcut.

## Private generated output

A local `RaceOne5.exe` output references the user's original game folder. Only six small scene overrides are derived alongside the runtime; saves use the current Windows user's Saved Games folder. Generation requires an empty output directory outside the source game. Desktop shortcuts are optional. Do not distribute generated output folders, user-derived overrides, SDK libraries, saves or caches as this project's public release.
