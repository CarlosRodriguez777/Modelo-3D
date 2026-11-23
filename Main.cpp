#include <iostream>
#include <vector>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE 
#include "tiny_gltf.h"

// --- 1. ESTRUCTURA DE VÉRTICES ---
struct Vertex {
    glm::vec3 Position;
    glm::vec2 TexCoords;
    glm::vec3 Normal;
    glm::vec3 Tangent;
};

// --- 2. CARGA DE TEXTURAS ---
unsigned int loadTexture(char const* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else {
        std::cout << "[ADVERTENCIA] No se encontro la imagen: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

// --- 4. CARGA DE MODELOS (TINYGLTF) ---
bool loadModelGLB(const std::string& path, std::vector<Vertex>& out_vertices, std::vector<unsigned int>& out_indices) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    if (!ret) {
        std::cout << "[ERROR GLB] " << err << std::endl;
        return false;
    }

    for (const auto& mesh : model.meshes) {
        for (const auto& primitive : mesh.primitives) {
            // Leer Índices
            const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
            const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
            int indexByteOffset = indexAccessor.byteOffset + indexBufferView.byteOffset;
            unsigned int vertexOffset = out_vertices.size();

            if (indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT) {
                const uint16_t* buf = reinterpret_cast<const uint16_t*>(&indexBuffer.data[indexByteOffset]);
                for (size_t i = 0; i < indexAccessor.count; i++) out_indices.push_back(buf[i] + vertexOffset);
            }
            else if (indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT) {
                const uint32_t* buf = reinterpret_cast<const uint32_t*>(&indexBuffer.data[indexByteOffset]);
                for (size_t i = 0; i < indexAccessor.count; i++) out_indices.push_back(buf[i] + vertexOffset);
            }

            // Leer Vértices (Posición)
            const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
            const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
            const float* posBuf = reinterpret_cast<const float*>(&model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]);

            // Leer Coordenadas de Textura 
            const float* texBuf = nullptr;
            if (primitive.attributes.count("TEXCOORD_0")) {
                const tinygltf::Accessor& texAcc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const tinygltf::BufferView& texView = model.bufferViews[texAcc.bufferView];
                texBuf = reinterpret_cast<const float*>(&model.buffers[texView.buffer].data[texAcc.byteOffset + texView.byteOffset]);
            }

            for (size_t i = 0; i < posAccessor.count; i++) {
                Vertex v;
                v.Position = glm::vec3(posBuf[i * 3], posBuf[i * 3 + 1], posBuf[i * 3 + 2]);
                v.TexCoords = texBuf ? glm::vec2(texBuf[i * 2], texBuf[i * 2 + 1]) : glm::vec2(0.0f, 0.0f);
                v.Normal = glm::vec3(0.0f, 1.0f, 0.0f); 
                v.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                out_vertices.push_back(v);
            }
        }
    }
    return true;
}

