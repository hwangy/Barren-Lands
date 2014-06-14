#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "matrix.h"

/*uint32_t AXIS_X = 0x01;
uint32_t AXIS_Y = 0x02;
uint32_t AXIS_Z = 0x04;*/

//Error Handling
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
	i->rm[12] = x;
	i->rm[13] = y;
	i->rm[14] = z;
	
	return matrixMult(m, i);
}
matrix* myRotate(matrix* m, float angle, void* mags) {
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
	
	float magnitude;
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

	r = matrixMult(m, r);
	
	return r;
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
