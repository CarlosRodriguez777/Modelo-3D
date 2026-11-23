#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 FragPos;
in mat3 TBN;

uniform sampler2D texturaDifusa;
uniform sampler2D texturaNormal;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main() {
    vec3 normal = texture(texturaNormal, TexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(TBN * normal);

    vec3 color = texture(texturaDifusa, TexCoord).rgb;

    float ambientStrength = 0.45;
    vec3 ambient = ambientStrength * lightColor * color;

    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor * color;

    float specularStrength = 0.6; 
    int shininess = 32;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}