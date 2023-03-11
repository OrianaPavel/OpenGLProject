//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"
#include <iostream>
#include <fstream>
#include <cmath>


//std::ofstream myFile("input.txt");
std::ifstream myFile("input.txt");

int glWindowWidth = 800;
int glWindowHeight = 600;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
GLuint lightColorLoc;

gps::Camera myCamera(
				glm::vec3(0.0f, 0.2f, 0.2f), 
				glm::vec3(0.0f, 0.0f, -0.5f),
				glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.0015f;
float cameraAngle = 0.5f;

//keyboard + mouse controll
bool pressedKeys[1024];
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;
bool firstMouse = true;
bool nightTime = false;
GLfloat lightAngle;

float angleY;
gps::Model3D city;
gps::Model3D nanosuit;
gps::Model3D ground;
gps::Model3D lightCube;
gps::Model3D screenQuad;
gps::Model3D mainRotor;
gps::Model3D smallRotor;
gps::Model3D heliBody; 
gps::Model3D busStopGlass;

gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;


GLuint shadowMapFBO;
GLuint depthMapTexture;

bool showDepthMap;

gps::SkyBox mySkyBox , mySkyBoxDark;
gps::Shader skyboxShader;

std::vector<const GLchar*> faces;
std::vector<const GLchar*> facesDark;

float delta = 0;
double lastTimeStamp = glfwGetTime();
float rotationSpeed = 0.0f;


GLenum glCheckError_(const char *file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO	
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; 
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.05f; 
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;


	myCamera.rotate(pitch, yaw);
	
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	fov -= (float)yoffset;
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 45.0f)
		fov = 45.0f;
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}
/*
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

}*/

const float epsilon = 0.001f;
float heliXAxis = 0.0f;
float heliZAxis = 0.0f;
float heliYAxis = 0.0f;

float elicopterSpeed = 0.5f;


float inaltime = 0.0f;
bool writeToFile = false;
bool forward = false, back = false, right = false, left = false, camup = false, camdown = false, camright = false, camleft = false;
bool startAnim = false;
void processMovement()
{
	//writeToFile = false;
	//if (myFile.is_open()) {
	forward = false, back = false, right = false, left = false, camup = false, camdown = false, camright = false, camleft = false;
	std::string line;
		if (myFile.is_open()) {
			if (std::getline(myFile, line)) {
				for (char& c : line) {
					if (c == 'w') forward = true;
					if (c == 'a') left = true;
					if (c == 's') back = true;
					if (c == 'd') right = true;
					if (c == 'i') camup = true;
					if (c == 'j') camleft = true;
					if (c == 'k') camdown = true;
					if (c == 'l') camright = true;
				}
			}
		}
	heliYAxis = 0.0f;
	heliXAxis = 0.0f;
	heliZAxis = 0.0f;
	if (pressedKeys[GLFW_KEY_T]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	if (pressedKeys[GLFW_KEY_Y]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	/*
	if (pressedKeys[GLFW_KEY_V]) {
		startAnim = true;
	}
	*/
	if (pressedKeys[GLFW_KEY_G]) {
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_POLYGON_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	}

	if (pressedKeys[GLFW_KEY_H]) {
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	if (pressedKeys[GLFW_KEY_B]) {
		float fogDensity = 0.4f;
		myCustomShader.useShaderProgram();
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), fogDensity);
	}
	if (pressedKeys[GLFW_KEY_N]) {
		float fogDensity = 0.0f;
		myCustomShader.useShaderProgram();
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), fogDensity);
	}

	if (pressedKeys[GLFW_KEY_UP]) {
		heliYAxis = elicopterSpeed;
		/*
		myFile << 'i';
		writeToFile = true;
		pitch += cameraAngle;
		if (pitch > 89.0f)
			pitch = 89.0f;
		myCamera.rotate(pitch, yaw);*/
			
	}
	if (camup) {
		pitch += cameraAngle;
		if (pitch > 89.0f)
			pitch = 89.0f;
		myCamera.rotate(pitch, yaw); 
	}
	if (pressedKeys[GLFW_KEY_DOWN]) {
		heliYAxis = -elicopterSpeed;
		/*
		myFile << 'k';
		writeToFile = true;
		pitch -= cameraAngle;
		if (pitch < -89.0f)
			pitch = -89.0f;
		myCamera.rotate(pitch, yaw);*/
	}
	if (camdown) {
		pitch -= cameraAngle;
		if (pitch < -89.0f)
			pitch = -89.0f;
		myCamera.rotate(pitch, yaw); 
	}
	if (pressedKeys[GLFW_KEY_LEFT]) {
		heliXAxis -= elicopterSpeed;
		/*
		myFile << 'j';
		writeToFile = true;
		yaw -= cameraAngle;
		myCamera.rotate(pitch, yaw);*/
	}
	if (camleft) {
		yaw -= cameraAngle;
		myCamera.rotate(pitch, yaw); 
	}
	if (pressedKeys[GLFW_KEY_RIGHT]) {
		heliXAxis += elicopterSpeed;
		/*
		myFile << 'l';
		writeToFile = true;
		yaw += cameraAngle;
		myCamera.rotate(pitch, yaw);*/
	}
	if (camright) {
		yaw += cameraAngle;
		myCamera.rotate(pitch, yaw); 
	}
	if (pressedKeys[GLFW_KEY_Z]) {
		heliZAxis -= elicopterSpeed;
	}

	if (pressedKeys[GLFW_KEY_X]) {
		heliZAxis += elicopterSpeed;
	}

	if (pressedKeys[GLFW_KEY_W] || forward) {
		/*
		writeToFile = true;
		myFile << 'w';*/
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_S] || back) {
		/*
		writeToFile = true;
		myFile << 's';*/
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_A] || left) {
		/*
		writeToFile = true;
		myFile << 'a';*/
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_D] || right) {
		/*
		writeToFile = true;
		myFile << 'd';*/
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}
	if (pressedKeys[GLFW_KEY_O]) {
		lightColor = glm::vec3(0.15f, 0.15f, 0.15f);
		nightTime = true;
		myCustomShader.useShaderProgram();
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
	}
	if (pressedKeys[GLFW_KEY_P]) {
		lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		nightTime = false;
		myCustomShader.useShaderProgram();
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
	}
	/*
	if(writeToFile)
		myFile << '\n';*/
