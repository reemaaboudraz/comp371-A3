/**
 * Reema Aboudraz - 40253549
 * Wissem Oumsalem - 40291712  
 * Assignment 3, COMP 371 
 * Summer 2026
 * Professor Nagi Basha
 */


import bpy
import math
import os
from mathutils import Vector

# ============================================================
# Blender chair generator
# ------------------------------------------------------------
# Run this file inside Blender's Scripting workspace.
# It creates a complete chair scene, adds materials/lights,
# saves a .blend file, and exports an OBJ for the OpenGL part.
# ============================================================

# Output folder. Change this if you want the files elsewhere.
OUTPUT_DIR = os.path.join(os.path.expanduser("~"), "COMP371_Assignment3_Output")
os.makedirs(OUTPUT_DIR, exist_ok=True)

BLEND_PATH = os.path.join(OUTPUT_DIR, "assignment3_chair.blend")
OBJ_PATH = os.path.join(OUTPUT_DIR, "chair.obj")


def clear_scene():
    """Delete all objects from the current Blender scene."""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)


def make_material(name, base_color, roughness=0.45):
    """Create a simple Principled BSDF material."""
    material = bpy.data.materials.new(name=name)
    material.use_nodes = True
    bsdf = material.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*base_color, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    return material


def add_beveled_cube(name, location, scale, material, bevel=0.06):
    """
    Add a cube, scale it to the requested dimensions, and bevel
    the edges slightly so the chair looks less blocky/CG-like.
    """
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.active_object
    obj.name = name

    # Blender's default cube has side length 2, so half-dimensions
    # are used here to obtain the requested final dimensions.
    obj.scale = (scale[0] / 2.0, scale[1] / 2.0, scale[2] / 2.0)

    # Apply scale so bevel width behaves consistently.
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    # Add a bevel modifier to soften the chair edges.
    bevel_modifier = obj.modifiers.new(name="Bevel", type='BEVEL')
    bevel_modifier.width = bevel
    bevel_modifier.segments = 3
    bevel_modifier.limit_method = 'ANGLE'

    # Apply the bevel so it is included in the exported OBJ.
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel_modifier.name)

    # Add the material to this object.
    obj.data.materials.append(material)
    return obj


def add_leg(name, x, y, z, height, material, tilt_x=0.0, tilt_y=0.0):
    """Create one slightly tapered-looking/angled chair leg."""
    leg = add_beveled_cube(
        name=name,
        location=(x, y, z),
        scale=(0.18, 0.18, height),
        material=material,
        bevel=0.035,
    )
    leg.rotation_euler = (
        math.radians(tilt_x),
        math.radians(tilt_y),
        0.0,
    )
    return leg