int main() {
    // --- 5. CONFIGURACIÓN GLFW Y OPENGL ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200,1200, "Proyecto 3D ", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glViewport(0, 0, 1200, 1200);
    glEnable(GL_DEPTH_TEST);

    // --- 5. COMPILACIÓN DE SHADERS (DESDE ARCHIVOS) ---

    Shader ourShader("default.vert", "default.frag");

    // --- 7. CARGA DE MODELOS 3D ---
    // Modelo 1: Objeto 
    std::vector<Vertex> vertices1;
    std::vector<unsigned int> indices1;
    if (!loadModelGLB("modelo.glb", vertices1, indices1)) { return -1; }

    // Modelo 2: Escenario
    std::vector<Vertex> vertices2;
    std::vector<unsigned int> indices2;
    if (!loadModelGLB("escenario.glb", vertices2, indices2)) { return -1; }

    // --- 8. CONFIGURACIÓN DE BUFFERS (VAO/VBO) ---
    // Buffers Modelo 1
    GLuint VAO1, VBO1, EBO1;
    glGenVertexArrays(1, &VAO1); glGenBuffers(1, &VBO1); glGenBuffers(1, &EBO1);
    glBindVertexArray(VAO1);
    glBindBuffer(GL_ARRAY_BUFFER, VBO1); glBufferData(GL_ARRAY_BUFFER, vertices1.size() * sizeof(Vertex), &vertices1[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO1); glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices1.size() * sizeof(unsigned int), &indices1[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

    // Buffers Modelo 2
    GLuint VAO2, VBO2, EBO2;
    glGenVertexArrays(1, &VAO2); glGenBuffers(1, &VBO2); glGenBuffers(1, &EBO2);
    glBindVertexArray(VAO2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO2); glBufferData(GL_ARRAY_BUFFER, vertices2.size() * sizeof(Vertex), &vertices2[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2); glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices2.size() * sizeof(unsigned int), &indices2[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

    // --- 9. TEXTURAS, LUZ Y MATRICES ---
    stbi_set_flip_vertically_on_load(true);

    // 1. Texturas del Personaje
    unsigned int texturaPersonajeColor = loadTexture("personaje_BaseColor.png"); 
    unsigned int texturaPersonajeNormal = loadTexture("personaje_Normal.png");   

    // 2. Texturas del Escenario
    unsigned int texturaEscenarioColor = loadTexture("escenario_BaseColor.jpeg");
    unsigned int texturaEscenarioNormal = loadTexture("escenario_Normal.jpeg");

    ourShader.use();
    ourShader.setInt("texturaDifusa", 0);
    ourShader.setInt("texturaNormal", 1);

    glm::vec3 lightPos(2.0f, 5.0f, 4.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

    ourShader.setVec3("lightPos", lightPos);
    ourShader.setVec3("lightColor", lightColor);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 800.0f, 0.1f, 100.0f);
    ourShader.setMat4("projection", projection);

    // --- 10. BUCLE PRINCIPAL ---
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float camAngleX = 0.0f, camAngleY = -5.0f, camRadius = 10.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Controles de cámara orbital
        float cameraSpeed = 2.5f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camAngleX -= cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camAngleX += cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camAngleY += cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camAngleY -= cameraSpeed;
        if (camAngleY > 1.5f) camAngleY = 1.5f;
        if (camAngleY < -1.5f) camAngleY = -1.5f;

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Matriz de Vista (Cámara)
        float camX = sin(camAngleX) * cos(camAngleY) * camRadius;
        float camY = sin(camAngleY) * camRadius;
        float camZ = cos(camAngleX) * cos(camAngleY) * camRadius;
        glm::vec3 cameraPos = glm::vec3(camX, camY, camZ);
        glm::mat4 view = glm::lookAt(glm::vec3(camX, camY, camZ), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("view", view);
        ourShader.setVec3("viewPos", cameraPos);

        // --- RENDERIZAR MODELO 1 (Personaje)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturaPersonajeColor);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texturaPersonajeNormal);

        glBindVertexArray(VAO1);
        glm::mat4 model1 = glm::mat4(1.0f);
        model1 = glm::translate(model1, glm::vec3(0.0f, 0.0f, 0.0f));
        model1 = glm::scale(model1, glm::vec3(0.3f));
        ourShader.setMat4("model", model1);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices1.size(), GL_UNSIGNED_INT, 0);

        // --- RENDERIZAR MODELO 2 (Escenario) 
       
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturaEscenarioColor);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texturaEscenarioNormal);

        glBindVertexArray(VAO2);

        glm::mat4 model2 = glm::mat4(1.0f);

        model2 = glm::translate(model2, glm::vec3(0.0f, 0.0f, -1.0f));
        model2 = glm::scale(model2, glm::vec3(3.0f));
        ourShader.setMat4("model", model2);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices2.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- 11. LIMPIEZA ---
    glDeleteVertexArrays(1, &VAO1); glDeleteBuffers(1, &VBO1); glDeleteBuffers(1, &EBO1);
    glDeleteVertexArrays(1, &VAO2); glDeleteBuffers(1, &VBO2); glDeleteBuffers(1, &EBO2);
    glDeleteProgram(ourShader.ID);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}