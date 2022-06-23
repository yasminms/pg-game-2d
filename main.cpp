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

int g_gl_width = 1200;
int g_gl_height = 600;

enum TileType
{
	Initial = 0,
	Grass = 1,
	Healing = 2,
	Damage = 3,
	Key = 4,
	Default = 5
};

const int columns = 9, rows = 9;

const int healingAmount = 3, damageAmount = 10, keyAmount = 1;

int health = 2, currentRow = 8, currentColumn = 0;

float charOffsetY = 0.0f, charOffsetX = 0.0f;

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

	map[rows - 1][0][0] = TileType::Default;
	map[rows - 1][0][1] = TileType::Default;

	map[0][columns - 1][0] = TileType::Default;
	map[0][columns - 1][1] = TileType::Default;
}

void getTileTexture(TileType tileType, float &spriteOffsetY)
{
	switch (tileType)
	{
	case TileType::Initial:
		spriteOffsetY = 0.0f;
		break;
	case TileType::Grass:
		spriteOffsetY = 0.1667f;
		break;
	case TileType::Healing:
		spriteOffsetY = 0.3334f;
		break;
	case TileType::Damage:
		spriteOffsetY = 0.5001f;
		break;
	case TileType::Key:
		spriteOffsetY = 0.6668f;
		break;
	case TileType::Default:
		spriteOffsetY = 0.8335f;
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

void showHiddenTile(int row, int column)
{
	map[row][column][0] = map[row][column][1];
}

void showTilePath(int sourceRow, int sourceColumn, int targetRow, int targetColumn)
{
	if (sourceRow < targetRow)
	{
		for (int row = sourceRow; row <= targetRow; row++)
		{
			showHiddenTile(row, sourceColumn);
		}
	}
	else
	{
		for (int row = sourceRow; row >= targetRow; row--)
		{
			showHiddenTile(row, sourceColumn);
		}
	}

	if (sourceColumn < targetColumn)
	{
		for (int column = sourceColumn; column <= targetColumn; column++)
		{
			showHiddenTile(targetRow, column);
		}
	}
	else
	{
		for (int column = sourceColumn; column >= targetColumn; column--)
		{
			showHiddenTile(targetRow, column);
		}
	}
}

void onMouseClick(double &mx, double &my)
{
	mx -= (g_gl_width / 2.0f) - (tileWidth / 2.0f);

	float x, y;
	int finalX, finalY;

	isometricToCartesian(mx, my, x, y);

	getTileCoordinates(x, y, finalX, finalY);

	if (finalY >= rows || finalX >= columns || finalY < 0 || finalX < 0)
	{
		return;
	}

	showHiddenTile(finalY, finalX);

	showTilePath(currentRow, currentColumn, finalY, finalX);

	currentRow = finalY;
	currentColumn = finalX;

	cout << "currentRow: " << currentRow << " - currentColumn: " << currentColumn << endl;
}

void calculateDrawPosition(int column, int row, float &targetx, float &targety)
{
	targetx = (column - row) * (tileWidth / 2.0f) + (g_gl_width / 2.0f) - (tileWidth / 2.0f);
	targety = (column + row) * (tileHeight / 2.0f);
}

int loadTexture(unsigned int &texture, char *filename)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
		cout << "Failed to load texture" << endl;
	}
	stbi_image_free(data);
}

