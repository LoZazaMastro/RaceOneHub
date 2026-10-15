# Source and binary release boundaries

This repository contains the authored FM5 compatibility adapters, adapted GPL builder UI, tests, numeric/tyre icons, patch metadata and the explicitly included pre-game background. Background credit belongs to Microsoft and the respective rights holders. No actual game executable, game/SDK DLL, populated game-derived shader include, archive, save, private log or local tool installation belongs in the source tree.

The single-file `RaceOneHub.exe` artifact is separate from the Git source snapshot. It packages the reviewed engine/support components and unhydrated adapter template as internal resources; the app extracts its support cache automatically. The user's own FM5 folder supplies the actual game and the data needed to create the private generated output. There is no separate user-facing engine or toolchain setup.

Building the source interface alone does not assemble the engine payload. Native developer inputs, licensing boundaries, template preparation and final verification are covered in [BUILDING.md](BUILDING.md). Engine and third-party notices retain their original copyright and license terms; this project's GPL license does not relicense them.

Gameplay screenshots, other game artwork, original game packages, SDK distributions, private development material and work on other games are outside this release. Package exact reviewed files rather than a whole working directory. A successful component-container test alone does not establish a playable release.
