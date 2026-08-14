/**
 * Reema Aboudraz - 40253549
 * Wissem Oumsalem - 40291712  
 * Assignment 3, COMP 371 
 * Summer 2026
 * Professor Nagi Basha
 */

#include <GL/glew.h>              // Loads modern OpenGL function pointers.
#include <GLFW/glfw3.h>            // Creates the window and handles keyboard input.
#include <glm/glm.hpp>             // Provides vectors and matrices used by OpenGL math.
#include <glm/gtc/matrix_transform.hpp> // Provides translate, rotate, scale, lookAt, perspective.
#include <glm/gtc/type_ptr.hpp>    // Converts GLM matrices to raw pointers for glUniformMatrix4fv.

#include <algorithm>               // Provides std::min and std::max.
#include <cstddef>                 // Provides standard size-related types.
#include <fstream>                 // Reads the OBJ file from disk.
#include <iostream>                // Prints messages and errors to the console.
#include <sstream>                 // Parses each OBJ text line.
#include <string>                  // Stores file paths and shader source text.
#include <vector>                  // Stores vertex data dynamically.

// -----------------------------------------------------------------------------
// Window constants.
// -----------------------------------------------------------------------------
constexpr int WINDOW_WIDTH = 1000;  // Initial width of the OpenGL window.
constexpr int WINDOW_HEIGHT = 800;  // Initial height of the OpenGL window.

// -----------------------------------------------------------------------------
// Transformation state.
// These variables store the user's current transformation of the chair.
// -----------------------------------------------------------------------------
glm::vec3 gTranslation(0.0f, 0.0f, 0.0f); // Current X/Y/Z movement of the chair.
glm::vec3 gScale(1.0f, 1.0f, 1.0f);       // Current X/Y/Z scale of the chair.
float gRotationZ = 0.0f;                    // Current rotation angle around the Z axis, in degrees.

// Increment values used whenever a keyboard key is pressed.
constexpr float TRANSLATION_STEP = 0.12f;   // Amount of movement per key press.
constexpr float ROTATION_STEP = 30.0f;      // Assignment-style 30-degree rotation step.
constexpr float SCALE_STEP = 0.10f;         // Amount added/subtracted from scale per key press.
constexpr float MIN_SCALE = 0.10f;          // Prevents the model from shrinking to zero/negative size.

// -----------------------------------------------------------------------------
// Small data structure representing one 3D position.
// The assignment only needs wireframe geometry, so positions are sufficient.
// -----------------------------------------------------------------------------
struct Vertex
{
    float x; // X coordinate of the vertex.
    float y; // Y coordinate of the vertex.
    float z; // Z coordinate of the vertex.
};

// -----------------------------------------------------------------------------
// Compile one shader stage and return its OpenGL object ID.
// -----------------------------------------------------------------------------
GLuint compileShader(GLenum shaderType, const char* source)
{
    GLuint shader = glCreateShader(shaderType); // Ask OpenGL to create a shader object.
    glShaderSource(shader, 1, &source, nullptr); // Attach the GLSL source code to the shader object.
    glCompileShader(shader);                     // Compile the GLSL code on the GPU driver.

    GLint success = GL_FALSE;                    // Will become GL_TRUE if compilation succeeds.
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); // Query the shader compilation result.

    if (success != GL_TRUE)                      // If compilation failed, print the full driver error.
    {
        GLint logLength = 0;                     // Stores how many characters are in the error log.
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength); // Ask OpenGL for log length.
        std::vector<char> log(static_cast<std::size_t>(logLength)); // Allocate enough memory for log.
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());  // Copy the error log into memory.
        std::cerr << "Shader compilation failed:\n" << log.data() << '\n'; // Print the error.
        glDeleteShader(shader);                   // Free the failed shader object.
        return 0;                                 // Return 0 so the caller knows compilation failed.
    }

    return shader;                                // Return the successfully compiled shader ID.
}

