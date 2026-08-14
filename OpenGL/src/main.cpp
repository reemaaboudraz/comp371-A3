// COMP 371 - Computer Graphics
// Assignment 3 - Summer 2026
//
// Team Members:
// Reema Aboudraz  - 40253549
// Wissem Oumsalem - 40291712

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>


// Window size
const int WIDTH = 800;
const int HEIGHT = 600;


// Transformation values
float positionX = 0.0f;
float positionY = 0.0f;

float rotationZ = 0.0f;

float scaleValue = 1.0f;


// Same transformation amounts as Assignment 2
const float MOVE_DISTANCE = 0.2f;
const float ROTATION_ANGLE = 30.0f;
const float SCALE_AMOUNT = 0.1f;


// Stores one vertex position
struct Vertex
{
    float x;
    float y;
    float z;
};



//vertex shader
const char* vertexShaderSource = R"glsl(

    #version 330 core

    layout(location = 0) in vec3 aPos;

    uniform mat4 MVP;

    void main()
    {
        gl_Position = MVP * vec4(aPos, 1.0);
    }

)glsl";


//fragment shader
const char* fragmentShaderSource = R"glsl(

    #version 330 core

    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }

)glsl";



//create and compile one shader
GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        std::cout << "Error compiling shader." << std::endl;
    }

    return shader;
}


//create shader program
GLuint createShaderProgram()
{
    GLuint vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexShaderSource);

    GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}



// Get the vertex number from an obj face value
int getVertexIndex(const std::string& value)
{
    std::size_t slashPosition = value.find('/');

    std::string number;

    if (slashPosition == std::string::npos)
    {
        number = value;
    }
    else
    {
        number = value.substr(0, slashPosition);
    }

    return std::stoi(number) - 1;
}


//load vertices and triangular faces from the obj file
bool loadOBJ(
    const std::string& filename,
    std::vector<Vertex>& vertices)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cout << "Could not open " << filename << std::endl;
        return false;
    }


    //vertices read directly from the OBJ
    std::vector<glm::vec3> positions;
    std::string line;

    while (std::getline(file, line))
    {
        std::stringstream stream(line);
        std::string type;
        stream >> type;


        //vertex position
        if (type == "v")
        {
            glm::vec3 position;

            stream >> position.x
                   >> position.y
                   >> position.z;

            positions.push_back(position);
        }


        //triangle face
        else if (type == "f")
        {
            std::string v1;
            std::string v2;
            std::string v3;

            stream >> v1 >> v2 >> v3;

            int index1 = getVertexIndex(v1);
            int index2 = getVertexIndex(v2);
            int index3 = getVertexIndex(v3);

            glm::vec3 p1 = positions[index1];
            glm::vec3 p2 = positions[index2];
            glm::vec3 p3 = positions[index3];

            vertices.push_back({p1.x, p1.y, p1.z});
            vertices.push_back({p2.x, p2.y, p2.z});
            vertices.push_back({p3.x, p3.y, p3.z});
        }
    }


    file.close();

    return true;
}



//center the model at the origin.
void centerModel(std::vector<Vertex>& vertices)
{
    if (vertices.empty())
    {
        return;
    }

    float minX = vertices[0].x;
    float maxX = vertices[0].x;

    float minY = vertices[0].y;
    float maxY = vertices[0].y;

    float minZ = vertices[0].z;
    float maxZ = vertices[0].z;


    //find the minimum and maximum coordinates
    for (const Vertex& vertex : vertices)
    {
        if (vertex.x < minX) minX = vertex.x;
        if (vertex.x > maxX) maxX = vertex.x;

        if (vertex.y < minY) minY = vertex.y;
        if (vertex.y > maxY) maxY = vertex.y;

        if (vertex.z < minZ) minZ = vertex.z;
        if (vertex.z > maxZ) maxZ = vertex.z;
    }


    //find the center
    float centerX = (minX + maxX) / 2.0f;
    float centerY = (minY + maxY) / 2.0f;
    float centerZ = (minZ + maxZ) / 2.0f;


    //move every vertex so that the model is centered
    for (Vertex& vertex : vertices)
    {
        vertex.x -= centerX;
        vertex.y -= centerY;
        vertex.z -= centerZ;
    }
}


