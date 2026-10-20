# Skater Nation (PS Vita): Release v0.1-beta

> **Status: beta release.** The SceGxm loading crash is fixed (run 31). Gameplay runs at 25–28 FPS in fullscreen with audio (run 33). Audio and the HUD ring opacity are not yet verified on console (see Known Issues).

---

## Resolved: SceGxm crash during level loading (runs 22–31)

- **Symptom**: abort in `SceGxm seg1 + 0xa548` (`sceGxmSetViewport`, via `gles_present → glClear → scene_reset`) while a level was loading. The registers were identical in every run.
- **Cause**: the graphics path of the widescreen feature added in `297e1cc` (`gles.c`). Run 30 showed that no GXM call returned an error, which ruled out the "failed scene" theory. Run 31 restored the `7e6ebbe` graphics path, and the crash went away.
- **Fixed along the way**: a heap-garbage crash in `WorldManager + 0x4528` (run 28). `operator new` did not zero memory as iOS does, and the constructor never initializes that field. Guest `malloc`/`operator new` now return zero-filled memory.

| Run | Change | Result |
|---|---|---|
| 23–27 | Tile clipper, region guards, scene count, `call_guest`, `glFlush` no-op | Same SceGxm crash, or the fault moved |
| 28 | Audio paused | Crash in game code: garbage in `WorldManager + 0x4528` |
| 29 | Zero-filled `malloc` / `operator new` | `WorldManager` crash fixed; SceGxm crash back |
| 30 | Log GXM scene error codes | No GXM errors logged: scenes do not fail |
| 31 | `gles.c` + `input.c` restored to `7e6ebbe` | **No crash.** About 7 FPS (Debug build plus per-frame logging) |
| 32 | Release build | No crash, still 7–8 FPS, black side bars |
| 33 | Blades of Fury fullscreen stretch, audio on, perf log | **25–28 FPS**, fullscreen, audio |
| 34 | Correct HudMenu offsets (`mc_pad` ring) | Pending console test |

Per-run details (in Spanish): [`port_progress.md`](port_progress.md).

---

## Release Checklist

- [x] Resolve the loading crash and play a level on console.
- [x] Build in Release (no `DEBUG_LOADER`).
- [x] Fullscreen with no black bars.
- [ ] Verify music and sound effects with audio re-enabled (`AUDIO_ENABLED 1`).
- [ ] Verify the 1% opacity on the joystick ring (`mc_pad`), thumb, and A/B (run 34).
- [x] Generate `rebase.bin` for version 1.2.1 (60,307 words, magic `RBS1`).
- [x] Tag `v0.1-beta`.
- [ ] Publish the GitHub release with `skaternation.vpk` + `rebase.bin` attached.

---

## Package Contents

1. **`skaternation.vpk`**: native Mach-O loader, vitaGL, FreeType, OpenAL Soft, and physical + touch controls.
2. **`rebase.bin`**: relocation table (magic `'RBS1'`, word count, then 32-bit `vmaddr` values) for the iOS 1.2.1 executable mapped at `0x98000000`.

Installation and controls: see [`README.md`](README.md).

---

## Changelog (since 7e6ebbe)

- **Memory**: guest `malloc` and `operator new`/`new[]` return zero-filled memory, matching iOS.
- **Graphics**: fullscreen stretch from 480×320 to 960×544, the same method as the Blades of Fury port, with no black bars. The `297e1cc` widescreen path was removed because it crashed SceGxm. Touch is mapped `x*480/960`.
- **Performance**: 25–28 FPS in gameplay. Per-frame logging was removed (`AudioSessionGetProperty('othr')`). `gles_present` writes a `perf:` line every 5 s (fps, frame ms, swap ms). Swap takes 0.1–0.3 ms, so the frame time is CPU-bound.
- **HUD**: 1% opacity on `btn_dpad`, `mc_pad`, `mc_thumb`, `mc_btn_a` and `mc_btn_b` through `gameswf::character::set_member("_alpha")` (called via `call_guest`), with an L+R toggle. It is re-applied when the HUD is rebuilt. Offsets come from `HudMenu::Init`. The minimap (`mc_map`) was hidden by mistake before and is now visible.
- **Audio**: re-enabled (OpenAL Soft + RemoteIO on sceAudioOut, `MAIN` port), plus `MPMusicPlayerController` stubs.
- **vitaGL**: scene errors are reported to the loader log (`vgl_gxm_error_hook`), and `vglSetupRenderTargetScenesNum` is clamped to [1, 8].
- **Logging**: the game's `printf`/stdout output moved to `l_debug`, which is compiled out in Release.

## Known Issues

- Music/sound effects and the 1% opacity on the joystick ring (`mc_pad`) are not yet verified on console.

- Frame rate drops to 0.6–3 FPS during level loads.
- The fullscreen stretch makes the image slightly wider than the original 3:2.
- The camera sensitivity OSD is disabled (`patch_overlay` is empty).
