"""
Imports the rigged heroes (SourceArt/Hero/Rigging/SK_*.fbx, from Tools/rig_hero.py) as skeletal meshes, using the
materials Tools/import_models.py makes, so ALightWalkerPawn can pose them in code.

  /Game/Hero/SK_Lightwalker         (+ SK_Lightwalker_Skeleton)
  /Game/Hero/SK_LightwalkerFemale   (+ SK_LightwalkerFemale_Skeleton)

Run headless with the editor closed:
  UnrealEditor-Cmd.exe "<path>/LightWalker.uproject" -run=pythonscript -script="<path>/Tools/import_hero_rig.py"
"""

import os

import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
RIG_DIR = os.path.join(PROJECT_DIR, "SourceArt", "Hero", "Rigging")
assets = unreal.EditorAssetLibrary

for name in ("Lightwalker", "LightwalkerFemale"):
    source = os.path.join(RIG_DIR, f"SK_{name}.fbx")
    if not os.path.isfile(source):
        unreal.log_warning(f"[LW] Missing {source}")
        continue

    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("create_physics_asset", False)
    skeletal = options.get_editor_property("skeletal_mesh_import_data")
    skeletal.set_editor_property("import_morph_targets", False)
    skeletal.set_editor_property("convert_scene", True)
    skeletal.set_editor_property("use_t0_as_ref_pose", False)

    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = "/Game/Hero"
    task.destination_name = f"SK_{name}"
    task.automated = True
    task.replace_existing = True
    task.save = True
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    mesh = assets.load_asset(f"/Game/Hero/SK_{name}")
    look = {"Lightwalker": "Lightwalker", "LightwalkerFemale": "Lightwalker_female"}[name]
    material = assets.load_asset(f"/Game/Hero/{look}/MI_{look}_0")
    if not isinstance(mesh, unreal.SkeletalMesh):
        unreal.log_error(f"[LW] SK_{name} didn't import as a skeletal mesh")
        continue
    if material:
        # The glow material was only ever used on static meshes; without this flag a skinned mesh renders grey.
        parent = material
        while isinstance(parent, unreal.MaterialInstance):
            parent = parent.get_editor_property("parent")
        if isinstance(parent, unreal.Material) and not parent.get_editor_property("used_with_skeletal_mesh"):
            parent.set_editor_property("used_with_skeletal_mesh", True)
            unreal.MaterialEditingLibrary.recompile_material(parent)
            assets.save_asset(parent.get_path_name().split(".")[0], only_if_is_dirty=False)
        # Editing the returned slots changes copies, so build new ones.
        slots = []
        for old in mesh.get_editor_property("materials"):
            slot = unreal.SkeletalMaterial()
            slot.set_editor_property("material_interface", material)
            slot.set_editor_property("material_slot_name", old.get_editor_property("material_slot_name"))
            slots.append(slot)
        mesh.set_editor_property("materials", slots)
    assets.save_asset(f"/Game/Hero/SK_{name}", only_if_is_dirty=False)

    mesh = assets.load_asset(f"/Game/Hero/SK_{name}")
    used = [slot.get_editor_property("material_interface") for slot in mesh.get_editor_property("materials")]
    unreal.log(f"[LW] SK_{name}: {mesh.get_bounds().box_extent.z * 2:.1f} units tall, materials {[m.get_name() if m else None for m in used]}")
