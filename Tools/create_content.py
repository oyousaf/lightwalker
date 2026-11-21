"""
Creates the few editor assets the C++ code expects. Safe to run again: existing assets are left alone.

  /Game/Materials/M_Neon   unlit, emissive = Color * Intensity (used by LWNeon::ApplyGlow)
  /Game/Materials/M_ModelGlow  lit, glowing with its BaseColor texture * Glow (imported enemy models)
  /Game/Maps/L_Arena       empty level; ArenaGameMode spawns the arena into it

Run headless with the editor closed:
  UnrealEditor-Cmd.exe "<path>/LightWalker.uproject" -run=pythonscript -script="<path>/Tools/create_content.py"
"""

import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def ensure_usage(path, flag):
    """Materials only render on mesh types they're marked for (else the default material in packaged builds):
    used_with_instanced_static_meshes for particles, used_with_nanite for the imported models."""
    material = unreal.EditorAssetLibrary.load_asset(path)
    if material and not material.get_editor_property(flag):
        material.set_editor_property(flag, True)
        mel.recompile_material(material)
        unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
        unreal.log(f"{path}: enabled {flag}")


def allow_instancing(path):
    ensure_usage(path, "used_with_instanced_static_meshes")


def create_neon_material():
    path = "/Game/Materials/M_Neon"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("M_Neon already exists")
        allow_instancing(path)
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

    material.set_editor_property("used_with_instanced_static_meshes", True)
    mel.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log("Created M_Neon")


def create_model_glow_material():
    path = "/Game/Materials/M_ModelGlow"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("M_ModelGlow already exists")
        ensure_usage(path, "used_with_nanite")
        return

    material = asset_tools.create_asset("M_ModelGlow", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("two_sided", True)
    material.set_editor_property("used_with_nanite", True)

    base = mel.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0)
    base.set_editor_property("parameter_name", "BaseColor")
    mel.connect_material_property(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    normal = mel.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -800, 300)
    normal.set_editor_property("parameter_name", "Normal")
    normal.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    normal.set_editor_property("texture", unreal.load_asset("/Engine/EngineMaterials/DefaultNormal"))
    mel.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)

    glow = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 200)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 1.5)

    multiply = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -300, 100)
    mel.connect_material_expressions(base, "RGB", multiply, "A")
    mel.connect_material_expressions(glow, "", multiply, "B")
    mel.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    roughness = mel.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 400)
    roughness.set_editor_property("r", 0.55)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log("Created M_ModelGlow")


def create_arena_map():
    path = "/Game/Maps/L_Arena"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("L_Arena already exists")
        return

    asset_tools.create_asset("L_Arena", "/Game/Maps", unreal.World, unreal.WorldFactory())
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log("Created L_Arena")


create_neon_material()
create_model_glow_material()
create_arena_map()
