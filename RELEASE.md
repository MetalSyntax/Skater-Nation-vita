# Skater Nation (PS Vita): Release v0.1-beta (ON HOLD)

> **Status: not releasable.** A regression is still open, and the game crashes during startup or level loading. The last playable build is commit `7e6ebbe`.

---

## Active Issue (blocker)

**SceGxm crash during level loading** (a failed `sceGxmBeginScene` followed by an abort in `sceGxmSetViewport`). A related heap-garbage crash was fixed in run 29.

- **Latest crash (run 28)**: `Application::LoadOrientationStyle` → `SetStatusBarOrientation(bool)+264` reads `WorldManager + 0x4528`, gets garbage (`0x1a000003`) and faults.
- **Root cause**: `WorldManager` is allocated with `operator new(0x46d0)`, and its constructor never initializes `+0x4528`. iOS returns zero-filled fresh memory; newlib does not.
- **Related**: runs 22–27 crashed in `SceGxm seg1 + 0xa548` (`sceGxmSetViewport` after a failed `sceGxmBeginScene`) during level loading, with identical registers. Unrelated changes kept moving the fault, which is consistent with the same heap-garbage cause.
- **Run 29**: zero-filled `malloc`/`operator new` fixed the `WorldManager` crash, but the SceGxm crash is back (`gles_present → glClear → scene_reset`). Audio (muted stubs) and HUD opacity (never executed per the log) are ruled out.
- **Run 30 (instrumentation)**: the patched vitaGL now reports the error code of every failed `sceGxmBeginScene`/`sceGxmEndScene`/`sceGxmCreateRenderTarget` to the loader log, along with free VRAM/RAM/PHYCONT, before SceGxm aborts.
- **History**:

  | Run | Change | Result |
  |---|---|---|
  | 23 | `NO_TILE_CLIPPER` | Same SceGxm crash |
  | 24 | Tile clipper restored + `region.w/h > 0` guards | Same SceGxm crash |
  | 25 | `vglSetupRenderTargetScenesNum(4, 16)` | New boot crash (sceGxm allows at most 8). Reverted. |
  | 26 | `call_guest` for the HUD opacity calls | Same SceGxm crash (r9 was already protected by `-ffixed-r9`) |
  | 27 | `glFlush` as a no-op | Crash moved from `endScene → glFlush` to `gles_present → glClear` |
  | 28 | Audio paused (`AUDIO_ENABLED 0`) | Crash moved into game code: garbage in `WorldManager + 0x4528` |
  | 29 | Zero-filled `malloc` / `operator new` | `WorldManager` crash fixed; SceGxm crash back |
  | 30 | Log GXM scene error codes + free GPU memory | Same crash, **no GXM errors logged**: scenes do not fail, so the "failed scene" theory is wrong |
  | 31 | Bisect: `gles.c` graphics path + `input.c` restored to 7e6ebbe | **Pending console test** |

- **Run 30 finding**: no GXM call returns an error; the abort is deterministic inside SceGxm (identical registers every run) with an intact GXM context.
- **Run 31 (bisect)**: the only part of the `7e6ebbe → 297e1cc` delta that runs during loading is the `gles.c` graphics path, so it is restored verbatim (pillarboxed 3:2 output, real `glFlush`). The widescreen feature is temporarily off.
- **Next steps**: if run 31 loads, reintroduce widescreen in steps (perspective-only `wide_begin` first, then `wide_span` removal). Then, once loading is stable, re-enable audio (`AUDIO_ENABLED 1`) in its own run. If the SceGxm crash comes back, bisect the `7e6ebbe → 297e1cc` delta (`wide_begin` projection, then HUD opacity, then scissor).

Per-run details (in Spanish): [`port_progress.md`](port_progress.md).

---

## Release Checklist

- [ ] Resolve the active issue and play a full level on console.
- [ ] Re-enable audio (`AUDIO_ENABLED 1`) and verify music and sound effects.
- [ ] Verify 16:9 fullscreen without stretching and the 1% opacity (joystick and A/B).
- [ ] Build in Release (no `DEBUG_LOADER`).
- [ ] Generate `rebase.bin` for version 1.2.1 (60,307 words).
- [ ] Commit, tag `v0.1-beta`, and attach `skaternation.vpk` + `rebase.bin`.

---

## Planned Package Contents

1. **`skaternation.vpk`**: native Mach-O loader, vitaGL, FreeType, OpenAL Soft, and physical + touch controls.
2. **`rebase.bin`**: relocation table (magic `'RBS1'`, word count, then 32-bit `vmaddr` values) for the iOS 1.2.1 executable mapped at `0x98000000`.

Installation and controls: see [`README.md`](README.md).

---

## Changelog (since 7e6ebbe)

- **Memory**: guest `malloc` and `operator new`/`new[]` return zero-filled memory, matching iOS.
- **Graphics**: 16:9 fullscreen. Perspective projections are scaled by `WIDE_K` (wider FOV) and the 2D UI stays 1:1. Touch is mapped from 960 to 480 with no bars.
- **HUD**: 1% opacity on the joystick and A/B buttons through `gameswf::character::set_member("_alpha")` (called via `call_guest`), with an L+R toggle. It is re-applied when the HUD is rebuilt.
- **vitaGL**: guards against degenerate regions in `sceGxmSetRegionClip`; `vglSetupRenderTargetScenesNum` is clamped to [1, 8].
- **GL**: `glFlush` is a no-op (Irrlicht calls it on every `endScene`).
- **Audio**: `MAIN` port for RemoteIO, logging wrappers for OpenAL, `MPMusicPlayerController` stubs. Currently paused.
- **Logging**: the game's `printf`/stdout output moved to `l_debug`.

## Known Issues

- The active issue above (blocker).
- Audio is paused while the crash is isolated.
- The camera sensitivity OSD is disabled (`patch_overlay` is empty).
