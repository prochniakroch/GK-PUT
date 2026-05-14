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

ShaderProgram* sp;

float speed_x = 0;
float speed_y = 0;
float aspectRatio = 1;

// Zamiast luźnych zmiennych, robimy "pudełko" na dane modelu
struct ModelData {
	GLuint vao;
	GLuint vbo;
	int vertexCount;
};

// Tworzymy 5 osobnych "pudełek" dla 5 części silnika
ModelData tlok, korbowod, wal, zawors, zaworw, walek1, walek2;

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

			// 1. Wrzucamy do wagonika pozycję wierzchołka (3 liczby: XYZ)
			vertexData.push_back(attrib.vertices[3 * size_t(idx.vertex_index) + 0]);
			vertexData.push_back(attrib.vertices[3 * size_t(idx.vertex_index) + 1]);
			vertexData.push_back(attrib.vertices[3 * size_t(idx.vertex_index) + 2]);

			// 2. Wrzucamy do wagonika kierunek, w którym patrzy wierzchołek (3 liczby: Normale)
			if (idx.normal_index >= 0) {
				vertexData.push_back(attrib.normals[3 * size_t(idx.normal_index) + 0]);
				vertexData.push_back(attrib.normals[3 * size_t(idx.normal_index) + 1]);
				vertexData.push_back(attrib.normals[3 * size_t(idx.normal_index) + 2]);
			}
			else {
				vertexData.push_back(0.0f); vertexData.push_back(1.0f); vertexData.push_back(0.0f);
			}

			// 3. Wrzucamy do wagonika KOLOR (3 liczby: RGB) - sami go tu "wymyślamy", bo plik .obj go nie ma
			vertexData.push_back(0.7f); // Czerwony
			vertexData.push_back(0.7f); // Zielony
			vertexData.push_back(0.7f); // Niebieski (daje nam to szary metal)
		}
	}

	data.vertexCount = vertexData.size() / 9;

	// Tworzymy i otwieramy bufory
	glGenVertexArrays(1, &data.vao);
	glGenBuffers(1, &data.vbo);

	glBindVertexArray(data.vao);
	glBindBuffer(GL_ARRAY_BUFFER, data.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

	// Pytamy klasę o numery adresów
	GLuint adres_vertex = sp->a("vertex");
	GLuint adres_normal = sp->a("normal");
	GLuint adres_color = sp->a("color");

	// Włączamy adresy
	glEnableVertexAttribArray(adres_vertex);
	glEnableVertexAttribArray(adres_normal);
	glEnableVertexAttribArray(adres_color);

	// Tłumaczymy układ w wagoniku z 9 liczbami
	glVertexAttribPointer(adres_vertex, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
	glVertexAttribPointer(adres_normal, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
	glVertexAttribPointer(adres_color, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));

	glBindVertexArray(0);

	return data; // Zwracamy gotowe bufory z karty graficznej
}

// Procedura obsługi zmiany rozmiaru okna
void windowResizeCallback(GLFWwindow* window, int width, int height) {
	if (height == 0) return;
	aspectRatio = (float)width / (float)height;
	glViewport(0, 0, width, height);
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

	glfwSetWindowSizeCallback(window, windowResizeCallback);

	sp = new ShaderProgram("v_simplest.glsl", NULL, "f_simplest.glsl");

	// Wczytujemy pliki z dysku i zapisujemy ich dane do kart pamięci VRAM
	tlok = LoadModelOBJ("tlok.obj");
	wal = LoadModelOBJ("wal1.obj");
	korbowod = LoadModelOBJ("korbowod.obj");
	zawors = LoadModelOBJ("zawors.obj");
	zaworw = LoadModelOBJ("zaworw.obj");
	walek1 = LoadModelOBJ("walek1.obj");
	walek2 = LoadModelOBJ("walek2.obj");
}


//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
    freeShaders();
    //************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************	
}



