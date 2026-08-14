# =============================================================================
# COMP 371 - Computer Graphics
# Assignment 3 - Summer 2026
#
# Team members:
#   Reema Aboudraz  - 40253549
#   Wissem Oumsalem - 40291712
#
# Purpose:
#   Build a detailed chair in Blender, UV unwrap every chair mesh, create and
#   apply a wood texture map, set up a simple lighting scene, save the .blend
#   file, and export the chair as an OBJ file for the OpenGL portion.
#
# How to run:
#   1. Open Blender.
#   2. Go to the Scripting workspace.
#   3. Open this file in Blender's Text Editor.
#   4. Press Alt+P (or click Run Script).
#
# Output:
#   Blender/assignment3_chair.blend
#   Blender/pine_wood_texture.png
#   OpenGL/chair.obj
# =============================================================================

import bpy
import math
import os
import random
from mathutils import Vector


# -----------------------------------------------------------------------------
# Output paths
# -----------------------------------------------------------------------------
# When this script is opened from disk, Blender defines __file__.  The fallback
# keeps the script usable if it is pasted into Blender's Text Editor manually.
SCRIPT_DIR = r"C:\Users\Reema\comp371-A3\comp371-A3\Blender"

# The repository layout has Blender/ and OpenGL/ as sibling folders.
PROJECT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir))
OPENGL_DIR = os.path.join(PROJECT_DIR, "OpenGL")
os.makedirs(OPENGL_DIR, exist_ok=True)

BLEND_PATH = os.path.join(SCRIPT_DIR, "assignment3_chair.blend")
TEXTURE_PATH = os.path.join(SCRIPT_DIR, "pine_wood_texture.png")
OBJ_PATH = os.path.join(OPENGL_DIR, "chair.obj")

# Keep an explicit list of chair objects.  This lets the OBJ exporter select
# only the chair and prevents the floor, lights, and camera from being exported.
CHAIR_OBJECTS = []


# -----------------------------------------------------------------------------
# General scene utilities
# -----------------------------------------------------------------------------
def clear_scene():
    """Delete all objects and unused scene data from the current Blender file."""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)

    # Remove leftover meshes/materials/images from previous runs when possible.
    # The try/except keeps repeated execution safe even if Blender still has a
    # temporary internal reference to one of these datablocks.
    for datablocks in (bpy.data.meshes, bpy.data.materials, bpy.data.cameras, bpy.data.lights):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def point_object_at(obj, target):
    """Rotate a camera or light so that its local -Z axis points at target."""
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()


