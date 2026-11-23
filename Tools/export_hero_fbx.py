"""
Exports the hero models to FBX for rigging on Mixamo (mixamo.com: Upload Character, place the markers, then download
the rigged character and its animations as FBX).

  SourceArt/Hero/Rigging/SM_Lightwalker.fbx
  SourceArt/Hero/Rigging/SM_LightwalkerFemale.fbx

Run headless with the editor closed:
  UnrealEditor-Cmd.exe "<path>/LightWalker.uproject" -run=pythonscript -script="<path>/Tools/export_hero_fbx.py"
"""

import os

import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
OUT_DIR = os.path.join(PROJECT_DIR, "SourceArt", "Hero", "Rigging")
os.makedirs(OUT_DIR, exist_ok=True)

for asset_path in ("/Game/Hero/SM_Lightwalker", "/Game/Hero/SM_LightwalkerFemale"):
    mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not mesh:
        unreal.log_error(f"[LW] Missing {asset_path}")
        continue
    options = unreal.FbxExportOption()
    options.set_editor_property("collision", False)
    options.set_editor_property("level_of_detail", False)
    task = unreal.AssetExportTask()
    task.object = mesh
    task.filename = os.path.join(OUT_DIR, asset_path.split("/")[-1] + ".fbx")
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.options = options
    ok = unreal.Exporter.run_asset_export_task(task)
    triangles = mesh.get_num_triangles(0) if hasattr(mesh, "get_num_triangles") else "?"
    unreal.log(f"[LW] {'Exported' if ok else 'FAILED'} {task.filename} ({triangles} triangles)")
