# Race One Hub

A Windows PC builder and compatibility-adapter project for **Forza Motorsport 5**.

Choose your extracted game folder, graphics, audio and language settings. With a locally prepared runtime, the builder generates `RaceOne5.exe` and its supporting files. Your original game stays in its own folder and is not modified. The optional pre-game settings screen uses the same native window and process as the game.

**This release contains source code. It does not include a prepared runtime or a ready-to-play game.** Building the WPF interface is supported from this checkout; preparing the complete native runtime still requires external user-supplied dependencies and locally generated shader/guard inputs. See [Building and required inputs](docs/BUILDING.md).

No game executable, SDK DLL, game artwork, archive, save, private research log, or proprietary base engine is included or downloaded. Generated game folders are private local outputs and must not be published as the builder download.

![Race One Hub FM5 builder](docs/screenshots/builder.png)

The preview uses example folder paths. No game content appears in the image.

## How to play

You need your own complete, extracted Xbox One copy of Forza Motorsport 5, version **1.0.0.20**, and the locally prepared runtime described in [Building and required inputs](docs/BUILDING.md).

1. Open `RaceOneHub.exe` and select the game folder containing `Mount` and `EmbeddedXvd`.
2. Choose an output location. The builder creates a **Forza Motorsport 5** folder there for the executable and its supporting files.
3. Choose your video, audio and language settings. Leave **Skip pre-game settings** unchecked if you want to adjust them before each launch. You can also create a Desktop shortcut.
4. Click **Build EXE** and wait for it to finish.
5. Open the generated `RaceOne5.exe`, or use the Desktop shortcut. If the pre-game screen is enabled, select **Start game**.

Keep the generated files together and leave the original game folder in place. The game reads its files from that folder; moving or deleting it will stop the game from launching.

## Options

- Resolutions from 1080p to 8K; 30, 60 and 120 FPS targets.
- Original, Low, High and Ultra graphics presets; MSAA off, 2x or 4x; VSync and fullscreen/windowed display.
- Languages detected from the selected game; stereo or system multichannel audio.
- Optional Desktop shortcut; pre-game settings enabled by default.

![FM5 video settings](docs/screenshots/video.png)

## Known issue

The Career menu can look washed out after a race. Press **B** to return to mode selection, then enter Career again. The underlying issue remains unresolved.

## Credits

- **Almost There Games** - [Azimuth](https://almost-there-games.itch.io/azimuth), the external base compatibility project. Its original/adapted binaries are not included.
- [WinDurango](https://github.com/WinDurango/WinDurango) and [XWine1](https://github.com/xwine1) - research and API references.
- **Turn 10 Studios / Microsoft Studios** - Forza Motorsport 5 and its trademarks. This project is independent and is not affiliated with them.

Authored project source and adapted builder UI are distributed under [GPL-3.0](LICENSE). This does not relicense user-supplied games or external runtimes. See [third-party notices](launcher/THIRD-PARTY-NOTICES.txt).

[Support LoZazaMastro](https://ko-fi.com/lozazamastro)