# -----------------------------------------------------------------------------
# Texture creation
# -----------------------------------------------------------------------------
def create_pine_texture(width=512, height=512):
    """
    Create a deterministic wood-grain image texture entirely in Blender.

    This avoids an external texture dependency while still satisfying the
    assignment's texture-map requirement.  The generated image is saved as a
    PNG and later connected to the Principled BSDF through UV coordinates.
    """
    image_name = "COMP371_Pine_Wood_Texture"

    # Remove an old generated image if the script is run more than once.
    existing = bpy.data.images.get(image_name)
    if existing is not None:
        bpy.data.images.remove(existing)

    image = bpy.data.images.new(
        name=image_name,
        width=width,
        height=height,
        alpha=False,
        float_buffer=False,
    )

    rng = random.Random(371)  # Fixed seed -> same texture every run.
    pixels = [0.0] * (width * height * 4)

    # A small amount of slowly varying row noise prevents perfectly regular
    # sine bands and makes the generated wood look more natural.
    row_offsets = [rng.uniform(-0.12, 0.12) for _ in range(height)]

    for y in range(height):
        v = y / max(1, height - 1)
        row_offset = row_offsets[y]

        for x in range(width):
            u = x / max(1, width - 1)

            # Long narrow grain follows the U direction.  A second wave bends
            # the grain slightly so it does not look like perfectly straight
            # stripes.  A broad band adds larger natural tonal variation.
            bent_u = u + 0.035 * math.sin(v * math.tau * 3.0) + row_offset * 0.02
            fine_grain = 0.5 + 0.5 * math.sin((bent_u * 32.0 + 1.8 * math.sin(v * 8.0)) * math.tau)
            broad_grain = 0.5 + 0.5 * math.sin((bent_u * 5.5 + v * 0.7) * math.tau)

            # Add two subtle "knot" rings.  These are intentionally gentle so
            # the chair still resembles light stained pine rather than marble.
            knot_1 = math.sin(math.sqrt((u - 0.31) ** 2 + (v - 0.38) ** 2) * 95.0)
            knot_2 = math.sin(math.sqrt((u - 0.73) ** 2 + (v - 0.67) ** 2) * 78.0)
            knot_mix = 0.5 + 0.25 * knot_1 + 0.25 * knot_2

            variation = 0.55 * fine_grain + 0.30 * broad_grain + 0.15 * knot_mix

            # Light-brown / pine palette.  Clamp all values to Blender's 0..1
            # image range before writing them into the image buffer.
            red = min(1.0, max(0.0, 0.62 + 0.24 * variation))
            green = min(1.0, max(0.0, 0.38 + 0.22 * variation))
            blue = min(1.0, max(0.0, 0.17 + 0.13 * variation))

            i = (y * width + x) * 4
            pixels[i + 0] = red
            pixels[i + 1] = green
            pixels[i + 2] = blue
            pixels[i + 3] = 1.0

    # foreach_set is much faster than assigning every pixel one at a time.
    image.pixels.foreach_set(pixels)
    image.update()

    # Save the generated texture so the submission contains a real texture map.
    image.filepath_raw = TEXTURE_PATH
    image.file_format = 'PNG'
    image.save()

    return image