// keyboard controls
void keyCallback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mods)
{
    if (action != GLFW_PRESS)
    {
        return;
    }


    // to close program
    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(window, true);
    }


    // for translation
    else if (key == GLFW_KEY_W)
    {
        positionY += MOVE_DISTANCE;
    }

    else if (key == GLFW_KEY_S)
    {
        positionY -= MOVE_DISTANCE;
    }

    else if (key == GLFW_KEY_A)
    {
        positionX -= MOVE_DISTANCE;
    }

    else if (key == GLFW_KEY_D)
    {
        positionX += MOVE_DISTANCE;
    }


    // for rotation
    else if (key == GLFW_KEY_Q)
    {
        rotationZ += ROTATION_ANGLE;
    }

    else if (key == GLFW_KEY_E)
    {
        rotationZ -= ROTATION_ANGLE;
    }


    // for scaling
    else if (key == GLFW_KEY_R)
    {
        scaleValue += SCALE_AMOUNT;
    }

    else if (key == GLFW_KEY_F)
    {
        scaleValue -= SCALE_AMOUNT;

        if (scaleValue < 0.1f)
        {
            scaleValue = 0.1f;
        }
    }
}


// Main ------------------------------------
int main()
{
    //initialize GLFW
    if (!glfwInit())
    {
        std::cout << "Could not initialize GLFW." << std::endl;
        return -1;
    }


    //OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    //mac wissem
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


    //create window
    GLFWwindow* window =
        glfwCreateWindow(
            WIDTH,
            HEIGHT,
            "COMP 371 - Assignment 3",
            NULL,
            NULL);


    if (window == NULL)
    {
        std::cout << "Could not create window." << std::endl;

        glfwTerminate();

        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);


    //initialize GLEW
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK)
    {
        std::cout << "Could not initialize GLEW." << std::endl;

        glfwTerminate();

        return -1;
    }


    //load chair OBJ
    std::vector<Vertex> vertices;

    if (!loadOBJ("chair.obj", vertices))
    {
        glfwTerminate();

        return -1;
    }


    //center chair so that rotation happens around itself
    centerModel(vertices);


    //VAO and VBO

    GLuint VAO;
    GLuint VBO;


    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);


    glBindVertexArray(VAO);


    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW);


    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)0);


    glEnableVertexAttribArray(0);


    //shaders

    GLuint shaderProgram = createShaderProgram();

    GLint MVPLocation =
        glGetUniformLocation(shaderProgram, "MVP");


    //enable the depth testing
    glEnable(GL_DEPTH_TEST);


    //display triangles as wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    // Main rendering loop
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(
            0.1f,
            0.1f,
            0.1f,
            1.0f);


        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);


        //Model matrix

        glm::mat4 model =
            glm::mat4(1.0f);


        //Translation
        model = glm::translate(
            model,
            glm::vec3(
                positionX,
                positionY,
                0.0f));


        //Rotation
        model = glm::rotate(
            model,
            glm::radians(rotationZ),
            glm::vec3(
                0.0f,
                0.0f,
                1.0f));


        //Scaling
        model = glm::scale(
            model,
            glm::vec3(
                scaleValue,
                scaleValue,
                scaleValue));


        // Camera
        glm::mat4 view =
            glm::lookAt(
                glm::vec3(
                    4.0f,
                    -6.0f,
                    3.0f),

                glm::vec3(
                    0.0f,
                    0.0f,
                    0.0f),

                glm::vec3(
                    0.0f,
                    0.0f,
                    1.0f));


        //Perspective projection
        glm::mat4 projection =
            glm::perspective(
                glm::radians(45.0f),
                (float)WIDTH / (float)HEIGHT,
                0.1f,
                100.0f);


        // Complete transformation
        glm::mat4 MVP =
            projection * view * model;


        // Draw chair
        glUseProgram(shaderProgram);


        glUniformMatrix4fv(
            MVPLocation,
            1,
            GL_FALSE,
            glm::value_ptr(MVP));


        glBindVertexArray(VAO);


        glDrawArrays(
            GL_TRIANGLES,
            0,
            vertices.size());


        //Display frame
        glfwSwapBuffers(window);

        glfwPollEvents();
    }


    //Cleanup

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderProgram);


    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}