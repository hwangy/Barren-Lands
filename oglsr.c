#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>
#include <time.h>
#include <math.h>
#include "utils.h"

int dimX = 400, dimY = 400;
static SDL_Window* mainWindow;

struct timespec start = {.tv_sec = 0, .tv_nsec = 0};
struct timespec end = {.tv_sec = 0, .tv_nsec = 0};
struct timespec p;

static GLuint matrixId, uniColorId, vbo;

void setColorWhite();
void initGfx();
void drawBox(float x1, float y1, float x2, float y2);
void drawTriangle();

inline float toRadians(float degrees) {
	return degrees*(float)(M_PI/180.0f);
}

//Deep Matrix Methods --> OpenGL is column-major, I think
//Also, uses the definitions for OpenGL perspective and lookat matrixes, as found on:
//http://www.opengl.org/sdk/docs/man2/xhtml/gluPerspective.xml
//http://www.opengl.org/sdk/docs/man2/xhtml/gluLookAt.xml
typedef struct matrix{
	float* rm;
	int nr;
	int nc;
} matrix;

void zero(matrix* myMatrix) {
	//Not that it matters, column-major zeroing
	int r, c;
	for (r = 0; r < myMatrix->nr; r++) {
		for (c = 0; c < myMatrix->nc; c++) {
			myMatrix->rm[c + myMatrix->nc*r] = 0;
		}
	}
}
matrix* initMatrix(int nr, int nc) {
	matrix* myMatrix = malloc(sizeof(myMatrix));
	myMatrix->rm = malloc(sizeof(float)*nr*nc);
	myMatrix->nc = nc;
	myMatrix->nr = nr;
	zero(myMatrix);
	return myMatrix;
}
void freeMatrix(matrix* myMatrix) {
	free(myMatrix->rm);
	free(myMatrix);
}
//Performs cross products on 3 dim. vectors
matrix* cross(matrix* m1, matrix* m2) {
	matrix* r = initMatrix(1, 3);

	if (m1->nr != 1 || m2->nr != 1) return NULL;
	if (m2->nc != 3 || m2->nc != 3) return NULL;
	
	r->rm[0] = m1->rm[1]*m2->rm[2] - m1->rm[2]*m2->rm[1];	//i comp
	r->rm[1] = m1->rm[2]*m2->rm[0] - m1->rm[0]*m2->rm[2];	//j comp
	r->rm[2] = m1->rm[0]*m2->rm[1] - m1->rm[1]*m2->rm[0];	//k comp

	return r;
}
float mag(matrix* r) {
	if (r->nc != 3 || r->nr != 1) return -1.0f;
	else return sqrt(r->rm[0]*r->rm[0] + r->rm[1]*r->rm[1] + r->rm[2]*r->rm[2]);
}
matrix* matrixMult(matrix* m1, matrix* m2) {
	//m1->nc must be equal to m2->nr
	if (m1->nc != m2->nr) {
		printf("ERROR: MATRIX MULT DIM\n");
		return NULL;
	}
	matrix* re = initMatrix(m1->nr, m2->nc);

	int c, r;
	for (c = 0; c < re->nc; c++) {
		for (r = 0; r < re->nr; r++) {
			int c1;
			float sum = 0.0f;
			for (c1 = 0; c1 < m1->nc; c1++) sum += m1->rm[c1 + r*m1->nc]*m2->rm[c + c1*m2->nc];
			re->rm[c + r*re->nc] = sum;
		}
	}
	return re;
}
matrix* initIdentityMatrix(int dim) {
	matrix* re = initMatrix(dim, dim);

	int c;
	for (c = 0; c < dim; c++) re->rm[c + c*re->nc] = 1;

	return re;
}
matrix* myTranslate(matrix* m, int x, int y, int z) {
	matrix* i = initIdentityMatrix(4);
	i->rm[3] = x;
	i->rm[7] = y;
	i->rm[11] = z;
	
	return matrixMult(m, i);
}
matrix* initPerspectiveMatrix(float FoV, float aspect, /*Celine Dion*/float near, float far) {//where ever you are 
	matrix* myMatrix = initMatrix(4, 4);

	float f = 1/tan(toRadians(FoV/2.0f));
	myMatrix->rm[0] = (float)f/aspect;
	myMatrix->rm[5] = f;
	myMatrix->rm[10] = (float)(far + near)/(near - far);
	myMatrix->rm[14] = (float)(2*far*near)/(near - far);
	myMatrix->rm[11] = -1.0f;

	return myMatrix;
}
matrix* initLookMatrix(float eX, float eY, float eZ,	//eye x y z	(camera position)
		       float cX, float cY, float cZ,	//center x y z	(looking towards)
		       float uX, float uY, float uZ	//uppper x y z	(defines right side up, generally (0, 1, 0))
		   ) {
	matrix* myMatrix = initMatrix(4, 4);

	matrix* f, *up, *s, *u, *sU;
	f = initMatrix(1, 3); up = initMatrix(1, 3);
	sU = initMatrix(1, 3);

	float magF = sqrt((cX-eX)*(cX-eX) + (cY-eY)*(cY-eY) + (cZ-eZ)*(cZ-eZ));
	float magU = sqrt(uX*uX + uY*uY + uZ*uZ);
	f->rm[0] = (cX - eX)/magF;
	f->rm[1] = (cY - eY)/magF;
	f->rm[2] = (cZ - eZ)/magF;
	up->rm[0] = uX/magU;
	up->rm[1] = uY/magU;
	up->rm[2] = uZ/magU;

	s = cross(f, up);
	float sMag = mag(s);
	sU->rm[0] = s->rm[0]/sMag;
	sU->rm[1] = s->rm[1]/sMag;
	sU->rm[2] = s->rm[2]/sMag;
	u = cross(sU, f);

	myMatrix->rm[0] = s->rm[0];    myMatrix->rm[4] = s->rm[1];    myMatrix->rm[8] = s->rm[2];
	myMatrix->rm[1] = u->rm[0];    myMatrix->rm[5] = u->rm[1];    myMatrix->rm[9] = u->rm[2];
	myMatrix->rm[2] = -1*f->rm[0]; myMatrix->rm[6] = -1*f->rm[1]; myMatrix->rm[10]= -1*f->rm[2];
	myMatrix->rm[15] = 1;

	myMatrix = myTranslate(myMatrix, -1.0f*eX, -1.0f*eY, -1.0f*eZ);

	freeMatrix(f);  freeMatrix(u);
	freeMatrix(up);	freeMatrix(sU);
	freeMatrix(s);

	return myMatrix;
}
void printM(matrix* myMatrix) {
	int r, c;
	for (r = 0; r < myMatrix->nr; r++) {
		for (c = 0; c < myMatrix->nc; c++) {
			printf("%2.2f, ", myMatrix->rm[c + r*myMatrix->nc]);
		}
		puts("");
	}
}
void printLM(float* rm, int nr, int nc) {
	int r, c;
	for (r = 0; r < nr; r++) {
		for (c = 0; c < nc; c++) {
			printf("%2.2f, ", rm[c + r*nc]);
		}
		puts("");
	}
}

