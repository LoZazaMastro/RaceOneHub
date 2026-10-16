# Developer build instructions

These instructions are for building Race One Hub from source. They are not prerequisites for using the single-file `RaceOneHub.exe` download. End users supply only their own extracted FM5 folder.

## Interface and support container

Use Windows x64 and the .NET 8 SDK:

```powershell
dotnet build launcher/ForzaMastro.Launcher/ForzaMastro.Launcher.csproj -c Release
```

`RaceOneHub.exe --self-test ABS_REPORT_PATH` checks settings, persistence, backend requests and component extraction with offscreen UI fixtures. It does not launch a game. `--check-backend ABS_REPORT_PATH` reads backend capabilities without generating an executable or starting a game.

The release interface is published self-contained, so the user does not need a separate .NET installation:

```powershell
dotnet publish launcher/ForzaMastro.Launcher/ForzaMastro.Launcher.csproj -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -p:EnableCompressionInSingleFile=true -p:RaceOnePayload=ABS_SUPPORT_ARCHIVE
```

`RaceOnePayload` embeds the reviewed support archive as `RaceOneHub.Support.zip`. The archive is an internal resource, not a second download. `tools/build_single_exe_support.py` builds it from an explicit file/provenance map. Its manifest records each relative path, size and SHA-256. The app validates the archive and extracts its contents atomically to `%LOCALAPPDATA%\RaceOneHub\components\<payload-hash>`. It rechecks cached component hashes on later starts.

Only named support files enter the payload. The actual game executable, game assets, SDK libraries taken from the user's game, saves, logs and private development files must stay out. The authorized pre-game background is the sole game-artwork exception. Never package an entire private runtime directory or development checkout.

## Native engine and adapter development

Native development requires the Visual Studio 2022 x64 C++ compiler, MASM and Windows SDK. The verified developer toolchain is MSVC 14.44.35207 with Windows SDK 10.0.26100.0. Python 3.11+ is used for patch recipes, tests and packaging; the backend is frozen for the release, so Python is not required on the user's PC.

The compatibility host is derived from Azimuth 0.9.9. It is not rebuilt from these adapter sources. Developer host preparation requires that exact original engine and its notices; native source and recipe files do not substitute for the engine. Original game/engine identity checks are recorded in `recipes/runtime-inputs.json` and the other reviewed recipe files. All external copyrights and license notices remain applicable.

The four ignored language/shader includes contain game-dependent data and are generated locally for developer builds with `tools/generate_fm5_includes.py`. Its two engine inputs are locale routines that the prepared host keeps unchanged, so `--prepared-host` with the packaged `runtime/pc/ForzaMastro5.exe` can stand in for `--engine-exe` when the original engine is not available; the generated includes and the adapter built from them are identical. They must not enter Git or be embedded as populated game-data arrays in the public builder. The single-file packaging path uses an unhydrated adapter template; the builder fills the required data from the user's selected FM5 folder before writing the private generated output. Developer template-generation and hydration tools must retain exact bounds, identity checks and negative tests.

`tools/stage_single_exe_runtime.py` is a developer staging tool. Its explicit allowlist excludes both XG libraries from the user's game and the modern SDK XG dependency used by earlier private builds. Build the native adapter with `tools/build_pc_bridge.py --graphics-adapters --portable-xg`; this selects the authored adapter for the XG libraries in the user's FM5 folder. No modern XG library is shipped. Use `tools/fm5_bridge_template.py make-template` to strip the developer-built DLL, then stage that template and its manifest. The actual builder hydrates the template only inside each new generated output, leaving the shared component cache unchanged. Earlier `prepare_public_runtime.py` experiments are developer diagnostics, not an installation step or required download for end users.

## Verification and publication

Compiling the UI, reading capabilities, compiling an adapter and booting a generated game are separate checks. An embedded archive that extracts correctly is not proof that gameplay works. Before release, verify the actual frozen backend generates an FM5 output from only the selected original game folder, then check that output's pre-game and game startup. Keep the result tied to the exact final executable and payload hashes.

The known washed-out Career-menu issue remains documented in the README. Do not turn a packaging or bootstrap test into a claim that all gameplay has been validated.

Do not commit or publish generated `RaceOne5.exe` folders, hydrated adapters, user-derived include files, SDK libraries, saves or caches. The public binary artifact is the single reviewed `RaceOneHub.exe`, accompanied by source and third-party notices as required by their licenses.
