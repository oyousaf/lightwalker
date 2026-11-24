# Models

Drop the generated models here, shrink their textures (`Tools/shrink_glb.py`), then run the import (editor closed):

```powershell
& "D:\Games\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Documents\Unreal Projects\LightWalker\LightWalker.uproject" -run=pythonscript -script="D:\Documents\Unreal Projects\LightWalker\Tools\import_models.py" -unattended -nosplash
```

| File | Becomes | What |
|---|---|---|
| `shaytaan.glb` | `/Game/Enemies/SM_Shaytaan` | the devil (chaser) |
| `dajjal.glb` | `/Game/Enemies/SM_Dajjal` | the all-seeing pyramid (wanderer), and the boss's guardians |
| `one_eyed_liar.glb` | `/Game/Enemies/SM_OneEyedLiar` | the boss, built from its picture by `Tools/make_boss_model.py` |
| `lightwalker.glb`, `lightwalker_female.glb` | materials and textures in `/Game/Hero` | the heroes; their meshes are the rigged ones (`Tools/rig_hero.py`, then `Tools/import_hero_rig.py`) |

`.gltf` and `.fbx` work too. The game scales each model to fit and turns it to face forward; a missing one shows as a plain glowing ball.

Export settings: a single unrigged mesh with PBR textures, around 10–30k triangles, front facing the camera in the generator's default view.

## Concept image prompts (Flux 2 in ComfyUI)

**Devil**

> Concept art of a small mischievous devil imp, chubby round body, two sharp horns sweeping forward, glowing yellow eyes, rosy-red skin with subtle texture, short stubby arms and legs, no tail, three-quarter front view, full body, centred, plain white background, game character design, highly detailed, realistic

**Pyramid**

> A floating golden pyramid artefact with a single realistic human eye set into its front face, Eye of Horus markings engraved around the eye, polished ancient gold with fine hieroglyph engravings, three-quarter front view, centred, plain white background, game asset concept, highly detailed, realistic

For image-to-3D, use one object per image, plain background, nothing cropped. The devil has no tail because the game adds its glowing trail.
