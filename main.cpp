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

int g_gl_width = 900;
int g_gl_height = 600;

enum TileType
{
	Initial = 0,
	Grass = 1,
	Healing = 2,
	Damage = 3,
	Spawn = 4,
	Key = 5,
	End = 6
};

const int columns = 9, rows = 9;

const int healingAmount = 2, damageAmount = 5, keyAmount = 1;

float tileWidth = (float)(g_gl_width / columns);
float tileHeight = (float)(g_gl_height / rows);

GLFWwindow *g_window = NULL;
glm::mat4 matrix = glm::mat4(1);

int map[rows][columns][2];

void generateSpecialTiles(TileType tileType, int amount)
{
	for (int i = 0; i < amount; i++)
	{
		int randomRow, randomColumn;

		do
		{
			randomRow = rand() % rows;
			randomColumn = rand() % columns;

		} while (map[randomRow][randomColumn][1] != TileType::Grass);

		map[randomRow][randomColumn][1] = tileType; // hidden tile
	}
}

void generateMap()
{
	for (int row = 0; row < rows; row++)
	{
		for (int column = 0; column < columns; column++)
		{
			map[row][column][0] = TileType::Initial; // tile type
			map[row][column][1] = TileType::Grass;	 // hidden tile
		}
	}

	map[0][0][0] = TileType::Spawn;
	map[rows - 1][columns - 1][0] = TileType::End;
}

void getTileTexture(TileType tileType, float &spriteOffsetY)
{
	switch (tileType)
	{
	case TileType::Initial:
		spriteOffsetY = 0.0f;
		break;
	case TileType::Grass:
		spriteOffsetY = 0.2f;
		break;
	case TileType::Healing:
		spriteOffsetY = 0.4f;
		break;
	case TileType::Damage:
		spriteOffsetY = 0.6f;
		break;
	default:
		cout << "Tile Type not found!";
		break;
	}
}

void cartesianToIsometric(float mx, float my, float &targetX, float &targetY)
{
	targetX = mx - my;
	targetY = (mx + my) / 2;
}

void isometricToCartesian(float mx, float my, float &targetX, float &targetY)
{
	targetX = (2 * my + mx) / 2;
	targetY = (2 * my - mx) / 2;
}

void getTileCoordinates(float mx, float my, int &targetX, int &targetY)
{
	targetX = floor(mx / tileHeight);
	targetY = floor(my / tileHeight);
}

void onMouseClick(double &mx, double &my)
{
	mx -= (g_gl_width / 2.0f) - (tileWidth / 2.0f);

	float x, y;
	int finalX, finalY;

	isometricToCartesian(mx, my, x, y);

	getTileCoordinates(x, y, finalX, finalY);

	map[finalY][finalX][0] = map[finalY][finalX][1];

	cout << "x: " << finalX << " - y: " << finalY << endl;
}

void calculateDrawPosition(int column, int row, float &targetx, float &targety)
{
	targetx = (column - row) * (tileWidth / 2.0f) + (g_gl_width / 2.0f) - (tileWidth / 2.0f);
	targety = (column + row) * (tileHeight / 2.0f);
	// targetx = column * (tileWidth / 2.0f) + row * (tileWidth / 2.0f);
	// targety = column * (tileHeight / 2.0f) - row * (tileHeight / 2.0f);
	// targety = column * (tileHeight / 2.0f) - row * (tileHeight / 2.0f) + (g_gl_height / 2.0f) - (tileHeight / 2.0f);
}

int loadTexture(unsigned int &texture, char *filename)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

	int width, height, nrChannels;

	unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
	if (data)
	{
		if (nrChannels == 4)
		{
			cout << "Alpha channel" << endl;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			cout << "Without Alpha channel" << endl;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
}

int main()
{
	srand(time(NULL));

	generateMap();
	generateSpecialTiles(TileType::Healing, healingAmount);
	generateSpecialTiles(TileType::Damage, damageAmount);

	restart_gl_log();
	start_gl();

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS);

	GLuint tex;
	loadTexture(tex, "tilemap.png");

	cout << "tileWidth: " << tileWidth << endl;
	cout << "tileHeight: " << tileHeight << endl;

	GLfloat vertices[] = {
		(tileWidth / 2.0f), 0.0f, 0.5f, 0.0f,			   // top
		tileWidth, (tileHeight / 2.0f), 1, (0.20f / 2.0f), // right
		0.0f, (tileHeight / 2.0f), 0.0f, (0.20f / 2.0f),   // left
		(tileWidth / 2.0f), tileHeight, 0.5f, 0.20f,	   // bottom
		tileWidth, (tileHeight / 2.0f), 1, (0.20f / 2.0f), // right
		0.0f, (tileHeight / 2.0f), 0.0f, (0.20f / 2.0f)	   // left
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
		glfwPollEvents();
		double mx, my;
		glfwGetCursorPos(g_window, &mx, &my);

		const int state = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_LEFT);

		if (state == GLFW_PRESS)
		{
			onMouseClick(mx, my);
		}

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glViewport(0, 0, g_gl_width * 2, g_gl_height * 2);

		glUseProgram(shader_programme);

		glBindVertexArray(VAO);

		glUniformMatrix4fv(glGetUniformLocation(shader_programme, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		for (int row = 0; row < rows; row++)
		{
			for (int column = 0; column < columns; column++)
			{
				float spriteOffsetY = 0.0f;

				TileType tileType = (TileType)map[row][column][1];

				getTileTexture(tileType, spriteOffsetY);

				float targetx, targety;
				calculateDrawPosition(column, row, targetx, targety);

				matrix = glm::translate(glm::mat4(1), glm::vec3(targetx, targety, 0));
				glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(matrix));

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, tex);
				glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
				glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_y"), spriteOffsetY);

				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
		}

		glBindVertexArray(0);

		glfwSwapBuffers(g_window);
	}

	glfwTerminate();
	return 0;
}