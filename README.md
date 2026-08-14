# COMP 371 - Computer Graphics
## Assignment 3 - Summer 2026

### Team Members

| Name | Student ID |
|---|---:|
| Reema Aboudraz | 40253549 |
| Wissem Oumsalem | 40291712 |

## What this code covers

The Blender script creates the chair model, adds realistic beveled details, creates four **vertical** backrest slats to match the provided reference chair, UV unwraps every chair mesh, generates and applies a real PNG wood texture map, sets up a simple three-light scene and camera, saves the `.blend` file, and exports `OpenGL/chair.obj`.

The C++ program creates an OpenGL window using GLFW and GLEW, loads the exported OBJ, displays it in wireframe, and uses GLM matrices to translate, rotate, and scale the chair. The OBJ geometry is centered before transformations so rotation occurs around the chair itself and the model stays in place on screen.

## 1. Run the Blender part first

1. Open Blender.
2. Select the **Scripting** workspace.
3. Open `Blender/create_chair.py` in the Text Editor.
4. Press **Alt + P** or click **Run Script**.
5. Wait until the console prints `generated successfully`.

The script creates:

```text
Blender/assignment3_chair.blend
Blender/pine_wood_texture.png
OpenGL/chair.obj
```

Open the generated `.blend` and inspect the model/materials/lighting before taking your Blender screenshots.

## 2. Install the OpenGL dependencies

With vcpkg on Windows:

```powershell
vcpkg install glfw3:x64-windows glew:x64-windows glm:x64-windows
```

You also need Visual Studio with **Desktop development with C++** and CMake.

## 3. Configure the OpenGL project

Open PowerShell in the `OpenGL` folder. `chair.obj` must already exist, so run the Blender script first.

```powershell
cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

Replace `C:/vcpkg` with your real vcpkg installation folder.

## 4. Build

```powershell
cmake --build build --config Release
```

## 5. Run

```powershell
.\build\Release\COMP371_Assignment3.exe
```

The program also accepts a custom OBJ path:

```powershell
.\build\Release\COMP371_Assignment3.exe "C:\path\to\chair.obj"
```

## Keyboard controls

| Key | Action |
|---|---|
| `W` | Translate up |
| `S` | Translate down |
| `A` | Translate left |
| `D` | Translate right |
| `Q` | Rotate +30 degrees around Z |
| `E` | Rotate -30 degrees around Z |
| `R` | Scale up |
| `F` | Scale down |
| `ESC` | Quit |

## Important implementation points for the demo

- `create_pine_texture()` generates an actual PNG wood texture map, so no external image download is needed.
- `smart_uv_unwrap()` UV unwraps each final beveled mesh before texture mapping.
- The Blender material uses the mesh UV coordinates to sample the PNG texture.
- `export_chair_to_obj()` exports only chair meshes, not the floor, lights, or camera.
- `loadOBJ()` reads `v` and `f` records and triangulates polygon faces.
- `normalizeModel()` computes the model bounding box, moves its center to `(0,0,0)`, and scales it to a predictable size.
- Centering the chair is why Q/E rotation happens **in place** instead of orbiting around another point.
- `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)` provides the required OpenGL wireframe view.
- The model matrix is built as translation, rotation, and scaling and then combined as `Projection * View * Model`.

## Files you still create yourself for submission

The code generates the `.blend` and `.obj`. You still need to take your own screenshots and create the PDFs/report required by the assignment.