// -----------------------------------------------------------------------------
// Build a complete shader program from a vertex and fragment shader.
// -----------------------------------------------------------------------------
GLuint createShaderProgram()
{
    // The vertex shader receives one 3D position and transforms it into clip space.
    const char* vertexShaderSource = R"GLSL(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        uniform mat4 uMVP;

        void main()
        {
            gl_Position = uMVP * vec4(aPosition, 1.0);
        }
    )GLSL";

    // The fragment shader outputs a constant light color for every wireframe line.
    const char* fragmentShaderSource = R"GLSL(
        #version 330 core
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(0.92, 0.92, 0.92, 1.0);
        }
    )GLSL";

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);   // Compile vertex stage.
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource); // Compile fragment stage.

    if (vertexShader == 0 || fragmentShader == 0) // Stop if either shader failed to compile.
    {
        if (vertexShader != 0) glDeleteShader(vertexShader); // Clean up any successful partial object.
        if (fragmentShader != 0) glDeleteShader(fragmentShader); // Clean up any successful partial object.
        return 0;                                  // Signal failure to the caller.
    }

    GLuint program = glCreateProgram();            // Create the final GPU shader program object.
    glAttachShader(program, vertexShader);          // Attach the compiled vertex shader.
    glAttachShader(program, fragmentShader);        // Attach the compiled fragment shader.
    glLinkProgram(program);                         // Link both shader stages into one executable program.

    GLint success = GL_FALSE;                       // Will become true if linking succeeds.
    glGetProgramiv(program, GL_LINK_STATUS, &success); // Query the link result.

    if (success != GL_TRUE)                         // Print a readable error if linking failed.
    {
        GLint logLength = 0;                        // Stores the number of characters in the link log.
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength); // Request the log length.
        std::vector<char> log(static_cast<std::size_t>(logLength)); // Allocate storage for the log.
        glGetProgramInfoLog(program, logLength, nullptr, log.data()); // Read the program link log.
        std::cerr << "Shader program linking failed:\n" << log.data() << '\n'; // Display the log.
        glDeleteProgram(program);                   // Delete the invalid program.
        program = 0;                                // Mark it as invalid.
    }

    glDeleteShader(vertexShader);                   // Individual shader objects are no longer needed.
    glDeleteShader(fragmentShader);                 // The linked program keeps their compiled code.
    return program;                                 // Return the completed OpenGL program ID.
}

// -----------------------------------------------------------------------------
// Parse one OBJ vertex index token.
// It accepts forms such as "12", "12/4", or "12/4/9" and returns the vertex index.
// -----------------------------------------------------------------------------
int parseVertexIndex(const std::string& token, int vertexCount)
{
    const std::size_t slashPosition = token.find('/'); // Find where texture/normal indices begin.
    const std::string indexText = token.substr(0, slashPosition); // Keep only the position index text.
    int index = std::stoi(indexText);                   // Convert the OBJ index from text to integer.

    if (index > 0)                                      // Positive OBJ indices start at 1.
    {
        return index - 1;                               // Convert from OBJ's 1-based indexing to C++ 0-based.
    }

    // Negative OBJ indices are relative to the end of the current vertex list.
    return vertexCount + index;                         // Example: -1 refers to the newest vertex.
}

