"""
Rigs the hero models in Blender: fits a humanoid skeleton (Unreal mannequin bone names) to each robed figure, skins
the mesh to it, and exports a skeletal-mesh FBX for Tools/import_hero_rig.py. The game animates the bones in code
(ALightWalkerPawn::AnimateRig), so no animation clips are needed.

Skinning rules, since the meshes are single robed figures with the staff fused to the right hand:
  - The staff and lantern follow the right hand rigidly.
  - The robe below the belt blends from the pelvis at the waist to both thighs at the hem, by side, so it flares and
    swings with the stride rather than splitting between the legs.
  - Everything else is weighted by distance to the nearest bones.

  "C:/Program Files/Blender Foundation/Blender 5.3/blender.exe" -b --factory-startup -P Tools/rig_hero.py

Reads the source models (SourceArt/Models/lightwalker.glb, lightwalker_female.glb, which keep their texture UVs) and
writes SourceArt/Hero/Rigging/SK_<Hero>.fbx.
Add "-- --preview <folder>" to also render weight and test-pose previews there.
"""

import math
import os
import sys

import bpy
from mathutils import Matrix, Vector

SOURCE_ART = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SourceArt"))
RIG_DIR = os.path.join(SOURCE_ART, "Hero", "Rigging")

# Joint positions in the model's space: 1 unit tall, feet at z -0.5, facing -Y, the character's left at +X.
# Measured from orthographic renders of each model.
COMMON = {
    "root": ((0.01, 0.06, -0.5), (0.01, 0.06, -0.4), None),
    "pelvis": ((0.01, 0.06, 0.0), (0.01, 0.06, 0.10), "root"),
    "spine_01": ((0.01, 0.06, 0.10), (0.01, 0.065, 0.19), "pelvis"),
    "spine_02": ((0.01, 0.065, 0.19), (0.01, 0.07, 0.29), "spine_01"),
    "neck_01": ((0.01, 0.07, 0.29), (0.01, 0.06, 0.35), "spine_02"),
    "head": ((0.01, 0.06, 0.35), (0.01, 0.05, 0.50), "neck_01"),
    "clavicle_l": ((0.03, 0.07, 0.28), (0.12, 0.08, 0.28), "spine_02"),
    "upperarm_l": ((0.12, 0.08, 0.28), (0.165, 0.10, 0.105), "clavicle_l"),
    "lowerarm_l": ((0.165, 0.10, 0.105), (0.19, 0.07, -0.03), "upperarm_l"),
    "hand_l": ((0.19, 0.07, -0.03), (0.19, 0.06, -0.085), "lowerarm_l"),
    "clavicle_r": ((-0.01, 0.07, 0.28), (-0.105, 0.08, 0.28), "spine_02"),
    "thigh_l": ((0.07, 0.06, 0.0), (0.085, 0.05, -0.23), "pelvis"),
    "calf_l": ((0.085, 0.05, -0.23), (0.10, 0.07, -0.45), "thigh_l"),
    "foot_l": ((0.10, 0.07, -0.45), (0.105, -0.02, -0.49), "calf_l"),
    "thigh_r": ((-0.05, 0.06, 0.0), (-0.055, 0.05, -0.23), "pelvis"),
    "calf_r": ((-0.055, 0.05, -0.23), (-0.06, 0.07, -0.45), "thigh_r"),
    "foot_r": ((-0.06, 0.07, -0.45), (-0.065, -0.02, -0.49), "calf_r"),
}
HEROES = {
    "Lightwalker": {
        "source": "lightwalker.glb",
        "grip": (-0.17, -0.10, 0.11),
        "elbow_r": (-0.15, 0.07, 0.14),
        "staff": ((-0.17, -0.01, -0.49), (-0.18, -0.14, 0.40)),
        "hem_z": -0.42,
    },
    "LightwalkerFemale": {
        "source": "lightwalker_female.glb",
        "grip": (-0.18, -0.12, 0.17),
        "elbow_r": (-0.15, 0.06, 0.17),
        "staff": ((-0.17, 0.0, -0.49), (-0.18, -0.14, 0.42)),
        "hem_z": -0.40,
    },
}
BODY_CENTRE_X = 0.01


def bones_for(hero):
    bones = dict(COMMON)
    grip = Vector(hero["grip"])
    elbow = Vector(hero["elbow_r"])
    wrist = elbow.lerp(grip, 0.8)
    bones["upperarm_r"] = ((-0.105, 0.08, 0.28), tuple(elbow), "clavicle_r")
    bones["lowerarm_r"] = (tuple(elbow), tuple(wrist), "upperarm_r")
    bones["hand_r"] = (tuple(wrist), tuple(grip + (grip - wrist).normalized() * 0.04), "lowerarm_r")
    return bones


def segment_distance(p, a, b):
    ab = b - a
    t = max(0.0, min(1.0, (p - a).dot(ab) / max(ab.length_squared, 1e-12)))
    return (a + ab * t - p).length


def smoothstep(e0, e1, x):
    t = max(0.0, min(1.0, (x - e0) / (e1 - e0)))
    return t * t * (3 - 2 * t)


