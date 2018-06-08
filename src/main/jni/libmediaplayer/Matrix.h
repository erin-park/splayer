#ifndef FFMPEG_MATRIX_H
#define FFMPEG_MATRIX_H


#include <math.h>

inline
void mx4transform(float x, float y, float z, float w, const float* pM, float* pDest);

/**
 * Multiply two 4x4 matrices together and store the result in a third 4x4
 * matrix. In matrix notation: result = lhs x rhs. Due to the way
 * matrix multiplication works, the result matrix will have the same
 * effect as first multiplying by the rhs matrix, then multiplying by
 * the lhs matrix. This is the opposite of what you might expect.
 *
 * The same float array may be passed for result, lhs, and/or rhs. However,
 * the result element values are undefined if the result elements overlap
 * either the lhs or rhs elements.
 *
 * @param result The float array that holds the result.
 * @param resultOffset The offset into the result array where the result is
 *        stored.
 * @param lhs The float array that holds the left-hand-side matrix.
 * @param lhsOffset The offset into the lhs array where the lhs is stored
 * @param rhs The float array that holds the right-hand-side matrix.
 * @param rhsOffset The offset into the rhs array where the rhs is stored.
 *
 * @throws IllegalArgumentException if result, lhs, or rhs are null, or if
 * resultOffset + 16 > result.length or lhsOffset + 16 > lhs.length or
 * rhsOffset + 16 > rhs.length.
 */

#define I(_i, _j) ((_j) + 4*(_i))

void multiplyMM(float *r, int resultOffset,
		float *lhs, int lhsOffset, float *rhs, int rhsOffset);

/**
 * Transposes a 4 x 4 matrix.
 *
 * @param mTrans the array that holds the output inverted matrix
 * @param mTransOffset an offset into mInv where the inverted matrix is
 *        stored.
 * @param m the input array
 * @param mOffset an offset into m where the matrix is stored.
 */
void transposeM(float *mTrans, int mTransOffset, float *m, int mOffset);

/**
 * Inverts a 4 x 4 matrix.
 *
 * @param mInv the array that holds the output inverted matrix
 * @param mInvOffset an offset into mInv where the inverted matrix is
 *        stored.
 * @param m the input array
 * @param mOffset an offset into m where the matrix is stored.
 * @return true if the matrix could be inverted, false if it could not.
 */
bool invertM(float *mInv, int mInvOffset, float *m, int mOffset);

/**
 * Computes an orthographic projection matrix.
 *
 * @param m returns the result
 * @param mOffset
 * @param left
 * @param right
 * @param bottom
 * @param top
 * @param near
 * @param far
 */
void orthoM(float *m, int mOffset,
	float left, float right, float bottom, float top,
	float near, float far);

/**
 * Define a projection matrix in terms of six clip planes
 * @param m the float array that holds the perspective matrix
 * @param offset the offset into float array m where the perspective
 * matrix data is written
 * @param left
 * @param right
 * @param bottom
 * @param top
 * @param near
 * @param far
 */

void frustumM(float *m, int offset,
		float left, float right, float bottom, float top,
		float near, float far);

/**
 * Define a projection matrix in terms of a field of view angle, an
 * aspect ratio, and z clip planes
 * @param m the float array that holds the perspective matrix
 * @param offset the offset into float array m where the perspective
 * matrix data is written
 * @param fovy field of view in y direction, in degrees
 * @param aspect width to height aspect ratio of the viewport
 * @param zNear
 * @param zFar
 */
void perspectiveM(float *m, int offset,
	  float fovy, float aspect, float zNear, float zFar);

/**
 * Computes the length of a vector
 *
 * @param x x coordinate of a vector
 * @param y y coordinate of a vector
 * @param z z coordinate of a vector
 * @return the length of a vector
 */
float length(float x, float y, float z);

/**
 * Sets matrix m to the identity matrix.
 * @param sm returns the result
 * @param smOffset index into sm where the result matrix starts
 */