//}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouse_callback);
	glfwSetScrollCallback(glWindow, scroll_callback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3, 0.3, 0.3, 1.0);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	//glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glEnable(GL_FRAMEBUFFER_SRGB);

	
}

void initObjects() {

	lightCube.LoadModel("objects/cube/cube.obj");
	screenQuad.LoadModel("objects/quad/quad.obj");

	//city map
	city.LoadModel("objects/CityWithBusStop/CityWithBusStop.obj");
	//Rotors
	mainRotor.LoadModel("objects/eliceMare/eliceMare.obj");
	smallRotor.LoadModel("objects/eliceMica/eliceMica.obj");
	heliBody.LoadModel("objects/heliBody/heliBody.obj");

	busStopGlass.LoadModel("objects/BusStopGlass/BusStopGlass.obj");
}

void initShaders() {
	depthMapShader.loadShader("shaders/depthShader.vert", "shaders/depthShader.frag");
	depthMapShader.useShaderProgram();
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	lightShader.useShaderProgram();
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();

	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();


}

void initSkyBoxfaces() {

	//facesDark
	facesDark.push_back("textures/darkgloomSkyBox/right.tga");
	facesDark.push_back("textures/darkgloomSkyBox/left.tga");
	facesDark.push_back("textures/darkgloomSkyBox/top.tga");
	facesDark.push_back("textures/darkgloomSkyBox/bottom.tga");
	facesDark.push_back("textures/darkgloomSkyBox/back.tga");
	facesDark.push_back("textures/darkgloomSkyBox/front.tga");

	faces.push_back("textures/skybox/right.tga");
	faces.push_back("textures/skybox/left.tga");
	faces.push_back("textures/skybox/top.tga");
	faces.push_back("textures/skybox/bottom.tga");
	faces.push_back("textures/skybox/back.tga");
	faces.push_back("textures/skybox/front.tga");

	mySkyBox.Load(faces);
	skyboxShader.useShaderProgram();
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));


	//mySkyBoxDark
	mySkyBoxDark.Load(facesDark);
	skyboxShader.useShaderProgram();
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}



void initUniforms() {
	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	
	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");	
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	//lightColor = glm::vec3(0.2f, 0.2f, 0.2f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));


}

