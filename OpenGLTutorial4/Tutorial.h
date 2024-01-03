#pragma once
#include <memory>
#include <cmath>
#include "Camera.h"
#include <SOIL2/SOIL2.h>
#include <SOIL2/stb_image.h>

extern float deltaTime;
bool firstMouse = true;
GLfloat lastX = 0, lastY = 0;
glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
const unsigned int SCR_WIDTH = 1600, SCR_HEIGHT = 1200;

struct KeyRecord {
	int key;
	int action;
	double timestamp;
	glm::vec3 cameraPosition;  
	glm::vec3 cameraFront;     
};
std::vector<KeyRecord> keyRecordList;

class Tutorial : public Window {
public:
	Tutorial(const char* title) :Window(title)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
	}
	~Tutorial()
	{
		glDeleteVertexArrays(1, &cubeVAO);
		glDeleteVertexArrays(1, &cubeVBO);
		glDeleteVertexArrays(1, &planeVAO);
		glDeleteBuffers(1, &planeVBO);
	}
	inline void RenderScreen() override final
	{
		float currentFrame = static_cast<float>(glfwGetTime());

		glfwPollEvents();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// --------------------------------------------------------------
		// 1. render depth of scene to texture (from light's perspective)
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		//    Configure Shader And Matrices  
		DepthShader->UseProgram();
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		float near_plane = 1.0f, far_plane = 7.5f, view_half_size = 3.0f;
		lightProjection = glm::ortho(-view_half_size, view_half_size, -view_half_size, view_half_size, near_plane, far_plane);
		lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		lightSpaceMatrix = lightProjection * lightView;
		DepthShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
		//		激活纹理单元
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		//		渲染3D物体
		renderScene(DepthShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// reset viewport
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// --------------------------------------------------------------
		// 2. render scene as normal using the generated depth/shadow map  
		pShader->UseProgram();
		glm::mat4 projection = glm::perspective(glm::radians(pCamera->GetZoom()), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		//		set light uniforms
		pShader->SetMat4("projection", projection);
		pShader->SetMatrix4fv("view", pCamera->GetViewMartix());
		pShader->SetVec3("viewPos", pCamera->getPosition());
		pShader->SetVec3("lightPos", lightPos);
		pShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
		pShader->SetInt("diffuseTexture", 0);
		pShader->SetInt("shadowMap", 1);
		//		激活纹理单元0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		//		激活纹理单元1
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		//		渲染3D物体
		renderScene(pShader);

		// --------------------------------------------------------------
		// ps:render Depth map to quad for visual debugging
		//		Configure Shader And Matrices
		//debugDepthQuad->UseProgram();
		//debugDepthQuad->SetFloat("near_plane", near_plane);
		//debugDepthQuad->SetFloat("far_plane", far_plane);
		//debugDepthQuad->SetInt("depthMap", 0);
		//		激活纹理单元
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, depthMap);
		//		渲染3D物体
		//renderQuad();
		
		glfwSwapBuffers(window);
	}

	// renders the 3D scene:floor, cubes
	void renderScene(const std::unique_ptr<Shader>& shader)
	{
		// floor
		glm::mat4 model = glm::mat4(1.0f);
		shader->SetMat4("model", model);
		glBindVertexArray(planeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// cubes
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
		model = glm::scale(model, glm::vec3(0.5f));
		shader->SetMat4("model", model);
		renderCube();
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0));
		model = glm::scale(model, glm::vec3(0.5f));
		shader->SetMat4("model", model);
		renderCube();
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 2.0));
		model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
		model = glm::scale(model, glm::vec3(0.25));
		shader->SetMat4("model", model);
		renderCube();
	}
	void InitUI()
	{
		glfwSetKeyCallback(window, KeyCallback);
		glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
		glfwSetCursorPosCallback(window, MouseCallback);
		glfwSetMouseButtonCallback(window, MouseButtonCallback);
		//glfwSetReplayCallback(window, MouseButtonCallback);

	}
	void CreateObjects()
	{
		creatPlane();
		LoadTextureFromFile("res/image/woodshadow.jpg", woodTexture);
		createDepthTexture(depthMapFBO, depthMap);
	}
	void CreateShaders()
	{
		
		pShader = std::unique_ptr<Shader>(new Shader("res/shaders/shadowMapping.vs", "res/shaders/shadowMapping.fs"));
		DepthShader = std::unique_ptr<Shader>(new Shader("res/shaders/shadow_mapping_depth.vs", "res/shaders/shadow_mapping_depth.fs"));
		debugDepthQuad = std::unique_ptr<Shader>(new Shader("res/shaders/debug_quad.vs", "res/shaders/debug_quad_depth.fs"));

	}
	bool IsWindowClosed()
	{
		return glfwWindowShouldClose(window);
	}
