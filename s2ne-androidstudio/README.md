# S2NE Android Studio port

Builds the shared C++ game with an SDL3 Android launcher. No game assets are included.
The app does not pack, unpack, import or move game resources.

## Resources and launch

Place already-extracted resources in `/sdcard/s2ne`. Keep the original case of
`SUPERSP.PAL`, `SUPERSP.BIN`, `SUPERSP.CEL`, `SUPERSP.PRG`, `SUPERSP.DAT`,
`KINDSP.BIN`, `ITEMSP.BIN`, `ITEMHLSP.BIN`, `FSTAGE.BIN` and `SUPERSP.CFG`.
The font must be named `chinese.ttf` (rename a supplied `chinese.TTF` when copying).
Copy the `texture`, `json`, `cc` and `audio` directories too; MIDI synthesis uses
`audio/GeneralUser-GS.sf2`. DAT/PRG/PAL must be the matching merged runtime bundle,
not unrelated original disc files. Do not copy Windows executables or DLLs.

Open S2NE to enter the game directly, without a launcher page or launch button.
If storage access is missing, approve the automatically opened Android system
permission request; the game starts automatically after approval. Android 10 uses runtime storage
permissions; Android 11+ opens the application's all-files-access settings page.
This broad permission is needed for the requested fixed shared-storage directory
and native file IO (including saves); no permission is granted automatically.
Denied permission or missing resources produce an explicit error dialog, not a guide page.
This is a sideload/development setup, not a claim of Play Store policy eligibility.

The game uses this directory as its working directory before cheat/texture setup.
Saves and configuration are written there; the launcher does not replace resources
or delete existing saves. Optional `cheat` must be placed there explicitly by the user.
SDL owns the entire immersive fullscreen surface, with no Java button overlays.
GameActivity enforces landscape even when SDL requests another orientation.
Normal touch maps to SDL mouse input; external mouse/keyboard input remains available.
Tap the bottom-right corner (rightmost 20% of screen width and bottom 20% of height)
to send a right click at the current game cursor, without moving it to the corner.
The whole corner touch gesture is consumed, so it does not also send a left click.
Cancelled or multi-finger corner gestures do not click. No overlay is drawn.
The previous system Back-to-Escape mapping has been removed; Back uses SDL defaults.

## Build

- JDK compatible with Gradle 8.12; AGP 8.10.1; SDK 35.
- NDK `29.0.14206865`, CMake `3.22.1`; Android 10/API29 minimum; ARM64 only currently.
- Set `ANDROID_HOME` or an ignored `local.properties` SDK path.
- Set `-Ps2neDependencies=<Android dependency prefix>` containing matching SDL3,
  SDL3_ttf, SDL3_mixer with FluidSynth decoder, FluidSynth and transitive shared libs.
  Fallback: `VCPKG_ROOT/installed/arm64-android-dynamic`, then project
  `deps/installed/arm64-android-dynamic`.
  With vcpkg, install `sdl3-mixer[fluidsynth,mpg123]:arm64-android-dynamic` explicitly:
  the core-only package disables both MIDI and MP3. Keep `MP3` beside the other
  runtime directories when `CDDA=ON`.
- Set `-PsdlJavaDirectory=<matching SDL3 source>/android-project/app/src/main/java`.
  Fallback: `deps/SDL/android-project/app/src/main/java`. Do not use SDL2 Java sources.
- Set `-Ps2neJsonInclude=<include containing nlohmann/json.hpp>` if not in the
  Android dependency prefix's include directory.

```powershell
.\gradlew.bat :app:assembleDebug :app:lintDebug
```

For optimized experiments, use `:app:assembleOptimized :app:lintOptimized` with
the same dependency properties. This variant uses native `RelWithDebInfo`
(`-O2 -g -DNDEBUG`), keeps Java debugging and the debug signing key, and updates
the same application without changing resources or saves. APK:
`app/build/outputs/apk/optimized/app-optimized.apk`. Unstripped native symbols
remain in the local `.cxx/RelWithDebInfo` build directory; packaged libraries are stripped.
Ordinary Debug stays unoptimized for comparison.

Native CMake builds `libmain.so` from the shared production sources. Runtime library
staging excludes gtest/gmock. Build/dependency directories and machine paths are ignored.
Release uses the local debug keystore, matching the reference kys-cpp project for
sideload testing; this is not a production signing identity. Keep that keystore
stable for compatible updates. Signed Release APK: `app/build/outputs/apk/release/app-release.apk`.
Debug APK: `app/build/outputs/apk/debug/app-debug.apk`.
The installed label is 北方密使复刻版. Adaptive launcher icon artwork is cropped from
the runtime `texture/opening/19.png` image, with density-specific PNGs stored in
the Android resources. No other game assets are embedded.

On this Windows workspace, run `build-android-release.bat` in the parent game
resource directory to build Release, run lint, verify its signature and export
`S2NE-android-release.apk` beside the script. It does not install or restart the
game. Environment overrides: `JAVA_HOME`, `ANDROID_HOME`, `VCPKG_ROOT`,
`S2NE_DEPENDENCIES`, `S2NE_JSON_INCLUDE`, `SDL_JAVA_DIRECTORY`. Defaults match the
current machine, including its matching SDL3 source checkout; change overrides
when moving the build environment. Keep the same debug keystore for updates.

## Verified and pending

Native/Java APK build and Android lint pass. Windows Release, shared Big5 conversion
regression and isolated save-store regression pass. Modified C++ passed required
clang-format dry-run. Packaged dependencies, SDL_main export and ELF LOAD >=16KB
alignment were checked. Lint has narrow exclusions for permission-guarded upstream
SDL HID Bluetooth code and its API-guarded legacy receiver overload.

On 2026-11-27, resources were copied to `/sdcard/s2ne` on the authorized Android10
device and the APK installed successfully. After removing the guide page, launch
reached GameActivity directly (659ms); storage permissions were user-granted.
Logs confirm SDL_main execution, renderer initialization and audio device startup;
GameActivity remained resumed and the process alive. No adb permission bypass was
used. Visual output, audio playback, touch, background/resume and save/load still
need interactive verification.

`tests/text_encoding_test.cpp` checks Big5 Chinese round trips, ASCII, empty input
and invalid UTF-8. It has run on Windows; the Android JNI branch needs a running VM.
