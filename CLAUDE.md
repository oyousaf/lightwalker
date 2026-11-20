# Lightwalker

Unreal Engine 5.8 C++ remake of Omar's 2014 Flash game Lightwalker, a Robotron-style arena shooter. The original games and their extracted ActionScript are in [legacy/](legacy/). Repo: `oyousaf/lightwalker` (public).

This is a game project, kept in `D:\Documents\Unreal Projects`, separate from the web work in `D:\Documents\Projects`. The website rules in the global CLAUDE.md (footer signature, scroll-to-top, subdomains, portfolio entries) don't apply here. The git rules do.

## Design (agreed 2026-11-16)

- **One game, three modes**, tied together by light against darkness:
  1. **Arena** (core, from Lightwalker): top-down twin-stick waves of shaytaan (chasers) and dajjal (wanderers).
  2. **The Path** (from Green Ball): side-on 2.5D run-and-jump stage between arenas. Light fades over time; light orbs recharge it.
  3. **The Shield** (from block-breaker): deflect your own beam off a shield to break walls of darkness. Lowest priority, first to cut.
- **Structure:** Menu → Arena 1 → Path → Arena 2 → Shield → Boss, plus an Endless Arena with a leaderboard (standing in for the unfinished 2014 `get_score.php`).
- **Look:** top-down neon: dark arena, emissive glow, bloom. Camera tilted about 50–60° rather than straight down. Third person was rejected (breaks the 360° swarm design and needs far more art).
- **Devices:** scale with UE scalability settings, never by changing the camera.
- **Frame rate:** 60 fps by default (with V-sync); players can pick 30, 120, 144 or unlimited in Settings. Gameplay is frame-rate independent.

## Milestones (each one playable)

1. **Arena slice** (done in code): player, lasers, both enemy types, 15 waves (the two 2014 ones, then 13 that build on them), score, three lives, respawn, HUD. Dajjal take three shots.
2. **Polish:** lighting, Niagara effects, MetaSounds audio, camera shake, menus, gamepad UX. Menus done 2026-11-20: intro (Epic's official animated Unreal Engine logo as a startup movie, `Content/Movies/UE_Splash.mp4`, a 4K60 H.264 encode of their ProRes master, which stays out of the repo; then the title), main menu with Lightwalker in the dark arena, pause and settings. Still to do: control rebinding, and wiring the saved volumes and vibration once audio and rumble exist.
3. **Hero and pickups** (agreed 2026-11-18; pickups, the hero model and the light-orb glide done 2026-11-19; rigging and a run animation still to do, with the long robe as a cloth layer):
   - A realistic Lightwalker model holding a light weapon (lantern-staff or light gauntlet), designed to read from above, rigged (e.g. Mixamo) for run and aim.
   - Timed power-ups dropped by killed enemies (dajjal 40%, shaytaan 3%): light orb (bigger light radius and bolts), spread shot, piercing beam, shield (blocks one hit).
   - A rare **book of knowledge**: a plain glowing book with invented, non-scriptural writing (not modelled on the Quran, Bible or any real holy book). It gives a lasting upgrade for the rest of the run and shows a short line of general wisdom about light or knowledge.
4. **The Path.**
5. **The Shield.**
6. **Boss, endless mode, leaderboard, packaged Windows build.**

## Code layout (`Source/LightWalker`)

| Folder | Contents |
|---|---|
| `Core/` | `ULightWalkerGameInstance` (run score, lives, best score and book upgrades, shared by every mode); `ULightWalkerUserSettings` (saved graphics, display, volumes, vibration, hero choice); `ULWMusicSubsystem` (menu intro and loop, combat stems layered by wave, stings); `LWNeon` (applies the glow material) |
| `Pickups/` | `APickup` (power-ups and the book of knowledge; collected by walking over them, effects live on the pawn) |
| `Player/` | `ALightWalkerPawn`, `ALaser`, `ALightWalkerPlayerController` (builds Enhanced Input actions and bindings in code) |
| `Enemies/` | `AEnemyBase` (parts on a turning, bobbing Visual pivot), `AShaytaan` (chaser: a little devil), `ADajjal` (wanderer: a gold pyramid whose Horus-style eye turns to watch the player) |
| `Arena/` | `AArenaGameMode` (wave and life rules), `AArena` (floor, edges, camera, light, post-process), `UArenaWaveSet` (optional data asset for waves) |
| `UI/` | `AArenaHUD` (canvas HUD during play); `LWMenuWidgets` (intro, main, pause and settings menus, UMG widgets built in code, owned by the player controller) |

Conventions:
- Gameplay lives in C++. Blueprints only for editor-side wiring (assigning meshes, effects, sounds to C++ properties).
- The game runs with no project assets beyond `M_Neon` and `L_Arena`: input, the arena and the camera are all built in code. Engine basic shapes stand in for art.
- Screen up is world +X, screen right is world +Y. Input arrives as screen-space 2D vectors; `ALightWalkerPawn::ScreenToWorld` converts.
- Tunables are `UPROPERTY(EditDefaultsOnly)` with the 2014 value noted in the comment. 2014 pixels convert at 5 units per pixel; Flash ran at 24 fps.
- Each future mode gets its own `GameMode` and map (`APathGameMode`, `AShieldGameMode`), reusing the game instance and pawn pieces.

## Music

Two original themes, written as MIDI by `Tools/music/compose_*_theme.py`: a 16-bit platformer-style menu theme and a UK drill combat theme in six stems that layer in by wave. `Tools/music/render_music.py` renders placeholder WAVs into each theme's `SourceArt/Music/<Theme>/Export/` with FluidSynth and the GeneralUser GS SoundFont (both kept outside the repo in `D:\AI\Tools\audio`; the SoundFont's licence allows commercial use). `Tools/import_audio.py` imports them to `/Game/Audio/Music/`. Omar may rebuild either theme in FL Studio using the `SESSION.md` beside it: exporting over a placeholder of the same name and re-running the import swaps it in with no code changes.