void setIdentityM(float *sm, int smOffset);

/**
 * Scales matrix  m by x, y, and z, putting the result in sm
 * @param sm returns the result
 * @param smOffset index into sm where the result matrix starts
 * @param m source matrix
 * @param mOffset index into m where the source matrix starts
 * @param x scale factor x
 * @param y scale factor y
 * @param z scale factor z
 */
void scaleM(float *sm, int smOffset,
		float *m, int mOffset,
		float x, float y, float z);

/**
 * Scales matrix m in place by sx, sy, and sz
 * @param m matrix to scale
 * @param mOffset index into m where the matrix starts
 * @param x scale factor x
 * @param y scale factor y
 * @param z scale factor z
 */
void scaleM(float *m, int mOffset,
		float x, float y, float z);

/**
 * Translates matrix m by x, y, and z, putting the result in tm
 * @param tm returns the result
 * @param tmOffset index into sm where the result matrix starts
 * @param m source matrix
 * @param mOffset index into m where the source matrix starts
 * @param x translation factor x
 * @param y translation factor y
 * @param z translation factor z
 */
void translateM(float *tm, int tmOffset,
		float *m, int mOffset,
		float x, float y, float z);

/**
 * Translates matrix m by x, y, and z in place.
 * @param m matrix
 * @param mOffset index into m where the matrix starts
 * @param x translation factor x
 * @param y translation factor y
 * @param z translation factor z
 */
void translateM(
		float *m, int mOffset,
		float x, float y, float z);

/**
 * Rotates matrix m by angle a (in degrees) around the axis (x, y, z)
 * @param rm returns the result
 * @param rmOffset index into rm where the result matrix starts
 * @param m source matrix
 * @param mOffset index into m where the source matrix starts
 * @param a angle to rotate in degrees
 * @param x scale factor x
 * @param y scale factor y
 * @param z scale factor z
 */
void rotateM(float *rm, int rmOffset,
		float *m, int mOffset,
		float a, float x, float y, float z);

/**
 * Rotates matrix m in place by angle a (in degrees)
 * around the axis (x, y, z)
 * @param m source matrix
 * @param mOffset index into m where the matrix starts
 * @param a angle to rotate in degrees
 * @param x scale factor x
 * @param y scale factor y
 * @param z scale factor z
 */
void rotateM(float *m, int mOffset,
		float a, float x, float y, float z);

/**
 * Rotates matrix m by angle a (in degrees) around the axis (x, y, z)
 * @param rm returns the result
 * @param rmOffset index into rm where the result matrix starts
 * @param a angle to rotate in degrees
 * @param x scale factor x
 * @param y scale factor y
 * @param z scale factor z
 */
void setRotateM(float *rm, int rmOffset,
		float a, float x, float y, float z);

/**
 * Converts Euler angles to a rotation matrix
 * @param rm returns the result
 * @param rmOffset index into rm where the result matrix starts
 * @param x angle of rotation, in degrees
 * @param y angle of rotation, in degrees
 * @param z angle of rotation, in degrees
 */
void setRotateEulerM(float *rm, int rmOffset,
		float x, float y, float z);

/**
 * Define a viewing transformation in terms of an eye point, a center of
 * view, and an up vector.
 *
 * @param rm returns the result
 * @param rmOffset index into rm where the result matrix starts
 * @param eyeX eye point X
 * @param eyeY eye point Y
 * @param eyeZ eye point Z
 * @param centerX center of view X
 * @param centerY center of view Y
 * @param centerZ center of view Z
 * @param upX up vector X
 * @param upY up vector Y
 * @param upZ up vector Z
 */
void setLookAtM(float *rm, int rmOffset,
		float eyeX, float eyeY, float eyeZ,
		float centerX, float centerY, float centerZ, float upX, float upY,
		float upZ);


#endif	//FFMPEG_MATRIX_H