def make_wood_material(image):
    """Create a UV-based pine material using the generated image texture."""
    material = bpy.data.materials.new(name="Light_Stained_Pine")
    material.use_nodes = True

    nodes = material.node_tree.nodes
    links = material.node_tree.links

    # Start from a clean node graph so every connection is explicit.
    nodes.clear()

    output_node = nodes.new(type="ShaderNodeOutputMaterial")
    output_node.location = (620, 0)

    bsdf = nodes.new(type="ShaderNodeBsdfPrincipled")
    bsdf.location = (330, 0)
    bsdf.inputs["Roughness"].default_value = 0.42

    # UV coordinates are required by the assignment and are used directly by
    # the image texture node below.
    tex_coord = nodes.new(type="ShaderNodeTexCoord")
    tex_coord.location = (-620, 0)

    mapping = nodes.new(type="ShaderNodeMapping")
    mapping.location = (-420, 0)
    # Repeat the texture a little so wood grain is visible on smaller pieces.
    mapping.inputs["Scale"].default_value = (1.5, 4.0, 1.0)

    image_node = nodes.new(type="ShaderNodeTexImage")
    image_node.location = (-170, 70)
    image_node.image = image
    image_node.interpolation = 'Linear'
    image_node.extension = 'REPEAT'

    # A subtle bump node uses the same texture as height information.  It adds
    # visual depth to the wood without changing the exported OBJ geometry.
    bump = nodes.new(type="ShaderNodeBump")
    bump.location = (100, -130)
    bump.inputs["Strength"].default_value = 0.16
    bump.inputs["Distance"].default_value = 0.06

    links.new(tex_coord.outputs["UV"], mapping.inputs["Vector"])
    links.new(mapping.outputs["Vector"], image_node.inputs["Vector"])
    links.new(image_node.outputs["Color"], bsdf.inputs["Base Color"])
    links.new(image_node.outputs["Color"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    links.new(bsdf.outputs["BSDF"], output_node.inputs["Surface"])

    return material


def make_floor_material():
    """Create a simple neutral floor material for the lighting scene."""
    material = bpy.data.materials.new(name="Floor_Material")
    material.use_nodes = True
    bsdf = material.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (0.055, 0.065, 0.08, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.72
    return material


# -----------------------------------------------------------------------------
# Mesh creation and UV unwrapping
# -----------------------------------------------------------------------------
def smart_uv_unwrap(obj):
    """Create a UV map for the complete mesh using Blender's Smart UV Project."""
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project()
    bpy.ops.object.mode_set(mode='OBJECT')


def add_beveled_cube(name, location, dimensions, material, bevel=0.04, rotation=(0.0, 0.0, 0.0)):
    """
    Add one chair component as a beveled, UV-unwrapped cube.

    dimensions are full X/Y/Z sizes, not Blender scale factors.
    rotation values are given in degrees for readability.
    """
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.active_object
    obj.name = name

    # Blender's default cube is 2 units wide, so half-dimensions create the
    # requested final size.
    obj.scale = (
        dimensions[0] / 2.0,
        dimensions[1] / 2.0,
        dimensions[2] / 2.0,
    )

    # Apply the scale before beveling and UV unwrapping.  This gives the bevel a
    # consistent physical width and makes the UV result independent of object
    # scale values.
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    bevel_modifier = obj.modifiers.new(name="Soft_Edges", type='BEVEL')
    bevel_modifier.width = bevel
    bevel_modifier.segments = 3
    bevel_modifier.limit_method = 'ANGLE'
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel_modifier.name)

    # UV unwrap AFTER applying the bevel so every final exported face receives
    # texture coordinates.
    smart_uv_unwrap(obj)

    obj.data.materials.append(material)

    # Apply object rotation last.  The OBJ exporter includes object transforms,
    # while the mesh UV coordinates remain valid.
    obj.rotation_euler = tuple(math.radians(value) for value in rotation)

    CHAIR_OBJECTS.append(obj)
    return obj


def add_leg(name, x, y, height, material, tilt_x=0.0, tilt_y=0.0):
    """Create a front leg with a very small outward splay for realism."""
    return add_beveled_cube(
        name=name,
        location=(x, y, height / 2.0),
        dimensions=(0.20, 0.20, height),
        material=material,
        bevel=0.028,
        rotation=(tilt_x, tilt_y, 0.0),
    )


# -----------------------------------------------------------------------------
# Chair modeling
# -----------------------------------------------------------------------------
def build_chair(wood_material):
    """Build a chair closely matching the provided light-brown IKEA reference."""

    # ------------------------------------------------------------------
    # Seat: four closely spaced wooden boards create subtle board seams.
    # ------------------------------------------------------------------
    seat_width = 2.20
    plank_depth = 0.475
    plank_gap = 0.014
    seat_z = 1.82

    for index in range(4):
        y = (index - 1.5) * (plank_depth + plank_gap)
        add_beveled_cube(
            name=f"Seat_Plank_{index + 1}",
            location=(0.0, y, seat_z),
            dimensions=(seat_width, plank_depth, 0.22),
            material=wood_material,
            bevel=0.035,
        )

    # ------------------------------------------------------------------
    # Apron/frame directly underneath the seat.
    # ------------------------------------------------------------------
    add_beveled_cube(
        "Front_Apron",
        (0.0, -0.89, 1.60),
        (1.92, 0.16, 0.32),
        wood_material,
        bevel=0.03,
    )
    add_beveled_cube(
        "Back_Apron",
        (0.0, 0.89, 1.60),
        (1.92, 0.16, 0.32),
        wood_material,
        bevel=0.03,
    )
    add_beveled_cube(
        "Left_Apron",
        (-1.01, 0.0, 1.60),
        (0.16, 1.62, 0.32),
        wood_material,
        bevel=0.03,
    )
    add_beveled_cube(
        "Right_Apron",
        (1.01, 0.0, 1.60),
        (0.16, 1.62, 0.32),
        wood_material,
        bevel=0.03,
    )

    # ------------------------------------------------------------------
    # Front legs end beneath the seat.  A small splay prevents a perfectly
    # mechanical box shape and better matches real chair construction.
    # ------------------------------------------------------------------
    front_leg_height = 1.70
    add_leg("Front_Left_Leg", -0.95, -0.80, front_leg_height, wood_material, tilt_x=-1.5, tilt_y=1.5)
    add_leg("Front_Right_Leg", 0.95, -0.80, front_leg_height, wood_material, tilt_x=-1.5, tilt_y=-1.5)

    # ------------------------------------------------------------------
    # Rear posts are continuous from floor to the top of the backrest, as on
    # the reference chair.  This avoids the visible seam created by separate
    # rear-leg and upper-post objects.
    # ------------------------------------------------------------------
    rear_post_height = 4.38
    add_beveled_cube(
        "Back_Left_Post",
        (-0.96, 0.82, rear_post_height / 2.0),
        (0.21, 0.21, rear_post_height),
        wood_material,
        bevel=0.03,
    )
    add_beveled_cube(
        "Back_Right_Post",
        (0.96, 0.82, rear_post_height / 2.0),
        (0.21, 0.21, rear_post_height),
        wood_material,
        bevel=0.03,
    )

    # ------------------------------------------------------------------
    # Backrest rails and FOUR VERTICAL SLATS.  The original repo used
    # horizontal slats; the assignment reference clearly shows vertical ones.
    # ------------------------------------------------------------------
    add_beveled_cube(
        "Back_Lower_Rail",
        (0.0, 0.82, 2.50),
        (1.76, 0.16, 0.20),
        wood_material,
        bevel=0.035,
    )
    add_beveled_cube(
        "Back_Top_Rail",
        (0.0, 0.82, 4.20),
        (1.88, 0.20, 0.26),
        wood_material,
        bevel=0.045,
    )

    slat_x_positions = (-0.57, -0.19, 0.19, 0.57)
    for index, x in enumerate(slat_x_positions, start=1):
        add_beveled_cube(
            name=f"Back_Vertical_Slat_{index}",
            location=(x, 0.82, 3.35),
            dimensions=(0.12, 0.13, 1.48),
            material=wood_material,
            bevel=0.025,
        )

    # ------------------------------------------------------------------
    # Side stretchers connect front and rear legs near the floor.  They are
    # clearly visible in the supplied chair reference and add structural detail.
    # ------------------------------------------------------------------
    add_beveled_cube(
        "Left_Side_Stretcher",
        (-0.93, 0.0, 0.72),
        (0.13, 1.48, 0.15),
        wood_material,
        bevel=0.025,
    )
    add_beveled_cube(
        "Right_Side_Stretcher",
        (0.93, 0.0, 0.72),
        (0.13, 1.48, 0.15),
        wood_material,
        bevel=0.025,
    )


# -----------------------------------------------------------------------------
# Lighting, floor, camera, and render settings
# -----------------------------------------------------------------------------
def setup_environment():
    """Create a simple three-light studio scene that clearly shows the chair."""
    floor_material = make_floor_material()

    bpy.ops.mesh.primitive_plane_add(size=14.0, location=(0.0, 0.0, -0.04))
    floor = bpy.context.active_object
    floor.name = "Floor"
    floor.data.materials.append(floor_material)

    # Key light: strongest light from the front-left/top.
    bpy.ops.object.light_add(type='AREA', location=(4.5, -4.8, 6.6))
    key = bpy.context.active_object
    key.name = "Key_Light"
    key.data.energy = 850
    key.data.shape = 'DISK'
    key.data.size = 4.0
    point_object_at(key, (0.0, 0.0, 2.0))

    # Fill light: softer light from the opposite side reduces harsh shadows.
    bpy.ops.object.light_add(type='AREA', location=(-4.0, -2.0, 4.8))
    fill = bpy.context.active_object
    fill.name = "Fill_Light"
    fill.data.energy = 430
    fill.data.size = 3.5
    point_object_at(fill, (0.0, 0.0, 2.0))

    # Rim light separates the rear edges of the chair from the background.
    bpy.ops.object.light_add(type='AREA', location=(0.0, 4.8, 6.0))
    rim = bpy.context.active_object
    rim.name = "Rim_Light"
    rim.data.energy = 560
    rim.data.size = 3.0
    point_object_at(rim, (0.0, 0.4, 2.2))

    # Camera uses a three-quarter perspective similar to the assignment image.
    bpy.ops.object.camera_add(location=(5.8, -7.2, 5.1))
    camera = bpy.context.active_object
    camera.name = "Camera"
    camera.data.lens = 52
    point_object_at(camera, (0.0, 0.05, 2.05))
    bpy.context.scene.camera = camera

    scene = bpy.context.scene

    # Use Eevee when available.  The fallback supports slightly older Blender
    # releases without making the script fail just because the engine enum name
    # changed between Blender versions.
    render_engine_set = False
    for engine_name in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
        try:
            scene.render.engine = engine_name
            render_engine_set = True
            break
        except (TypeError, ValueError):
            pass

    if not render_engine_set:
        # Cycles is bundled with standard Blender and is a safe final fallback
        # if neither Eevee identifier is available.
        scene.render.engine = 'CYCLES'

    scene.render.resolution_x = 900
    scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100

    # Slightly brighten the world so unlit areas are not completely black.
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    background = scene.world.node_tree.nodes.get("Background")
    if background is not None:
        background.inputs["Color"].default_value = (0.035, 0.04, 0.05, 1.0)
        background.inputs["Strength"].default_value = 0.45


# -----------------------------------------------------------------------------
# OBJ export
# -----------------------------------------------------------------------------
def export_chair_to_obj():
    """Export only the modeled chair meshes as OpenGL/chair.obj."""
    bpy.ops.object.select_all(action='DESELECT')

    for obj in CHAIR_OBJECTS:
        if obj.name in bpy.context.scene.objects:
            obj.select_set(True)

    if not CHAIR_OBJECTS:
        raise RuntimeError("No chair objects were created; OBJ export cannot continue.")

    bpy.context.view_layer.objects.active = CHAIR_OBJECTS[0]

    # Blender 4.x uses bpy.ops.wm.obj_export.  The fallback supports Blender
    # versions where the legacy OBJ exporter operator is still installed.
    try:
        bpy.ops.wm.obj_export(
            filepath=OBJ_PATH,
            export_selected_objects=True,
            export_materials=True,
        )
    except (AttributeError, TypeError, RuntimeError):
        try:
            bpy.ops.export_scene.obj(
                filepath=OBJ_PATH,
                use_selection=True,
                use_materials=True,
            )
        except Exception as exc:
            raise RuntimeError(
                "OBJ export failed. Use a Blender version with the built-in OBJ exporter enabled."
            ) from exc


# -----------------------------------------------------------------------------
# Main script
# -----------------------------------------------------------------------------
def main():
    """Build, texture, light, save, and export the complete assignment model."""
    clear_scene()

    wood_image = create_pine_texture()
    wood_material = make_wood_material(wood_image)

    build_chair(wood_material)
    setup_environment()

    # Pack the generated texture into the .blend file as well as keeping the
    # standalone PNG.  The .blend is therefore self-contained for submission.
    try:
        wood_image.pack()
    except RuntimeError:
        # The standalone PNG still exists even if this Blender version decides
        # the generated image is already packed.
        pass

    bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)
    export_chair_to_obj()

    # Save again because the exporter changes object selection state.  This is
    # not required for geometry, but it leaves a deterministic final .blend.
    bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)

    print("\n============================================================")
    print("COMP 371 Assignment 3 generated successfully")
    print("Reema Aboudraz  - 40253549")
    print("Wissem Oumsalem - 40291712")
    print("------------------------------------------------------------")
    print(f"Blender file : {BLEND_PATH}")
    print(f"Texture map  : {TEXTURE_PATH}")
    print(f"OBJ file     : {OBJ_PATH}")
    print("============================================================\n")


if __name__ == "__main__":
    main()
