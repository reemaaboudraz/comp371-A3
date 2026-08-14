# COMP 371 – Computer Graphics
## Assignment 3 – Summer 2026

### Group Members

| Student | Student ID |
|---|---:|
| Reema Aboudraz | 40253549 |
| Wissem Oumsalem | 40291712 |

---

## 1. Project Overview

This project completes the programming portion of COMP 371 Assignment 3.

The assignment has two connected parts:

1. **Blender:** create a detailed 3D chair model, apply materials, and set up a lighting scene.
2. **OpenGL:** export the chair from Blender as an OBJ file, load the OBJ in C++, display it as a wireframe, and control translation, rotation, and scaling from the keyboard.

The package is organized so that the Blender script generates the chair first. The resulting `chair.obj` is then used by the OpenGL application.

---

## 2. Folder Structure

```text
COMP371_Assignment3_Code/
│
├── README.md
│
├── Blender/
│   └── create_chair.py
│
└── OpenGL/
    ├── CMakeLists.txt
    └── src/
        └── main.cpp
```

After running the Blender script, two additional files are generated:

```text
assignment3_chair.blend
chair.obj
```

By default, Blender saves them in:

```text
C:\Users\YOUR_USERNAME\COMP371_Assignment3_Output\
```

The exact path is also printed in Blender's console after the script finishes.

---

# PART A – BLENDER

## 3. What `create_chair.py` Does

`Blender/create_chair.py` automatically creates the Blender scene required for the assignment.

The script:

- clears Blender's default scene;
- creates the chair seat;
- creates four legs;
- creates the frame beneath the seat;
- creates the rear chair posts;
- creates multiple backrest slats;
- creates the top rail;
- creates side support stretchers;
- bevels the chair pieces so the model does not look completely block-shaped;
- creates wood-style materials;
- assigns materials to chair components;
- creates a floor;
- creates a three-light lighting setup;
- creates and positions a camera;
- saves the Blender project as `assignment3_chair.blend`;
- exports the chair geometry as `chair.obj` for the OpenGL program.

The script is heavily commented so that each major modeling operation can be followed and explained during the demo.

---

## 4. How to Run the Blender Part

### Step 1 – Open Blender

Start Blender normally.

### Step 2 – Open the Scripting workspace

At the top of Blender, select:

```text
Scripting
```

### Step 3 – Open the script

Inside the Text Editor:

1. Click **Open**.
2. Navigate to the project folder.
3. Open:

```text
Blender/create_chair.py
```

### Step 4 – Run the script

Click:

```text
Run Script
```

or press:

```text
Alt + P
```

while the mouse is inside the Text Editor.

### Step 5 – Check the result

The complete chair scene should appear in Blender.

The script automatically saves:

```text
assignment3_chair.blend
chair.obj
```

inside:

```text
C:\Users\YOUR_USERNAME\COMP371_Assignment3_Output\
```

### Step 6 – Save screenshots

Take the screenshots required for the Blender PDF while inspecting the model.

Useful screenshots include:

- basic chair geometry;
- seat and legs;
- backrest construction;
- support/frame details;
- materials;
- lighting setup;
- final chair model;
- final rendered or material-preview view.

The assignment expects screenshots demonstrating the modeling process, so make sure the final submission contains enough screenshots to clearly show how the chair was constructed.

---

# PART B – OPENGL

## 5. Required Software

The OpenGL project uses:

- C++17
- OpenGL
- GLFW
- GLEW
- GLM
- CMake

A convenient Windows setup is:

- Visual Studio 2022 with **Desktop development with C++** installed;
- CMake;
- vcpkg.

---

## 6. Important Step Before Building

After running the Blender script, locate:

```text
chair.obj
```

in:

```text
C:\Users\YOUR_USERNAME\COMP371_Assignment3_Output\
```

Copy it into the `OpenGL` folder so the structure becomes:

```text
OpenGL/
├── chair.obj
├── CMakeLists.txt
└── src/
    └── main.cpp
```

**Do this before configuring/building the CMake project.**