void initFBO() {
	//TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
	
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	//glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,0);

	//Deoarece prima trecere a algoritmului de mapare a umbrelor nu necesită atașament de coloare sau șablon,
	//dar un obiect framebuffer nu ar fi complet fără ele, putem să atribuim în mod explicit nimic acestor puncte
	//	de atașament :
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	// ar trebui să îl dezactivăm până când suntem gata să îl folosim
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	//TODO - Return the light-space transformation matrix
	//test
	//glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	//const GLfloat near_plane = -1.0f, far_plane = 500.0f;
	//glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
	

	//glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	//glm::mat4 lightView = glm::lookAt(lightDir, myCamera->getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));
	//glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

	// cod initial
	/////
	glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	const GLfloat near_plane = -0.1f, far_plane = 7.5f;
	glm::mat4 lightProjection = glm::ortho(-2.5f, 2.5f, -2.5f, 2.5f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
	
	return lightSpaceTrMatrix;
}


glm::vec3 PointLightPosition[] = {
		glm::vec3(-0.83f, 0.026f, -1.81f), //albastru
		glm::vec3(-0.72f, 0.026f, -1.778f), //rosu
		glm::vec3(-0.60f, 0.026f, -1.81f), //albastru
		glm::vec3(-0.54f, 0.026f, -1.91f), //rosu

		glm::vec3(-0.54f,  0.026f, -2.03f), //rosu
		glm::vec3(-0.6f,   0.026f, -2.12f), //albastru
		glm::vec3(-0.72f,  0.026f, -2.15f), //rosu
		glm::vec3(-0.83f,  0.026f, -2.12f), //albastru


		glm::vec3(-0.9f,   0.026f, -2.02f), //rosu
		glm::vec3(-0.9f,   0.026f, -1.9f) //rosu
};
glm::vec3 red = glm::vec3(1.0f, 0.0f, 0.0f), blue = glm::vec3(0.0f, 0.0f, 1.0f);

glm::vec3 pointLightColors[] = {
		glm::vec3(blue), //albastru
		glm::vec3(red), //rosu
		glm::vec3(blue), //albastru
		glm::vec3(red), //rosu

		glm::vec3(red), //rosu
		glm::vec3(blue), //albastru
		glm::vec3(red), //rosu
		glm::vec3(blue), //albastru


		glm::vec3(red), //rosu
		glm::vec3(red) //rosu
};
void setLights() {
	myCustomShader.useShaderProgram();
	for (int i = 0; i < 10; i++)
	{
		std::string number = std::to_string(i);

		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].position").c_str()), 1, glm::value_ptr(PointLightPosition[i]));
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].ambient").c_str()), 1, glm::value_ptr(pointLightColors[i]));
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].diffuse").c_str()), 1, glm::value_ptr(pointLightColors[i]));
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].specular").c_str()), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].constant").c_str()), 1.0f);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].linear").c_str()), 0.0f);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, ("pointLights[" + number + "].quadratic").c_str()), 1500.0f);
	}
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "viewPos"), 1, glm::value_ptr(myCamera.getCameraPosition()));
}
float incrementRotationSpeed = 20.0f;
float pragRotationSpeed = 80.0f;
float pragInaltime = 1.0f;

double firstTimeStamp = glfwGetTime();
double currentTimeStamp;


float actualHeliXAxis = 0.0f;
float actualHeliZAxis = 0.0f;
float actualHeliYAxis = 0.0f;
void updateDelta(double elapsedSeconds) {
	if (rotationSpeed < pragRotationSpeed || actualHeliYAxis < pragInaltime)
		rotationSpeed += elapsedSeconds * incrementRotationSpeed;
	if (rotationSpeed > pragRotationSpeed && actualHeliYAxis < pragInaltime) {
		if (heliYAxis > 0.0f || (actualHeliYAxis - heliYAxis * elapsedSeconds ) > epsilon)
			actualHeliYAxis += heliYAxis * elapsedSeconds;
	}
	if (actualHeliYAxis > 0.0f) {
		actualHeliXAxis += heliXAxis * elapsedSeconds;
		actualHeliZAxis += heliZAxis * elapsedSeconds;
	}
	if (actualHeliYAxis >= pragInaltime)
		rotationSpeed = pragRotationSpeed;
	delta += rotationSpeed * elapsedSeconds;
	lastTimeStamp = currentTimeStamp;
}

