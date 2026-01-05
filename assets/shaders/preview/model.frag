#version 430 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;

out vec4 FragColor;

void main() {
    vec3 baseColor = vec3(0.7, 0.7, 0.75);
    
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(lightDir);
    
    float ambient = 0.3;
    float diff = max(dot(norm, lightDirection), 0.0);
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    
    vec3 result = (ambient + diff + spec * 0.5) * baseColor * lightColor;
    
    FragColor = vec4(result, 1.0);
}
