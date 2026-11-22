"""
Imports the UI artwork (made with ChatGPT image generation, then cut up into SourceArt/UI/Sliced/) as textures
set up for UI use: no mipmaps, UI texture group, full-quality compression with alpha.

  SourceArt/UI/Sliced/<Name>.png  ->  /Game/UI/T_<Name>

The game looks for these by name (see UI/LWStyle) and falls back to its built-in neon style for any that are
missing. Re-running replaces the previous import.

Run headless with the editor closed:
  UnrealEditor-Cmd.exe "<path>/LightWalker.uproject" -run=pythonscript -script="<path>/Tools/import_ui.py"
"""

import glob
import os

import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE = os.path.join(PROJECT_DIR, "SourceArt", "UI", "Sliced")
DESTINATION = "/Game/UI"

assets = unreal.EditorAssetLibrary
tasks = []
for path in sorted(glob.glob(os.path.join(SOURCE, "*.png"))):
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
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("srgb", True)
    assets.save_asset(asset_path, only_if_is_dirty=False)
    unreal.log(f"[LW] {asset_path} {texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}")
