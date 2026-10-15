# Race One Hub

A Windows PC builder and compatibility-adapter project for **Forza Motorsport 5**.

Choose your extracted game folder, graphics, audio and language settings. With a locally prepared runtime, the builder generates `RaceOne5.exe` and its supporting files. Your original game stays in its own folder and is not modified. The optional pre-game settings screen uses the same native window and process as the game.

**This release contains source code. It does not include a prepared runtime or a ready-to-play game.** Building the WPF interface is supported from this checkout; preparing the complete native runtime still requires external user-supplied dependencies and locally generated shader/guard inputs. See [Building and required inputs](docs/BUILDING.md).

No game executable, SDK DLL, game artwork, archive, save, private research log, or proprietary base engine is included or downloaded. Generated game folders are private local outputs and must not be published as the builder download.

![Race One Hub FM5 builder](docs/screenshots/builder.png)

The preview uses example folder paths. No game content appears in the image.

## Options

- Resolutions from 1080p to 8K; 30, 60 and 120 FPS targets.
- Original, Low, High and Ultra graphics presets; MSAA off, 2x or 4x; VSync and fullscreen/windowed display.
- Languages detected from the selected game; stereo or system multichannel audio.
- Optional Desktop shortcut; pre-game settings enabled by default.

![FM5 video settings](docs/screenshots/video.png)

## Known issue

The Career menu can look washed out after a race. Press **B** to return to mode selection, then enter Career again. The underlying issue remains unresolved.

## Build the interface

On Windows with the .NET 8 SDK:

```powershell
dotnet build launcher/ForzaMastro.Launcher/ForzaMastro.Launcher.csproj -c Release
```

The interface accurately keeps **Build EXE** unavailable until its backend reports a prepared runtime. Compiling the interface alone does not prepare that runtime. [The backend protocol](launcher/README.md) and [source layout](docs/BUILDING.md) describe the boundaries.

## Credits

- **Almost There Games** - [Azimuth](https://almost-there-games.itch.io/azimuth), the external base compatibility project. Its original/adapted binaries are not included.
- [WinDurango](https://github.com/WinDurango/WinDurango) and [XWine1](https://github.com/xwine1) - research and API references.
- **Turn 10 Studios / Microsoft Studios** - Forza Motorsport 5 and its trademarks. This project is independent and is not affiliated with them.

Authored project source and adapted builder UI are distributed under [GPL-3.0](LICENSE). This does not relicense user-supplied games or external runtimes. See [third-party notices](launcher/THIRD-PARTY-NOTICES.txt).

[Support LoZazaMastro](https://ko-fi.com/lozazamastro)