`CMakeLists.txt` contains a post-build command that copies `chair.obj` beside the generated executable when the OBJ exists in the OpenGL project folder.

---

# 7. Installing the C++ Libraries with vcpkg

If vcpkg is already installed, open **PowerShell** or a **Developer Command Prompt for Visual Studio** and run:

```powershell
vcpkg install glfw3:x64-windows glew:x64-windows glm:x64-windows
```

The libraries provide:

| Library | Purpose |
|---|---|
| GLFW | Creates the application window and handles keyboard input. |
| GLEW | Loads modern OpenGL functions. |
| GLM | Provides vectors, matrices, transformations, camera math, and projection math. |
| OpenGL | Renders the actual 3D geometry. |

---

# 8. Build and Run – Recommended Windows Method

Open PowerShell inside:

```text
COMP371_Assignment3_Code\OpenGL
```

For example:

```powershell
cd C:\path\to\COMP371_Assignment3_Code\OpenGL
```

## Step 1 – Configure CMake

Run:

```powershell
cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

Replace:

```text
C:/vcpkg
```

with the actual folder where your copy of vcpkg is installed.

Example:

```powershell
cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/Users/Reema/vcpkg/scripts/buildsystems/vcpkg.cmake
```

A successful configuration should end with messages similar to:

```text
Configuring done
Generating done
Build files have been written to: .../OpenGL/build
```

## Step 2 – Compile the project

Run:

```powershell
cmake --build build --config Release
```

CMake/Visual Studio will compile `src/main.cpp` and create the executable.

The executable will normally be located at:

```text
OpenGL\build\Release\COMP371_Assignment3.exe
```

Because `chair.obj` was placed in the OpenGL folder before configuration, CMake also copies it beside the executable.

## Step 3 – Run the application

Run:

```powershell
.\build\Release\COMP371_Assignment3.exe
```

An OpenGL window titled:

```text
COMP 371 - Assignment 3 - Chair
```

should open and display the chair as a wireframe.

---

# 9. Keyboard Controls

The controls match the transformation mapping used for Assignment 2.

| Key | Action |
|---|---|
| `W` | Translate chair upward |
| `S` | Translate chair downward |
| `A` | Translate chair left |
| `D` | Translate chair right |
| `Q` | Rotate chair +30° around the Z axis |
| `E` | Rotate chair -30° around the Z axis |
| `R` | Increase chair scale |
| `F` | Decrease chair scale |
| `ESC` | Close the OpenGL program |

The program supports holding a key down because GLFW repeat events are also processed.

---

# 10. How the OpenGL Program Works

The full OpenGL implementation is inside:

```text
OpenGL/src/main.cpp
```

The file contains detailed comments explaining the major statements and OpenGL functions.

## 10.1 GLFW creates the window

The program first initializes GLFW and creates an OpenGL 3.3 Core Profile window.

Conceptually:

```text
GLFW initialization
        ↓
Create window
        ↓
Create OpenGL context
        ↓
Register keyboard callback
```

---

## 10.2 GLEW loads OpenGL functions

After an OpenGL context exists, GLEW is initialized.

GLEW gives the C++ program access to OpenGL functions such as:

```text
glGenVertexArrays
glGenBuffers
glBufferData
glCreateShader
glUseProgram
glDrawArrays
```

---

## 10.3 The OBJ file is loaded

The custom `loadOBJ()` function reads `chair.obj` line by line.

OBJ files contain entries such as:

```text
v 1.0 2.0 3.0
```

which describe vertex positions, and:

```text
f 1 2 3 4
```

which describe faces.

The loader:

1. reads all vertex positions;
2. reads each face;
3. extracts the position index from each OBJ face token;
4. triangulates polygons using a triangle fan;
5. stores the final triangles in a C++ vertex vector.

This means the project does not depend on a separate OBJ-loading library.

---

## 10.4 The chair is centered before rotation

The function:

```cpp
normalizeModel(vertices);
```

finds the minimum and maximum coordinates of the imported chair.

It then calculates the center of the model and moves the geometry so that the chair is centered around the origin:

```text
(0, 0, 0)
```

The chair is also uniformly resized to a predictable size.

This step is especially important for the assignment requirement that the chair **rotate while remaining in its place on the screen**.

If a model is far away from the origin and rotation is applied directly, it can appear to orbit around another point. Centering the geometry gives the chair a natural local rotation center.

---

## 10.5 Vertex Array Object and Vertex Buffer Object

The model's vertex data is uploaded to the GPU using a VBO.

The VAO stores the description of how that vertex information is organized.

Conceptually:

```text
OBJ file
   ↓
