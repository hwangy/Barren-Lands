#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*#define GL3_PROTOTYPES 1*/
#include <GL/gl.h>
#include <SDL.h>
#include <math.h>

#include "utils.h"

/* Multiply 4x4 matrix m1 by 4x4 matrix m2 and store the result in m1 */
void multiply4x4(GLfloat *m1, GLfloat *m2)
{
    GLfloat temp[16];

    int x,y;

    for (x=0; x < 4; x++)
    {
        for(y=0; y < 4; y++)
        {
            temp[y + (x*4)] = (m1[x*4] * m2[y]) +
                              (m1[(x*4)+1] * m2[y+4]) +
                              (m1[(x*4)+2] * m2[y+8]) +
                              (m1[(x*4)+3] * m2[y+12]);
        }
    }

    memcpy(m1, temp, sizeof(GLfloat) << 4);
}

/* Generate a perspective view matrix using a field of view angle fov,
 * window aspect ratio, near and far clipping planes */
void perspective(GLfloat *matrix, GLfloat fov, GLfloat aspect, GLfloat nearz, GLfloat farz)
{
    GLfloat range;

    range = tan(fov * 0.00872664625) * nearz; /* 0.00872664625 = PI/360 */
    memset(matrix, 0, sizeof(GLfloat) * 16);
    matrix[0] = (2 * nearz) / ((range * aspect) - (-range * aspect));
    matrix[5] = (2 * nearz) / (2 * range);
    matrix[10] = -(farz + nearz) / (farz - nearz);
    matrix[11] = -1;
    matrix[14] = -(2 * farz * nearz) / (farz - nearz);
}

/* Perform translation operations on a matrix */
void translate(GLfloat *matrix, GLfloat x, GLfloat y, GLfloat z)
{
    GLfloat newmatrix[16] = IDENTITY_MATRIX4;

    newmatrix[12] = x;
    newmatrix[13] = y;
    newmatrix[14] = z;

    multiply4x4(matrix, newmatrix);
}

/* Rotate a matrix by an angle on a X, Y, or Z axis */
void rotate(GLfloat *matrix, GLfloat angle, AXIS axis)
{
    const GLfloat d2r = 0.0174532925199; /* PI / 180 */
    const int cos1[3] = { 5, 0, 0 };
    const int cos2[3] = { 10, 10, 5 };
    const int sin1[3] = { 6, 2, 1 };
    const int sin2[3] = { 9, 8, 4 };
    GLfloat newmatrix[16] = IDENTITY_MATRIX4;

    newmatrix[cos1[axis]] = cos(d2r * angle);
    newmatrix[sin1[axis]] = -sin(d2r * angle);
    newmatrix[sin2[axis]] = -newmatrix[sin1[axis]];
    newmatrix[cos2[axis]] = newmatrix[cos1[axis]];

    multiply4x4(matrix, newmatrix);
}

/* A simple function that will read a file into an allocated char pointer buffer */
char* filetobuf(char *file)
{
    FILE *fptr;
    long length;
    char *buf;

    fptr = fopen(file, "r"); /* Open file for reading */
    if (!fptr) /* Return NULL on failure */
        return NULL;
    fseek(fptr, 0, SEEK_END); /* Seek to the end of the file */
    length = ftell(fptr); /* Find out how many bytes into the file we are */
    buf = (char*)malloc(length + 1); /* Allocate a buffer for the entire length of the file plus a null terminator */
    fseek(fptr, 0, SEEK_SET); /* Go back to the beginning of the file */
    fread(buf, length, 1, fptr); /* Read the contents of the file in to the buffer */
    fclose(fptr); /* Close the file */
    buf[length] = 0; /* Null terminator */

    return buf; /* Return the buffer */
}

/* A simple function that prints a message, the error code returned by SDL, and quits the application */
void sdldie(char *msg)
{
    printf("%s: %s\n", msg, SDL_GetError());
    SDL_Quit();
    exit(1);
}