int main()
{
	srand(time(NULL));

	generateMap();
	generateSpecialTiles(TileType::Healing, healingAmount);
	generateSpecialTiles(TileType::Damage, damageAmount);
	generateSpecialTiles(TileType::Key, keyAmount);

	restart_gl_log();
	start_gl();

	// glEnable(GL_DEPTH_TEST); // enable depth-testing
	// glDepthFunc(GL_LESS);

	GLuint tex;
	loadTexture(tex, "texture.png");

	GLuint tex2;
	loadTexture(tex2, "spritesheet.png");

	GLfloat tileVertices[] = {
		(tileWidth / 2.0f), 0.0f, 0.5f, 0.0f,				 // top
		tileWidth, (tileHeight / 2.0f), 1, (0.1667f / 2.0f), // right
		0.0f, (tileHeight / 2.0f), 0.0f, (0.1667f / 2.0f),	 // left
		(tileWidth / 2.0f), tileHeight, 0.5f, 0.1667f,		 // bottom
		tileWidth, (tileHeight / 2.0f), 1, (0.1667f / 2.0f), // right
		0.0f, (tileHeight / 2.0f), 0.0f, (0.1667f / 2.0f)	 // left
	};

	GLfloat charVertices[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		65.0f, 0.0f, 0.333f, 0.0f,
		0.0f, 106.25f, 0.0f, 0.25f,
		65.0f, 0.0f, 0.333f, 0.0f,
		0.0f, 106.25f, 0.0f, 0.25f,
		65.0f, 106.25f, 0.333f, 0.25f};

	glm::mat4 projection = glm::ortho(0.0f, (float)g_gl_width, (float)g_gl_height, 0.0f, -1.0f, 1.0f);

	GLuint VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tileVertices), tileVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLfloat *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLfloat *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	GLuint VBO2, VAO2;

	glGenVertexArrays(1, &VAO2);
	glGenBuffers(1, &VBO2);

	glBindVertexArray(VAO2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(charVertices), charVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(float)));
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

	bool pressedMouse = false;
	float stepsX = currentColumn, stepsY = currentRow;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		// double mx, my;
		// glfwGetCursorPos(g_window, &mx, &my);

		// const int state = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_LEFT);

		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_RIGHT))
		{
			stepsX += 0.05f;
			charOffsetX += (1.0f / 3.0f);
			charOffsetY = 0.50f;
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_LEFT))
		{
			stepsX -= 0.05f;
			charOffsetX += (1.0f / 3.0f);
			charOffsetY = 0.25f;
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_UP))
		{
			stepsY -= 0.05f;
			charOffsetX += (1.0f / 3.0f);
			charOffsetY = 0.75f;
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_DOWN))
		{
			stepsY += 0.05f;
			charOffsetX += (1.0f / 3.0f);
			charOffsetY = 0.0f;
		}
		if (GLFW_RELEASE == glfwGetKey(g_window, GLFW_KEY_LEFT) && GLFW_RELEASE == glfwGetKey(g_window, GLFW_KEY_RIGHT) && GLFW_RELEASE == glfwGetKey(g_window, GLFW_KEY_UP) && GLFW_RELEASE == glfwGetKey(g_window, GLFW_KEY_DOWN))
		{
			charOffsetX = (1.0f / 3.0f);
		}

		// if (state == GLFW_PRESS)
		// {
		// 	pressedMouse = true;
		// }
		// else if (state == GLFW_RELEASE && pressedMouse)
		// {
		// 	onMouseClick(mx, my);

		// 	pressedMouse = false;
		// }

		glClearColor(0.69f, 0.91f, 0.98f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glViewport(0, 0, g_gl_width * 2, g_gl_height * 2);

		glUseProgram(shader_programme);

		glBindVertexArray(0);

		glBindVertexArray(VAO);

		glUniformMatrix4fv(glGetUniformLocation(shader_programme, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		for (int row = 0; row < rows; row++)
		{
			for (int column = 0; column < columns; column++)
			{
				float spriteOffsetY = 0.0f;

				TileType tileType = (TileType)map[row][column][0];

				getTileTexture(tileType, spriteOffsetY);

				float targetx, targety;
				calculateDrawPosition(column, row, targetx, targety);

				matrix = glm::translate(glm::mat4(1), glm::vec3(targetx, targety, 0));
				glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(matrix));

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, tex);
				glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
				glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_y"), spriteOffsetY);
				glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_x"), 0.0f);

				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
		}

		glBindVertexArray(0);

		glBindVertexArray(VAO2);

		float charX, charY;

		currentColumn = (int)stepsX;
		currentRow = (int)stepsY;

		calculateDrawPosition(currentColumn, currentRow, charX, charY);

		matrix = glm::translate(glm::mat4(1), glm::vec3(charX + ((tileWidth / 3.5f)), charY - (tileHeight * 0.75f), 1));
		glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(matrix));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex2);

		glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
		glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_y"), charOffsetY);
		glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_x"), charOffsetX);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindVertexArray(0);

		glfwSwapBuffers(g_window);
	}

	glfwTerminate();
	return 0;
}