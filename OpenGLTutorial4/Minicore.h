#pragma once
#include <GL/glew.h>
#define ERROR_LOG(ErrorMessage) std::cout << ErrorMessage << std::endl;glfwTerminate();exit(-1);
#define WINDOWWIDTH 1600
#define WINDOWHEIGHT 1200

typedef void(*CheckStatus)(GLuint, GLenum, GLint*);
typedef void(*ReadLog)(GLuint, GLsizei, GLsizei*, GLchar*);
