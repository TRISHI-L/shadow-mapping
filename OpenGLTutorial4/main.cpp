#include <iostream>
#include "Shader.h"
#include "Minicore.h"
#include "Windows.h"
#include "Tutorial.h"

float deltaTime = 0.0f;
int main()
{
	Tutorial tutorial("Tutorial4");
	tutorial.InitUI();
	tutorial.CreateShaders();
	tutorial.CreateObjects();
	float currentTime = 0.0f;
	float lastTime = 0.0f;

	while (!tutorial.IsWindowClosed())
	{
		currentTime = glfwGetTime();
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;
		tutorial.RenderScreen();
	}
	return 0;
}