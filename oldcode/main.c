#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "draw/draw.h"
#include "time.h"
#include "SDL_gfxPrimitives_font.h"

int dimX = 800, dimY = 400;
int fieldX = 1000, fieldY = 1000;
int zFieldX = 1000, zFieldY = 400;
int baseX = 500, baseY = 500;
uint32_t* field1;
uint32_t* field2;
uint32_t* toDisplay;
SDL_Renderer* render;
SDL_Texture* texture;
char displayText[100];

int frameNum = 0;

int a = 0, w = 0, s = 0, d1 = 0;
int l = 0, u = 0, r = 0, d2 = 0;
int sh = 0, sp = 0;
int mX, mY;

struct Ship {
	int x;
	int y;
	int z;
	int vX, vY, vZ, vT, vP;

	int theta;
	int phi;

	int dim;	
};

typedef struct Ship Ship;

int viewAngle = 30;

const unsigned char* fontData = gfxPrimitivesFontdata;

struct timespec start = {.tv_sec = 0, .tv_nsec = 0};
struct timespec end = {.tv_sec = 0, .tv_nsec = 0};
struct timespec p;

Ship master = {.x = 500, .y = 500, .z = 200, .vX = 0, .vY = 0, .vT = 0, .vP = 0, .theta = 0, .phi = 0, .dim = 40};

void processKeys();
void drawSquare(uint32_t* mField, int x, int y, int dim, uint32_t color);
void updateValues(Ship* ship);
void act();

int main(int argv, char** argc) {
	//init SDL
	toDisplay = malloc(sizeof(uint32_t)*dimX*dimY);
	field1 = malloc(sizeof(uint32_t)*fieldX*fieldY);
	field2 = malloc(sizeof(uint32_t)*zFieldX*zFieldY);
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	SDL_Window* window = SDL_CreateWindow("SPACE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dimX, dimY, 0);
	render = SDL_CreateRenderer(window, -1, 0);
	texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, dimX, dimY);	

	//main loop
	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &start);
	
		SDL_Event event;
		SDL_GetMouseState(&mX, &mY);	
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				SDL_Quit();
				return 0;
			}

			if (event.type == SDL_KEYDOWN) {
				int val = event.key.keysym.sym;
				if (val == SDLK_a) a = 1;
				if (val == SDLK_w) w = 1;
				if (val == SDLK_s) s = 1;
				if (val == SDLK_d) d1 = 1;

				if (val == SDLK_LEFT) l = 1;
				if (val == SDLK_RIGHT) r = 1;
				if (val == SDLK_UP) u = 1;
				if (val == SDLK_DOWN) d2 = 1;

				if (val == SDLK_SPACE) sp = 1;
				if (val == SDLK_LSHIFT) sh = 1;
			} else if (event.type == SDL_KEYUP) {
				int val = event.key.keysym.sym;
				if (val == SDLK_a) a = 0;
				if (val == SDLK_w) w = 0;
				if (val == SDLK_s) s = 0;
				if (val == SDLK_d) d1 = 0;

				if (val == SDLK_LEFT) l = 0;
				if (val == SDLK_RIGHT) r = 0;
				if (val == SDLK_UP) u = 0;
				if (val == SDLK_DOWN) d2 = 0;

				if (val == SDLK_SPACE) sp = 0;
				if (val == SDLK_LSHIFT) sh = 0;
			}
		}

		processKeys();
		act();

		clock_gettime(CLOCK_MONOTONIC, &end);
		long int sleep = 25000000 - (end.tv_nsec - start.tv_nsec);
		if (sleep > 0) {
			p.tv_nsec = sleep;
			nanosleep(&p, NULL);
		}
		frameNum++;
	}	

	return 0;
}

void processKeys() {
	if (l && baseX > 0) baseX-=2;
	if (r && baseX < fieldX-dimX/2) baseX+=2;
	if (u && baseY > 0) baseY-=2;
	if (d2 && baseY < fieldY-dimY) baseY+=2;

	if (frameNum%2) {
		if (d1) master.vX++;
		if (a) master.vX--;
		if (s) master.vY++;
		if (w) master.vY--;
		if (sh) master.vZ++;
		if (sp) master.vZ--;
	}
}

