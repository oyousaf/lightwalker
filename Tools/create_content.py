"""
Creates the few editor assets the C++ code expects. Safe to run again: existing assets are left alone.

  /Game/Materials/M_Neon   unlit, emissive = Color * Intensity (used by LWNeon::ApplyGlow)
  /Game/Maps/L_Arena       empty level; ArenaGameMode spawns the arena into it

Run headless with the editor closed:
  UnrealEditor-Cmd.exe "<path>/LightWalker.uproject" -run=pythonscript -script="<path>/Tools/create_content.py"
"""

import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def create_neon_material():
    path = "/Game/Materials/M_Neon"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("M_Neon already exists")
        return

    material = asset_tools.create_asset("M_Neon", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    color = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.1, 0.85, 1.0, 1.0))

    intensity = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -600, 250)
    intensity.set_editor_property("parameter_name", "Intensity")
    intensity.set_editor_property("default_value", 5.0)

    multiply = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -300, 100)
    mel.connect_material_expressions(color, "", multiply, "A")
    mel.connect_material_expressions(intensity, "", multiply, "B")
    mel.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mel.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log("Created M_Neon")


def create_arena_map():
    path = "/Game/Maps/L_Arena"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("L_Arena already exists")
        return

    asset_tools.create_asset("L_Arena", "/Game/Maps", unreal.World, unreal.WorldFactory())
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log("Created L_Arena")


create_neon_material()
create_arena_map()
