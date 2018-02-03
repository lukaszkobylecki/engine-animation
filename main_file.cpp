#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <lodepng.h>
#include <iostream>
#include <string.h>
#include "piston.h"
#include "connecting_rod.h"
#include "sparkPlug.h"
#include "valve.h"
#include "crankshaft.h"
#include "cylinder.h"
#include "valveCylinder.h"

using namespace glm;

const float PI = 3.141592653589793f;

float aspect = 1.0f; //Aktualny stosunek szerokości do wysokości okna

float speed_x = 0; //Szybkość kątowa obrotu obiektu w radianach na sekundę wokół osi x
float speed_y = 0; //Szybkość kątowa obrotu obiektu w radianach na sekundę wokół osi y
float speed_zoom = 0;

float valveOffsetX = 0.8f;
float valveOffsetY = 0.92f;
float valveAngle = -30.0f*PI/180.0f;
float valveSpeed = 0.001f;

float pistonOffsetY = 0.45f;
float pistonSpeed = 0.001f;

float connectingRodOffsetY = -0.8f;

float crankshaftOffsetY = -1.65f;
float crankshaftOffsetZ = -0.2f;

float sparkPlugOffsetY = 1.2f;

float cylinderOffsetY = -0.6f;
float cylinderOffsetZ = 0.2f;

float valveCylinderOffsetX = 0.9f;
float valveCylinderOffsetY = 0.85f;
float valveCylinderOffsetZ = 0.10f;

const int numTextures = 3;
const char * textureNames[numTextures] = { "metal.png", "metal_pattern.png", "solid_grey.png" };
GLuint metalTextures[numTextures];

//Procedura obsługi błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

//Procedura obługi zmiany rozmiaru bufora ramki
void windowResize(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height); //Obraz ma być generowany w oknie o tej rozdzielczości
    aspect=(float)width/(float)height; //Stosunek szerokości do wysokości okna
}

//Procedura obsługi klawiatury
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_LEFT) speed_y = PI/2;
        if (key == GLFW_KEY_RIGHT) speed_y = -PI/2;
        if (key == GLFW_KEY_UP) speed_x = PI/2;
        if (key == GLFW_KEY_DOWN) speed_x = -PI/2;
        if (key == GLFW_KEY_W) speed_zoom = PI/2;
        if (key == GLFW_KEY_S) speed_zoom = -PI/2;
    }

    if (action == GLFW_RELEASE)
    {
        if (key == GLFW_KEY_LEFT) speed_y = 0;
        if (key == GLFW_KEY_RIGHT) speed_y = 0;
        if (key == GLFW_KEY_UP) speed_x = 0;
        if (key == GLFW_KEY_DOWN) speed_x = 0;
        if (key == GLFW_KEY_W) speed_zoom = 0;
        if (key == GLFW_KEY_S) speed_zoom = 0;
    }
}

//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
    glfwSetFramebufferSizeCallback(window, windowResize); //Zarejestruj procedurę obsługi zmiany rozdzielczości bufora ramki
    glfwSetKeyCallback(window, key_callback); //Zarejestruj procedurę obsługi klawiatury

	glClearColor(0,0,0,1); //Ustaw kolor czyszczenia ekranu

	glEnable(GL_LIGHTING); //Włącz tryb cieniowania
	glEnable(GL_LIGHT0); //Włącz zerowe źródło światła
	glEnable(GL_DEPTH_TEST); //Włącz używanie budora głębokości
    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D); // wlaczenie teksturowania

    float amb[]={0.8,0.8,0.85,1};
    float diff[]={0.7,0.7,0.7,1};
    float spec[]={0.7,0.9,1,1};
    float shine = 70;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb );
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diff );
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec );
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shine );

    //teksturowanie
    glGenTextures(numTextures, metalTextures); //Zainicjuj jeden uchwyt
    for(int i=0; i < numTextures; i++) {
        std::vector<unsigned char> image;   //Alokuj wektor do wczytania obrazka
        unsigned width, height;   //Zmienne do których wczytamy wymiary obrazka
        unsigned error = lodepng::decode(image, width, height, textureNames[i]);
        glBindTexture(GL_TEXTURE_2D, metalTextures[i]); //Uaktywnij uchwyt
        //Wczytaj obrazek do pamięci KG skojarzonej z uchwytem
        glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*) image.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // dziala z aktywna tekstura
    }
}