// -----------------------------------------------------------------------------
// Load an OBJ file and expand every face into triangles.
// This loader is intentionally small and self-contained so no extra OBJ library is required.
// -----------------------------------------------------------------------------
bool loadOBJ(const std::string& path, std::vector<Vertex>& triangleVertices)
{
    std::ifstream file(path);                           // Open the OBJ file for reading.
    if (!file.is_open())                                // If the path is wrong, report it clearly.
    {
        std::cerr << "Could not open OBJ file: " << path << '\n'; // Print the missing file path.
        return false;                                   // Tell main() loading failed.
    }

    std::vector<glm::vec3> positions;                   // Stores all raw 'v' positions from the OBJ.
    std::string line;                                   // Holds one text line at a time.

    while (std::getline(file, line))                    // Continue until every OBJ line is processed.
    {
        if (line.empty() || line[0] == '#')             // Ignore blank lines and comments.
        {
            continue;                                   // Move directly to the next line.
        }

        std::istringstream stream(line);                // Allows convenient token-by-token parsing.
        std::string prefix;                             // Stores the first token, such as v or f.
        stream >> prefix;                               // Read the OBJ command prefix.

        if (prefix == "v")                             // A 'v' line describes one vertex position.
        {
            glm::vec3 position;                         // Temporary vector for x, y, and z.
            stream >> position.x >> position.y >> position.z; // Read all three coordinates.
            positions.push_back(position);              // Append this position to the position table.
        }
        else if (prefix == "f")                        // An 'f' line describes one polygonal face.
        {
            std::vector<int> faceIndices;               // Stores every position index used by this face.
            std::string token;                          // Holds one face element token at a time.

            while (stream >> token)                     // Read all vertices on this face.
            {
                int index = parseVertexIndex(token, static_cast<int>(positions.size())); // Resolve OBJ index.
                if (index < 0 || index >= static_cast<int>(positions.size())) // Validate index range.
                {
                    std::cerr << "Invalid face index found in OBJ file.\n"; // Explain malformed data.
                    return false;                       // Stop rather than reading invalid memory.
                }
                faceIndices.push_back(index);           // Save the valid vertex index.
            }

            if (faceIndices.size() < 3)                 // A valid polygon must contain at least 3 vertices.
            {
                continue;                               // Ignore incomplete faces safely.
            }

            // Triangulate any polygon using a triangle fan:
            // (0,1,2), (0,2,3), (0,3,4), ...
            for (std::size_t i = 1; i + 1 < faceIndices.size(); ++i)
            {
                const glm::vec3& a = positions[faceIndices[0]];     // First triangle point.
                const glm::vec3& b = positions[faceIndices[i]];     // Second triangle point.
                const glm::vec3& c = positions[faceIndices[i + 1]]; // Third triangle point.

                triangleVertices.push_back({a.x, a.y, a.z}); // Add triangle vertex A.
                triangleVertices.push_back({b.x, b.y, b.z}); // Add triangle vertex B.
                triangleVertices.push_back({c.x, c.y, c.z}); // Add triangle vertex C.
            }
        }
    }

    if (triangleVertices.empty())                       // A model without triangle data cannot be displayed.
    {
        std::cerr << "OBJ file did not contain any usable faces.\n"; // Explain the issue.
        return false;                                   // Report failure.
    }

    return true;                                        // OBJ loading completed successfully.
}

// -----------------------------------------------------------------------------
// Center the model at the origin and scale it to a predictable size.
// This is essential because OBJ files can use arbitrary coordinates.
// Keeping the chair centered also makes rotation happen "in place".
// -----------------------------------------------------------------------------
void normalizeModel(std::vector<Vertex>& vertices)
{
    glm::vec3 minimum(vertices[0].x, vertices[0].y, vertices[0].z); // Initial bounding-box minimum.
    glm::vec3 maximum = minimum;                                    // Initial bounding-box maximum.

    for (const Vertex& vertex : vertices)                            // Inspect every loaded vertex.
    {
        glm::vec3 p(vertex.x, vertex.y, vertex.z);                   // Convert the Vertex into a GLM vector.
        minimum = glm::min(minimum, p);                              // Update smallest x/y/z values.
        maximum = glm::max(maximum, p);                              // Update largest x/y/z values.
    }

    const glm::vec3 center = (minimum + maximum) * 0.5f;             // Find the geometric center of the box.
    const glm::vec3 size = maximum - minimum;                        // Measure width, depth, and height.
    const float largestDimension = std::max({size.x, size.y, size.z}); // Find the largest model dimension.
    const float normalizationScale = (largestDimension > 0.0f) ? (3.0f / largestDimension) : 1.0f; // Fit to view.

    for (Vertex& vertex : vertices)                                  // Modify every vertex in place.
    {
        glm::vec3 p(vertex.x, vertex.y, vertex.z);                    // Read the current position.
        p = (p - center) * normalizationScale;                        // Center at origin, then resize uniformly.
        vertex.x = p.x;                                              // Store normalized X back into the array.
        vertex.y = p.y;                                              // Store normalized Y back into the array.
        vertex.z = p.z;                                              // Store normalized Z back into the array.
    }
}

