"""
Creates the few editor assets the C++ code expects. Safe to run again: existing assets are left alone.

  /Game/Materials/M_Neon   unlit, emissive = Color * Intensity, and * custom data 0 on instances that have it (LWNeon::ApplyGlow)
  /Game/Materials/M_Art    unlit and see-through: the Art texture * Color * Intensity (LWNeon::ApplyArt)
  /Game/Materials/M_PathRock  lit rock, textured by where it is in the world (LWNeon::ApplyRock; needs Tools/import_ui.py first)
  /Game/Materials/M_ModelGlow  lit, glowing with its BaseColor texture * Glow (imported enemy models)
  /Game/Maps/L_Arena       empty level; ArenaGameMode spawns the arena into it
  /Game/Maps/L_Path        empty level; PathGameMode spawns The Path into it
  /Game/Maps/L_Shield      empty level; ShieldGameMode spawns The Shield into it

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

    # Instances can each be dimmed (the devils' trails).
    dim = mel.create_material_expression(material, unreal.MaterialExpressionPerInstanceCustomData, -300, 300)
    dim.set_editor_property("const_default_value", 1.0)
    dimmed = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -100, 150)
    mel.connect_material_expressions(multiply, "", dimmed, "A")
    mel.connect_material_expressions(dim, "", dimmed, "B")
    mel.connect_material_property(dimmed, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

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


def add(material, kind, x, y, **properties):
    node = mel.create_material_expression(material, kind, x, y)
    for name, value in properties.items():
        node.set_editor_property(name, value)
    return node


def join(material, kind, a, b, x, y):
    """A node of two inputs fed by two others; each is a node, or (node, output)."""
    node = mel.create_material_expression(material, kind, x, y)
    for source, pin in ((a, "A"), (b, "B")):
        source, output = source if isinstance(source, tuple) else (source, "")
        mel.connect_material_expressions(source, output, node, pin)
    return node


def create_art_material():
    path = "/Game/Materials/M_Art"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("M_Art already exists")
        return

    material = asset_tools.create_asset("M_Art", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("two_sided", True)

    multiply = unreal.MaterialExpressionMultiply
    art = add(material, unreal.MaterialExpressionTextureSampleParameter2D, -900, 0, parameter_name="Art")
    color = add(material, unreal.MaterialExpressionVectorParameter, -900, 300, parameter_name="Color", default_value=unreal.LinearColor(1, 1, 1, 1))
    intensity = add(material, unreal.MaterialExpressionScalarParameter, -900, 500, parameter_name="Intensity", default_value=1.0)
    opacity = add(material, unreal.MaterialExpressionScalarParameter, -900, 600, parameter_name="Opacity", default_value=1.0)
    tinted = join(material, multiply, (art, "RGB"), color, -600, 100)
    mel.connect_material_property(join(material, multiply, tinted, intensity, -350, 150), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(join(material, multiply, (art, "A"), opacity, -350, 400), "", unreal.MaterialProperty.MP_OPACITY)

    mel.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log("Created M_Art")


def create_rock_material():
    path = "/Game/Materials/M_PathRock"
    rock = unreal.load_asset("/Game/UI/T_PathRock")
    if unreal.EditorAssetLibrary.does_asset_exist(path) or not rock:
        unreal.log("M_PathRock already exists" if rock else "No T_PathRock yet (run import_ui.py first)")
        return

    material = asset_tools.create_asset("M_PathRock", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    multiply, plus = unreal.MaterialExpressionMultiply, unreal.MaterialExpressionAdd

    # Across is Y (plus X, so the ends of blocks get their own rock), up is Z; on top it runs back along X instead.
    place = add(material, unreal.MaterialExpressionWorldPosition, -2000, 0)
    x = add(material, unreal.MaterialExpressionComponentMask, -1800, -100, r=True, g=False, b=False, a=False)
    y = add(material, unreal.MaterialExpressionComponentMask, -1800, 0, r=False, g=True, b=False, a=False)
    z = add(material, unreal.MaterialExpressionComponentMask, -1800, 100, r=False, g=False, b=True, a=False)
    for mask in (x, y, z):
        mel.connect_material_expressions(place, "", mask, "")
    scale = add(material, unreal.MaterialExpressionConstant, -1800, 250, r=-1.0 / 620)
    across = join(material, plus, x, y, -1600, 0)
    front = join(material, multiply, join(material, unreal.MaterialExpressionAppendVector, across, z, -1400, 0), scale, -1200, 0)
    top = join(material, multiply, join(material, unreal.MaterialExpressionAppendVector, y, x, -1400, 200), scale, -1200, 200)

    def sample(uv, sample_y):
        node = add(material, unreal.MaterialExpressionTextureSample, -1000, sample_y, texture=rock)
        mel.connect_material_expressions(uv, "", node, "UVs")
        return node

    normal = add(material, unreal.MaterialExpressionVertexNormalWS, -1000, 500)
    facing_up = add(material, unreal.MaterialExpressionComponentMask, -850, 500, r=False, g=False, b=True, a=False)
    mel.connect_material_expressions(normal, "", facing_up, "")
    up = add(material, unreal.MaterialExpressionSaturate, -700, 500)
    mel.connect_material_expressions(facing_up, "", up, "")
    blend = add(material, unreal.MaterialExpressionLinearInterpolate, -700, 100)
    mel.connect_material_expressions(sample(front, 0), "RGB", blend, "A")
    mel.connect_material_expressions(sample(top, 250), "RGB", blend, "B")
    mel.connect_material_expressions(up, "", blend, "Alpha")

    color = add(material, unreal.MaterialExpressionVectorParameter, -700, 300, parameter_name="Color", default_value=unreal.LinearColor(1, 1, 1, 1))
    tinted = join(material, multiply, blend, color, -500, 150)
    strength = add(material, unreal.MaterialExpressionScalarParameter, -500, 300, parameter_name="Strength", default_value=6.0)
    seams = add(material, unreal.MaterialExpressionScalarParameter, -500, 400, parameter_name="SeamGlow", default_value=0.35)
    mel.connect_material_property(join(material, multiply, tinted, strength, -250, 100), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(join(material, multiply, tinted, seams, -250, 300), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    rough = add(material, unreal.MaterialExpressionConstant, -250, 450, r=0.75)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log("Created M_PathRock")


def create_map(name):
    path = f"/Game/Maps/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log(f"{name} already exists")
        return

    asset_tools.create_asset(name, "/Game/Maps", unreal.World, unreal.WorldFactory())
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log(f"Created {name}")


create_neon_material()
create_model_glow_material()
create_art_material()
create_rock_material()
create_map("L_Arena")
create_map("L_Path")
create_map("L_Shield")