void applyUserRotation(mat4 &M, float angle_x, float angle_y)
{
    M = rotate(M, angle_x, vec3(1.0f, 0.0f, 0.0f));
    M = rotate(M, angle_y, vec3(0.0f, 1.0f, 0.0f));
}

void drawObject(const float *positions, const float *normals, const float *textures, const int verticies, const int texNum)
{
    if (texNum > numTextures || texNum < 0) {
        std::cout << "Invalid texture number" << std::endl;
        return;
    }

    glBindTexture(GL_TEXTURE_2D, metalTextures[texNum]);
    glEnableClientState(GL_VERTEX_ARRAY); //Podczas rysowania używaj tablicy wierzchołków
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glVertexPointer(3,GL_FLOAT, 0, positions);
    glNormalPointer(GL_FLOAT, 0, normals);
    glTexCoordPointer(2, GL_FLOAT, 0, textures);

    glDrawArrays(GL_TRIANGLES, 0, verticies); //Rysuj model

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}

void drawValves(mat4 V, float angle, float angle_x, float angle_y, const int texNum)
{
    //lewy zawór
    mat4 M = mat4(1.0f);

    applyUserRotation(M, angle_x, angle_y);

    M = translate(M, vec3(valveOffsetX, valveOffsetY, 0.0f));
    M = rotate(M, valveAngle, vec3(0.0f, 0.0f, 1.0f));

    //ruch góra-dół
    if (angle < 180)
        M = translate(M, vec3(0.0f, angle * -valveSpeed, 0.0f));
    else if (angle >= 180 && angle <= 360)
        M = translate(M, vec3(0.0f, 180 * -valveSpeed, 0.0f));
    else if (angle > 360 && angle < 540)
        M = translate(M, vec3(0.0f, (540 - angle) * -valveSpeed, 0.0f));

    M = scale(M, vec3(0.5f, 0.6f, 0.6f));

    glLoadMatrixf(value_ptr(V*M));
    drawObject(valvePositions, valveNormals, valveTexels, valveVertices, texNum);


    //prawy zawór
    M = mat4(1.0f);

    applyUserRotation(M, angle_x, angle_y);

    M = translate(M, vec3(-valveOffsetX, valveOffsetY, 0.0f));
    M = rotate(M, -valveAngle, vec3(0.0f, 0.0f, 1.0f));

    //ruch góra-dół
    if (angle > 720 && angle < 1080)
        M = translate(M, vec3(0.0f, (720 - angle) * valveSpeed/2, 0.0f));
    else if (angle >= 1080 && angle < 1440)
        M = translate(M, vec3(0.0f, (1440 - angle) * -valveSpeed/2, 0.0f));

    M = scale(M, vec3(0.6f, 0.6f, 0.6f));

    glLoadMatrixf(value_ptr(V*M));
    drawObject(valvePositions, valveNormals, valveTexels, valveVertices, texNum);
}

void drawPiston(mat4 V, float angle, float angle_x, float angle_y, const int texNum)
{
    angle = (int)angle % 720;
    mat4 M = mat4(1.0f);

    applyUserRotation(M, angle_x, angle_y);

    M = translate(M, vec3(0.0f, pistonOffsetY, 0.0f));

    //ruch góra-dół
    if (angle < 360)
        M = translate(M, vec3(0.0f, angle * -pistonSpeed, 0.0f));
    else
        M = translate(M, vec3(0.0f, (720 - angle)* -pistonSpeed, 0.0f));

    M = rotate(M, 90.0f*PI/180.0f, vec3(0.0f, 1.0f, 0.0f));
    M = scale(M, vec3(0.42f, 0.42f, 0.42f));

    glLoadMatrixf(value_ptr(V*M));
    drawObject(pistonPositions, pistonNormals, pistonTexels, pistonVertices, texNum);
}

