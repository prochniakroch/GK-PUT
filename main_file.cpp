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

// Tworzymy 5 osobnych "pudełek" dla 5 części silnika
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
	// 1. Czyszczenie ekranu i bufora głębokości 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 2. Uruchomienie shadera
	spColored->use();

	// Stałe
	static float angle = 0.0f;
	float speed = 0.05f;
	float r = 16.125f; // promień wału 16.125
	float l = 52.0f; // długość korbowodu 52

	float tlok14 = r * cos(angle) + sqrt(l * l - r * r * sin(angle) * sin(angle)); // pozycja tłoka w osi Y, wynik z twierdzenia Pitagorasa)
	float angle_180 = angle + 3.14f;
	float tlok23 = r * cos(angle_180) + sqrt(l * l - r * r * sin(angle_180) * sin(angle_180));;

	// Klawisze
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		angle += speed;
	};

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		angle -= speed;
	};


	// 3. Kamera 
	// 1 -> -3.4, 2 -> -1.2, 3 -> 1.2, 4 -> 3.4

	glm::mat4 V = glm::lookAt(
		glm::vec3(0.0f, 5.0f, 15.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	glm::mat4 P = glm::perspective(glm::radians(50.0f), 1.0f, 0.1f, 100.0f);
	glUniformMatrix4fv(spColored->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(spColored->u("P"), 1, false, glm::value_ptr(P));

	// WYMUSZAMY RYSOWANIE SIATKI (WIREFRAME) DLA WSZYSTKICH OBIEKTÓW
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// Wspólna skala i pozycja 
	glm::mat4 M = glm::mat4(1.0f);
	M = glm::scale(M, glm::vec3(0.05f, 0.05f, 0.05f));


	// TDC - 15 w górę 
	float TDC = 0.0f;
	float TDC_Korbowod = 15.0f;
	float TDC_Tlok = 75.5f;
	float TDC_Zawors = 103.0f;
	float TDC_Zaworw = 103.0f;

	// BDC - 15 w dół
	float BDC = 0.0f;
	float BDC_Korbowod = -15.0f;
	float BDC_Tlok = 45.5f;
	float BDC_Zawors = 103.0f;
	float BDC_Zaworw = 103.0f;

	float odstep = 41.0f; // Odstęp między cylindrami

	// 4. Macierze obiektów
	// 4.0 WAŁ
	glm::mat4 mWal = M; // On jest w punkcie 0 naszego projektu (całość jest dostosowana do niego), więc nie musimy go przesuwać
	mWal = glm::rotate(mWal, angle, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mWal));
	glBindVertexArray(wal.vao);
	glDrawArrays(GL_TRIANGLES, 0, wal.vertexCount);


	// 4.1 CYLINDER 1
	// 4.1.1 KORBOWÓD
	glm::mat4 mKorbowod1 = M;
	mKorbowod1 = glm::translate(mKorbowod1, glm::vec3(-65.0f, TDC_Korbowod, 0.0f)); // Przesuwamy korbowód na wał
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mKorbowod1));
	glBindVertexArray(korbowod.vao);
	glDrawArrays(GL_TRIANGLES, 0, korbowod.vertexCount);

	// 4.1.2 TŁOK
	glm::mat4 mTlok1 = M;
	mTlok1 = glm::translate(mTlok1, glm::vec3(-65.0f, tlok14, 0.0f)); // Przesuwamy tłok na korbowód
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mTlok1));
	glBindVertexArray(tlok.vao);
	glDrawArrays(GL_TRIANGLES, 0, tlok.vertexCount);

	// 4.1.3 ZAWÓR SSĄCY
	glm::mat4 mZawors1 = M;
	mZawors1 = glm::translate(mZawors1, glm::vec3(-65.0f, TDC_Zawors, 5.0f)); // Przesuwamy zawór ssący na tłok
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZawors1));
	glBindVertexArray(zawors.vao);
	glDrawArrays(GL_TRIANGLES, 0, zawors.vertexCount);

	// 4.1.4 ZAWÓR WYDECHOWY
	glm::mat4 mZaworw1 = M;
	mZaworw1 = glm::translate(mZaworw1, glm::vec3(-65.0f, TDC_Zaworw, -5.0f)); // Przesuwamy zawór wydechowy na tłok
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZaworw1));
	glBindVertexArray(zaworw.vao);
	glDrawArrays(GL_TRIANGLES, 0, zaworw.vertexCount);

	// 4.2 CYLINDER 2
	// 4.2.1 KORBOWOD
	glm::mat4 mKorbowod2 = M;
	mKorbowod2 = glm::translate(mKorbowod2, glm::vec3(-65.0f + odstep, BDC_Korbowod, 0.0f)); // Przesuwamy korbowód na wał
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mKorbowod2));
	glBindVertexArray(korbowod.vao);
	glDrawArrays(GL_TRIANGLES, 0, korbowod.vertexCount);

	// 4.2.2 TŁOK
	glm::mat4 mTlok2 = M;
	mTlok2 = glm::translate(mTlok2, glm::vec3(-65.0f + odstep, tlok23, 0.0f)); // Przesuwamy tłok na korbowód
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mTlok2));
	glBindVertexArray(tlok.vao);
	glDrawArrays(GL_TRIANGLES, 0, tlok.vertexCount);

	// 4.2.3 ZAWÓR SSĄCY
	glm::mat4 mZawors2 = M;
	mZawors2 = glm::translate(mZawors2, glm::vec3(-65.0f + odstep, BDC_Zawors, 5.0f)); // Przesuwamy zawór ssący na tłok
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZawors2));
	glBindVertexArray(zawors.vao);
	glDrawArrays(GL_TRIANGLES, 0, zawors.vertexCount);

	// 4.2.4 ZAWÓR WYDECHOWY
	glm::mat4 mZaworw2 = M;
	mZaworw2 = glm::translate(mZaworw2, glm::vec3(-65.0f + odstep, BDC_Zaworw, -5.0f)); // Przesuwamy zawór wydechowy na tłok
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZaworw2));
	glBindVertexArray(zaworw.vao);
	glDrawArrays(GL_TRIANGLES, 0, zaworw.vertexCount);

	// 4.3 CYLINDER 3
	// 4.3.1 KORBOWOD
	glm::mat4 mKorbowod3 = M;
	mKorbowod3 = glm::translate(mKorbowod3, glm::vec3(-65.0f + odstep * 2, BDC_Korbowod, 0.0f)); // Przesuwamy korbowód na wał
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mKorbowod3));
	glBindVertexArray(korbowod.vao);
	glDrawArrays(GL_TRIANGLES, 0, korbowod.vertexCount);

	// 4.3.2 TŁOK
	glm::mat4 mTlok3 = M;
	mTlok3 = glm::translate(mTlok3, glm::vec3(-65.0f + odstep * 2, tlok23, 0.0f)); // Przesuwamy tłok na korbowód
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mTlok3));
	glBindVertexArray(tlok.vao);
	glDrawArrays(GL_TRIANGLES, 0, tlok.vertexCount);

	// 4.3.3 ZAWÓR SSĄCY
	glm::mat4 mZawors3 = M;
	mZawors3 = glm::translate(mZawors3, glm::vec3(-65.0f + odstep * 2, BDC_Zawors, 5.0f)); // Przesuwamy zawór ssący na tłok
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZawors3));
	glBindVertexArray(zawors.vao);
	glDrawArrays(GL_TRIANGLES, 0, zawors.vertexCount);

	// 4.3.4 ZAWÓR WYDECHOWY
	glm::mat4 mZaworw3 = M;
	mZaworw3 = glm::translate(mZaworw3, glm::vec3(-65.0f + odstep * 2, BDC_Zaworw, -5.0f)); // Przesuwamy zawór wydechowy na tłok
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZaworw3));
	glBindVertexArray(zaworw.vao);
	glDrawArrays(GL_TRIANGLES, 0, zaworw.vertexCount);

	// 4.4 CYLINDER 4
	// 4.4.1 KORBOWÓD
	glm::mat4 mKorbowod4 = M;
	mKorbowod4 = glm::translate(mKorbowod4, glm::vec3(-65.0f + odstep * 3, TDC_Korbowod, 0.0f)); // Przesuwamy korbowód na wał
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mKorbowod4));
	glBindVertexArray(korbowod.vao);
	glDrawArrays(GL_TRIANGLES, 0, korbowod.vertexCount);

	// 4.4.2 TŁOK
	glm::mat4 mTlok4 = M;
	mTlok4 = glm::translate(mTlok4, glm::vec3(-65.0f + odstep * 3, tlok14, 0.0f)); // Przesuwamy tłok na korbowód
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mTlok4));
	glBindVertexArray(tlok.vao);
	glDrawArrays(GL_TRIANGLES, 0, tlok.vertexCount);

	// 4.4.3 ZAWÓR SSĄCY
	glm::mat4 mZawors4 = M;
	mZawors4 = glm::translate(mZawors4, glm::vec3(-65.0f + odstep * 3, TDC_Zawors, 5.0f)); // Przesuwamy zawór ssący na tłok
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZawors4));
	glBindVertexArray(zawors.vao);
	glDrawArrays(GL_TRIANGLES, 0, zawors.vertexCount);

	// 4.4.4 ZAWÓR WYDECHOWY
	glm::mat4 mZaworw4 = M;
	mZaworw4 = glm::translate(mZaworw4, glm::vec3(-65.0f + odstep * 3, TDC_Zaworw, -5.0f)); // Przesuwamy zawór wydechowy na tłok
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZaworw4));
	glBindVertexArray(zaworw.vao);
	glDrawArrays(GL_TRIANGLES, 0, zaworw.vertexCount);


	glBindVertexArray(0);

	// PRZYWRACAMY RYSOWANIE SOLIDNE (dobra praktyka, by nie psuć innych części programu)
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// 5. Przerzucenie buforów
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
