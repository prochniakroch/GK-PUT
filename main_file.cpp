/*
Niniejszy program jest wolnym oprogramowaniem; możesz go
rozprowadzać dalej i / lub modyfikować na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundację Wolnego
Oprogramowania - według wersji 2 tej Licencji lub(według twojego
wyboru) którejś z późniejszych wersji.

Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
ZASTOSOWAŃ.W celu uzyskania bliższych informacji sięgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"
#include "allmodels.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include <vector>

// To makro jest wymagane, aby tinyobjloader wygenerował swój kod
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// Zamiast luźnych zmiennych, robimy "pudełko" na dane modelu
struct ModelData {
	GLuint vao;
	GLuint vbo;
	int vertexCount;
};

// Tworzymy 4 osobne "pudełka" dla 4 części silnika
ModelData tlok, korbowod, wal, zawors, zaworw;

ModelData LoadModelOBJ(const char* path) {
	ModelData data = { 0, 0, 0 }; // Puste pudełko na start

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path);
	if (!ret) {
		printf("Blad wczytywania modelu %s: %s\n", path, err.c_str());
		return data; // Zwróć puste, jeśli się nie udało
	}

	std::vector<float> vertexData;

	for (size_t s = 0; s < shapes.size(); s++) {
		for (size_t i = 0; i < shapes[s].mesh.indices.size(); i++) {
			tinyobj::index_t idx = shapes[s].mesh.indices[i];

			// Pozycje XYZ
			vertexData.push_back(attrib.vertices[3 * size_t(idx.vertex_index) + 0]);
			vertexData.push_back(attrib.vertices[3 * size_t(idx.vertex_index) + 1]);
			vertexData.push_back(attrib.vertices[3 * size_t(idx.vertex_index) + 2]);

			// Normale
			if (idx.normal_index >= 0) {
				vertexData.push_back(attrib.normals[3 * size_t(idx.normal_index) + 0]);
				vertexData.push_back(attrib.normals[3 * size_t(idx.normal_index) + 1]);
				vertexData.push_back(attrib.normals[3 * size_t(idx.normal_index) + 2]);
			}
			else {
				vertexData.push_back(0.0f); vertexData.push_back(1.0f); vertexData.push_back(0.0f);
			}
		}
	}

	data.vertexCount = vertexData.size() / 6;

	glGenVertexArrays(1, &data.vao);
	glGenBuffers(1, &data.vbo);

	glBindVertexArray(data.vao);
	glBindBuffer(GL_ARRAY_BUFFER, data.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	return data; // Zwracamy gotowe bufory z karty graficznej
}

//Procedura obsługi błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}


//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
	initShaders();

	// WŁĄCZAMY TEST GŁĘBOKOŚCI (To absolutnie kluczowe w 3D!)
	glEnable(GL_DEPTH_TEST);

	// Wczytujemy pliki z dysku i zapisujemy ich dane do kart pamięci VRAM
	tlok = LoadModelOBJ("tlok.obj");
	wal = LoadModelOBJ("wal.obj");
	korbowod = LoadModelOBJ("korbowod.obj");
	zawors = LoadModelOBJ("zawors.obj");
	zaworw = LoadModelOBJ("zaworw.obj");
}


//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
    freeShaders();
    //************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************	
}



//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window) {
	// 1. Czyszczenie ekranu i bufora głębokości (teraz będzie działać poprawnie)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 2. Uruchomienie shadera
	spColored->use();

	// 3. Kamera 
	glm::mat4 V = glm::lookAt(
		glm::vec3(0.0f, 5.0f, 15.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	glm::mat4 P = glm::perspective(glm::radians(50.0f), 1.0f, 0.1f, 100.0f);

	glUniformMatrix4fv(spColored->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(spColored->u("P"), 1, false, glm::value_ptr(P));

	// 4. Macierz modelu
	glm::mat4 M = glm::mat4(1.0f);
	M = glm::scale(M, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(M));

	// WYMUSZAMY RYSOWANIE SIATKI (WIREFRAME) DLA WSZYSTKICH OBIEKTÓW
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// 5. WŁAŚCIWE RYSOWANIE
	glBindVertexArray(tlok.vao);
	glDrawArrays(GL_TRIANGLES, 0, tlok.vertexCount);

	glBindVertexArray(korbowod.vao);
	glDrawArrays(GL_TRIANGLES, 0, korbowod.vertexCount);

	glBindVertexArray(wal.vao);
	glDrawArrays(GL_TRIANGLES, 0, wal.vertexCount);

	glBindVertexArray(zawors.vao);
	glDrawArrays(GL_TRIANGLES, 0, zawors.vertexCount);

	glBindVertexArray(zaworw.vao);
	glDrawArrays(GL_TRIANGLES, 0, zaworw.vertexCount);

	glBindVertexArray(0);

	// PRZYWRACAMY RYSOWANIE SOLIDNE (dobra praktyka, by nie psuć innych części programu)
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// 6. Przerzucenie buforów
	glfwSwapBuffers(window);
}

int main(void)
{
	GLFWwindow* window; //Wskaźnik na obiekt reprezentujący okno

	glfwSetErrorCallback(error_callback);//Zarejestruj procedurę obsługi błędów

	if (!glfwInit()) { //Zainicjuj bibliotekę GLFW
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(500, 500, "OpenGL", NULL, NULL);  //Utwórz okno 500x500 o tytule "OpenGL" i kontekst OpenGL.

	if (!window) //Jeżeli okna nie udało się utworzyć, to zamknij program
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Od tego momentu kontekst okna staje się aktywny i polecenia OpenGL będą dotyczyć właśnie jego.
	glfwSwapInterval(1); //Czekaj na 1 powrót plamki przed pokazaniem ukrytego bufora

	if (glewInit() != GLEW_OK) { //Zainicjuj bibliotekę GLEW
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Operacje inicjujące

	//Główna pętla	
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamknięte
	{		
		drawScene(window); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}