void drawConnectingRod(mat4 V, float angle, float angle_x, float angle_y, const int texNum)
{
    angle = (int)angle % 720;
    mat4 M = mat4(1.0f);

    applyUserRotation(M, angle_x, angle_y);

    M = translate(M, vec3(0.0f, connectingRodOffsetY, 0.0f));

    //ruch góra-dół
    if (angle < 360)
        M = translate(M, vec3(0.0f, angle * -pistonSpeed, 0.0f));
    else
        M = translate(M, vec3(0.0f, (720 - angle)* -pistonSpeed, 0.0f));

    //przesunięcie pivota ze środka na górę
    if (angle < 180)
        M = translate(M, vec3( -(angle/180)/10, 0.0f, 0.0f));
    else if (angle < 360)
        M = translate(M, vec3( ((angle - 360)/180)/10, 0.0f, 0.0f));
          else if (angle < 540)
        M = translate(M, vec3( ((angle - 360)/180)/10, 0.0f, 0.0f));
    else
        M = translate(M, vec3( -((angle - 720)/180)/10, 0.0f, 0.0f));

    M = rotate(M, 90.0f*PI/180.0f, vec3(1.0f, 0.0f, 0.0f));
    M = rotate(M, -90.0f*PI/180.0f, vec3(0.0f, 1.0f, 0.0f));

    //wahadło lewo-prawo
    if (angle < 180)
        M = rotate(M, -(angle/180)/10, vec3(0.0f, 1.0f, 0.0f));
    else if (angle < 360)
        M = rotate(M, ((angle - 360)/180)/10, vec3(0.0f, 1.0f, 0.0f));
    else if (angle < 540)
         M = rotate(M, ((angle - 360)/180)/10, vec3(0.0f, 1.0f, 0.0f));
    else
        M = rotate(M, -((angle - 720)/180)/10, vec3(0.0f, 1.0f, 0.0f));

    M = scale(M, vec3(0.08f, 0.12f, 0.12f));

    glLoadMatrixf(value_ptr(V*M));
    drawObject(connectingRodPositions, connectingRodNormals, connectingRodTexels, connectingRodVertices, texNum);
}

void drawCrankshaft(mat4 V, float angle, float angle_x, float angle_y, const int texNum)
{
    mat4 M = mat4(1.0f);

    applyUserRotation(M, angle_x, angle_y);

    M = translate(M, vec3(0.0f, crankshaftOffsetY, crankshaftOffsetZ));
    M = rotate(M, 90.0f*PI/180.0f, vec3(0.0f, 1.0f, 0.0f));

    //ruch obrotowy
    M = rotate(M, -(angle/2 + 180)*PI/180.0f, vec3(1.0f, 0.0f, 0.0f));

    M = scale(M, vec3(0.3f, 0.3f, 0.3f));

    glLoadMatrixf(value_ptr(V*M));
    drawObject(crankshaftPositions, crankshaftNormals, crankshaftTexels, crankshaftVertices, texNum);
}

void drawSparkPlug(mat4 V, float angle_x, float angle_y, const int texNum)
{
    mat4 M = mat4(1.0f);

    applyUserRotation(M, angle_x, angle_y);

    M = translate(M, vec3(0.0f, sparkPlugOffsetY, 0.0f));
    M = rotate(M, 180.0f*PI/180, vec3(0.0f, 0.0f, 1.0f));
    M = scale(M, vec3(0.003f, 0.003f, 0.003f));

    glLoadMatrixf(value_ptr(V*M));
    drawObject(sparkPlugPositions, sparkPlugNormals, sparkPlugTexels, sparkPlugVertices, texNum);
}

void drawCylinder(mat4 V, float angle_x, float angle_y, const int texNum)
{
    mat4 M = mat4(1.0f);

    applyUserRotation(M, angle_x, angle_y);

    M = translate(M, vec3(0.0f, cylinderOffsetY, cylinderOffsetZ));
    M = scale(M, vec3(0.65f, 0.7f, 1.0f));

    glLoadMatrixf(value_ptr(V*M));
    drawObject(cylinderPositions, cylinderNormals, cylinderTexels, cylinderVertices, texNum);
}

