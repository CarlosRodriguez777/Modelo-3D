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

struct Vertex {
    glm::vec3 Position;
    glm::vec2 TexCoords;
    glm::vec3 Normal;
    glm::vec3 Tangent;
};

// Carga una imagen desde disco, crea una textura en OpenGL y genera mipmaps
unsigned int loadTexture(char const* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data) {
        // Detecta si la imagen tiene RGBA o RGB
        GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Repite la textura si las coordenadas UV salen del rango [0, 1] y usa filtrado suave
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

// --- CARGA DE MODELOS (TINYGLTF) ---
bool loadModelGLB(const std::string& path, std::vector<Vertex>& out_vertices, std::vector<unsigned int>& out_indices) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    if (!ret) {
        std::cout << "[ERROR GLB] " << err << std::endl;
        return false;
    }

    // Recorre todas las mallas y primitivas contenidas dentro del archivo GLB
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

            // Leer normales para el cálculo de iluminación
            const float* normBuf = nullptr;
            if (primitive.attributes.count("NORMAL")) {
                const tinygltf::Accessor& normAcc = model.accessors[primitive.attributes.at("NORMAL")];
                const tinygltf::BufferView& normView = model.bufferViews[normAcc.bufferView];
                normBuf = reinterpret_cast<const float*>(&model.buffers[normView.buffer].data[normAcc.byteOffset + normView.byteOffset]);
            }

            // Construye cada struct Vertex y lo agrega al arreglo de salida
            for (size_t i = 0; i < posAccessor.count; i++) {
                Vertex v;
                v.Position = glm::vec3(posBuf[i * 3], posBuf[i * 3 + 1], posBuf[i * 3 + 2]);
                v.TexCoords = texBuf ? glm::vec2(texBuf[i * 2], texBuf[i * 2 + 1]) : glm::vec2(0.0f, 0.0f);
                v.Normal = normBuf ? glm::vec3(normBuf[i * 3], normBuf[i * 3 + 1], normBuf[i * 3 + 2]) : glm::vec3(0.0f, 1.0f, 0.0f);
                v.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                out_vertices.push_back(v);
            }
        }
    }
    return true;
}

// Calcula una curva de Bézier (entre 0.0 y 1.0)
glm::vec3 calcularBezier(glm::vec3 P0, glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, float t) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    glm::vec3 p = uuu * P0;         
    p += 3 * uu * t * P1;          
    p += 3 * u * tt * P2;           
    p += ttt * P3;                

    return p;
}

