#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 FragPos;
in mat3 TBN;

uniform sampler2D texturaDifusa;
uniform sampler2D texturaNormal;
uniform vec3 lightPos;

void main() {
    vec3 normal = texture(texturaNormal, TexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(TBN * normal);
    vec3 color = texture(texturaDifusa, TexCoord).rgb;
    vec3 ambient = 0.6 * color;
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color * 1.5;
    FragColor = vec4(ambient + diffuse, 1.0);
}