#include <stdint.h>
#include <setjmp.h>
#include <signal.h>

uint32_t AXIS_X;
uint32_t AXIS_Y;
uint32_t AXIS_Z;

typedef struct matrix{
	float* rm;
	int nr;
	int nc;
} matrix;

//Error Handling
jmp_buf ex_buf__;
struct sigaction newAction, oldAction;
void catch_fault (int signum, siginfo_t* si, void* arg);

//Conversions
inline float toRadians(float degrees);

//Matrix Methods
void zero(matrix* myMatrix);
matrix* initMatrix(int nr, int nc);
void freeMatrix(matrix* myMatrix);
matrix* cross(matrix* m1, matrix* m2);
float mag(matrix* r);
matrix* matrixMult(matrix* m1, matrix* m2);
matrix* initIdentityMatrix(int dim);
matrix* myTranslate(matrix* m, int x, int y, int z);
matrix* myRotate(matrix* m, float angle, void* mags);
matrix* initPerspectiveMatrix(float FoV, float aspect, /*Celine Dion*/float near, float far); //where ever you are 
void printM(matrix* myMatrix);
void printLM(float* rm, int nr, int nc);
