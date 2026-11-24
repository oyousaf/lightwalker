"""
Imports the AI-generated models from SourceArt/Models into the project.

  SourceArt/Models/shaytaan.glb     ->  /Game/Enemies/SM_Shaytaan  (plus its materials and textures)
  SourceArt/Models/dajjal.glb       ->  /Game/Enemies/SM_Dajjal
  SourceArt/Models/one_eyed_liar.glb  ->  /Game/Enemies/SM_OneEyedLiar
  SourceArt/Models/lightwalker.glb, lightwalker_female.glb  ->  /Game/Hero (materials and textures only: the heroes'
      meshes are the rigged ones, so run Tools/import_hero_rig.py after this)

.glb, .gltf and .fbx all work. Re-running replaces the previous import, and a model that is missing is skipped. Run
Tools/shrink_glb.py on new models first. Set LW_MODELS to a comma-separated list of names to import only those.

Run headless with the editor closed:
  UnrealEditor-Cmd.exe "<path>/LightWalker.uproject" -run=pythonscript -script="<path>/Tools/import_models.py"
"""

import os

import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceArt", "Models")
EXTENSIONS = (".glb", ".gltf", ".fbx")

# Source name -> (content folder, mesh name; None keeps only its material). The game looks for <folder>/<mesh name>.
MODELS = {
    "shaytaan": ("/Game/Enemies", "SM_Shaytaan"),
    "dajjal": ("/Game/Enemies", "SM_Dajjal"),
    "one_eyed_liar": ("/Game/Enemies", "SM_OneEyedLiar"),
    "lightwalker": ("/Game/Hero", None),
    "lightwalker_female": ("/Game/Hero", None),
}

GLOW_MATERIAL = "/Game/Materials/M_ModelGlow"

assets = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
materials = unreal.MaterialEditingLibrary


def find_source(name):
    for extension in EXTENSIONS:
        path = os.path.join(SOURCE_DIR, name + extension)
        if os.path.isfile(path):
            return path
    return None


def import_model(name, root, mesh_name):
    source = find_source(name)
    if not source:
        unreal.log_warning(f"[LW] No {name} model in {SOURCE_DIR}, skipping")
        return

    folder = f"{root}/{name.capitalize()}"
    final_path = f"{root}/{mesh_name}" if mesh_name else None

    # Start clean so stale meshes, materials and textures from an earlier import don't linger.
    for path in filter(None, (folder, final_path)):
        if assets.does_directory_exist(path):
            assets.delete_directory(path)
        elif assets.does_asset_exist(path):
            assets.delete_asset(path)

    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = folder
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    meshes = [
        path for path in assets.list_assets(folder, recursive=True)
        if isinstance(assets.load_asset(path), unreal.StaticMesh)
    ]
    if not meshes:
        unreal.log_error(f"[LW] Importing {source} produced no static mesh (export it unrigged, as a single mesh)")
        return
    if len(meshes) > 1:
        unreal.log_warning(f"[LW] {source} produced {len(meshes)} meshes; using the largest. Export a single merged mesh to avoid this.")
        meshes.sort(key=lambda path: assets.load_asset(path).get_bounds().sphere_radius, reverse=True)

    # The arena is near black, so each slot moves to M_ModelGlow (create_content.py), which glows with the model's
    # own base colour; the game sets the strength.
    glow_parent = assets.load_asset(GLOW_MATERIAL)
    mesh = assets.load_asset(meshes[0])
    if glow_parent:
        for slot_index, slot in enumerate(mesh.static_materials):
            source_material = slot.material_interface
            if not isinstance(source_material, unreal.MaterialInstance):
                continue
            glow = asset_tools.create_asset(f"MI_{name.capitalize()}_{slot_index}", folder,
                                            unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
            glow.set_editor_property("parent", glow_parent)
            for source_name, target_name in (("BaseColorTexture", "BaseColor"), ("NormalTexture", "Normal")):
                texture = materials.get_material_instance_texture_parameter_value(source_material, source_name)
                if texture:
                    materials.set_material_instance_texture_parameter_value(glow, target_name, texture)
            materials.update_material_instance(glow)
            mesh.set_material(slot_index, glow)
        unreal.log(f"[LW] {name} uses {GLOW_MATERIAL}")
    else:
        unreal.log_warning(f"[LW] {GLOW_MATERIAL} is missing (run create_content.py); {name} keeps its glTF material")

    # The mesh moves up beside the folder under the fixed name the game looks for; its material stays in the folder.
    source_mesh = meshes[0].split(".")[0]
    if final_path:
        if not assets.rename_asset(source_mesh, final_path):
            unreal.log_error(f"[LW] Couldn't move {source_mesh} to {final_path}")
            return
        assets.save_asset(final_path, only_if_is_dirty=False)

    # Keep the glow material and its textures, beside each other; the glTF material and everything else it brought go.
    keep = [path.split(".")[0] for path in assets.list_assets(folder, recursive=False)]
    for path in list(keep):
        glow = assets.load_asset(path)
        if not isinstance(glow, unreal.MaterialInstanceConstant):
            continue
        for parameter in ("BaseColor", "Normal"):
            texture = materials.get_material_instance_texture_parameter_value(glow, parameter)
            if texture and texture.get_path_name().startswith(folder):
                target = f"{folder}/T_{name.capitalize()}_{parameter}"
                assets.rename_asset(texture.get_path_name().split(".")[0], target)
                keep.append(target)
    for path in [path.split(".")[0] for path in assets.list_assets(folder, recursive=True)]:
        if path not in keep:
            assets.delete_asset(path)
    assets.save_directory(root, only_if_is_dirty=False, recursive=True)
    unreal.log(f"[LW] Imported {source} as {final_path or folder}")


wanted = [name for name in os.environ.get("LW_MODELS", "").split(",") if name]
for model_name, (content_root, asset_name) in MODELS.items():
    if not wanted or model_name in wanted:
        import_model(model_name, content_root, asset_name)

for content_root, asset_name in MODELS.values():
    asset_path = f"{content_root}/{asset_name}"
    if asset_name and assets.does_asset_exist(asset_path):
        bounds = assets.load_asset(asset_path).get_bounds()
        unreal.log(f"[LW] {asset_path} bounds origin {bounds.origin} extent {bounds.box_extent}")
