#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>
#include <time.h>

int dimX = 400, dimY = 400;
int frame = 0;
int ri = 100;
float r = 1.0, g = 1.0, b = 1.0, a = 1.0;
SDL_Renderer* render;
SDL_Texture* texture;
SDL_Window* mainWindow;
SDL_GLContext mainContext;

//GLuint vertexArrayID;
static GLuint vertexBuffer;

struct timespec start = {.tv_sec = 0, .tv_nsec = 0};
struct timespec end = {.tv_sec = 0, .tv_nsec = 0};
struct timespec p;

//Drawing
void initGL();
void drawTriangle();
void setColorWhite();

void act();
void sdldie(const char* msg);
void sdlerror(int line);

int main(int argc, char** argv) {
	//SDL init
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	mainWindow = SDL_CreateWindow("SPACE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dimX, dimY, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	puts(SDL_GetError());
	SDL_ClearError();
	if (!mainWindow) sdldie("Unable to create window...");
	//sdlerror(__LINE__);
	mainContext = SDL_GL_CreateContext(mainWindow);
	puts(SDL_GetError());
	SDL_ClearError();
	while (glGetError() != GL_NO_ERROR);
	
	initGL();
	glClearColor(0, 0, 0, 1);
	glUniform3f(uniColorId, 0.5, 0.5, 0.5);
	puts(SDL_GetError());
	SDL_ClearError();
	//sdlerror(__LINE__);

	
	//SDL_GL_SetSwapInterval(1);

	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		act();
		
		GLenum error = glGetError();
		if (error) {
			printf("%s\n", (const char*)gluErrorString(error));
			sdldie("\tGL ERROR");
		}
		SDL_ClearError();



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

void initGL() {
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK) printf("Error: %s\n", glewGetErrorString(err));

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
}


void drawTriangle() {
	float vertexData[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
	 	 0.0f,  1.0f, 0.0f
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STREAM_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	
	glMapBufferRange(GL_ARRAY_BUFFER, 0, 0, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT|GL_MAP_INVALIDATE_BUFFER_BIT);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	//glDisableVertexAttribArray(0);	
}

void setColorWhite(){glUniform3f(0, 0.5, 0.5, 0.5);}

void act() {
	drawTriangle();
	SDL_GL_SwapWindow(mainWindow);
	glClear(GL_COLOR_BUFFER_BIT);
}

void sdldie(const char* msg) {
	printf("%s: %s\n", msg, SDL_GetError());
	if (mainContext) SDL_GL_DeleteContext(mainContext);
	if (mainWindow) SDL_DestroyWindow(mainWindow);
	SDL_Quit();
	exit(1);
}

void sdlerror(int line) {
	const char* error = SDL_GetError();
	if (*error != '\0') {
		printf("SDL Error: %s\n", error);
		if (line != -1) printf(" + line: %i\n", line);
		SDL_ClearError();
	}
}
