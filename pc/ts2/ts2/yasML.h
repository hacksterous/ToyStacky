#ifndef __YASML_H__
#define __YASML_H__
#define SUCCESS true
#define FAIL false

#include "TS-core-math.h"
#include "TS-core-tokenize.h"
#include "TS-core-numString.h"

bool matbuild(Matrix* m, char* input);
bool matbuildd(Matrixd* m, char* input);
bool matidentity(int length, Matrix *m);
bool matidentityd(int length, Matrixd* m);
bool matinversion(Matrix *m, Matrix *invert);
bool matinversiond(Matrixd *m, Matrixd *invert);
bool matprint(Matrix *m);
bool matprintd(Matrixd *m);
bool matrow_swap(Matrix *m, int a, int b);
bool matrow_swapd(Matrixd *m, int a, int b);
bool matscalar_multiply(Matrix *m, ComplexDouble f);
bool matscalar_multiplyd(Matrixd *m, long double f);
bool matreduce(Matrix *m, int a, int b, ComplexDouble factor);
bool matreduced(Matrixd *m, int a, int b, long double factor);
bool matequals(Matrix *m1, Matrix *m2);
bool matequalsd(Matrixd *m1, Matrixd *m2);
bool matclonemat(Matrix *m, Matrix *copy);
bool matclonematd(Matrixd *m, Matrixd *copy);
bool mattranspose(Matrix *m, Matrix* trans);
bool mattransposed(Matrixd *m, Matrixd *trans);
bool matmultiply(Matrix *m1, Matrix *m2, Matrix* product);
bool matmultiplyd(Matrixd *m1, Matrixd *m2, Matrixd* product);
bool matadd(Matrix *m1, Matrix *m2);
bool matsubtract(Matrix *m1, Matrix *m2);
bool matzero_vector(Matrix *);
bool matzero_vectord(Matrixd *);
bool matdeterminant(Matrix *m, ComplexDouble *det);
bool matdeterminantd(Matrixd *m, long double* det);
bool mateigenvalues(Matrix *m, ComplexDouble *values);
bool mateigenvaluesd(Matrixd *m, long double *values);
bool matrow_scalar_multiply(Matrix *m, int row, ComplexDouble factor);
bool matrow_scalar_multiplyd(Matrixd *m, int row, long double factor);
ComplexDouble matvector_multiply(ComplexDouble *col, ComplexDouble *row, int length);
long double matvector_multiplyd(long double *col, long double *row, int length);
void matvector_addition(ComplexDouble *v1, ComplexDouble *v2, int length);
void matvector_additiond(long double *col, long double *row, int length);
void matscalar_vector_multiplication(ComplexDouble factor, ComplexDouble *vector, int length);
void matscalar_vector_multiplicationd(long double factor, long double *vector, int length);
void matvector_subtraction(ComplexDouble *v1, ComplexDouble *v2, int length);
void matvector_subtractiond(long double *v1, long double *v2, int length);
bool matprojection(Matrix *m, ComplexDouble *v, int length, ComplexDouble* proj);
bool matprojectiond(Matrixd *m, long double *v, int length, long double* proj);
bool matgram_schmidt(Matrix *m, Matrix *ortho);
bool matgram_schmidtd(Matrixd *m, Matrixd *ortho);
bool orthonormal_basis(Matrix *, Matrix *);
bool orthonormal_basisd(Matrixd *, Matrixd *);

bool matrixToString (Matrix *m, char* str, uint8_t precision, char* notationStr);
#define FAILMATVEC(failcondition) if (failcondition) return false;	

#endif