// -----------------------------------------------------------------------------
// Keyboard callback.
// The controls intentionally match the Assignment 2 mapping discussed earlier:
// W/S/A/D = translation, Q/E = rotation, R/F = scaling, ESC = quit.
// -----------------------------------------------------------------------------
void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT) // Ignore key-release events.
    {
        return;                                        // Only react to actual key presses/repeats.
    }

    switch (key)                                       // Decide which transformation to update.
    {
        case GLFW_KEY_ESCAPE:                          // ESC closes the application.
            glfwSetWindowShouldClose(window, GLFW_TRUE); // Ask GLFW to end the main loop.
            break;                                     // Stop handling this key.

        case GLFW_KEY_W:                               // W moves the chair upward.
            gTranslation.y += TRANSLATION_STEP;        // Increase Y translation.
            break;                                     // Finish this case.

        case GLFW_KEY_S:                               // S moves the chair downward.
            gTranslation.y -= TRANSLATION_STEP;        // Decrease Y translation.
            break;                                     // Finish this case.

        case GLFW_KEY_A:                               // A moves the chair left.
            gTranslation.x -= TRANSLATION_STEP;        // Decrease X translation.
            break;                                     // Finish this case.

        case GLFW_KEY_D:                               // D moves the chair right.
            gTranslation.x += TRANSLATION_STEP;        // Increase X translation.
            break;                                     // Finish this case.

        case GLFW_KEY_Q:                               // Q rotates counter-clockwise around Z.
            gRotationZ += ROTATION_STEP;               // Add 30 degrees.
            break;                                     // Finish this case.

        case GLFW_KEY_E:                               // E rotates clockwise around Z.
            gRotationZ -= ROTATION_STEP;               // Subtract 30 degrees.
            break;                                     // Finish this case.

        case GLFW_KEY_R:                               // R enlarges the chair uniformly.
            gScale += glm::vec3(SCALE_STEP);           // Increase X, Y, and Z scale together.
            break;                                     // Finish this case.

        case GLFW_KEY_F:                               // F shrinks the chair uniformly.
            gScale -= glm::vec3(SCALE_STEP);           // Decrease X, Y, and Z scale together.
            gScale = glm::max(gScale, glm::vec3(MIN_SCALE)); // Never allow zero/negative scale.
            break;                                     // Finish this case.

        default:                                       // All other keys are ignored.
            break;                                     // No transformation change is needed.
    }
}