private:
	GLuint planeVAO = -1;
	GLuint cubeVAO = -1;
	GLuint quadVAO = -1;
	GLuint cubeVBO = -1;
	GLuint planeVBO = -1;
	GLuint quadVBO = -1;
	GLuint depthMapFBO;//为渲染的深度贴图创建一个帧缓冲对象

	static bool keys[1024];//存储键盘的输入
	std::unique_ptr<Shader> pShader;//2-PASS正常渲染场景
	std::unique_ptr<Shader> DepthShader;//1-PASS,光视角下生成阴影贴图
	std::unique_ptr<Shader> debugDepthQuad;//把阴影贴图渲染显示在2D平面（对depth texture进行debugging）

	static::std::unique_ptr<Camera> pCamera;

	GLuint textureID;
	GLuint woodTexture;
	GLuint depthMap;

	void creatPlane() {
		float planeVertices[] = {
			// positions            // normals         // texcoords
			 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

			 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
			 25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
		};
		// plane VAO
		glGenVertexArrays(1, &planeVAO);
		glGenBuffers(1, &planeVBO);
		glBindVertexArray(planeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindVertexArray(0);
	}
	void renderCube()
	{
		// initialize (if necessary)
		if (cubeVAO == -1)
		{
			float vertices[] = {
				// back face
				-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
				 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
				 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
				 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
				-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
				-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
				// front face
				-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
				 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
				 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
				 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
				-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
				-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
				// left face
				-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
				-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
				-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
				-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
				-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
				-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
				// right face
				 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
				 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
				 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
				 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
				 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
				 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
				 // bottom face
				 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
				  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
				  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
				  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
				 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
				 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
				 // top face
				 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
				  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
				  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
				  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
				 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
				 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
			};
			glGenVertexArrays(1, &cubeVAO);
			glGenBuffers(1, &cubeVBO);
			// fill buffer
			glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			// link vertex attributes
			glBindVertexArray(cubeVAO);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}
		// render Cube
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
	}
	void renderQuad()
	{
		if (quadVAO == -1)
		{
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};
			// setup plane VAO
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}
	void LoadTextureFromFile(char const* path, GLuint& textureID) {
		glGenTextures(1, &textureID);
		//绑定纹理对象
		glBindTexture(GL_TEXTURE_2D, textureID);
		// 为当前绑定的纹理对象设置环绕、过滤方式
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		int width, height;
		unsigned char* image = SOIL_load_image(path, &width, &height, 0, SOIL_LOAD_RGBA);
		//使用前面载入的图片数据生成一个纹理
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		//为当前绑定的纹理自动生成所有需要的多级渐远纹理
		glGenerateMipmap(GL_TEXTURE_2D);
		//释放图像的内存
		SOIL_free_image_data(image);
		glBindTexture(GL_TEXTURE_2D, 0);

	}
	void createDepthTexture(GLuint& depthMapFBO, GLuint& depthMap) {
		glGenFramebuffers(1, &depthMapFBO);
		glGenTextures(1, &depthMap);
		//绑定纹理对象
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		//为当前绑定的纹理自动生成所有需要的多级渐远纹理
		// 为当前绑定的纹理对象设置环绕、过滤方式
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		
		// attach depth texture as FBO's depth buffer
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
		glDrawBuffer(GL_NONE);//显式告诉OpenGL我们不适用任何颜色数据进行渲染
		glReadBuffer(GL_NONE);//显式告诉OpenGL我们不适用任何颜色数据进行渲染
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		

	}
	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)//回调函数
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}


		if (key >= 0 && key < 1024) {
			if (action == GLFW_PRESS) {
				keys[key] = true;
			}
			else if (action == GLFW_RELEASE) {
				keys[key] = false;
			}
		}

		if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) {
			KeyRecord record = { GLFW_KEY_W, GLFW_PRESS, glfwGetTime(), pCamera->getPosition(), pCamera->getFront() };
			pCamera->ProcessKeyboard(FORWARD, deltaTime);
			keyRecordList.push_back(record);
			std::cout << "加入动作W" << std::endl;
		}
		if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) {
			KeyRecord record = { GLFW_KEY_S, GLFW_PRESS, glfwGetTime(), pCamera->getPosition(), pCamera->getFront() };
			pCamera->ProcessKeyboard(BACKWARD, deltaTime);
			keyRecordList.push_back(record);
			std::cout << "加入动作S" << std::endl;	
		}
		if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]) {
			KeyRecord record = { GLFW_KEY_A, GLFW_PRESS, glfwGetTime(), pCamera->getPosition(), pCamera->getFront() };
			pCamera->ProcessKeyboard(RIGHT, deltaTime);
			keyRecordList.push_back(record);
			std::cout << "加入动作A" << std::endl;
		}
		if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) {
			KeyRecord record = { GLFW_KEY_D, GLFW_PRESS, glfwGetTime(), pCamera->getPosition(), pCamera->getFront() };
			pCamera->ProcessKeyboard(LEFT, deltaTime);
			keyRecordList.push_back(record);
			std::cout << "加入动作D" << std::endl;
		}
	}
	static void MouseCallback(GLFWwindow* window, double xPos, double yPos) {
		if (firstMouse) {
			lastX = xPos;
			lastY = yPos;
			firstMouse = false;
		}
		GLfloat xOffset, yOffset;
		xOffset = xPos - lastX;
		yOffset = lastY - yPos;
		lastX = xPos;
		lastY = yPos;
		pCamera->ProcessMouseMove(xOffset, yOffset);

	}
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
			// 鼠标右键按下，启用八倍镜
			pCamera->isZoomed = true;
			std::cout << "Zoomed In" << std::endl;
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
			// 鼠标右键释放，关闭八倍镜
			pCamera->isZoomed = false;
			std::cout << "Zoomed Out" << std::endl;
		}
	}

	static void FramebufferSizeCallback(GLFWwindow
		* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}



};
std::unique_ptr<Camera> Tutorial::pCamera = std::make_unique <Camera>(Camera(glm::vec3(1.0f, 1.5f, 5.0f)));
bool Tutorial::keys[1024] = { false };