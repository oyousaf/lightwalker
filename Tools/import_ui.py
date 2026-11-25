"""
Imports the artwork (made with ChatGPT image generation, then cut up into SourceArt/UI/Sliced by Tools/cut_art.py) as
textures: BC7 (a quarter of the size of uncompressed, with no visible loss), mipmaps for anything drawn smaller than its
pixels, and always fully loaded.

  SourceArt/UI/Sliced/<Name>.png  ->  /Game/UI/T_<Name>

The game looks for these by name (see UI/LWStyle) and falls back to its built-in look for any that are missing.
Re-running replaces the previous import. Set LW_ART to a comma-separated list of names to import only those.

Run headless with the editor closed:
  UnrealEditor-Cmd.exe "<path>/LightWalker.uproject" -run=pythonscript -script="<path>/Tools/import_ui.py"
"""

import glob
import os

import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE = os.path.join(PROJECT_DIR, "SourceArt", "UI", "Sliced")
DESTINATION = "/Game/UI"

# Full-screen pictures are drawn at about their own size, so they need no mipmaps.
FULL_SCREEN = ("MenuBackground", "Victory", "GameOver")
TILING = ("PathRock", "PathSky", "PathMountains", "PathRidges")

assets = unreal.EditorAssetLibrary
tasks = []
wanted = [name for name in os.environ.get("LW_ART", "").split(",") if name]
for path in sorted(glob.glob(os.path.join(SOURCE, "*.png"))):
    if wanted and os.path.splitext(os.path.basename(path))[0] not in wanted:
        continue
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = DESTINATION
    task.destination_name = "T_" + os.path.splitext(os.path.basename(path))[0]
    task.automated = True
    task.replace_existing = True
    task.save = True
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for task in tasks:
    asset_path = f"{DESTINATION}/{task.destination_name}"
    texture = assets.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        unreal.log_error(f"[LW] {asset_path} didn't import as a texture")
        continue
    name = task.destination_name[2:]
    full_screen = name.startswith(FULL_SCREEN)
    address = unreal.TextureAddress.TA_WRAP if name in TILING else unreal.TextureAddress.TA_CLAMP
    for setting, value in {
        "compression_settings": unreal.TextureCompressionSettings.TC_BC7,
        "lod_group": unreal.TextureGroup.TEXTUREGROUP_WORLD if name in TILING else unreal.TextureGroup.TEXTUREGROUP_UI,
        "mip_gen_settings": unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS if full_screen else unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE,
        "power_of_two_mode": unreal.TexturePowerOfTwoSetting.NONE,
        "filter": unreal.TextureFilter.TF_TRILINEAR,
        "address_x": address,
        "address_y": address,
        "never_stream": True,
        "srgb": True,
    }.items():
        texture.set_editor_property(setting, value)
    assets.save_asset(asset_path, only_if_is_dirty=False)
    unreal.log(f"[LW] {asset_path} {texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}")
