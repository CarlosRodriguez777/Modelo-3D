# Proyecto 3D en OpenGL

Nombre: Carlos Andres Rodriguez Diaz
Código: 6000748

Video Youtube: https://youtu.be/eCXgKtEkYuo

1. **Estructuras y utilidades de carga**
   * `Vertex`: define los atributos por vértice enviados a la GPU (`Position`, `TexCoords`, `Normal`, `Tangent`).
   * `loadTexture`: lee imágenes con `stb_image`, crea texturas en OpenGL, genera *mipmaps* y configura repetición y filtrado.
   * `loadModelGLB`: lee archivos binarios `.glb` usando `tinygltf` y extrae índices, posiciones, coordenadas UV y normales.
   * `calcularBezier`: evalúa una curva de Bézier cúbica con cuatro puntos de control (`P0`, `P1`, `P2`, `P3`) y el parámetro `t` para desplazar la cámara.

2. **Inicialización y buffers de GPU (`main`)**
   * Configura GLFW y GLAD con contexto OpenGL 3.3 Core y activa `GL_DEPTH_TEST`.
   * Compila dos programas de *shaders*: `ourShader` (`default.vert` / `default.frag`) para los modelos texturizados e iluminados, y `lightShader` (`light.vert` / `light.frag`) para el cubo que representa la luz.
   * Crea y enlaza `VAO`, `VBO` y `EBO` para el personaje (`modelo.glb`), el escenario (`escenario.glb`) y el cubo de luz.
   * Configura la matriz de proyección en perspectiva y carga las texturas difusas y de normales.

3. **Bucle principal de renderizado (`while (!glfwWindowShouldClose(window))`)**
   * Calcula `deltaTime` para que animaciones y controles sean independientes de los FPS.
   * Lee la entrada de teclado (`GLFW_KEY_UP` y `GLFW_KEY_DOWN`) para ajustar `lightIntensity` entre `0.0` y `5.0`.
   * Actualiza `bezierTime` y calcula la posición de la cámara y la matriz `view` (`glm::lookAt`).
   * Dibuja personaje, escenario y cubo de luz en cada cuadro, intercambia búferes (`glfwSwapBuffers`) y procesa eventos (`glfwPollEvents`).

4. **Limpieza de memoria**
   * Libera buffers, *VAOs*, programas de *shaders*, ventana y contexto de GLFW al cerrar la aplicación.

archivos del proyecto
├── main.cpp                  # Código principal, carga de modelos/texturas y bucle de render
├── Shader.h                  # Clase auxiliar para cargar y compilar shaders
├── tiny_gltf.h               # Librería para cargar archivos GLB/GLTF
├── stb_image.h               # Librería para cargar imágenes (incluida vía tinygltf/stb)
├── default.vert              # Vertex shader de los modelos
├── default.frag              # Fragment shader (texturas + iluminación)
├── light.vert                # Vertex shader del cubo de luz
├── light.frag                # Fragment shader del cubo de luz
├── modelo.glb                # Modelo 3D del personaje
├── escenario.glb             # Modelo 3D del escenario
├── personaje_BaseColor.png   # Textura de color del personaje
├── personaje_Normal.png      # Mapa de normales del personaje
├── escenario_BaseColor.jpeg  # Textura de color del escenario
└── escenario_Normal.jpeg     # Mapa de normales del escenario# YoutubeOpenGL