void drawSquare(uint32_t* mField, int x, int y, int dim, uint32_t color) {
	drawLine(mField, fieldX, fieldY, x, y, x+dim, y, color);
	drawLine(mField, fieldX, fieldY, x, y, x, y+dim, color);
	drawLine(mField, fieldX, fieldY, x+dim, y, x+dim, y+dim, color);
	drawLine(mField, fieldX, fieldY, x, y+dim, x+dim, y+dim, color);
}

void updateValues(Ship* ship) {	
	if (ship->x + ship->vX < fieldX-ship->dim - 1 && ship->x + ship->vX > 0) ship->x += ship->vX;
	else if (ship->x + ship->vX > fieldX-ship->dim) {ship->x = fieldX-ship->dim-1;ship->vX = 0;}
	else if (ship->x + ship->vX < 1) {ship->x = 1;ship->vX = 0;}
	if (ship->y + ship->vY < fieldY-ship->dim - 1 && ship->y + ship->vY > 0) ship->y += ship->vY;
	else if (ship->y + ship->vY > fieldY-ship->dim) {ship->y = fieldY-ship->dim-1;ship->vY = 0;}
	else if (ship->y + ship->vY < 1) {ship->y = 1;ship->vY = 0;}
	if (ship->z + ship->vZ < zFieldY - 1 && ship->z + ship->vZ > 1) ship->z += ship->vZ;
	else if (ship->z + ship->vZ > zFieldY) {ship->z = zFieldY-1;ship->vZ = 0;}
	else if (ship->z + ship->vZ < 1) {ship->z = 1;ship->vZ = 0;}

	//ship->z += ship->vZ;
	ship->theta += ship->vT;
	ship->phi += ship->vP;
}

void act() {
	snprintf(displayText, 100, "CORNER: %d, %d", baseX, baseY);
	updateValues(&master);

	int i,j;
	for (i = 0; i < fieldX; i++) {
		for (j = 0; j < fieldY; j++) {
			field1[i+j*fieldX] = 0x000000;
			field2[i+j*fieldX] = 0x000000;
		}
	}

	//Temporary ship visualization
	//	I need to figure out how to properly
	//	visualize 3D in 2D SDL
	//
	//	The rate at which an image shortens, depends on how high
	//	the "viewer" is
	//
	//	field1 - The left side of the screen is XY depictment
	//	field2 - The right side of the screen Z depictment
	//UPDATE LEFT
	drawSquare(field1, master.x, master.y, master.dim, 0xFFFFFF);
	drawLine(field1, fieldX, fieldY, fieldX/2, 0, fieldX/2, fieldY-1, 0xFF0000);
	drawLine(field1, fieldX, fieldY, 0, fieldY/2, fieldX-1, fieldY/2, 0xFF0000);
	//UPDATE RIGHT
	drawLine(field2, zFieldX, zFieldY, master.x, zFieldY - master.z, master.x+master.dim, zFieldY - master.z, 0xFFFFFF);
	int len = ((float)(master.y)/fieldY)*(float)master.dim;
	int startX = master.x+(master.dim - len)/2;
	drawLine(field2, zFieldX, zFieldY, startX, zFieldY - master.z - master.dim, startX+len, zFieldY - master.z - master.dim, 0xFFFFFF);


	//PRINT TO SCREEN
	for (i = 0; i < dimX; i++) {
		for (j = 0; j < dimY; j++) {
			toDisplay[i+j*dimX] = (i < dimX/2)?field1[(i+baseX)+(j+baseY)*fieldX]:field2[(i+baseX-dimX/2)+(j)*fieldX];
		}
	}
	//DRAW TEXT
	drawText(toDisplay, dimX, dimY, fontData, displayText, 1, 0, 0, 0xFFFFFF);
	drawLine(toDisplay, dimX, dimY, 400, 0, 400, 400, 0xFFFFFF);
	SDL_UpdateTexture(texture, NULL, toDisplay, dimX*4);
	SDL_RenderClear(render);
	SDL_RenderCopy(render, texture, 0, 0);
	SDL_RenderPresent(render);
}
