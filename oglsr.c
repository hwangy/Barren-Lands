#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <setjmp.h>
#include "utils.h"

#define AXIS_X 0x01
#define AXIS_Y 0x02
#define AXIS_Z 0x04

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

//Error Handling
jmp_buf ex_buf__;
struct sigaction newAction, oldAction;
void catch_fault (int signum, siginfo_t* si, void* arg) {
	longjmp(ex_buf__, 1);
}

inline float toRadians(float degrees) {
	return degrees*(float)(M_PI/180.0f);
}

//Deep Matrix Methods --> OpenGL is column major, but through the use of right-handed operations,
//this can be ignored. Matrices are stored in general C/C++ row-major order.
//EX:
//	Normal left-handed operations
//	P*V*M*v1 = v2
//	Right-handed
//	v1*M*V*P = v2
//
//Also, uses the definitions for OpenGL perspective, translation, and rotational matrixes, as found on:
//http://www.opengl.org/sdk/docs/man2/xhtml/gluPerspective.xml
//http://www.opengl.org/sdk/docs/man2/xhtml/glTranslate.xml
//https://www.opengl.org/sdk/docs/man2/xhtml/glRotate.xml
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
void myRotate(matrix* m, float angle, void* mags) {
	/**
	 * So this is one of those things that are completely unnecessary.
	 * But. I felt the need to do it. For science. JK. But yeah. This 
	 * code tests to see whether the void* was originally a uint32_t
	 * or a matrix* through the use of sigaction signal catching and
	 * setjmp, which is a rough try/catch analogy
	 *
	 * Awkward side effect. SIGSEGV will no longer terminate the program,
	 * instead it will do something really tippy to the program. So on that note,
	 * don't segfault.
	 * */
	matrix* comp = (matrix*)mags;
	uint32_t flags;
	
	memset(&newAction, 0, sizeof(struct sigaction));
	sigemptyset(&newAction.sa_mask);
	newAction.sa_sigaction = catch_fault;
	newAction.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &newAction, NULL);
	
	if ( !setjmp(ex_buf__) ) {
		int val;
		val = ((matrix*)mags)->rm[0];
	} else {
		flags = *(uint32_t*)mags;
	
		comp = initMatrix(1, 3);
		if (flags & AXIS_X) comp->rm[0] = 1;
		if (flags & AXIS_Y) comp->rm[1] = 1;
		if (flags & AXIS_Z) comp->rm[2] = 1;
	}

	sigrelse(SIGSEGV);

	printf("%f, %f, %f\n", comp->rm[0], comp->rm[1], comp->rm[2]);

	/*float magnitude;
	if ((magnitude = mag(comp)) != 1) {
		//normalize
		comp->rm[0] /= magnitude;
		comp->rm[1] /= magnitude;
		comp->rm[2] /= magnitude;
	}

	float c = cos(angle*M_PI/180.0f);
	float s = sin(angle*M_PI/180.0f);
	float x = comp->rm[0], y = comp->rm[1], z = comp->rm[2]; //Stored for quick access

	matrix* r = initMatrix(4, 4);
	r->rm[0] = x*x*(1.0f - c) + c;	   r->rm[1] = x*y*(1.0f - c) - z*s;	r->rm[2] = x*z*(1.0f - c) + y*s;
	r->rm[4] = x*y*(1.0f - c) + z*s;   r->rm[5] = y*y*(1.0f - c) + c;	r->rm[6] = y*z*(1.0f - c) - x*s;
	r->rm[8] = x*z*(1.0f - c) - y*s;   r->rm[9] = y*z*(1.0f - c) + x*s;	r->rm[10] = z*z*(1.0f - c) + c;
	r->rm[15] = 1;

	return r;*/
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

	//Testing wierd myRotate
	uint32_t flag = AXIS_X | AXIS_Y;
	matrix* somematrix = initMatrix(1, 3);
	somematrix->rm[0] = 1; somematrix->rm[2] = 1;
	myRotate(initMatrix(4,4), 50.0f, (void*)&flag);
	myRotate(initMatrix(4,4), 50.0f, (void*)somematrix);

	
	//ALT MVP GENERATION
	GLfloat modelmatrix[16], perspectivematrix[16];
	const GLfloat identityMatrix[16] = IDENTITY_MATRIX4;
	memcpy(modelmatrix, identityMatrix, sizeof(GLfloat)*16);
	//perspective(perspectivematrix, 45.0, 1.0, 0.1, 100.0);

      	rotate(modelmatrix, (GLfloat)50 * -1.0, X_AXIS);
      	rotate(modelmatrix, (GLfloat)50 * 1.0, Y_AXIS);
      	rotate(modelmatrix, (GLfloat)50 * 0.5, Z_AXIS);
      	translate(modelmatrix, 0, 0, -5.0);

	/*matrix* modelmatrix = initIdentityMatrix(4);
	modelmatrix = rotate(modelmatrix, (float)50 * -1.0, */
	
	matrix* projection = initPerspectiveMatrix(45.0f, 1.0, 0.1f, 100.0f);
	printM(projection);
      	//multiply4x4(modelmatrix, perspectivematrix);
	multiply4x4(modelmatrix, (GLfloat*)projection->rm);			//VERIFIED: My projection matrix is all good
	//END ALT
	
	glClearColor(0, 0, 0, 1);
	setColorWhite();
	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &start);
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