## Building

The editor must be closed (Live Coding blocks command-line builds), or press Ctrl+Alt+F11 in the editor to Live Code instead.

```powershell
& "D:\Games\Epic Games\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" LightWalkerEditor Win64 Development "-Project=D:\Documents\Unreal Projects\LightWalker\LightWalker.uproject" -WaitMutex
```

Content setup (only needed if `M_Neon` or `L_Arena` is missing; editor closed):

```powershell
& "D:\Games\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Documents\Unreal Projects\LightWalker\LightWalker.uproject" -run=pythonscript -script="D:\Documents\Unreal Projects\LightWalker\Tools\create_content.py" -unattended -nosplash
```

## Dev flags (non-shipping builds)

- `-LWStartWave=N` skips the intro and menu and starts the run on wave N.
- `-LWMenu` skips the intro and opens on the main menu (with `-LWShots`, the screenshot includes the menu UI).
- `-LWEarlyShot` takes the first `-LWShots` screenshot 1.5 s in, during the wave intro, before enemies move (good for checking models).
- `-LWDrops` makes every kill drop a pickup (a book a quarter of the time) and lays one of each around the centre when a wave starts.
- `-LWShots=N` takes N high-res screenshots (2x) three seconds apart, into `Saved/Screenshots/WindowsEditor`, then quits. Use this to check visuals rather than capturing the desktop:

```powershell
& "D:\Games\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "D:\Documents\Unreal Projects\LightWalker\LightWalker.uproject" -game -windowed -ResX=1600 -ResY=900 -LWShots=2 -LWStartWave=2
```

## Environment

- UE 5.8.4 at `D:\Games\Epic Games\UE_5.8`.
- Visual Studio Community 2026 (v18) with the C++ game development workload, MSVC 14.52 and Windows SDK.
- Git LFS tracks `.uasset`, `.umap`, images, audio, PDFs and the Flash files (see `.gitattributes`).
