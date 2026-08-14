/**
 * COMP 371 - Computer Graphics
 * Assignment 3 - Summer 2026
 *
 * Team members:
 *   Reema Aboudraz  - 40253549
 *   Wissem Oumsalem - 40291712
 *
 * Purpose:
 *   Load the chair OBJ exported from Blender, display it in OpenGL as a
 *   wireframe, and apply keyboard-controlled translation, rotation, and
 *   scaling.  The imported model is centered before rendering so rotation is
 *   performed around the chair itself and the chair stays in place on screen.
 */

#include <GL/glew.h>                         // Loads modern OpenGL function pointers.
#include <GLFW/glfw3.h>                      // Creates the window and handles keyboard input.
#include <glm/glm.hpp>                       // Provides vectors and matrices.
#include <glm/gtc/matrix_transform.hpp>      // Provides translate, rotate, scale, lookAt, perspective.
#include <glm/gtc/type_ptr.hpp>              // Converts GLM matrices for OpenGL uniform upload.

#include <algorithm>                         // std::max and std::min.
#include <cstddef>                           // std::size_t.
#include <filesystem>                        // Portable file-path handling for chair.obj.
#include <fstream>                           // Reads the OBJ file.
#include <iostream>                          // Console output and error messages.
#include <limits>                            // Numeric limits used by model normalization.
#include <sstream>                           // Parses OBJ lines.
#include <string>                            // Stores paths, tokens, and shader text.
#include <vector>                            // Dynamic storage for loaded geometry.

// -----------------------------------------------------------------------------
// Window settings.
// -----------------------------------------------------------------------------
constexpr int WINDOW_WIDTH = 1000;           // Initial OpenGL window width.
constexpr int WINDOW_HEIGHT = 800;           // Initial OpenGL window height.

// -----------------------------------------------------------------------------
// Keyboard-controlled transformation state.
// These keys intentionally match Assignment 2.
// -----------------------------------------------------------------------------
glm::vec3 gTranslation(0.0f, 0.0f, 0.0f);   // W/S/A/D modify X/Y translation.
glm::vec3 gScale(1.0f, 1.0f, 1.0f);         // R/F uniformly enlarge/shrink the chair.
float gRotationZ = 0.0f;                     // Q/E rotate around the Z axis in degrees.

constexpr float TRANSLATION_STEP = 0.12f;    // Translation amount for each key event.
constexpr float ROTATION_STEP = 30.0f;       // Required assignment-style 30-degree step.
constexpr float SCALE_STEP = 0.10f;          // Scale amount for each key event.
constexpr float MIN_SCALE = 0.10f;           // Prevents zero or negative scale.

// -----------------------------------------------------------------------------
// One vertex contains only a 3D position.  The OpenGL requirement is wireframe,
// so normals, colors, and texture coordinates are not required for rendering.
// -----------------------------------------------------------------------------
struct Vertex
{
    float x;                                 // X position.
    float y;                                 // Y position.
    float z;                                 // Z position.
};

// -----------------------------------------------------------------------------
// Shader helpers.
// -----------------------------------------------------------------------------
GLuint compileShader(GLenum shaderType, const char* source)
{
    GLuint shader = glCreateShader(shaderType);          // Create an empty shader object.
    glShaderSource(shader, 1, &source, nullptr);         // Attach GLSL source code.
    glCompileShader(shader);                             // Compile source using the OpenGL driver.

    GLint compiled = GL_FALSE;                           // Receives the compilation result.
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled); // Ask OpenGL whether compilation succeeded.

    if (compiled != GL_TRUE)
    {
        GLint logLength = 0;                             // Number of characters in the driver log.
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        // Allocate at least one byte so log.data() is always valid.
        std::vector<char> log(static_cast<std::size_t>(std::max(1, logLength)), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());

        std::cerr << "Shader compilation failed:\n" << log.data() << '\n';
        glDeleteShader(shader);                          // Release the failed GPU object.
        return 0;                                        // 0 signals failure to the caller.
    }

    return shader;                                       // Return the successfully compiled shader.
}