int main() {
    // Inicialización de GLFW 
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200,1200, "Proyecto 3D ", NULL, NULL);
    if (!window) { 
        glfwTerminate(); 
        return -1; }
    glfwMakeContextCurrent(window);

    // Carga las funciones de OpenGL y define el área de dibujo
    gladLoadGL();
    glViewport(0, 0, 1200, 1200);
    glEnable(GL_DEPTH_TEST);

    // COMPILACIÓN DE SHADERS //

    Shader ourShader("default.vert", "default.frag");
    Shader lightShader("light.vert", "light.frag");

    // Vértices de un cubo unitario para representar visualmente
    float lightVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f
    };

    GLuint lightVAO, lightVBO;
    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &lightVBO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lightVertices), lightVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // --- CARGA DE MODELOS 3D ---
    // Modelo 1: Objeto 
    std::vector<Vertex> vertices1;
    std::vector<unsigned int> indices1;
    if (!loadModelGLB("modelo.glb", vertices1, indices1)) { 
        return -1; }

    // Modelo 2: Escenario
    std::vector<Vertex> vertices2;
    std::vector<unsigned int> indices2;
    if (!loadModelGLB("escenario.glb", vertices2, indices2)) { 
        return -1; }

    // --- CONFIGURACIÓN DE BUFFERS (VAO/VBO) ---
    // Buffers Modelo 1
    GLuint VAO1, VBO1, EBO1;
    glGenVertexArrays(1, &VAO1); 
    glGenBuffers(1, &VBO1);
    glGenBuffers(1, &EBO1);
    glBindVertexArray(VAO1);
    glBindBuffer(GL_ARRAY_BUFFER, VBO1); 
    glBufferData(GL_ARRAY_BUFFER, vertices1.size() * sizeof(Vertex), &vertices1[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO1);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices1.size() * sizeof(unsigned int), &indices1[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

    // Buffers Modelo 2
    GLuint VAO2, VBO2, EBO2;
    glGenVertexArrays(1, &VAO2); 
    glGenBuffers(1, &VBO2);
    glGenBuffers(1, &EBO2);
    glBindVertexArray(VAO2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO2); 
    glBufferData(GL_ARRAY_BUFFER, vertices2.size() * sizeof(Vertex), &vertices2[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices2.size() * sizeof(unsigned int), &indices2[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

    // --- TEXTURAS, LUZ Y MATRICES ---
    stbi_set_flip_vertically_on_load(true);

    // 1. Texturas del Personaje
    unsigned int texturaPersonajeColor = loadTexture("personaje_BaseColor.png"); 
    unsigned int texturaPersonajeNormal = loadTexture("personaje_Normal.png");   

    // 2. Texturas del Escenario
    unsigned int texturaEscenarioColor = loadTexture("escenario_BaseColor.jpeg");
    unsigned int texturaEscenarioNormal = loadTexture("escenario_Normal.jpeg");

    
    glm::vec3 lightPos(1.2f,-0.4f, 0.0f);
    glm::vec3 lightColor(1.0f, 0.98f, 0.9f);
    float lightIntensity = 1.0f;
    float lightIntensitySpeed = 1.5f;

    ourShader.use();
    ourShader.setInt("texturaDifusa", 0);
    ourShader.setInt("texturaNormal", 1);
    ourShader.setVec3("lightPos", lightPos);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1200.0f / 1200.0f, 0.1f, 100.0f);
    ourShader.setMat4("projection", projection);

    // Variables de tiempo y trayectoria de cámara por curva de Bézier
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    float bezierTime = 0.0f;          
    float bezierSpeed = 0.1f;          

    glm::vec3 P0 = glm::vec3(5.0f, -10.0f, 5.0f);
    glm::vec3 P1 = glm::vec3(5.0f, -10.0f, 5.0f);
    glm::vec3 P2 = glm::vec3(2.0f, -5.0f, 0.0f);
    glm::vec3 P3 = glm::vec3(0.0f, -6.0f, 0.0f);

    // --- BUCLE PRINCIPAL ---
    while (!glfwWindowShouldClose(window)) {
        // Calcula el tiempo transcurrido
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            lightIntensity += lightIntensitySpeed * deltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            lightIntensity -= lightIntensitySpeed * deltaTime;
        }
        lightIntensity = glm::clamp(lightIntensity, 0.0f, 5.0f);

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 activeLightColor = lightColor * lightIntensity;

        ourShader.use();
        ourShader.setVec3("lightColor", activeLightColor);

        // Matriz de Cámara
        // EL TIEMPO DE LA CURVA 
        bezierTime += bezierSpeed * deltaTime;

        if (bezierTime > 1.0f) {
            bezierTime = 0.0f;
        }
        // CALCULAR POSICIÓN CON BÉZIER 
        glm::vec3 cameraPos = calcularBezier(P0, P1, P2, P3, bezierTime);

        // MATRIZ DE VISTA 
        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        ourShader.setMat4("view", view);
        ourShader.setVec3("viewPos", cameraPos);

        // --- RENDERIZAR MODELO 1 (Personaje)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturaPersonajeColor);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texturaPersonajeNormal);

        glBindVertexArray(VAO1);
        glm::mat4 model1 = glm::mat4(1.0f);
        model1 = glm::translate(model1, glm::vec3(-0.5f, 0.0f, 0.0f));
        model1 = glm::rotate(model1, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
        model2 = glm::translate(model2, glm::vec3(-1.5f, 0.0f, 0.0f));
        model2 = glm::rotate(model2, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model2 = glm::scale(model2, glm::vec3(3.0f));
        ourShader.setMat4("model", model2);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices2.size(), GL_UNSIGNED_INT, 0);

        // Renderiza el cubo pequeño
        lightShader.use();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view); 
        lightShader.setVec3("lightColor", lightColor);

        glm::mat4 modelLight = glm::mat4(1.0f);
        modelLight = glm::translate(modelLight, lightPos);
        modelLight = glm::scale(modelLight, glm::vec3(0.05f)); 
        lightShader.setMat4("model", modelLight);

        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // LIMPIEZA 
    glDeleteVertexArrays(1, &VAO1); glDeleteBuffers(1, &VBO1); glDeleteBuffers(1, &EBO1);
    glDeleteVertexArrays(1, &VAO2); glDeleteBuffers(1, &VBO2); glDeleteBuffers(1, &EBO2);
    glDeleteProgram(ourShader.ID);
    glDeleteVertexArrays(1, &lightVAO); glDeleteBuffers(1, &lightVBO); glDeleteProgram(lightShader.ID);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}