CPU vertex vector
   ↓
VBO – vertex data stored on GPU
   ↓
VAO – remembers how OpenGL reads the data
   ↓
Vertex shader
```

---

## 10.6 Shaders

The application uses a small vertex shader and fragment shader embedded directly in `main.cpp`.

### Vertex shader

The vertex shader receives each chair position and multiplies it by the MVP matrix:

```glsl
gl_Position = uMVP * vec4(aPosition, 1.0);
```

`uMVP` represents:

```text
Projection × View × Model
```

### Fragment shader

The fragment shader gives the wireframe a constant light color.

No OpenGL texture is necessary because the assignment only requires a wireframe for the OpenGL section.

---

# 11. Transformation System

The program stores three transformation states:

```cpp
gTranslation
gScale
gRotationZ
```

The GLFW keyboard callback changes these values whenever a transformation key is pressed.

The render loop then creates the model matrix from them.

Conceptually:

```text
Identity Matrix
      ↓
Translation
      ↓
Rotation
      ↓
Scaling
      ↓
Model Matrix
```

The model matrix is combined with the camera view matrix and projection matrix:

```text
MVP = Projection × View × Model
```

The completed MVP matrix is sent to the vertex shader every frame.

---

# 12. Wireframe Rendering

The assignment only requires a wireframe for the OpenGL portion.

The program enables wireframe rendering with:

```cpp
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
```

The model is still drawn as triangles, but OpenGL displays only the triangle edges instead of filling the surfaces.

---

# 13. Main Render Loop

Once initialization is complete, the application repeatedly performs the following operations:

```text
Clear screen
    ↓
Calculate transformation matrices
    ↓
Send MVP matrix to shader
    ↓
Bind chair VAO
    ↓
Draw chair
    ↓
Swap buffers
    ↓
Process keyboard/window events
    ↓
