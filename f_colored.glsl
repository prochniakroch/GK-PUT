#version 330 core

out vec4 FragColor;

// Dane odebrane z Vertex Shadera
in vec3 FragPos;
in vec3 Normal;

// Zmienne, które za chwilę wyślesz z kodu C++
uniform vec3 lightPos;    // Pozycja wirtualnej żarówki
uniform vec3 viewPos;     // Pozycja Twojej kamery
uniform vec3 objectColor; // Kolor metalu (np. szary)

void main() {
    // 1. Światło otoczenia (żeby w cieniu nie było smoliście czarno)
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0); // Białe światło zewsząd
  	
    // 2. Światło rozproszone (im bardziej ściana odwrócona do żarówki, tym jaśniejsza)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    
    // 3. Połysk (Odblask, który robi "efekt metalu")
    float specularStrength = 0.8; 
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); // 32 to skupienie odblasku
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);  
        
    // Mieszamy całe światło i nakładamy na kolor silnika
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}