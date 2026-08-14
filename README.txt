COMP 371 - Assignment 3 Code Package
=====================================

1) BLENDER
----------
Open Blender -> Scripting workspace -> open Blender/create_chair.py -> Run Script.

The script creates:
- assignment3_chair.blend
- chair.obj

By default they are written to:
  ~/COMP371_Assignment3_Output/

Take screenshots while modeling if your instructor expects screenshots of the process.
Because this script creates the model automatically, you can also execute it section-by-section
or recreate the same pieces manually if you want screenshots that clearly show progressive steps.

2) OPENGL
---------
Copy the generated chair.obj into the OpenGL folder beside CMakeLists.txt.

Dependencies:
- OpenGL
- GLFW
- GLEW
- GLM

Recommended Windows setup with vcpkg:
  vcpkg install glfw3 glew glm

Then configure/build with CMake, or open the folder directly in modern Visual Studio with CMake support.

Controls:
- W / S : translate up / down
- A / D : translate left / right
- Q / E : rotate +/-30 degrees around Z
- R / F : scale up / down uniformly
- ESC   : quit

The OBJ is normalized to the origin before transformations are applied. This is what makes rotation
occur around the chair itself instead of causing the model to orbit around a distant point.

The OpenGL display uses:
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
so the model is shown as wireframe, matching the assignment requirement.

3) SCREENSHOTS / SUBMISSION
---------------------------
After running everything, capture:
- Blender modeling steps
- Final textured Blender model
- Lighting scene
- OpenGL imported chair
- OpenGL translation example
- OpenGL rotation example
- OpenGL scaling example

Then prepare the PDFs and submission documents required by your assignment instructions.