//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window, float angle_x, float angle_y) {
	// 1. Czyszczenie ekranu i bufora głębokości 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Stałe

	static float angle = 0.0f;
	float speed = 0.05f;
	// promień wału 17.437 lub 17.40335
	float r = 17.40335f; // promień wału 16.125
	// długość korbowodu 52.0892 lub 52.68452
	float l = 52.68452f; // długość korbowodu 52 
	// tlok ma 8.01949 do swożnia
	float offset_tloka = 8.01949f; // offset tłoka 8.0f

	// 1 - 4
	float cylinder1 = -65.0f;
	float cylinder4 = -65.0f + 41.0f * 3;
	float tlok14 = r * cos(angle) + sqrt(l * l - r * r * sin(angle) * sin(angle)); // pozycja tłoka w osi Y, wynik z twierdzenia Pitagorasa)
	float pin_y_14 = r * cos(angle); // pozycja sworznia w osi X
	float pin_z_14 = r * sin(angle); // pozycja sworznia w osi Y
	float kat_korb_14 = asin((r * sin(angle)) / l);

	// 2 - 3
	float cylinder2 = -65.0f + 41.0f;
	float cylinder3 = -65.0f + 41.0f * 2;
	float angle_180 = angle + 3.14f;
	float tlok23 = r * cos(angle_180) + sqrt(l * l - r * r * sin(angle_180) * sin(angle_180));
	float pin_y_23 = r * cos(angle_180); // pozycja sworznia w osi X
	float pin_z_23 = r * sin(angle_180); // pozycja sworznia w osi Y
	float kat_korb_23 = asin((r * sin(angle_180)) / l);

	// Rorząd 1-3-4-2 
	float walek_angle = angle * 0.5f; // wałek rozrządu kręci się dwa razy wolniej
	float max_otwarcie = -1.5f;       // maks otwarcie zaworów

	// Fazy zapłonu przypisane do odpowiednich cylindrów (kolejność 1-3-4-2)
	// Odstęp to dokładnie 90 stopni wałka (PI/2)
	float faza_cyl1 = 0.0f;           // 1. w kolejności zapłonu
	float faza_cyl3 = 1.570796f;      // 2. w kolejności zapłonu (PI / 2)
	float faza_cyl4 = 3.141593f;      // 3. w kolejności zapłonu (PI)
	float faza_cyl2 = 4.712389f;      // 4. w kolejności zapłonu (3 * PI / 2)

	// Punkty szczytowe dla suwów (przesunięcie krzywki na wałku)
	// Żeby zawór otwierał się idealnie w połowie drogi tłoka
	float szczyt_wydech = 2.356194f; // Środek suwu wydechu (3/4 PI)
	float szczyt_wlot = 3.926991f; // Środek suwu ssania (5/4 PI)

	// --- CYLINDER 1 ---
	float ssanie_1 = pow(fmax(0.0f, cos(walek_angle - faza_cyl1 - szczyt_wlot)), 16.0f) * max_otwarcie;
	float wydech_1 = pow(fmax(0.0f, cos(walek_angle - faza_cyl1 - szczyt_wydech)), 16.0f) * max_otwarcie;

	// --- CYLINDER 2 ---
	float ssanie_2 = pow(fmax(0.0f, cos(walek_angle - faza_cyl2 - szczyt_wlot)), 16.0f) * max_otwarcie;
	float wydech_2 = pow(fmax(0.0f, cos(walek_angle - faza_cyl2 - szczyt_wydech)), 16.0f) * max_otwarcie;

	// --- CYLINDER 3 ---
	float ssanie_3 = pow(fmax(0.0f, cos(walek_angle - faza_cyl3 - szczyt_wlot)), 16.0f) * max_otwarcie;
	float wydech_3 = pow(fmax(0.0f, cos(walek_angle - faza_cyl3 - szczyt_wydech)), 16.0f) * max_otwarcie;

	// --- CYLINDER 4 ---
	float ssanie_4 = pow(fmax(0.0f, cos(walek_angle - faza_cyl4 - szczyt_wlot)), 16.0f) * max_otwarcie;
	float wydech_4 = pow(fmax(0.0f, cos(walek_angle - faza_cyl4 - szczyt_wydech)), 16.0f) * max_otwarcie;

	// Sterowanie z klawiatury
	// Sterowanie obrotem silnika
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		angle += speed;
	};
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		angle -= speed;
	};

	// Sterowanie kamerą 
	static float cam_yaw = 0.0f;   // Kąt lewo/prawo
	static float cam_pitch = 0.3f; // Kąt góra/dół (zaczynamy lekko od góry)
	float cam_radius = 20.0f;      // Odległość kamery od silnika
	float cam_speed = 0.03f;       // Czułość obrotu kamery

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { 
		cam_yaw -= cam_speed; 
	};
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { 
		cam_yaw += cam_speed; 
	};
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { 
		cam_pitch += cam_speed; 
	};
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { 
		cam_pitch -= cam_speed; 
	};

	if (cam_pitch > 1.5f) cam_pitch = 1.5f;
	if (cam_pitch < -1.5f) cam_pitch = -1.5f;


	// 2. Kamera 
	// 1 -> -3.4, 2 -> -1.2, 3 -> 1.2, 4 -> 3.4

	// Punkt, na który patrzy kamera (środek Twojego silnika)
	glm::vec3 target = glm::vec3(-3.2f, 0.0f, 0.0f);

	// Wyliczanie nowej pozycji kamery w przestrzeni 3D za pomocą trygonometrii
	glm::vec3 cam_pos;
	cam_pos.x = target.x + cam_radius * cos(cam_pitch) * sin(cam_yaw);
	cam_pos.y = target.y + cam_radius * sin(cam_pitch);
	cam_pos.z = target.z + cam_radius * cos(cam_pitch) * cos(cam_yaw);

	glm::mat4 V = glm::lookAt(
		cam_pos,                      // Skąd patrzymy (nasza nowa, wyliczona pozycja)
		target,                       // Na co patrzymy (środek silnika)
		glm::vec3(0.0f, 1.0f, 0.0f)   // Gdzie jest "góra" (oś Y)
	);

	glm::mat4 P = glm::perspective(50.0f * PI / 180.0f, aspectRatio, 0.01f, 50.0f);

	// 3. Uruchomienie shadera
	sp->use();

	glUniform4f(sp->u("light_pos"), 0.0f, 20.0f, 15.0f, 1.0f); 	// WŁĄCZAMY ŻARÓWKĘ (Wieszamy ją 20 metrów w górę i 15 w naszą stronę)
	glUniformMatrix4fv(sp->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(sp->u("V"), 1, false, glm::value_ptr(V));

	// WYMUSZAMY RYSOWANIE SIATKI (WIREFRAME) DLA WSZYSTKICH OBIEKTÓW
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Wspólna skala
	glm::mat4 M = glm::mat4(1.0f);
	M = glm::scale(M, glm::vec3(0.05f, 0.05f, 0.05f));


	// TDC - 15 w górę 
	float TDC = 0.0f;
	float TDC_Korbowod = 15.0f;
	float TDC_Tlok = 75.5f;
	float TDC_Zawors = 107.0f;
	float TDC_Zaworw = 107.0f;

	// BDC - 15 w dół
	float BDC = 0.0f;
	float BDC_Korbowod = -15.0f;
	float BDC_Tlok = 45.5f;
	float BDC_Zawors = 107.0f;
	float BDC_Zaworw = 107.0f;

	float odstep = 41.0f; // Odstęp między cylindrami
	float wal_w_lewo = -117.5f;
	float walek_w_gore = 125.0f;
	float rozstaw_walkow = 17.0f;
	float rozstaw_zaworow = 17.0f;
	float kat_zaworow = 0.20f; // Kąt rozwarcia zaworów (w radianach)

	float faza_walek1 = 0.0f;
	float faza_walek2 = 0.0f;

	// 4. Macierze obiektów
	// 4.0 WAŁ
	glm::mat4 mWal = M;
	mWal = glm::translate(mWal, glm::vec3(wal_w_lewo, 0.0f, 0.0f));
	mWal = glm::rotate(mWal, angle, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mWal));
	glBindVertexArray(wal.vao);
	glDrawArrays(GL_TRIANGLES, 0, wal.vertexCount);

	// 4.0.1 WAŁEK ROZRZĄDU 1
	glm::mat4 mWalek1 = M;
	mWalek1 = glm::translate(mWalek1, glm::vec3(-39.5f, walek_w_gore, rozstaw_walkow));
	mWalek1 = glm::rotate(mWalek1, walek_angle + faza_walek1, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mWalek1));
	glBindVertexArray(walek1.vao);
	glDrawArrays(GL_TRIANGLES, 0, walek1.vertexCount);

	// 4.0.1 WAŁEK ROZRZĄDU 2
	glm::mat4 mWalek2 = M;
	mWalek2 = glm::translate(mWalek2, glm::vec3(-101.5f, walek_w_gore, -rozstaw_walkow));
	mWalek2 = glm::rotate(mWalek2, -walek_angle + faza_walek2, glm::vec3(1.0f, 0.0f, 0.0f));
	mWalek2 = glm::scale(mWalek2, glm::vec3(1.0f, 1.0f, -1.0f)); // Odbicie lustrzane, bo wałek 2 jest "odwrotny");
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mWalek2));
	glBindVertexArray(walek2.vao);
	glDrawArrays(GL_TRIANGLES, 0, walek2.vertexCount);

	// 4.1 CYLINDER 1
	// 4.1.1 KORBOWÓD
	glm::mat4 mKorbowod1 = M;
	mKorbowod1 = glm::translate(mKorbowod1, glm::vec3(cylinder1, pin_y_14, pin_z_14)); // Przesuwamy korbowód na wał
	mKorbowod1 = glm::rotate(mKorbowod1, -kat_korb_14, glm::vec3(1.0f, 0.0f, 0.0f)); // Obracamy korbowód o odpowiedni kąt
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mKorbowod1));
	glBindVertexArray(korbowod.vao);
	glDrawArrays(GL_TRIANGLES, 0, korbowod.vertexCount);

	// 4.1.2 TŁOK
	glm::mat4 mTlok1 = M;
	mTlok1 = glm::translate(mTlok1, glm::vec3(-65.0f, tlok14 + offset_tloka, 0.0f)); // Przesuwamy tłok na korbowód
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mTlok1));
	glBindVertexArray(tlok.vao);
	glDrawArrays(GL_TRIANGLES, 0, tlok.vertexCount);

	// 4.1.3 ZAWÓR SSĄCY
	glm::mat4 mZawors1 = M;
	mZawors1 = glm::translate(mZawors1, glm::vec3(-65.0f, TDC_Zawors + ssanie_1, -rozstaw_zaworow)); // Przesuwamy zawór ssący na tłok
	mZawors1 = glm::rotate(mZawors1, -kat_zaworow, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZawors1));
	glBindVertexArray(zawors.vao);
	glDrawArrays(GL_TRIANGLES, 0, zawors.vertexCount);

	// 4.1.4 ZAWÓR WYDECHOWY
	glm::mat4 mZaworw1 = M;
	mZaworw1 = glm::translate(mZaworw1, glm::vec3(-65.0f, TDC_Zaworw + wydech_1, rozstaw_zaworow)); // Przesuwamy zawór wydechowy na tłok
	mZaworw1 = glm::rotate(mZaworw1, kat_zaworow, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZaworw1));
	glBindVertexArray(zaworw.vao);
	glDrawArrays(GL_TRIANGLES, 0, zaworw.vertexCount);

	// 4.2 CYLINDER 2
	// 4.2.1 KORBOWOD
	glm::mat4 mKorbowod2 = M;
	mKorbowod2 = glm::translate(mKorbowod2, glm::vec3(cylinder2, pin_y_23, pin_z_23)); // Przesuwamy korbowód na wał
	mKorbowod2 = glm::rotate(mKorbowod2, -kat_korb_23, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mKorbowod2));
	glBindVertexArray(korbowod.vao);
	glDrawArrays(GL_TRIANGLES, 0, korbowod.vertexCount);

	// 4.2.2 TŁOK
	glm::mat4 mTlok2 = M;
	mTlok2 = glm::translate(mTlok2, glm::vec3(-65.0f + odstep, tlok23 + offset_tloka, 0.0f)); // Przesuwamy tłok na korbowód
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mTlok2));
	glBindVertexArray(tlok.vao);
	glDrawArrays(GL_TRIANGLES, 0, tlok.vertexCount);

	// 4.2.3 ZAWÓR SSĄCY
	glm::mat4 mZawors2 = M;
	mZawors2 = glm::translate(mZawors2, glm::vec3(-65.0f + odstep, BDC_Zawors + ssanie_2, -rozstaw_zaworow)); // Przesuwamy zawór ssący na tłok
	mZawors2 = glm::rotate(mZawors2, -kat_zaworow, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZawors2));
	glBindVertexArray(zawors.vao);
	glDrawArrays(GL_TRIANGLES, 0, zawors.vertexCount);

	// 4.2.4 ZAWÓR WYDECHOWY
	glm::mat4 mZaworw2 = M;
	mZaworw2 = glm::translate(mZaworw2, glm::vec3(-65.0f + odstep, BDC_Zaworw + wydech_2, rozstaw_zaworow)); // Przesuwamy zawór wydechowy na tłok
	mZaworw2 = glm::rotate(mZaworw2, kat_zaworow, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZaworw2));
	glBindVertexArray(zaworw.vao);
	glDrawArrays(GL_TRIANGLES, 0, zaworw.vertexCount);

	// 4.3 CYLINDER 3
	// 4.3.1 KORBOWOD
	glm::mat4 mKorbowod3 = M;
	mKorbowod3 = glm::translate(mKorbowod3, glm::vec3(cylinder3, pin_y_23, pin_z_23)); // Przesuwamy korbowód na wał
	mKorbowod3 = glm::rotate(mKorbowod3, -kat_korb_23, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mKorbowod3));
	glBindVertexArray(korbowod.vao);
	glDrawArrays(GL_TRIANGLES, 0, korbowod.vertexCount);

	// 4.3.2 TŁOK
	glm::mat4 mTlok3 = M;
	mTlok3 = glm::translate(mTlok3, glm::vec3(-65.0f + odstep * 2, tlok23 + offset_tloka, 0.0f)); // Przesuwamy tłok na korbowód
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mTlok3));
	glBindVertexArray(tlok.vao);
	glDrawArrays(GL_TRIANGLES, 0, tlok.vertexCount);

	// 4.3.3 ZAWÓR SSĄCY
	glm::mat4 mZawors3 = M;
	mZawors3 = glm::translate(mZawors3, glm::vec3(-65.0f + odstep * 2, BDC_Zawors + ssanie_3, -rozstaw_zaworow)); // Przesuwamy zawór ssący na tłok
	mZawors3 = glm::rotate(mZawors3, -kat_zaworow, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZawors3));
	glBindVertexArray(zawors.vao);
	glDrawArrays(GL_TRIANGLES, 0, zawors.vertexCount);

	// 4.3.4 ZAWÓR WYDECHOWY
	glm::mat4 mZaworw3 = M;
	mZaworw3 = glm::translate(mZaworw3, glm::vec3(-65.0f + odstep * 2, BDC_Zaworw + wydech_3, rozstaw_zaworow)); // Przesuwamy zawór wydechowy na tłok
	mZaworw3 = glm::rotate(mZaworw3, kat_zaworow, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZaworw3));
	glBindVertexArray(zaworw.vao);
	glDrawArrays(GL_TRIANGLES, 0, zaworw.vertexCount);

	// 4.4 CYLINDER 4
	// 4.4.1 KORBOWÓD
	glm::mat4 mKorbowod4 = M;
	mKorbowod4 = glm::translate(mKorbowod4, glm::vec3(cylinder4, pin_y_14, pin_z_14)); // Przesuwamy korbowód na wał
	mKorbowod4 = glm::rotate(mKorbowod4, -kat_korb_14, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mKorbowod4));
	glBindVertexArray(korbowod.vao);
	glDrawArrays(GL_TRIANGLES, 0, korbowod.vertexCount);

	// 4.4.2 TŁOK
	glm::mat4 mTlok4 = M;
	mTlok4 = glm::translate(mTlok4, glm::vec3(-65.0f + odstep * 3, tlok14 + offset_tloka, 0.0f)); // Przesuwamy tłok na korbowód
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mTlok4));
	glBindVertexArray(tlok.vao);
	glDrawArrays(GL_TRIANGLES, 0, tlok.vertexCount);

	// 4.4.3 ZAWÓR SSĄCY
	glm::mat4 mZawors4 = M;
	mZawors4 = glm::translate(mZawors4, glm::vec3(-65.0f + odstep * 3, TDC_Zawors + ssanie_4, -rozstaw_zaworow)); // Przesuwamy zawór ssący na tłok
	mZawors4 = glm::rotate(mZawors4, -kat_zaworow, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(spColored->u("M"), 1, false, glm::value_ptr(mZawors4));
	glBindVertexArray(zawors.vao);
	glDrawArrays(GL_TRIANGLES, 0, zawors.vertexCount);

	// 4.4.4 ZAWÓR WYDECHOWY
	glm::mat4 mZaworw4 = M;
	mZaworw4 = glm::translate(mZaworw4, glm::vec3(-65.0f + odstep * 3, TDC_Zaworw + wydech_4, rozstaw_zaworow)); // Przesuwamy zawór wydechowy na tłok
	mZaworw4 = glm::rotate(mZaworw4, kat_zaworow, glm::vec3(1.0f, 0.0f, 0.0f));
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
	float angle_x = 0; //Aktualny kąt obrotu obiektu
	float angle_y = 0; //Aktualny kąt obrotu obiektu
	glfwSetTime(0); //Zeruj timer
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamknięte
	{
		angle_x += speed_x * glfwGetTime(); //Zwiększ/zmniejsz kąt obrotu na podstawie prędkości i czasu jaki upłynał od poprzedniej klatki
		angle_y += speed_y * glfwGetTime(); //Zwiększ/zmniejsz kąt obrotu na podstawie prędkości i czasu jaki upłynał od poprzedniej klatki
		glfwSetTime(0); //Zeruj timer
		drawScene(window, angle_x, angle_y); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}