// -----------------------------------------------------------------------------
// Program entry point.
// -----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // If the user gives a command-line path, use it; otherwise load chair.obj from the project folder.
    const std::string objPath = (argc > 1) ? argv[1] : "chair.obj";

    if (glfwInit() != GLFW_TRUE)                        // Start the GLFW library.
    {
        std::cerr << "Failed to initialize GLFW.\n"; // Explain startup failure.
        return 1;                                       // Exit with an error code.
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);      // Request OpenGL major version 3.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);      // Request OpenGL minor version 3.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Request modern core OpenGL.

    GLFWwindow* window = glfwCreateWindow(              // Create the actual application window.
        WINDOW_WIDTH,                                   // Set initial window width.
        WINDOW_HEIGHT,                                  // Set initial window height.
        "COMP 371 - Assignment 3 - Chair",              // Set the title shown by the OS.
        nullptr,                                        // Use windowed mode, not fullscreen.
        nullptr                                         // Do not share another OpenGL context.
    );

    if (window == nullptr)                              // If creation failed, clean up GLFW.
    {
        std::cerr << "Failed to create GLFW window.\n"; // Report the error.
        glfwTerminate();                                // Release GLFW resources.
        return 1;                                       // Exit with an error code.
    }

    glfwMakeContextCurrent(window);                     // Make this window's OpenGL context active.
    glfwSetKeyCallback(window, keyCallback);            // Tell GLFW which function handles keyboard input.
    glfwSwapInterval(1);                                // Enable V-sync to avoid unnecessary high frame rates.

    glewExperimental = GL_TRUE;                         // Enables loading core-profile OpenGL functions.
    if (glewInit() != GLEW_OK)                         // Load all OpenGL function pointers through GLEW.
    {
        std::cerr << "Failed to initialize GLEW.\n"; // Report GLEW failure.
        glfwDestroyWindow(window);                      // Destroy the already-created window.
        glfwTerminate();                                // Shut down GLFW.
        return 1;                                       // Exit with an error code.
    }

    // GLEW can produce one harmless GL_INVALID_ENUM on core contexts; clear it before normal rendering.
    glGetError();                                       // Consume that possible initialization error.

    std::vector<Vertex> vertices;                       // Will contain triangles read from the OBJ file.
    if (!loadOBJ(objPath, vertices))                    // Load the chair geometry from disk.
    {
        glfwDestroyWindow(window);                      // Clean up the window on file-loading failure.
        glfwTerminate();                                // Release GLFW resources.
        return 1;                                       // Exit because there is nothing to render.
    }

    normalizeModel(vertices);                           // Center and resize the chair for predictable transforms.

    GLuint shaderProgram = createShaderProgram();       // Compile/link the simple wireframe shaders.
    if (shaderProgram == 0)                             // Stop if shader creation failed.
    {
        glfwDestroyWindow(window);                      // Clean up GLFW window.
        glfwTerminate();                                // Shut down GLFW.
        return 1;                                       // Exit with an error code.
    }

    GLuint vao = 0;                                     // Vertex Array Object remembers vertex-input configuration.
    GLuint vbo = 0;                                     // Vertex Buffer Object stores vertex bytes on the GPU.
    glGenVertexArrays(1, &vao);                         // Allocate one VAO ID.
    glGenBuffers(1, &vbo);                              // Allocate one VBO ID.

    glBindVertexArray(vao);                             // Make this VAO the active configuration container.
    glBindBuffer(GL_ARRAY_BUFFER, vbo);                 // Bind the VBO as the active vertex buffer.
    glBufferData(                                       // Upload all chair vertices into GPU memory.
        GL_ARRAY_BUFFER,                                // The data will be used as per-vertex attributes.
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), // Number of bytes to upload.
        vertices.data(),                                // Pointer to the first CPU-side vertex.
        GL_STATIC_DRAW                                  // The geometry does not change after upload.
    );

    glVertexAttribPointer(                              // Describe how shader location 0 reads each vertex.
        0,                                              // Attribute location used by aPosition.
        3,                                              // Each position contains three floating-point values.
        GL_FLOAT,                                       // The values are 32-bit floats.
        GL_FALSE,                                       // Do not normalize ordinary position coordinates.
        sizeof(Vertex),                                 // Distance in bytes from one vertex to the next.
        reinterpret_cast<void*>(0)                      // Position starts at byte offset 0 in Vertex.
    );
    glEnableVertexAttribArray(0);                       // Turn on position attribute location 0.

    glBindBuffer(GL_ARRAY_BUFFER, 0);                   // Unbind the VBO to avoid accidental changes.
    glBindVertexArray(0);                               // Unbind the VAO after configuration.

    glEnable(GL_DEPTH_TEST);                            // Ensure closer chair lines correctly hide farther lines.
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);          // Required assignment behavior: render polygons as wireframe.
    glLineWidth(1.2f);                                  // Make wireframe edges slightly easier to see.

    const GLint mvpLocation = glGetUniformLocation(shaderProgram, "uMVP"); // Locate the shader's MVP matrix.

    std::cout << "\nCOMP 371 Assignment 3 controls\n"; // Print a quick control reference.
    std::cout << "W/S : move up/down\n";               // Explain vertical translation.
    std::cout << "A/D : move left/right\n";            // Explain horizontal translation.
    std::cout << "Q/E : rotate +/- 30 degrees\n";       // Explain rotation keys.
    std::cout << "R/F : scale up/down\n";              // Explain scaling keys.
    std::cout << "ESC : quit\n\n";                    // Explain how to close the application.

    while (!glfwWindowShouldClose(window))              // Main render loop runs until ESC/window close.
    {
        int framebufferWidth = 0;                       // Stores the real pixel width of the window.
        int framebufferHeight = 0;                      // Stores the real pixel height of the window.
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight); // Query current size.
        glViewport(0, 0, framebufferWidth, framebufferHeight); // Match OpenGL rendering to window size.

        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);        // Set a dark neutral background color.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear previous frame color and depth data.

        glm::mat4 model(1.0f);                          // Start from the identity model matrix.
        model = glm::translate(model, gTranslation);    // Move the chair according to W/S/A/D input.
        model = glm::rotate(                             // Rotate the already-centered chair around its own origin.
            model,                                      // Transform matrix to update.
            glm::radians(gRotationZ),                   // Convert rotation from degrees to radians.
            glm::vec3(0.0f, 0.0f, 1.0f)                // Rotate around the Z axis.
        );
        model = glm::scale(model, gScale);              // Apply uniform scale from R/F input.

        const glm::mat4 view = glm::lookAt(             // Build a camera/view matrix.
            glm::vec3(0.0f, -6.5f, 3.2f),              // Camera position in world coordinates.
            glm::vec3(0.0f, 0.0f, 0.4f),               // Point the camera slightly above model center.
            glm::vec3(0.0f, 0.0f, 1.0f)                // Define positive Z as the world's up direction.
        );

        const float aspectRatio =                       // Prevent division by zero on minimized windows.
            (framebufferHeight > 0)                     // Check for a valid non-zero height.
            ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight) // Normal ratio.
            : 1.0f;                                     // Safe fallback ratio.

        const glm::mat4 projection = glm::perspective(  // Build a realistic perspective projection.
            glm::radians(45.0f),                        // Use a 45-degree vertical field of view.
            aspectRatio,                                // Match the current window shape.
            0.1f,                                       // Near clipping plane.
            100.0f                                      // Far clipping plane.
        );

        const glm::mat4 mvp = projection * view * model; // Combine model, view, and projection transformations.

        glUseProgram(shaderProgram);                    // Activate the shader program for this draw call.
        glUniformMatrix4fv(                              // Send the computed MVP matrix to the vertex shader.
            mvpLocation,                                // Destination uniform variable.
            1,                                          // Upload one matrix.
            GL_FALSE,                                   // GLM already stores matrices in OpenGL-compatible order.
            glm::value_ptr(mvp)                         // Pointer to the first matrix float.
        );

        glBindVertexArray(vao);                         // Activate the chair's vertex-input configuration.
        glDrawArrays(                                   // Draw every uploaded triangle.
            GL_TRIANGLES,                               // Interpret each group of 3 vertices as one triangle.
            0,                                          // Begin with the first vertex.
            static_cast<GLsizei>(vertices.size())       // Draw every vertex loaded from the OBJ.
        );
        glBindVertexArray(0);                           // Unbind after drawing for cleaner state management.

        glfwSwapBuffers(window);                        // Present the finished back buffer on screen.
        glfwPollEvents();                               // Process keyboard, mouse, resize, and OS window events.
    }

    glDeleteBuffers(1, &vbo);                           // Release GPU vertex memory.
    glDeleteVertexArrays(1, &vao);                      // Release the VAO object.
    glDeleteProgram(shaderProgram);                     // Release compiled shader program resources.
    glfwDestroyWindow(window);                          // Destroy the GLFW window and its OpenGL context.
    glfwTerminate();                                    // Shut down the GLFW library completely.
    return 0;                                           // Return success to the operating system.
}