//End DMM
//End DMM
//End DMM

void myDrawScreen() {
	SDL_GL_SwapWindow(mainWindow);
	glClear(GL_COLOR_BUFFER_BIT);
}

void paint() {
	GLenum error = glGetError();
	if(error){
		puts((const char*)gluErrorString(error));
	}
	
	drawTriangle();
	//drawBox(0, 0, 1, 1);
	myDrawScreen();
}

int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	mainWindow = SDL_CreateWindow("SPACE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dimX, dimY, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	puts(SDL_GetError());
	SDL_ClearError();
	
	SDL_GLContext mainContext = SDL_GL_CreateContext(mainWindow);
	
	initGfx();
	
	//ALT MVP GENERATION
	GLfloat modelmatrix[16], perspectivematrix[16];
	const GLfloat identityMatrix[16] = IDENTITY_MATRIX4;
	memcpy(modelmatrix, identityMatrix, sizeof(GLfloat)*16);
	perspective(perspectivematrix, 45.0, 1.0, 0.1, 100.0);
	
      	rotate(modelmatrix, (GLfloat)50 * -1.0, X_AXIS);
      	rotate(modelmatrix, (GLfloat)50 * 1.0, Y_AXIS);
      	rotate(modelmatrix, (GLfloat)50 * 0.5, Z_AXIS);
      	translate(modelmatrix, 0, 0, -5.0);
	
	matrix* projection = initPerspectiveMatrix(45.0f, 1.0, 0.1f, 100.0f);
	printM(projection);
      	//multiply4x4(modelmatrix, perspectivematrix);
	multiply4x4(modelmatrix, (GLfloat*)projection->rm);			//VERIFIED: My projection matrix is all good
	//END ALT
	
	glClearColor(0, 0, 0, 1);
	setColorWhite();

	//TESTING Model-View-Projection Matrix
	//matrix* projection = initPerspectiveMatrix(45.0f, 1.0f, 0.1f, 100.0f);
	matrix* view       = initLookMatrix(0.0f, 0.0f, 5.0f,
				   	    0.0f, 0.0f, 0.0f,
				   	    0.0f, 1.0f, 0.0f
		  			   );
	matrix* model 	   = initIdentityMatrix(4);
	matrix* MVP	   = matrixMult(matrixMult(model, view), projection);		//NEXT: CHECK MATRIX MULT (Column vs. Row Major)
	puts("");printM(MVP);
	puts("");printLM(modelmatrix, 4, 4);

	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		//glUniformMatrix4fv(matrixId, 1, GL_FALSE, (GLfloat*)MVP->rm);
		glUniformMatrix4fv(matrixId, 1, GL_FALSE, modelmatrix);
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
	
	const GLchar* vertexPrg =
"#version 130\n"
"attribute vec3 pos;"
"uniform mat4 MVP;"
"void main(){"
"gl_Position = MVP * vec4(pos, 1.0);"
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

	GLint posAttrib = glGetAttribLocation(prgId, "pos");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);
	uniColorId = glGetUniformLocation(prgId, "uniColor");

	matrixId = glGetUniformLocation(prgId, "MVP");
	printf("Established POS ID: %d\n", (int)posAttrib);
	printf("Established COL ID: %d\n", (int)uniColorId);
	printf("Established MVP ID: %d\n", (int)matrixId);
}

void drawBox(float x1, float y1, float x2, float y2){
	float points[]={
		x1, y1, 0,
		x2, y1, 0,
		x2, y2, 0,
		x1, y2, 0
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STREAM_DRAW);
	glDrawArrays(GL_QUADS, 0, 4);
	
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