GLuint createShaderProgram()
{
    // Vertex shader: transform each OBJ vertex from model space to clip space.
    const char* vertexShaderSource = R"GLSL(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        uniform mat4 uMVP;

        void main()
        {
            gl_Position = uMVP * vec4(aPosition, 1.0);
        }
    )GLSL";

    // Fragment shader: output one light color for all visible wireframe lines.
    const char* fragmentShaderSource = R"GLSL(
        #version 330 core
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(0.94, 0.94, 0.96, 1.0);
        }
    )GLSL";

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (vertexShader == 0 || fragmentShader == 0)
    {
        if (vertexShader != 0)
        {
            glDeleteShader(vertexShader);
        }
        if (fragmentShader != 0)
        {
            glDeleteShader(fragmentShader);
        }
        return 0;
    }

    GLuint program = glCreateProgram();                  // Create a linkable GPU program.
    glAttachShader(program, vertexShader);               // Attach vertex stage.
    glAttachShader(program, fragmentShader);             // Attach fragment stage.
    glLinkProgram(program);                              // Link both stages together.

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (linked != GL_TRUE)
    {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<std::size_t>(std::max(1, logLength)), '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());

        std::cerr << "Shader program linking failed:\n" << log.data() << '\n';
        glDeleteProgram(program);
        program = 0;
    }

    // Once linked, the separate shader objects are no longer required.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// -----------------------------------------------------------------------------
// OBJ parsing helpers.
// -----------------------------------------------------------------------------
bool parseVertexIndex(const std::string& token, int vertexCount, int& resolvedIndex)
{
    // OBJ face elements may look like: 12, 12/4, 12//9, or 12/4/9.
    const std::size_t slash = token.find('/');
    const std::string indexText = token.substr(0, slash);

    if (indexText.empty())
    {
        return false;
    }

    int objIndex = 0;
    try
    {
        objIndex = std::stoi(indexText);                 // Convert the text index to an integer.
    }
    catch (const std::exception&)
    {
        return false;                                    // Reject malformed face tokens safely.
    }

    // OBJ index 0 is invalid.  Positive indices are 1-based.  Negative indices
    // are relative to the most recently defined vertex (-1 means last vertex).
    if (objIndex == 0)
    {
        return false;
    }

    if (objIndex > 0)
    {
        resolvedIndex = objIndex - 1;
    }
    else
    {
        resolvedIndex = vertexCount + objIndex;
    }

    return resolvedIndex >= 0 && resolvedIndex < vertexCount;
}