def weights_for(p, bones, hero):
    staff_a, staff_b = (Vector(v) for v in hero["staff"])
    grip = Vector(hero["grip"])
    # The staff (and the lantern on top, which is wider) follows the right hand rigidly.
    staff_radius = 0.065 if p.z > 0.25 else 0.045
    if segment_distance(p, staff_a, staff_b + Vector((0, 0, 0.1))) < staff_radius and (p.x < -0.145 or p.z > 0.25):
        return {"hand_r": 1.0}
    if (p - grip).length < 0.05:
        return {"hand_r": 1.0}

    # Distance weights to the nearest bones.
    distances = sorted((segment_distance(p, Vector(h), Vector(t)), name) for name, (h, t, _) in bones.items() if name != "root")
    nearest = distances[:3]
    raw = {name: 1.0 / max(d, 0.004) ** 4 for d, name in nearest}
    total = sum(raw.values())
    weights = {name: w / total for name, w in raw.items()}
    nearest_name = nearest[0][1]

    arm = nearest_name.startswith(("upperarm", "lowerarm", "hand"))
    if p.z < 0.06 and not arm:
        if p.z > hero["hem_z"] - 0.01:
            # The robe: pelvis at the belt, both thighs by side towards the hem.
            t = smoothstep(0.06, hero["hem_z"] + 0.12, p.z)
            left = smoothstep(-0.07, 0.09, p.x - BODY_CENTRE_X)
            weights = {"pelvis": 1 - 0.65 * t, "thigh_l": 0.65 * t * left, "thigh_r": 0.65 * t * (1 - left)}
        else:
            # Legs and shoes below the hem.
            side = "l" if p.x > BODY_CENTRE_X + 0.015 else "r"
            foot = smoothstep(-0.43, -0.46, p.z)
            weights = {f"calf_{side}": 1 - foot, f"foot_{side}": foot}
    return {k: v for k, v in weights.items() if v > 1e-4}


def rig(name, preview_dir=None):
    hero = HEROES[name]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=os.path.join(SOURCE_ART, "Models", hero["source"]))
    mesh = next(o for o in bpy.context.scene.objects if o.type == "MESH")
    # Bake the import transform into the vertices, so the joint table's coordinates match.
    mesh.data.transform(mesh.matrix_world)
    mesh.matrix_world = Matrix.Identity(4)
    for other in [o for o in bpy.context.scene.objects if o is not mesh]:
        bpy.data.objects.remove(other)

    bones = bones_for(hero)
    armature_data = bpy.data.armatures.new(f"SK_{name}")
    armature = bpy.data.objects.new(f"SK_{name}", armature_data)
    bpy.context.scene.collection.objects.link(armature)
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode="EDIT")
    for bone_name, (head, tail, parent) in bones.items():
        bone = armature_data.edit_bones.new(bone_name)
        bone.head, bone.tail = Vector(head), Vector(tail)
    for bone_name, (_, _, parent) in bones.items():
        if parent:
            armature_data.edit_bones[bone_name].parent = armature_data.edit_bones[parent]
    bpy.ops.object.mode_set(mode="OBJECT")

    groups = {bone_name: mesh.vertex_groups.new(name=bone_name) for bone_name in bones}
    for vertex in mesh.data.vertices:
        for bone_name, weight in weights_for(vertex.co, bones, hero).items():
            groups[bone_name].add([vertex.index], weight, "REPLACE")
    mesh.parent = armature
    modifier = mesh.modifiers.new("Armature", "ARMATURE")
    modifier.object = armature

    os.makedirs(RIG_DIR, exist_ok=True)
    out = os.path.join(RIG_DIR, f"SK_{name}.fbx")
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.fbx(filepath=out, use_selection=True, object_types={"ARMATURE", "MESH"}, add_leaf_bones=False,
                             bake_anim=False, mesh_smooth_type="FACE", armature_nodetype="NULL")
    print(f"[LW] Wrote {out} ({len(bones)} bones, {len(mesh.data.vertices)} vertices)")

    if preview_dir:
        render_previews(name, mesh, armature, bones, preview_dir)


def render_previews(name, mesh, armature, bones, folder):
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.render.resolution_x = scene.render.resolution_y = 600
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "VERTEX"
    palette = [(1, 0.2, 0.2), (0.2, 1, 0.2), (0.2, 0.4, 1), (1, 1, 0.2), (1, 0.2, 1), (0.2, 1, 1), (1, 0.6, 0.2), (0.6, 0.2, 1),
               (0.6, 1, 0.4), (1, 0.5, 0.7), (0.5, 0.5, 0.5), (0.9, 0.9, 0.9)]
    colours = mesh.data.color_attributes.new("weights", "FLOAT_COLOR", "POINT")
    index_of = {group.index: i for i, group in enumerate(mesh.vertex_groups)}
    for vertex in mesh.data.vertices:
        best = max(vertex.groups, key=lambda g: g.weight, default=None)
        colour = palette[index_of[best.group] % len(palette)] if best else (0, 0, 0)
        colours.data[vertex.index].color = (*colour, 1)
    camera_data = bpy.data.cameras.new("cam")
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = 1.2
    camera = bpy.data.objects.new("cam", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera

    def shoot(tag):
        for view, direction in (("back", Vector((0, -1, 0))), ("side", Vector((-1, 0, 0))), ("front", Vector((0, 1, 0)))):
            camera.location = -direction * 3
            camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
            scene.render.filepath = os.path.join(folder, f"{name}_{tag}_{view}.png")
            bpy.ops.render.render(write_still=True)

    shoot("rest")
    pose = armature.pose.bones
    for bone in pose:
        bone.rotation_mode = "XYZ"
    # A mid-stride test pose: left leg forward, right leg back, arms counter-swinging, a forward lean.
    pose["thigh_l"].rotation_euler = (math.radians(-35), 0, 0)
    pose["calf_l"].rotation_euler = (math.radians(20), 0, 0)
    pose["thigh_r"].rotation_euler = (math.radians(30), 0, 0)
    pose["calf_r"].rotation_euler = (math.radians(55), 0, 0)
    pose["upperarm_l"].rotation_euler = (math.radians(30), 0, 0)
    pose["spine_01"].rotation_euler = (math.radians(-8), 0, 0)
    shoot("stride")


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    preview = args[args.index("--preview") + 1] if "--preview" in args else None
    for name in HEROES:
        rig(name, preview)


main()