void dravValveCylinders(mat4 V, float angle_x, float angle_y, const int texNum)
{
    //lewy
    mat4 M = mat4(1.0f);

    applyUserRotation(M, angle_x, angle_y);

    M = translate(M, vec3(valveCylinderOffsetX, valveCylinderOffsetY, valveCylinderOffsetZ));
    M = rotate(M, valveAngle, vec3(0.0f, 0.0f, 1.0f));
    M = rotate (M, 60*PI/180.f, vec3(0.0f, 1.0f, 0.0f));
    M = scale(M, vec3(0.7f, 1.0f, 0.7f));

    glLoadMatrixf(value_ptr(V*M));
    drawObject(valveCylinderPositions, valveCylinderNormals, valveCylinderTexels, valveCylinderVertices, texNum);


    //prawy
    M = mat4(1.0f);

    applyUserRotation(M, angle_x, angle_y);

    M = translate(M, vec3(-valveCylinderOffsetX, valveCylinderOffsetY, valveCylinderOffsetZ));
    M = rotate(M, -valveAngle, vec3(0.0f, 0.0f, 1.0f));
    M = rotate (M, -60*PI/180.f, vec3(0.0f, 1.0f, 0.0f));
    M = scale(M, vec3(0.7f, 1.0f, 0.7f));

    glLoadMatrixf(value_ptr(V*M));
    drawObject(valveCylinderPositions, valveCylinderNormals, valveCylinderTexels, valveCylinderVertices, texNum);
}
//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window, float angle, float angle_x, float angle_y, float zoom)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); //Wyczyść bufor kolorów (czyli przygotuj "płótno" do rysowania)

    //***Przygotowanie do rysowania****
    mat4 P = perspective(50.0f*PI/180.0f,aspect,1.0f,50.0f); //Wylicz macierz rzutowania P
    mat4 V = lookAt(    //Wylicz macierz widoku
                  vec3(0.0f,0.0f, zoom - 6.0f),
                  vec3(0.0f,0.0f,0.0f),
                  vec3(0.0f,1.0f,0.0f));
    glMatrixMode(GL_PROJECTION); //Włącz tryb modyfikacji macierzy rzutowania
    glLoadMatrixf(value_ptr(P)); //Załaduj macierz rzutowania
    glMatrixMode(GL_MODELVIEW);  //Włącz tryb modyfikacji macierzy model-widok

    drawValves(V, angle, angle_x, angle_y, 2);
    drawPiston(V, angle, angle_x, angle_y, 0);
    drawConnectingRod(V, angle, angle_x, angle_y, 2);
    drawCrankshaft(V, angle, angle_x, angle_y, 0);
    drawSparkPlug(V, angle_x, angle_y, 0);
    drawCylinder(V, angle_x, angle_y, 1);
    dravValveCylinders(V, angle_x, angle_y, 0);

    glfwSwapBuffers(window); //Przerzuć tylny bufor na przedni
}

int main(void)
{
	GLFWwindow* window; //Wskaźnik na obiekt reprezentujący okno

	glfwSetErrorCallback(error_callback);//Zarejestruj procedurę obsługi błędów

	if (!glfwInit()) //Zainicjuj bibliotekę GLFW
    {
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(500, 500, "OpenGL", NULL, NULL);  //Utwórz okno 500x500 o tytule "OpenGL" i kontekst OpenGL.

	if (!window) //Jeżeli okna nie udało się utworzyć, to zamknij program
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Od tego momentu kontekst okna staje się aktywny i polecenia OpenGL będą dotyczyć właśnie jego.
	glfwSwapInterval(1); //Czekaj na 1 powrót plamki przed pokazaniem ukrytego bufora

	GLenum err;
	if ((err=glewInit()) != GLEW_OK) //Zainicjuj bibliotekę GLEW
    {
		fprintf(stderr, "Nie można zainicjować GLEW: %s\n", glewGetErrorString(err));
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Operacje inicjujące

    float angle = 0.0f;
    float angle_x = 0.0f; //Aktualny kąt obrotu obiektu wokół osi x
	float angle_y = 0.0f; //Aktualny kąt obrotu obiektu wokół osi y
	float zoom = 0.0f;
	glfwSetTime(0); //Wyzeruj timer

	//Główna pętla
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamknięte
	{
	    angle += 7;
	    if (angle > 4*360)
            angle = 0;

        angle_x += speed_x * glfwGetTime(); //Oblicz przyrost kąta obrotu i zwiększ aktualny kąt
        angle_y += speed_y * glfwGetTime(); //Oblicz przyrost kąta obrotu i zwiększ aktualny kąt
        if ((zoom < 2.7f || speed_zoom < 0) && (zoom > -3.2f || speed_zoom > 0)) //Sprawdź czy nie kamera nie jest za blisko lub za daleko modelu
            zoom += speed_zoom * glfwGetTime();

	    glfwSetTime(0); //Wyzeruj timer
		drawScene(window, angle, angle_x, angle_y, zoom); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

    glDeleteTextures(numTextures, metalTextures);
	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}