bool loadOBJ(const std::filesystem::path& path, std::vector<Vertex>& triangleVertices)
{
    std::ifstream file(path);                            // Open the Blender-exported OBJ file.
    if (!file.is_open())
    {
        std::cerr << "Could not open OBJ file: " << path << '\n';
        return false;
    }

    std::vector<glm::vec3> positions;                    // Raw OBJ 'v' positions.
    std::string line;                                    // One file line at a time.
    std::size_t lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;

        if (line.empty() || line[0] == '#')
        {
            continue;                                    // Ignore blank lines and comments.
        }

        std::istringstream stream(line);
        std::string prefix;
        stream >> prefix;

        if (prefix == "v")
        {
            glm::vec3 position(0.0f);
            if (!(stream >> position.x >> position.y >> position.z))
            {
                std::cerr << "Malformed vertex on OBJ line " << lineNumber << ".\n";
                return false;
            }
            positions.push_back(position);
        }
        else if (prefix == "f")
        {
            std::vector<int> faceIndices;
            std::string token;

            while (stream >> token)
            {
                int index = -1;
                if (!parseVertexIndex(token, static_cast<int>(positions.size()), index))
                {
                    std::cerr << "Invalid face index on OBJ line " << lineNumber << ".\n";
                    return false;
                }
                faceIndices.push_back(index);
            }

            if (faceIndices.size() < 3)
            {
                std::cerr << "Face with fewer than 3 vertices on OBJ line " << lineNumber << ".\n";
                return false;
            }

            // Triangulate any polygon as a triangle fan:
            // (0,1,2), (0,2,3), (0,3,4), ...
            for (std::size_t i = 1; i + 1 < faceIndices.size(); ++i)
            {
                const glm::vec3& a = positions[faceIndices[0]];
                const glm::vec3& b = positions[faceIndices[i]];
                const glm::vec3& c = positions[faceIndices[i + 1]];

                triangleVertices.push_back({a.x, a.y, a.z});
                triangleVertices.push_back({b.x, b.y, b.z});
                triangleVertices.push_back({c.x, c.y, c.z});
            }
        }

        // All other OBJ records (vt, vn, usemtl, mtllib, o, s, etc.) are not
        // needed by this wireframe renderer and are safely ignored.
    }

    if (positions.empty())
    {
        std::cerr << "OBJ file contains no vertex positions.\n";
        return false;
    }

    if (triangleVertices.empty())
    {
        std::cerr << "OBJ file contains no usable faces.\n";
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
// Centering is what makes rotation occur around the chair itself rather than
// making the chair orbit around a distant world-space origin.
// -----------------------------------------------------------------------------
void normalizeModel(std::vector<Vertex>& vertices)
{
    glm::vec3 minimum(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    );

    glm::vec3 maximum(
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    );

    for (const Vertex& vertex : vertices)
    {
        const glm::vec3 p(vertex.x, vertex.y, vertex.z);
        minimum = glm::min(minimum, p);
        maximum = glm::max(maximum, p);
    }

    const glm::vec3 center = (minimum + maximum) * 0.5f;
    const glm::vec3 size = maximum - minimum;
    const float largestDimension = std::max({size.x, size.y, size.z});

    // Fit the largest model dimension into approximately three world units.
    const float normalizationScale = (largestDimension > 0.0f)
        ? (3.0f / largestDimension)
        : 1.0f;

    for (Vertex& vertex : vertices)
    {
        glm::vec3 p(vertex.x, vertex.y, vertex.z);
        p = (p - center) * normalizationScale;
        vertex.x = p.x;
        vertex.y = p.y;
        vertex.z = p.z;
    }
}

// -----------------------------------------------------------------------------
// Find chair.obj reliably whether the executable is started from PowerShell,
// Visual Studio, or a build directory.
// -----------------------------------------------------------------------------
std::filesystem::path findOBJPath(int argc, char* argv[])
{
    if (argc > 1)
    {
        return std::filesystem::path(argv[1]);           // Explicit user-supplied path wins.
    }

    const std::filesystem::path executablePath = std::filesystem::absolute(argv[0]);
    const std::filesystem::path executableDir = executablePath.parent_path();

    const std::vector<std::filesystem::path> candidates = {
        std::filesystem::current_path() / "chair.obj",
        executableDir / "chair.obj",
        executableDir / ".." / "chair.obj",
        executableDir / ".." / ".." / "chair.obj",
        executableDir / ".." / ".." / ".." / "chair.obj"
    };

    for (const auto& candidate : candidates)
    {
        std::error_code error;
        if (std::filesystem::exists(candidate, error) && !error)
        {
            return std::filesystem::weakly_canonical(candidate, error);
        }
    }

    // Return the normal expected name so loadOBJ() prints a clear error path.
    return std::filesystem::current_path() / "chair.obj";
}

// -----------------------------------------------------------------------------
// Keyboard input: same keys as Assignment 2.
// -----------------------------------------------------------------------------
void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
    {
        return;                                          // Ignore key-release events.
    }

    switch (key)
    {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE); // ESC closes the application.
            break;

        case GLFW_KEY_W:
            gTranslation.y += TRANSLATION_STEP;          // Move up.
            break;

        case GLFW_KEY_S:
            gTranslation.y -= TRANSLATION_STEP;          // Move down.
            break;

        case GLFW_KEY_A:
            gTranslation.x -= TRANSLATION_STEP;          // Move left.
            break;

        case GLFW_KEY_D:
            gTranslation.x += TRANSLATION_STEP;          // Move right.
            break;

        case GLFW_KEY_Q:
            gRotationZ += ROTATION_STEP;                 // Rotate +30 degrees around Z.
            break;

        case GLFW_KEY_E:
            gRotationZ -= ROTATION_STEP;                 // Rotate -30 degrees around Z.
            break;

        case GLFW_KEY_R:
            gScale += glm::vec3(SCALE_STEP);             // Uniformly enlarge the chair.
            break;

        case GLFW_KEY_F:
            gScale -= glm::vec3(SCALE_STEP);             // Uniformly shrink the chair.
            gScale = glm::max(gScale, glm::vec3(MIN_SCALE));
            break;

        default:
            break;
    }
}