def build_chair():
    """Build the complete chair from multiple mesh components."""
    wood = make_material("Warm_Pine", (0.58, 0.32, 0.14), roughness=0.5)
    dark_wood = make_material("Dark_Accent", (0.28, 0.12, 0.05), roughness=0.55)

    # -----------------------------
    # Seat
    # -----------------------------
    add_beveled_cube(
        name="Seat",
        location=(0.0, 0.0, 1.75),
        scale=(2.25, 2.05, 0.22),
        material=wood,
        bevel=0.09,
    )

    # Small lower frame pieces under the seat improve realism.
    add_beveled_cube(
        name="Front_Apron",
        location=(0.0, -0.92, 1.52),
        scale=(1.95, 0.16, 0.34),
        material=dark_wood,
        bevel=0.04,
    )
    add_beveled_cube(
        name="Back_Apron",
        location=(0.0, 0.92, 1.52),
        scale=(1.95, 0.16, 0.34),
        material=dark_wood,
        bevel=0.04,
    )
    add_beveled_cube(
        name="Left_Apron",
        location=(-1.02, 0.0, 1.52),
        scale=(0.16, 1.70, 0.34),
        material=dark_wood,
        bevel=0.04,
    )
    add_beveled_cube(
        name="Right_Apron",
        location=(1.02, 0.0, 1.52),
        scale=(0.16, 1.70, 0.34),
        material=dark_wood,
        bevel=0.04,
    )

    # -----------------------------
    # Four legs
    # -----------------------------
    leg_height = 1.65
    leg_z = 0.78
    add_leg("Front_Left_Leg", -0.92, -0.78, leg_z, leg_height, wood, tilt_x=-2.0, tilt_y=2.0)
    add_leg("Front_Right_Leg", 0.92, -0.78, leg_z, leg_height, wood, tilt_x=-2.0, tilt_y=-2.0)
    add_leg("Back_Left_Leg", -0.92, 0.78, leg_z, leg_height, wood, tilt_x=2.0, tilt_y=2.0)
    add_leg("Back_Right_Leg", 0.92, 0.78, leg_z, leg_height, wood, tilt_x=2.0, tilt_y=-2.0)

    # -----------------------------
    # Backrest posts
    # -----------------------------
    add_beveled_cube(
        name="Back_Left_Post",
        location=(-0.94, 0.84, 3.05),
        scale=(0.18, 0.20, 2.70),
        material=wood,
        bevel=0.04,
    )
    add_beveled_cube(
        name="Back_Right_Post",
        location=(0.94, 0.84, 3.05),
        scale=(0.18, 0.20, 2.70),
        material=wood,
        bevel=0.04,
    )

    # -----------------------------
    # Backrest horizontal slats
    # -----------------------------
    slat_z_positions = [2.55, 3.00, 3.45, 3.90]
    for index, z_value in enumerate(slat_z_positions, start=1):
        slat = add_beveled_cube(
            name=f"Back_Slat_{index}",
            location=(0.0, 0.82, z_value),
            scale=(1.78, 0.14, 0.26),
            material=wood,
            bevel=0.055,
        )
        # A small backward tilt gives the backrest a more natural chair profile.
        slat.rotation_euler.x = math.radians(-3.0)

    # Top rail is slightly thicker than the slats.
    add_beveled_cube(
        name="Back_Top_Rail",
        location=(0.0, 0.81, 4.32),
        scale=(2.05, 0.20, 0.34),
        material=wood,
        bevel=0.08,
    )

    # -----------------------------
    # Side support stretchers
    # -----------------------------
    add_beveled_cube(
        name="Left_Stretcher",
        location=(-0.90, 0.0, 0.72),
        scale=(0.12, 1.50, 0.16),
        material=dark_wood,
        bevel=0.03,
    )
    add_beveled_cube(
        name="Right_Stretcher",
        location=(0.90, 0.0, 0.72),
        scale=(0.12, 1.50, 0.16),
        material=dark_wood,
        bevel=0.03,
    )


def setup_floor_and_lighting():
    """Add a floor, camera, and a simple three-light setup."""
    floor_material = make_material("Floor_Material", (0.16, 0.16, 0.16), roughness=0.7)

    bpy.ops.mesh.primitive_plane_add(size=14.0, location=(0.0, 0.0, -0.08))
    floor = bpy.context.active_object
    floor.name = "Floor"
    floor.data.materials.append(floor_material)

    # Key light.
    bpy.ops.object.light_add(type='AREA', location=(4.5, -4.5, 7.0))
    key = bpy.context.active_object
    key.name = "Key_Light"
    key.data.energy = 900
    key.data.shape = 'DISK'
    key.data.size = 4.0

    # Fill light.
    bpy.ops.object.light_add(type='AREA', location=(-4.0, -1.0, 4.5))
    fill = bpy.context.active_object
    fill.name = "Fill_Light"
    fill.data.energy = 500
    fill.data.size = 3.0

    # Rim/back light.
    bpy.ops.object.light_add(type='AREA', location=(0.0, 5.0, 6.0))
    rim = bpy.context.active_object
    rim.name = "Rim_Light"
    rim.data.energy = 650
    rim.data.size = 3.0

    # Camera.
    bpy.ops.object.camera_add(location=(6.2, -7.0, 5.4))
    camera = bpy.context.active_object
    camera.name = "Camera"
    bpy.context.scene.camera = camera

    # Point the camera toward the middle of the chair.
    target = Vector((0.0, 0.0, 2.0))
    direction = target - camera.location
    camera.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()

    # Render settings for clean screenshots.
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.render.resolution_x = 900
    scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100


def export_chair_only_to_obj():
    """Export every chair mesh except the floor as one OBJ file."""
    bpy.ops.object.select_all(action='DESELECT')

    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH' and obj.name != "Floor":
            obj.select_set(True)

    # Blender 4.x OBJ exporter.
    try:
        bpy.ops.wm.obj_export(
            filepath=OBJ_PATH,
            export_selected_objects=True,
            export_materials=True,
        )
    except Exception:
        # Fallback for Blender versions that still use the older exporter.
        bpy.ops.export_scene.obj(
            filepath=OBJ_PATH,
            use_selection=True,
            use_materials=True,
        )


# Build the final scene from scratch.
clear_scene()
build_chair()
setup_floor_and_lighting()

# Save the Blender project before exporting.
bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)

# Export the chair mesh for the OpenGL program.
export_chair_only_to_obj()

print("COMP 371 Assignment 3 chair generated successfully.")
print(f"Blend file: {BLEND_PATH}")
print(f"OBJ file:   {OBJ_PATH}")
