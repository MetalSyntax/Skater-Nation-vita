# Skater Nation PS Vita — Release v0.1-beta

**Title ID:** `PSVSN0001`  
**Target Platform:** PlayStation Vita / PlayStation TV  
**Base Game:** *Skater Nation* (Gameloft) — iOS version 1.2.1  
**Status:** Playable / In-Game 3D / 25–28 FPS / Fullscreen  

---

## 🛹 Introduction

This is the first public beta (**v0.1-beta**) of **Skater Nation** for the PlayStation Vita and PlayStation TV.

*Skater Nation* is Gameloft's 3D skateboarding game. You ride open street levels, chain ollies, grinds and tricks into combos, complete challenges, and unlock skaters and board parts (decks, trucks and wheels).

The port runs the original binary on the Vita through a custom 32-bit Mach-O loader. The original ARM executable runs natively on the Vita's ARM Cortex-A9. The Irrlicht engine and the gameswf Flash UI run unmodified, while the Apple frameworks are replaced with vitaGL, OpenAL Soft, FreeType and the Vita system libraries.

---

## 🌟 Features & Highlights

- **Native Execution (No Emulation):** Directly runs the original decrypted 32-bit iOS ARM Mach-O executable on the PS Vita CPU.
- **Fullscreen 960x544 Display:** The game's 480x320 frame is stretched to the full Vita screen, with no letterboxing or black bars.
- **Smooth Gameplay:** 25–28 FPS in-game (capped at 30 FPS), CPU-bound with a GPU swap cost under 1 ms.
- **Dual Input Subsystem:**
  - Full physical controls: Left Analog Stick, D-Pad, Face Buttons ($\square$, $\Delta$, $\bigcirc$, $\times$) and Shoulder Triggers (L / R), injected as touches on the game's own HUD.
  - Right Analog Stick camera orbit, with adjustable sensitivity (1–10) that is saved between sessions.
  - Front touchscreen support for all menus, shops and dialogs.
- **Clean HUD:** The on-screen joystick and A/B buttons are drawn at 1% opacity, since you play with the physical controls. L + R toggles them back to 100%.
- **Sound System:** OpenAL mapped to OpenAL Soft, and AudioUnit RemoteIO on top of `sceAudioOut`.
- **iOS Memory Semantics:** Zero-filled heap allocations (`malloc` / `operator new`), which the game relies on, as it does on iOS.
- **Runtime Apple CgBI PNG Decoding:** On-the-fly decompression and byte-swapping for all game textures.
- **LiveArea Integration:** Custom icon, startup gate and background graphics.

---

## 🎮 Controls

| Button | Skating Action | Menu Action |
| :--- | :--- | :--- |
| **Left Analog Stick / D-Pad** | Steering (virtual joystick) | — |
| **Right Analog Stick** | Camera Orbit | — |
| **$\times$ (Cross) / R** | Button A: Ollie / Jump | Confirm |
| **$\square$ (Square) / $\bigcirc$ (Circle) / L** | Button B: Grind / Trick | — |
| **$\Delta$ (Triangle)** | Trick Modifier | — |
| **START** | Pause | Pause / Menu |
| **SELECT** | Map / Objectives | — |
| **SELECT + D-Pad ↑/↓** | Camera Sensitivity 1–10 (saved) | — |
| **L + R together** | Toggle HUD joystick / A-B opacity (1% ↔ 100%) | — |
| **Touchscreen** | Full Touch HUD | Direct Touch |

---

## 📋 Installation Requirements

Make sure your PS Vita has Henkaku / Enso installed with the following plugin active in `ur0:tai/config.txt`:

```ini
*KERNEL
ur0:tai/kubridge.skprx
```

- **kubridge.skprx**
- **libshacccg.suprx** in `ur0:data/` (Extract using [ShaRKBR33D](https://github.com/OsirizX/ShaRKBR33D))

---

## 📦 How to Install

1. Download and install **`skaternation.vpk`** on your PS Vita using VitaShell.
2. Obtain a legal, decrypted copy of *Skater Nation* for iOS (v1.2.1 `.ipa`, FairPlay `cryptid 0`).
3. Extract the IPA file (rename to `.zip` and extract).
4. Inside `Payload/`, locate the `SkateNation.app` folder.
5. On your computer, run the staging tool to generate the relocation table and prepare the data folder:
   ```bash
   tools/stage_data.sh path/to/SkateNation.app
   ```
6. Copy the contents of `ux0_data/skaternation/` to `ux0:data/skaternation/` on your PS Vita via FTP or USB.

   The directory on your console should look like this:
   ```
   ux0:data/skaternation/
     ├── SkateNation.app/
     │     ├── SkateNation
     │     ├── Info.plist
     │     ├── _packMac/
     │     └── ...
     ├── rebase.bin
     ├── saves/
     └── logs/
   ```
7. Launch **Skater Nation** from your LiveArea and enjoy!

---

## ⚠️ Known Issues

- The frame rate drops to 0.6–3 FPS while a level is loading. It recovers once gameplay starts.
- The fullscreen stretch makes the image slightly wider than the original 3:2 aspect ratio.
- Music and sound effects have not been fully verified on every level yet.
- Online features (Gameloft servers, in-game ads) are not available.

---

## 🛠️ Release Artifacts

- **`skaternation.vpk`**: Standalone PlayStation Vita installable package (`PSVSN0001`).
- **`eboot.bin`**: Standalone executable SELF for quick deployment via FTP.

---

## 👥 Credits

- **Gameloft:** Original game developer and publisher.
- **Rinnegatamante:** Author of `vitaGL` and core contributor to the Vita homebrew ecosystem.
- **TheFloW:** Author of `kubridge`.
- **Wonder Diaz:** Port developer (loader, ABI layer, graphics, audio, controls, and optimization).