// -----------------------------------------------------------------------------
// Program entry point.
// -----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    const std::filesystem::path objPath = findOBJPath(argc, argv);

    if (glfwInit() != GLFW_TRUE)
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return 1;
    }

    // Request the OpenGL 3.3 Core Profile used by the shaders below.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        "COMP 371 - Assignment 3 - Chair",
        nullptr,
        nullptr
    );

    if (window == nullptr)
    {
        std::cerr << "Failed to create the GLFW window.\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);                       // Activate this window's OpenGL context.
    glfwSetKeyCallback(window, keyCallback);             // Register keyboard controls.
    glfwSwapInterval(1);                                 // Enable V-sync.

    glewExperimental = GL_TRUE;                          // Needed for core-profile function loading.
    const GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW: "
                  << reinterpret_cast<const char*>(glewGetErrorString(glewStatus))
                  << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // GLEW may generate one harmless GL_INVALID_ENUM during core-profile
    // initialization.  Clear it before ordinary rendering starts.
    glGetError();

    std::vector<Vertex> vertices;
    if (!loadOBJ(objPath, vertices))
    {
        std::cerr << "Expected Blender to export chair.obj before running OpenGL.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    normalizeModel(vertices);                             // Center model for in-place rotation.

    GLuint shaderProgram = createShaderProgram();
    if (shaderProgram == 0)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    GLuint vao = 0;
    GLuint vbo = 0;

    glGenVertexArrays(1, &vao);                          // Create a Vertex Array Object.
    glGenBuffers(1, &vbo);                               // Create a Vertex Buffer Object.

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,                                                // Shader attribute location.
        3,                                                // X, Y, Z.
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(0)
    );
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);                              // Correctly hide geometry behind closer surfaces.
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);            // Assignment requirement: wireframe display.
    glLineWidth(1.0f);                                   // 1.0 is portable across OpenGL implementations.

    const GLint mvpLocation = glGetUniformLocation(shaderProgram, "uMVP");
    if (mvpLocation < 0)
    {
        std::cerr << "Could not find shader uniform uMVP.\n";
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glDeleteProgram(shaderProgram);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::cout << "\n============================================================\n";
    std::cout << "COMP 371 - Assignment 3 - Chair\n";
    std::cout << "Reema Aboudraz  - 40253549\n";
    std::cout << "Wissem Oumsalem - 40291712\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Loaded OBJ: " << objPath << "\n\n";
    std::cout << "W / S : translate up / down\n";
    std::cout << "A / D : translate left / right\n";
    std::cout << "Q / E : rotate +30 / -30 degrees around Z\n";
    std::cout << "R / F : scale up / down\n";
    std::cout << "ESC   : quit\n";
    std::cout << "============================================================\n\n";

    while (!glfwWindowShouldClose(window))
    {
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        // A minimized window can temporarily have a zero-sized framebuffer.
        // Avoid division by zero in the perspective aspect ratio.
        if (framebufferWidth <= 0 || framebufferHeight <= 0)
        {
            glfwPollEvents();
            continue;
        }

        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glClearColor(0.07f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Model = Translation * Rotation * Scale.
        // Because normalizeModel() first centered the geometry at the origin,
        // rotation and scaling occur around the chair itself.  Translation is
        // then preserved, so a rotated chair stays at its current screen place.
        glm::mat4 model(1.0f);
        model = glm::translate(model, gTranslation);
        model = glm::rotate(
            model,
            glm::radians(gRotationZ),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        model = glm::scale(model, gScale);

        // Three-quarter camera view makes depth of the chair visible.
        const glm::mat4 view = glm::lookAt(
            glm::vec3(4.2f, -6.2f, 3.4f),
            glm::vec3(0.0f, 0.0f, 0.25f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        const float aspect = static_cast<float>(framebufferWidth)
                           / static_cast<float>(framebufferHeight);

        const glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            aspect,
            0.1f,
            100.0f
        );

        const glm::mat4 mvp = projection * view * model;

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));

        glBindVertexArray(vao);
        glDrawArrays(
            GL_TRIANGLES,
            0,
            static_cast<GLsizei>(vertices.size())
        );
        glBindVertexArray(0);

        glfwSwapBuffers(window);                          // Present completed frame.
        glfwPollEvents();                                // Process keyboard/window events.
    }

    // Release GPU and GLFW resources in reverse order of creation.
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
