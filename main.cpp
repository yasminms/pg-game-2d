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
#include <fstream>

using namespace std;

int g_gl_width = 1200;
int g_gl_height = 600;

enum TileType
{
	Hidden = 0,
	Grass = 1,
	Healing = 2,
	Damage = 3,
	Key = 4,
	Initial = 5,
	End = 6
};

const int columns = 9, rows = 9;

const int healingAmount = 3, damageAmount = 6, keyAmount = 1, maxHealth = 3;

int currentRow = rows - 1, currentColumn = 0;

int score = 0, health = maxHealth;

bool keyFound = false;

float charOffsetY = 0.0f, charOffsetX = 0.0f;
float stepsX = currentColumn, stepsY = currentRow;

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
			map[row][column][0] = TileType::Hidden; // tile type
			map[row][column][1] = TileType::Grass;	// hidden tile
		}
	}

	map[rows - 1][0][0] = TileType::Initial;
	map[rows - 1][0][1] = TileType::Initial;

	map[0][columns - 1][0] = TileType::End;
	map[0][columns - 1][1] = TileType::End;
}

bool isValidStep(int row, int column)
{
	return (row >= 0 && row < rows) && (column >= 0 && column < columns);
}

void getTileTexture(TileType tileType, float &spriteOffsetY)
{
	switch (tileType)
	{
	case TileType::Hidden:
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
	case TileType::Initial:
		spriteOffsetY = 0.8335f;
		break;
	case TileType::End:
		spriteOffsetY = 0.8335f;
		break;
	default:
		cout << "Tile type not found!";
		break;
	}
}

void showHiddenTile(int row, int column)
{
	if (map[row][column][0] == map[row][column][1])
	{
		return;
	}

	map[row][column][0] = map[row][column][1];
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

void resetGame()
{
	generateMap();

	generateSpecialTiles(TileType::Healing, healingAmount);
	generateSpecialTiles(TileType::Damage, damageAmount);
	generateSpecialTiles(TileType::Key, keyAmount);

	keyFound = false;
	health = maxHealth;

	currentRow = rows - 1;
	currentColumn = 0;

	stepsX = currentColumn;
	stepsY = currentRow;
}

void startNextLevel()
{
	cout << "Starting new level" << endl;

	resetGame();

	score++;

	cout << "Score: " << score << endl;
}

void restartGame()
{
	resetGame();

	score = 0;
}

void verifyTile()
{
	TileType currentTile = (TileType)map[currentRow][currentColumn][0];

	if (currentTile == TileType::Grass || currentTile == TileType::Initial)
	{
		return;
	}

	switch (currentTile)
	{
	case TileType::Healing:
		health++;
		break;
	case TileType::Damage:
		health--;
		break;
	case TileType::Key:
		keyFound = true;
		break;
	case TileType::End:
		if (keyFound)
		{
			startNextLevel();
		}
		return;
		break;
	}

	map[currentRow][currentColumn][0] = TileType::Grass;
	map[currentRow][currentColumn][1] = TileType::Grass;

	cout << "Health: " << health << endl;
	cout << "Key found: " << keyFound << endl;
}

bool isAlive()
{
	return health > 0;
}

int main()
{
	srand(time(NULL));

	resetGame();

	restart_gl_log();
	start_gl();

	GLuint tex;
	loadTexture(tex, "texture.png");

	GLuint tex2;
	loadTexture(tex2, "spritesheet-teste.png");

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
		65.0f, 0.0f, 0.03333333333f, 0.0f,
		0.0f, 106.25f, 0.0f, 0.25f,
		65.0f, 0.0f, 0.03333333333f, 0.0f,
		0.0f, 106.25f, 0.0f, 0.25f,
		65.0f, 106.25f, 0.03333333333f, 0.25f};

	GLfloat itemVertices[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		tileWidth, 0.0f, 0.03333333333f, 0.0f,
		0.0f, tileHeight, 0.0f, 0.25f,
		tileWidth, 0.0f, 0.03333333333f, 0.0f,
		0.0f, tileHeight, 0.0f, 0.25f,
		tileWidth, tileHeight, 0.03333333333f, 0.25f};

	glm::mat4 projection = glm::ortho(0.0f, (float)g_gl_width, (float)g_gl_height, 0.0f, -1.0f, 1.0f);

	// tilemap
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

	// character
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

	// items
	GLuint VBO3, VAO3;

	glGenVertexArrays(1, &VAO3);
	glGenBuffers(1, &VBO3);

	glBindVertexArray(VAO3);

	glBindBuffer(GL_ARRAY_BUFFER, VBO3);
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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_RIGHT))
		{
			stepsX += 0.05f;
			charOffsetX += (1.0f / 30.0f);
			charOffsetY = 0.50f;
		}
		else if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_LEFT))
		{
			stepsX -= 0.05f;
			charOffsetX += (1.0f / 30.0f);
			charOffsetY = 0.25f;
		}
		else if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_UP))
		{
			stepsY -= 0.05f;
			charOffsetX += (1.0f / 30.0f);
			charOffsetY = 0.75f;
		}
		else if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_DOWN))
		{
			stepsY += 0.05f;
			charOffsetX += (1.0f / 30.0f);
			charOffsetY = 0.0f;
		}
		else if (GLFW_RELEASE == glfwGetKey(g_window, GLFW_KEY_LEFT) && GLFW_RELEASE == glfwGetKey(g_window, GLFW_KEY_RIGHT) && GLFW_RELEASE == glfwGetKey(g_window, GLFW_KEY_UP) && GLFW_RELEASE == glfwGetKey(g_window, GLFW_KEY_DOWN))
		{
			charOffsetX = (1.0f / 30.0f) * 10;
		}

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

		if (isValidStep((int)stepsY, (int)stepsX))
		{
			currentColumn = (int)stepsX;
			currentRow = (int)stepsY;
		}
		else
		{
			stepsX = currentColumn;
			stepsY = currentRow;
		}

		showHiddenTile(currentRow, currentColumn);

		calculateDrawPosition(currentColumn, currentRow, charX, charY);

		matrix = glm::translate(glm::mat4(1), glm::vec3(charX + ((tileWidth / 4.2f)), charY - tileHeight, 1));
		glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(matrix));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex2);

		glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
		glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_y"), charOffsetY);
		glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_x"), charOffsetX);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		if (!isAlive())
		{
			char playAgain = 'n';

			cout << "\n-------- GAME OVER --------" << endl;
			cout << "> Score: " << score << endl;
			cout << "Restart game? y/n" << endl;

			cin >> playAgain;

			if (playAgain == 'y')
			{
				restartGame();
			}
			else
			{
				glfwSetWindowShouldClose(g_window, GLFW_TRUE);
			}
		}

		verifyTile();

		glBindVertexArray(0);

		glfwSwapBuffers(g_window);
	}

	glfwTerminate();
	return 0;
}