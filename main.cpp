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
#include <chrono>
#include <thread>

using namespace std;

int g_gl_width = 1200;
int g_gl_height = 600;

enum TileType
{
	Hidden = 0,
	Grass = 1,
	Initial = 2,
	End = 3,
	Explosion = 4
};

enum ItemType
{
	Empty = 0,
	Healing = 1,
	Damage = 2,
	Key = 3,
	Block = 4
};

enum ItemStatus
{
	Invisible = 0,
	Visible = 1
};

const int timeDelay = 167;

const int columns = 9, rows = 9;

const int healingAmount = 3, damageAmount = 7, keyAmount = 1, maxHealth = 3;

int currentRow = rows - 1, currentColumn = 0;

int score = 0, health = maxHealth;

bool keyFound = false;

float charOffsetY = 0.0f, charOffsetX = 0.0f;
float stepsX = currentColumn, stepsY = currentRow;

float tileWidth = (float)(g_gl_width / columns);
float tileHeight = (float)(g_gl_height / rows);

int specialItemTimer = 2;

GLFWwindow *g_window = NULL;
glm::mat4 matrix = glm::mat4(1);

int map[rows][columns];
int items[rows][columns][2];

void generateItems(ItemType itemType, int amount)
{
	for (int i = 0; i < amount; i++)
	{
		int randomRow, randomColumn;

		do
		{
			randomRow = rand() % rows;
			randomColumn = rand() % columns;
		} while (items[randomRow][randomColumn][0] != ItemType::Empty);

		items[randomRow][randomColumn][0] = itemType;
	}
}

void clearItems()
{
	for (int row = 0; row < rows; row++)
	{
		for (int column = 0; column < columns; column++)
		{
			items[row][column][0] = ItemType::Empty;
			items[row][column][1] = ItemStatus::Invisible;
		}
	}

	items[rows - 1][0][0] = ItemType::Block;
	items[0][columns - 1][0] = ItemType::Block;
}

void generateMap()
{
	for (int row = 0; row < rows; row++)
	{
		for (int column = 0; column < columns; column++)
		{
			map[row][column] = TileType::Hidden;
		}
	}

	map[rows - 1][0] = TileType::Initial;
	map[0][columns - 1] = TileType::End;
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
		spriteOffsetY = 0.25f;
		break;
	case TileType::Initial:
		spriteOffsetY = 0.5f;
		break;
	case TileType::End:
		spriteOffsetY = 0.5f;
		break;
	case TileType::Explosion:
		spriteOffsetY = 0.75f;
		break;
	default:
		cout << "Tile type not found!";
		break;
	}
}

void getItemTexture(ItemType itemType, float &spriteOffsetY)
{
	switch (itemType)
	{
	case ItemType::Healing:
		spriteOffsetY = 0.333f;
		break;
	case ItemType::Damage:
		spriteOffsetY = 0.0f;
		break;
	case ItemType::Key:
		spriteOffsetY = 0.666f;
		break;
	default:
		cout << "Item type not found!";
		break;
	}
}

void showHiddenTile(int row, int column)
{
	if (map[row][column] == TileType::Hidden)
	{
		map[row][column] = TileType::Grass;
	}
}

