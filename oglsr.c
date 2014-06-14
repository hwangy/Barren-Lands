#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>
#include <time.h>

#include "matrix.h"

int dimX = 400, dimY = 400;
static SDL_Window* mainWindow;

struct timespec start = {.tv_sec = 0, .tv_nsec = 0};
struct timespec end = {.tv_sec = 0, .tv_nsec = 0};
struct timespec p;

static GLuint matrixId, uniColorId, vertColorId, vbo, cbo;
GLint posAttrib, colAttrib;

void setColorWhite();
void initGfx();
void drawTriangle();
void drawCube();

void myDrawScreen();
void paint();

int main(int argc, char** argv) {
	AXIS_X = 0x01;
	AXIS_Y = 0x02;
	AXIS_Z = 0x04;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	mainWindow = SDL_CreateWindow("SPACE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dimX, dimY, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	puts(SDL_GetError());
	SDL_ClearError();
	
	SDL_GLContext mainContext = SDL_GL_CreateContext(mainWindow);
	
	initGfx();

	//Some custom MVP matrices
	matrix* testinput = initIdentityMatrix(4);
	testinput = myRotate(testinput, 50.0f * -1.0f, (void*)&AXIS_X);
	testinput = myRotate(testinput, 50.0f * -1.0f, (void*)&AXIS_Y);
	testinput = myRotate(testinput, 50.0f * 0.5f, (void*)&AXIS_Z);
	testinput = myTranslate(testinput, 0.0, 0.0, -5.0);
	
	matrix* projection = initPerspectiveMatrix(45.0f, 1.0, 0.1f, 100.0f);
	testinput = matrixMult(testinput, projection);
	
	glClearColor(0, 0, 0, 1);
	setColorWhite();
	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		glUniformMatrix4fv(matrixId, 1, GL_FALSE, (GLfloat*)testinput->rm);
		paint();
		
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				SDL_GL_DeleteContext(mainContext);
				SDL_DestroyWindow(mainWindow);
				SDL_Quit();
				return 0;
			}
		}
		
		clock_gettime(CLOCK_MONOTONIC, &end);
		long int sleep = 25000000 - (end.tv_nsec - start.tv_nsec);
		if (sleep > 0) {
			p.tv_nsec = sleep;
			nanosleep(&p, NULL);
		}
	}

	return 0;
}

void setColorWhite(){glUniform3f(uniColorId, 1, 1, 1);}

void initGfx(){
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		printf("Error: %s\n", glewGetErrorString(err));
	}

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	/*glGenBuffers(1, &cbo);
	glBindBuffer(GL_ARRAY_BUFFER, cbo);*/
	
	const GLchar* vertexPrg =
"#version 130\n"
"attribute vec3 pos;"
"uniform mat4 MVP;"
//"in vec3 vertexColor"
//"out vec3 fragmentColor"
"void main(){"
"gl_Position = MVP * vec4(pos, 1.0);"
//"fragmentColor = vertexColor"
"}";
	const GLchar* fragmentPrg =
"#version 130\n"
"uniform vec3 uniColor;"
"void main(){"
"gl_FragColor = vec4(uniColor, 1.0);"
"}";
	GLuint vertexPrgId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexPrgId, 1, &vertexPrg, NULL);
	glCompileShader(vertexPrgId);

	GLuint fragmentPrgId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentPrgId, 1, &fragmentPrg, NULL);
	glCompileShader(fragmentPrgId);

	GLuint prgId = glCreateProgram();
	glAttachShader(prgId, vertexPrgId);
	glAttachShader(prgId, fragmentPrgId);
	
	glLinkProgram(prgId);
	glUseProgram(prgId);

	GLint status;
	glGetShaderiv(vertexPrgId, GL_COMPILE_STATUS, &status);
	if(status!=GL_TRUE){
		puts("Vertex Shader Error\n");
		char buffer[512];
		glGetShaderInfoLog(vertexPrgId, 512, NULL, buffer);
		puts(buffer);
	}
	glGetShaderiv(fragmentPrgId, GL_COMPILE_STATUS, &status);
	if(status!=GL_TRUE){
		puts("Frag Shader ERROR\n");
		char buffer[512];
		glGetShaderInfoLog(fragmentPrgId, 512, NULL, buffer);
		puts(buffer);
	}

	posAttrib = glGetAttribLocation(prgId, "pos");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);
	uniColorId = glGetUniformLocation(prgId, "uniColor");
	
	/*vertColorId = glGetAttribLocation(prgId, "vertexColor");
	glEnableVertexAttribArray(vertColorId);*/
	
	matrixId = glGetUniformLocation(prgId, "MVP");
}

void drawTriangle() {
	float points[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 0.0f,  1.0f, 0.0f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STREAM_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void drawCube() {
	/*float colors[] = { 
		0.583f,  0.771f,  0.014f,
		0.609f,  0.115f,  0.436f,
		0.327f,  0.483f,  0.844f,
		0.822f,  0.569f,  0.201f,
		0.435f,  0.602f,  0.223f,
		0.310f,  0.747f,  0.185f,
		0.597f,  0.770f,  0.761f,
		0.559f,  0.436f,  0.730f,
		0.359f,  0.583f,  0.152f,
		0.483f,  0.596f,  0.789f,
		0.559f,  0.861f,  0.639f,
		0.195f,  0.548f,  0.859f,
		0.014f,  0.184f,  0.576f,
		0.771f,  0.328f,  0.970f,
		0.406f,  0.615f,  0.116f,
		0.676f,  0.977f,  0.133f,
		0.971f,  0.572f,  0.833f,
		0.140f,  0.616f,  0.489f,
		0.997f,  0.513f,  0.064f,
		0.945f,  0.719f,  0.592f,
		0.543f,  0.021f,  0.978f,
		0.279f,  0.317f,  0.505f,
		0.167f,  0.620f,  0.077f,
		0.347f,  0.857f,  0.137f,
		0.055f,  0.953f,  0.042f,
		0.714f,  0.505f,  0.345f,
		0.783f,  0.290f,  0.734f,
		0.722f,  0.645f,  0.174f,
		0.302f,  0.455f,  0.848f,
		0.225f,  0.587f,  0.040f,
		0.517f,  0.713f,  0.338f,
		0.053f,  0.959f,  0.120f,
		0.393f,  0.621f,  0.362f,
		0.673f,  0.211f,  0.457f,
		0.820f,  0.883f,  0.371f,
		0.982f,  0.099f,  0.879f
	};*/
	float points[] = {
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f
	};
	//glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STREAM_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, 12*3);
}

void myDrawScreen() {
	SDL_GL_SwapWindow(mainWindow);
	glClear(GL_COLOR_BUFFER_BIT);
}

void paint() {
	GLenum error = glGetError();
	if(error){
		puts((const char*)gluErrorString(error));
	}
	
	//drawTriangle();
	drawCube();
	myDrawScreen();
}
