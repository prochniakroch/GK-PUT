#version 330 core

// Odbieramy dane z Twojego LoadModelOBJ (atrybuty 0 i 1)
layout (location = 0) in vec3 aPos;    
layout (location = 1) in vec3 aNormal; 

// Wysy³amy te dane dalej, do Fragment Shadera
out vec3 FragPos;
out vec3 Normal;

// Macierze, które ju¿ wysy³asz z C++
uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

void main() {
    // Obliczamy fizyczn¹ pozycjê punktu w œwiecie 3D (potrzebne ¿arówce)
    FragPos = vec3(M * vec4(aPos, 1.0));
    
    // Zabezpieczenie wektorów normalnych przed zniekszta³ceniami przy obracaniu wa³u
    Normal = mat3(transpose(inverse(M))) * aNormal;  
    
    // Ostateczna pozycja punktu na ekranie Twojego monitora
    gl_Position = P * V * vec4(FragPos, 1.0);
}