void calculateDrawPosition(int column, int row, float &targetX, float &targetY)
{
	targetX = (column - row) * (tileWidth / 2.0f) + (g_gl_width / 2.0f) - (tileWidth / 2.0f);
	targetY = (column + row) * (tileHeight / 2.0f);
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
	clearItems();
	generateMap();

	generateItems(ItemType::Healing, healingAmount);
	generateItems(ItemType::Damage, damageAmount);
	generateItems(ItemType::Key, keyAmount);

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

void verifyItem()
{
	ItemType currentItem = (ItemType) items[currentRow][currentColumn][0];
	ItemStatus currentStatus = (ItemStatus) items[currentRow][currentColumn][1];

	if (currentStatus == ItemStatus::Visible)
	{
		specialItemTimer++;
	}

	if (currentItem == ItemType::Empty || specialItemTimer % timeDelay != 0)
	{
		return;
	}

	switch (currentItem)
	{
	case ItemType::Healing:
		health++;
		break;
	case ItemType::Damage:
		map[currentRow][currentColumn] = TileType::Explosion;
		health--;
		break;
	case ItemType::Key:
		keyFound = true;
		break;
	}

	items[currentRow][currentColumn][0] = ItemType::Empty;
	items[currentRow][currentColumn][1] = ItemStatus::Invisible;

	cout << "Health: " << health << endl;
	cout << "Key found: " << keyFound << endl;
}

void verifyWin()
{
	bool isWinner = keyFound && map[currentRow][currentColumn] == TileType::End;

	if (isWinner)
	{
		startNextLevel();
	}
}

bool isAlive()
{
	return health > 0;
}

void showInvisibleItem(int currentRow, int currentColumn)
{
	items[currentRow][currentColumn][1] = ItemStatus::Visible;
}

int main()
{
	srand(time(NULL));

	resetGame();

	restart_gl_log();
	start_gl();

	GLuint tex;
	loadTexture(tex, "tileset.png");

	GLuint tex2;
	loadTexture(tex2, "spritesheet.png");

	GLuint tex3;
	loadTexture(tex3, "items.png");

	GLuint tex4;
	loadTexture(tex4, "gameover.png");

	GLfloat tileVertices[] = {
		(tileWidth / 2.0f), 0.0f, 0.5f, 0.0f,				 // top
		tileWidth, (tileHeight / 2.0f), 1, (0.25f / 2.0f), // right
		0.0f, (tileHeight / 2.0f), 0.0f, (0.25f / 2.0f),	 // left
		(tileWidth / 2.0f), tileHeight, 0.5f, 0.25f,		 // bottom
		tileWidth, (tileHeight / 2.0f), 1, (0.25f / 2.0f), // right
		0.0f, (tileHeight / 2.0f), 0.0f, (0.25f / 2.0f)	 // left
	};

	GLfloat charVertices[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		65.0f, 0.0f, 0.03333333333f, 0.0f,
		0.0f, 106.25f, 0.0f, 0.25f,
		65.0f, 0.0f, 0.03333333333f, 0.0f,
		0.0f, 106.25f, 0.0f, 0.25f,
		65.0f, 106.25f, 0.03333333333f, 0.25f
	};

	GLfloat itemVertices[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		tileWidth, 0.0f, 1.0f, 0.0f,
		0.0f, tileHeight, 0.0f, (1.0f / 3.0f),
		tileWidth, 0.0f, 1.0f, 0.0f,
		0.0f, tileHeight, 0.0f, (1.0f / 3.0f),
		tileWidth, tileHeight, 1.0f, (1.0f / 3.0f)
	};

	GLfloat screenVertices[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		g_gl_width, 0.0f, 1.0f, 0.0f,
		0.0f, g_gl_height, 0.0f, 1.0f,
		g_gl_width, 0.0f, 1.0f, 0.0f,
		0.0f, g_gl_height, 0.0f, 1.0f,
		g_gl_width, g_gl_height, 1.0f, 1.0f
	};

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
	glBufferData(GL_ARRAY_BUFFER, sizeof(itemVertices), itemVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// health
	GLuint VBO4, VAO4;

	glGenVertexArrays(1, &VAO4);
	glGenBuffers(1, &VBO4);

	glBindVertexArray(VAO4);

	glBindBuffer(GL_ARRAY_BUFFER, VBO4);
	glBufferData(GL_ARRAY_BUFFER, sizeof(itemVertices), itemVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// key
	GLuint VBO5, VAO5;

	glGenVertexArrays(1, &VAO5);
	glGenBuffers(1, &VBO5);

	glBindVertexArray(VAO5);

	glBindBuffer(GL_ARRAY_BUFFER, VBO5);
	glBufferData(GL_ARRAY_BUFFER, sizeof(itemVertices), itemVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// game over
	GLuint VBO6, VAO6;

	glGenVertexArrays(1, &VAO6);
	glGenBuffers(1, &VBO6);

	glBindVertexArray(VAO6);

	glBindBuffer(GL_ARRAY_BUFFER, VBO6);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);

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

		glClearColor(1.00f, 0.98f, 0.74f, 1.0f);
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

				TileType tileType = (TileType) map[row][column];

				getTileTexture(tileType, spriteOffsetY);

				float targetX, targetY;
				calculateDrawPosition(column, row, targetX, targetY);

				matrix = glm::translate(glm::mat4(1), glm::vec3(targetX, targetY, 0));
				glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(matrix));

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, tex);

				glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
				glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_y"), spriteOffsetY);
				glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_x"), 0.0f);

				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
		}

		glBindVertexArray(VAO2);

		float charX, charY;

		if (isValidStep((int)stepsY, (int)stepsX) && items[currentRow][currentColumn][0] == ItemType::Empty)
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

		showInvisibleItem(currentRow, currentColumn);

		verifyItem();

		glBindVertexArray(VAO3);

		for (int row = 0; row < rows; row++)
		{
			for (int column = 0; column < columns; column++)
			{
				float spriteOffsetY = 0.0f;
				
				ItemType itemType = (ItemType) items[row][column][0];
				ItemStatus itemStatus = (ItemStatus) items[row][column][1];

				if (itemType == ItemType::Empty || itemType == ItemType::Block || itemStatus == ItemStatus::Invisible)
				{
					continue;
				}

				getItemTexture(itemType, spriteOffsetY);

				float targetX, targetY;
				calculateDrawPosition(column, row, targetX, targetY);

				matrix = glm::translate(glm::mat4(1), glm::vec3(targetX, targetY, 0));
				glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(matrix));

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, tex3);

				glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
				glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_y"), spriteOffsetY);
				glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_x"), 0.0f);

				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
		}
	
		glBindVertexArray(VAO4);

		float targetX = -(tileWidth/4.0f);

		for (int i = 0; i < health; i++)
		{
			float spriteOffsetY = 0.0f;
			
			getItemTexture(ItemType::Healing, spriteOffsetY);

			matrix = glm::translate(glm::mat4(1), glm::vec3(targetX, g_gl_height - 70, 0));
			glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(matrix));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex3);

			glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
			glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_y"), spriteOffsetY);
			glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_x"), 0.0f);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			targetX += (tileWidth / 2.5f);
		}

		glBindVertexArray(VAO5);

		if (keyFound)
		{
			float spriteOffsetY = 0.0f;
			
			getItemTexture(ItemType::Key, spriteOffsetY);

			matrix = glm::translate(glm::mat4(1), glm::vec3(g_gl_width - (tileWidth/1.35f), g_gl_height - 70, 0));
			glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(matrix));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex3);

			glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
			glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_y"), spriteOffsetY);
			glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_x"), 0.0f);

			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		glBindVertexArray(VAO6);

		if (!isAlive())
		{
			matrix = glm::translate(glm::mat4(1), glm::vec3(0, 0, 0));
			glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(matrix));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex4);

			glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
			glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_y"), 0.0f);
			glUniform1f(glGetUniformLocation(shader_programme, "sprite_offset_x"), 0.0f);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_X))
			{
				restartGame();
			}
		}

		verifyWin();

		glBindVertexArray(0);

		glfwSwapBuffers(g_window);
	}

	glfwTerminate();
	return 0;
}