#include "stb_image.h"
#include "gl_utils.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <time.h>
#define GL_LOG_FILE "gl.log"
#include <iostream>
#include <vector>
#include "TileMap.h"
#include "DiamondView.h"
#include "ltMath.h"
#include <fstream>

using namespace std;

int g_gl_width = 800;
int g_gl_height = 800;

int columns = 9;
int rows = 9;

float tileWidth = (float)(g_gl_width / columns);
float tileHeight = (float)(g_gl_height / rows);

GLFWwindow *g_window = NULL;

// int loadTexture(unsigned int &texture, char *filename)
// {
// 	glGenTextures(1, &texture);
// 	glBindTexture(GL_TEXTURE_2D, texture);

// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

// 	GLfloat max_aniso = 0.0f;
// 	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
// 	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

// 	int width, height, nrChannels;

// 	unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
// 	if (data)
// 	{
// 		if (nrChannels == 4)
// 		{
// 			cout << "Alpha channel" << endl;
// 			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
// 		}
// 		else
// 		{
// 			cout << "Without Alpha channel" << endl;
// 			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
// 		}
// 		glGenerateMipmap(GL_TEXTURE_2D);
// 	}
// 	else
// 	{
// 		std::cout << "Failed to load texture" << std::endl;
// 	}
// 	stbi_image_free(data);
// }

bool load_texture(const char *file_name, GLuint *tex)
{
	int x, y, n;
	int force_channels = 4;

	glEnable(GL_TEXTURE_2D);

	unsigned char *image_data = stbi_load(file_name, &x, &y, &n, force_channels);

	if (!image_data)
	{
		fprintf(stderr, "ERROR: could not load %s\n", file_name);
		return false;
	}

	if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0)
	{
		fprintf(stderr, "WARNING: texture %s is not power-of-2 dimensions\n", file_name);
	}

	glGenTextures(1, tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat max_aniso = 0.0f;

	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

	return true;
}

int main()
{
	restart_gl_log();
	start_gl();

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS);

	GLuint tex;
	// loadTexture(tex, "terrain.png");

	load_texture("terrain.png", &tex);

	cout << "tileWidth: " << tileWidth << endl;
	cout << "tileHeight: " << tileHeight << endl;

	GLfloat vertices[] = {
		(tileWidth / 2), 0.0f, ((1 / columns) / 2), 0.0f,			  // top
		tileWidth, (tileHeight / 2), (1 / columns), ((1 / rows) / 2), // right
		0.0f, (tileHeight / 2), 0.0f, ((1 / rows) / 2),				  // left
		(tileWidth / 2), tileHeight, ((1 / columns) / 2), (1 / rows), // bottom
		tileWidth, (tileHeight / 2), (1 / columns), ((1 / rows) / 2), // right
		0.0f, (tileHeight / 2), 0.0f, ((1 / rows) / 2)				  // left
	};

	glm::mat4 projection = glm::ortho(0.0f, (float)g_gl_width, (float)g_gl_height, 0.0f, -1.0f, 1.0f);

	GLuint VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLfloat *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLfloat *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	char vertex_shader[1024 * 256];
	char fragment_shader[1024 * 256];

	parse_file_into_str("_geral_vs.glsl", vertex_shader, 1024 * 256);
	parse_file_into_str("_geral_fs.glsl", fragment_shader, 1024 * 256);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const GLchar *p = (const GLchar *)vertex_shader;
	glShaderSource(vs, 1, &p, NULL);
	glCompileShader(vs);

	int params = -1;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
		print_shader_info_log(vs);
		return 1;
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	p = (const GLchar *)fragment_shader;
	glShaderSource(fs, 1, &p, NULL);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
		print_shader_info_log(fs);
		return 1;
	}

	GLuint shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);

	glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: could not link shader programme GL index %i\n", shader_programme);
		return false;
	}

	while (!glfwWindowShouldClose(g_window))
	{
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glViewport(0, 0, g_gl_width * 2, g_gl_height * 2);

		glUseProgram(shader_programme);

		glBindVertexArray(VAO);

		glUniformMatrix4fv(glGetUniformLocation(shader_programme, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex);
		glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		glfwPollEvents();

		glfwSwapBuffers(g_window);
	}

	glfwTerminate();
	return 0;
}