void drawObjects(gps::Shader shader, bool depthPass) {
		
	shader.useShaderProgram();
	
	glUniform1f(glGetUniformLocation(shader.shaderProgram, "transp"), 1.0f);
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	//nanosuit.Draw(shader);

	//model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//model = glm::scale(model, glm::vec3(0.5f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	//model = glm::mat4(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	setLights();
	//ground.Draw(shader);	

	city.Draw(shader);

	currentTimeStamp = glfwGetTime();
	updateDelta(currentTimeStamp - lastTimeStamp);
	glm::mat4 modelMainRotor = glm::mat4(1.0f);
	modelMainRotor = glm::translate(modelMainRotor, glm::vec3(0.0f, 0.0f, actualHeliZAxis));
	modelMainRotor = glm::translate(modelMainRotor, glm::vec3(actualHeliXAxis, 0.0f, 0.0f));
	modelMainRotor = glm::translate(modelMainRotor, glm::vec3(0.0f, actualHeliYAxis,0.0f));
	modelMainRotor = glm::translate(modelMainRotor, glm::vec3(-0.727558f, 0.138967, -1.98127f));
	modelMainRotor = glm::rotate(modelMainRotor, glm::radians(delta), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMainRotor = glm::translate(modelMainRotor, glm::vec3(0.727558f,-0.138967, 1.98127f));

	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMainRotor));

	mainRotor.Draw(shader);

	glm::mat4 modelSmallRotor = glm::mat4(1.0f);
	modelSmallRotor = glm::translate(modelSmallRotor, glm::vec3(0.0f, 0.0f, actualHeliZAxis));
	modelSmallRotor = glm::translate(modelSmallRotor, glm::vec3(actualHeliXAxis, 0.0f, 0.0f));
	modelSmallRotor = glm::translate(modelSmallRotor, glm::vec3(0.0f, actualHeliYAxis, 0.0f));
	modelSmallRotor = glm::translate(modelSmallRotor, glm::vec3(-0.937937f, 0.15725f, -2.19539f));
	modelSmallRotor = glm::rotate(modelSmallRotor, glm::radians(138.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelSmallRotor = glm::rotate(modelSmallRotor, glm::radians(delta), glm::vec3(0.0f, 0.0f, 1.0f));
	modelSmallRotor =  glm::rotate(modelSmallRotor, glm::radians(-138.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelSmallRotor = glm::translate(modelSmallRotor, glm::vec3(0.937937f, -0.15725, 2.19539f));
	
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelSmallRotor));
	smallRotor.Draw(shader);

	glm::mat4 modelHeli = glm::mat4(1.0f);
	modelHeli = glm::translate(modelHeli, glm::vec3(0.0f, 0.0f, actualHeliZAxis));
	modelHeli = glm::translate(modelHeli, glm::vec3(actualHeliXAxis, 0.0f, 0.0f));
	modelHeli = glm::translate(modelHeli, glm::vec3(0.0f, actualHeliYAxis, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelHeli));
	heliBody.Draw(shader);
	


	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	glUniform1f(glGetUniformLocation(shader.shaderProgram, "transp"), 0.5f);
	busStopGlass.Draw(shader);
	glDisable(GL_BLEND);
}




void renderScene() {

	// depth maps creation pass
	//TODO - Send the light-space transformation matrix to the depth map creation shader and
	//		 render the scene in the depth map

	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawObjects(depthMapShader, true);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	

	// render depth map on screen - toggled with the M key

	if (showDepthMap) {
		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT);

		screenQuadShader.useShaderProgram();

		//bind the depth map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
	}
	else {

		// final scene rendering pass (with shadows)

		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		myCustomShader.useShaderProgram();

		
		
		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
				
		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(computeLightSpaceTrMatrix()));

		drawObjects(myCustomShader, false);

		//draw a white cube around the light

		lightShader.useShaderProgram();

		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		model = lightRotation;
		model = glm::translate(model, 1.0f * lightDir);
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		lightCube.Draw(lightShader);
		/*
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-0.83f, -0.026f, -1.81f));
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		lightCube.Draw(lightShader);*/

		/*
		glassShader.useShaderProgram();
		glUniformMatrix4fv(glGetUniformLocation(glassShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		//glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glUniformMatrix4fv(glGetUniformLocation(glassShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
		busStopGlass.Draw(glassShader);*/
		//glEnable(GL_CULL_FACE);
		

		if(nightTime)
			mySkyBoxDark.Draw(skyboxShader, view, projection);
		else
			mySkyBox.Draw(skyboxShader, view, projection);

	

	}
	
}

void cleanup() {
	glDeleteTextures(1,& depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();
}

int main(int argc, const char * argv[]) {
	

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}

	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms();
	initFBO();
	initSkyBoxfaces();
	glCheckError();

	while (!glfwWindowShouldClose(glWindow)) {
		processMovement();
		renderScene();		

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	cleanup();
	//std::cin.get();
	return 0;
}
