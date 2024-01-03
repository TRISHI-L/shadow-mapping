#pragma once
#include <GL/glew.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Minicore.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
	Shader(const char* vertexPath, const char* fragmentPath)
	{
		ShaderProgram = glCreateProgram();
		if (ShaderProgram == 0)
		{
			std::cout << "Error creating shader program!" << std::endl;
			exit(-1);
		}
		CompileShaderFromFile(vertexPath, GL_VERTEX_SHADER);
		CompileShaderFromFile(fragmentPath, GL_FRAGMENT_SHADER);
		LinkShaderProgram();
	}
	~Shader()
	{
		glDeleteProgram(ShaderProgram);
	}

	
	/*
	Shader& GetShader()
	{
		return *this;
	}
	*/
	void UseProgram()
	{
		glUseProgram(ShaderProgram);
	}
	void SetBool(const std::string& name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ShaderProgram, name.c_str()), static_cast<int>(value));
	}
	void SetInt(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(ShaderProgram, name.c_str()), value);
	}
	void SetFloat(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(ShaderProgram, name.c_str()), value);
	}
	void Set4Float(const std::string& name, float value1, float value2, float value3, float value4) const
	{
		glUniform4f(glGetUniformLocation(ShaderProgram, name.c_str()), value1, value2, value3, value4);
	}
	void SetMatrix4fv(const std::string& name, glm::mat4 matrix) const
	{
		glUniformMatrix4fv(glGetUniformLocation(ShaderProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
	}
	void SetVec3(const std::string& name, float value1, float value2, float value3) const
	{
		glUniform3f(glGetUniformLocation(ShaderProgram, name.c_str()), value1, value2, value3);
	}
	void SetVec3(const std::string& name, const glm::vec3& value) const
	{
		glUniform3fv(glGetUniformLocation(ShaderProgram, name.c_str()), 1, &value[0]);
	}
	void SetMat4(const std::string& name, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(ShaderProgram, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
private:
	GLuint ShaderProgram;
	void CompileShaderFromFile(const char* path, GLenum shaderType)
	{
		std::string code;
		std::ifstream shaderFile;
		shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			shaderFile.open(path);
			std::stringstream shaderStream;
			shaderStream << shaderFile.rdbuf();
			shaderFile.close();
			code = shaderStream.str();
		}
		catch (std::ifstream::failure& e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
		}
		const GLchar* shaderCode = code.c_str();
		GLuint shader = glCreateShader(shaderType);
		glShaderSource(shader, 1, &shaderCode, NULL);
		glCompileShader(shader);
		CheckErrors(shader, "SHADER", GL_COMPILE_STATUS);
		glAttachShader(ShaderProgram, shader);
		glDeleteShader(shader);
	}
	void CheckErrors(GLuint obj, std::string type, int checkStatus )
	{
		auto f = [checkStatus, obj](CheckStatus f1, ReadLog f2) {GLint success; f1(obj, checkStatus, &success);
		if (!success) {
			GLchar InfoLog[1024];
			f2(obj, 1024, NULL, InfoLog);
			std::cout << "ERROR::SHADER::PROGRAM_FAILED\n" << InfoLog << std::endl;
		}};
		if (type == "SHADER")f(glGetShaderiv, glGetShaderInfoLog);
		else if (type == "PROGRAM")f(glGetProgramiv, glGetProgramInfoLog);
	}
	void LinkShaderProgram()
	{
		glLinkProgram(ShaderProgram);
		CheckErrors(ShaderProgram, "PROGRAM", GL_LINK_STATUS);
#ifndef __APPLE__
		glValidateProgram(ShaderProgram);
		CheckErrors(ShaderProgram, "PROGRAM", GL_VALIDATE_STATUS);
#endif // !__APPLE__
	}
};