Repeat
```

The loop continues until the window is closed or `ESC` is pressed.

---

# 14. What to Screenshot for the OpenGL PDF

Recommended screenshots:

1. Initial imported chair displayed as a wireframe.
2. Chair translated upward or downward.
3. Chair translated left or right.
4. Chair rotated with `Q`.
5. Chair rotated with `E`.
6. Chair enlarged with `R`.
7. Chair reduced with `F`.

Try to make each transformation visually obvious so that the screenshots clearly demonstrate that the functionality works.

---

# 15. Troubleshooting

## Error: `Could not open OBJ file: chair.obj`

Cause: the application cannot find the OBJ file.

Fix:

1. Make sure Blender generated `chair.obj`.
2. Copy it into:

```text
OpenGL\chair.obj
```

3. Re-run the CMake configuration/build so the post-build copy executes.

You can also manually copy `chair.obj` beside:

```text
COMP371_Assignment3.exe
```

---

## CMake cannot find GLFW, GLEW, or GLM

Make sure they were installed with vcpkg:

```powershell
vcpkg install glfw3:x64-windows glew:x64-windows glm:x64-windows
```

Then make sure the CMake command contains the correct vcpkg toolchain path:

```powershell
-DCMAKE_TOOLCHAIN_FILE=C:/YOUR_VCPKG_PATH/scripts/buildsystems/vcpkg.cmake
```

---

## `cmake` is not recognized

Install CMake or enable the CMake component through Visual Studio Installer.

Then close and reopen the terminal so the PATH is refreshed.

---

## Compiler is missing

Open **Visual Studio Installer** and make sure this workload is installed:

```text
Desktop development with C++
```

---

## Blender reports an OBJ export error

The script first attempts Blender's newer OBJ exporter and contains a fallback for older Blender versions.

If export still fails, export manually from Blender:

```text
File → Export → Wavefront (.obj)
```

Make sure only the chair mesh is exported and not the floor/camera/lights.

---

## Chair is too small, too large, or outside the view

The OpenGL program automatically normalizes the imported geometry, so a normally exported chair should fit the camera view.

If unusual OBJ settings were used during a manual export, regenerate the OBJ using `create_chair.py` and try again.

---

# 16. Assignment Requirement Mapping

| Assignment Requirement | Implementation |
|---|---|
| Create a detailed 3D chair model | `Blender/create_chair.py` creates the chair from multiple modeled components. |
| Add/refine details | Beveled seat, legs, aprons, posts, slats, rail, and stretchers are generated. |
| Materials/texturing | Wood-style Blender materials are assigned to the chair components. |
| Lighting | Blender script creates key, fill, and rim lights. |
| Export OBJ | Blender script exports `chair.obj`. |
| OpenGL window using GLFW/GLEW | Implemented in `main.cpp`. |
| Import OBJ into OpenGL | Custom OBJ loader implemented in `main.cpp`. |
| Translation | `W`, `S`, `A`, and `D`. |
| Rotation | `Q` and `E`, in 30° increments. |
| Rotation stays in place | OBJ vertices are centered at the origin before model transformations. |
| Scaling | `R` and `F`. |
| Keyboard controls | Implemented through the GLFW key callback. |
| OpenGL wireframe | `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`. |
| C++ | OpenGL application is written in C++17. |
| GLM | Used for translation, rotation, scaling, camera, and projection matrices. |

---

# 17. Files to Prepare for Final Submission

The code package generates or supports the programming/modeling portions, but the final Moodle submission should be assembled after you run the project and capture your own screenshots.

Prepare the following required files:

```text
assignment3_chair.blend
chair.obj
blender.pdf
OpenGLRun.pdf
OpenGL project ZIP
Who-Did-What report
```

All submitted documents should contain the group members' names and student IDs:

```text
Omar El Akrae     – 40252799
Reema Aboudraz    – 40253549
Wissem Oumsalem   – 40291712
```

Only one final submission should be made for the group.

---

# 18. Demo Preparation

Every group member should understand the complete project, not only the section they personally worked on.

Before the demo, each member should be able to explain at least:

- how the Blender chair is constructed;
- why beveling is used;
- how the OBJ export works;
- what an OBJ `v` line represents;
- what an OBJ `f` line represents;
- how the custom OBJ loader works;
- why polygons are triangulated;
- what GLFW does;
- what GLEW does;
- what GLM does;
- what a VAO is;
- what a VBO is;
- what the vertex shader does;
- what the fragment shader does;
- what the Model, View, and Projection matrices do;
- how translation is implemented;
- how rotation is implemented;
- why the chair is centered before rotation;
- how scaling is implemented;
- why `glPolygonMode(..., GL_LINE)` creates a wireframe;
- how the main rendering loop works.

The source code contains extensive comments that can be used to review these topics before the demo.

---

## Quick Run Summary

If everything is already installed, the complete workflow is:

```text
1. Open Blender.
2. Open Blender/create_chair.py.
3. Run the script.
4. Find chair.obj in COMP371_Assignment3_Output.
5. Copy chair.obj into the OpenGL folder.
6. Open PowerShell in OpenGL.
7. Run:

   cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

8. Run:

   cmake --build build --config Release

9. Run:

   .\build\Release\COMP371_Assignment3.exe

10. Test W, S, A, D, Q, E, R, F, and ESC.
11. Capture the required screenshots.
```

---

**COMP 371 – Assignment 3 – Summer 2026**  
**Omar El Akrae – 40252799**  
**Reema Aboudraz – 40253549**  
**Wissem Oumsalem – 40291712**
