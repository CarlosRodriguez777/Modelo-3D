#version 330 core
out vec4 FragColor;

uniform vec3 lightColor;

void main() {
    // Color blanco brillante puro
    FragColor = vec4(lightColor, 1.0);
}