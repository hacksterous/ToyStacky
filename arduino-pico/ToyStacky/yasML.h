//https://github.com/nhomble/yasML/tree/master
//(C) author nhomble and others
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define SUCCESS true
#define FAIL false

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
bool matscalar_multiplyd(Matrixd *m, double f);
bool matreduce(Matrix *m, int a, int b, ComplexDouble factor);
bool matreduced(Matrixd *m, int a, int b, double factor);
bool matequals(Matrix *m1, Matrix *m2);
bool matequalsd(Matrixd *m1, Matrixd *m2);
bool matclonemat(Matrix *m, Matrix *copy);
bool matclonematd(Matrixd *m, Matrixd *copy);
bool mattranspose(Matrix *m, Matrix* trans);
bool mattransposed(Matrixd *m, Matrixd *trans);
bool matmultiply(Matrix *m1, Matrix *m2, Matrix* product);
bool matmultiplyd(Matrixd *m1, Matrixd *m2, Matrixd* product);
bool matadd(Matrix *m1, Matrix *m2);
bool mataddd(Matrixd *m1, Matrixd *m2);
bool matsubtract(Matrix *m1, Matrix *m2);
bool matsubtractd(Matrixd *m1, Matrixd *m2);
bool matzero_vector(Matrix *);
bool matzero_vectord(Matrixd *);
bool matdeterminant(Matrix *m, ComplexDouble *det);
bool matdeterminantd(Matrixd *m, double* det);
bool mateigenvalues(Matrix *m, ComplexDouble *values);
bool mateigenvaluesd(Matrixd *m, double *values);

bool matrow_scalar_multiply(Matrix *m, int row, ComplexDouble factor);
bool matrow_scalar_multiplyd(Matrixd *m, int row, double factor);
ComplexDouble matvector_multiply(ComplexDouble *col, ComplexDouble *row, int length);
double matvector_multiplyd(double *col, double *row, int length);
void matvector_addition(ComplexDouble *v1, ComplexDouble *v2, int length);
double matvector_additiond(double *col, double *row, int length);
void matscalar_vector_multiplication(ComplexDouble factor, ComplexDouble *vector, int length);
void matscalar_vector_multiplicationd(double factor, double *vector, int length);
void matvector_subtraction(ComplexDouble *v1, ComplexDouble *v2, int length);
void matvector_subtractiond(double *v1, double *v2, int length);

/* return SUCCESS if there is at least one zero vector in the matrix */
bool matzero_vector(Matrix *m){
	int i, j, counter;
	if(m == NULL)
		return FAIL;
	for(i = 0; i < m->columns; i++){
		counter = 0;	
		for(j = 0; j < m->rows; j++){
			if(alm0(m->numbers[i][j]))
				counter++;
		}
		if(counter == m->columns)
			return SUCCESS;
	}
	return FAIL;
}

bool matzero_vectord(Matrixd *m){
	int i, j, counter;
	if(m == NULL)
		return FAIL;
	for(i = 0; i < m->columns; i++){
		counter = 0;	
		for(j = 0; j < m->rows; j++){
			if(m->numbers[i][j] == 0)
				counter++;
		}
		if(counter == m->columns)
			return SUCCESS;
	}
	return FAIL;
}

bool matbuild(Matrix* m, char* input){
	int r = 0;
	int c = 0;
    char num[SHORT_STRING_SIZE];
	bool success = false;

	while (*input) {
		if (*input == '{' || *input == '[') {
			input++;
			c = 0;
		}
		else if (*input == ']') {
			r++; //increment row number
			input++;
		}
		else if (*input == '}') {
			input++;
		}
		else {
            input = extract_realnum(input, num);
			if (*num != '\0') {
				double d;
				success = stringToDouble(num, &d);
				m->numbers[r][c] = makeComplex(d, 0);
				c++;//increment column number
				continue;
			}
            input = extract_complexnum(input, num);
			if (*num != '\0') {
				ComplexDouble cd;
				success = stringToComplex(num, &cd);
				m->numbers[r][c] = cd;
				c++;//increment column number
				continue;
			}
			//No support for bigints yet
			//input = extract_bignum(input, num);
			//if (*num != '\0') {
			//	char *rawnum;
			//	char* stop;
			//	rawnum = removeDblQuotes(num);
			//	double d = strtod(rawnum, &stop); //bignum to double
			//	if (errno != 0 || stop[0] != '\0') d = 0;
			//	m->numbers[r][c] = makeComplex(d, 0);
			//	c++;
			//	continue;
			//}
		}
		input = skip_whitespace(input);
	}
	m->rows = r;
	m->columns = c;
	return success;
}

bool matbuildd(Matrixd* m, char* input){
	int r = 0;
	int c = 0;
    char num[SHORT_STRING_SIZE];
	bool success = false;

	while (*input) {
		if (*input == '{' || *input == '[') {
			input++;
			c = 0;
		}
		else if (*input == ']') {
			r++; //increment row number
			input++;
		}
		else if (*input == '}') {
			input++;
		}
		else {
            input = extract_realnum(input, num);
			if (*num != '\0') {
				double d;
				success = stringToDouble(num, &d);
				m->numbers[r][c] = d;
				c++;//increment column number
				continue;
			}
			//No support for bigints yet
            //input = extract_bignum(input, num);
            //if (*num != '\0') {
		    //	char *rawnum;
		    //	char* stop;
		    //	rawnum = removeDblQuotes(num);
		    //	double d = strtod(rawnum, &stop); //bignum to double
		    //	if (errno != 0 || stop[0] != '\0') d = 0;
		    //	m->numbers[r][c] = d;
		    //	c++;
		    //	continue;
            //}
		}
		input = skip_whitespace(input);
	}
	m->rows = r;
	m->columns = c;
	return success;
}

/* enter 1s along the main diagonal */
bool identity(int length, Matrix* m){
	int i, j;
	m->rows = m->columns = length;
	for(i = 0; i < length; i++){
		for(j = 0; j < length; j++){
			if (j == i) (m->numbers[i])[j] = makeComplex(1, 0);
			else (m->numbers[i])[j] = makeComplex(0, 0);
		}
	}
	return true;
}

bool identityd(int length, Matrixd* m){
	int i, j;
	m->rows = m->columns = length;
	for(i = 0; i < length; i++){
		for(j = 0; j < length; j++){
			if (j == i) (m->numbers[i])[j] = 1;
			else (m->numbers[i])[j] = 0;
		}
	}
	return true;
}

/* print the matrix  */
bool print(Matrix *m){
	int i, j;
	if(m == NULL)
		return FAIL;
	for(i = 0; i < m->rows; i++){
		for(j = 0; j < m->columns; j++){
			printf("(%f %f)", m->numbers[i][j].real, m->numbers[i][j].imag);
		}
		printf("\n");
	}
	return SUCCESS;
}

bool printd(Matrixd *m){
	int i, j;
	if(m == NULL)
		return FAIL;
	for(i = 0; i < m->rows; i++){
		for(j = 0; j < m->columns; j++){
			printf("%lf ", m->numbers[i][j]);
		}
		printf("\n");
	}
	return SUCCESS;
}

bool matrixToString (Matrix *m, char* str, uint8_t precision, char* notationStr){
	char* copy = str;
	char buf[SHORT_STRING_SIZE];
	int i, j;
	if(m == NULL)
		return false;
	strcpy(copy++, "{");
	for(i = 0; i < m->rows; i++){
		if (i != 0) strcat(copy++, " ");
		strcat(copy++, "[");
		for(j = 0; j < m->columns; j++){
			if (alm0double(m->numbers[i][j].imag)) //pure imag
				sprintf(buf, "%.15g", m->numbers[i][j].real);
			else if (alm0double(m->numbers[i][j].real)) //pure imag
				sprintf(buf, "(%.15g)", m->numbers[i][j].imag);
			else
				sprintf(buf, "(%.15g %.15g)", m->numbers[i][j].real, m->numbers[i][j].imag);
			strcat(copy, buf);
			copy += strlen(buf);
			if (j != m->columns - 1) strcat(copy++, " ");
		}
		strcat(copy++, "]");
	}
	strcat(copy, "}");
	return true;
}

bool matrow_swap(Matrix *m, int a, int b){
	ComplexDouble temp;
	int i;
	if(m == NULL)
		return FAIL;
	if(m->rows <= a || m->rows <= b)
		return FAIL;	
	for(i = 0; i < m->columns; i++){
		temp = m->numbers[i][a];
		m->numbers[i][a] = m->numbers[i][b];
		m->numbers[i][b] = temp;	
	}		
	return SUCCESS;	
}

bool matrow_swapd(Matrixd *m, int a, int b){
	double temp;
	int i;
	if(m == NULL)
		return FAIL;
	if(m->rows <= a || m->rows <= b)
		return FAIL;	
	for(i = 0; i < m->columns; i++){
		temp = m->numbers[i][a];
		m->numbers[i][a] = m->numbers[i][b];
		m->numbers[i][b] = temp;	
	}		
	return SUCCESS;	
}


bool scalar_multiply(Matrix *m, ComplexDouble scalar){
	int i, j;
	if(m == NULL)
		return FAIL;
	for(i = 0; i < m->columns; i++){
		for(j = 0; j < m->rows; j++) {
			(m->numbers[i])[j] = cmul(scalar, (m->numbers[i])[j]);
		}
	}
	return SUCCESS;
}

/* reduce row b by factor*a  */
bool matreduce(Matrix *m, int a, int b, ComplexDouble factor){
	int i;
	if(m == NULL)
		return FAIL;
	if(m->rows < a || m->rows < b)
		return FAIL;
	for(i = 0; i < m->columns; i++){
		m->numbers[i][b]  = csub(m->numbers[i][b], cmul(factor,  m->numbers[i][a]));
	}

	return SUCCESS;
}

/* matrix m will become the identity so the caller must save their matrix themselves  */
bool matinversion(Matrix *m, Matrix* invert){
	int i, j, l;
	ComplexDouble factor;
	if(m == NULL)
		return false;
	if(m->columns != m->rows)
		return false;
	identity(m->rows, invert);

	/* reduce each of the rows to get a lower triangle */	
	for(i = 0; i < m->columns; i++){
		for(j = i + 1; j < m->rows; j++){
			if(alm0(m->numbers[i][i])){
				for(l = i + 1; l < m->rows; l++){
					if(!alm0(m->numbers[l][l])){
						matrow_swap(m, i, l);
						break;
					}
				}	
				continue;
			}
			factor = cdiv(m->numbers[i][j], m->numbers[i][i]);
			matreduce(invert, i, j, factor);
			matreduce(m, i, j, factor);
		}
	}
	/* now finish the upper triangle  */
	for(i = m->columns - 1; i > 0; i--){
		for(j = i-1; j>=0; j--){
			if(alm0(m->numbers[i][i]))
				continue;
			if(j == -1)
				break;
			factor = cdiv(m->numbers[i][j], m->numbers[i][i]);
			matreduce(invert, i, j, factor);
			matreduce(m, i, j, factor);
		}
	}
	/* scale everything to 1 */
	for(i = 0; i < m->columns; i++){
		if(alm0(m->numbers[i][i]))
			continue;
		factor = cdiv(makeComplex(1, 0), m->numbers[i][i]);
		matrow_scalar_multiply(invert, i, factor);
		matrow_scalar_multiply(m, i, factor);
	}
	return true;
}

/* reduce row b by factor*a  */
bool matreduced(Matrixd *m, int a, int b, double factor){
	int i;
	if(m == NULL)
		return FAIL;
	if(m->rows < a || m->rows < b)
		return FAIL;
	for(i = 0; i < m->columns; i++){
		m->numbers[i][b]  -= m->numbers[i][a]*factor;
	}
	return SUCCESS;
}

/* matrix m will become the identity so the caller must save their matrix themselves  */
bool matinversiond(Matrixd *m, Matrixd* invert){
	int i, j, l;
	double factor;
	if(m == NULL)
		return false;
	if(m->columns != m->rows)
		return false;
	identityd(m->rows, invert);
	/* reduce each of the rows to get a lower triangle */	
	for(i = 0; i < m->columns; i++){
		for(j = i + 1; j < m->rows; j++){
			if(m->numbers[i][i] == 0){
				for(l = i + 1; l < m->rows; l++){
					if(m->numbers[l][l] != 0){
						matrow_swapd(m, i, l);
						break;
					}
				}	
				continue;
			}
			factor = m->numbers[i][j]/(m->numbers[i][i]);
			matreduced(invert, i, j, factor);
			matreduced(m, i, j, factor);
		}
	}
	/* now finish the upper triangle  */
	for(i = m->columns - 1; i > 0; i--){
		for(j = i - 1; j >= 0; j--){
			if(m->numbers[i][i] == 0)
				continue;
			if (j == -1)
				break;
			factor = m->numbers[i][j]/(m->numbers[i][i]);
			matreduced(invert, i, j, factor);
			matreduced(m, i, j, factor);
		}
	}
	/* scale everything to 1 */
	for(i = 0; i < m->columns; i++){
		if(m->numbers[i][i]==0)
			continue;
		factor = 1/(m->numbers[i][i]);
		matrow_scalar_multiplyd(invert, i, factor);
		matrow_scalar_multiplyd(m, i, factor);
	}
	return true;
}

bool matrow_scalar_multiply(Matrix *m, int row, ComplexDouble factor){
	int i;
	if(m == NULL)
		return FAIL;
	if(m->rows <= row)
		return FAIL;
	for(i = 0; i < m->columns; i++)
		m->numbers[i][row] = cmul(factor, m->numbers[i][row]);
	return SUCCESS;
}

bool matrow_scalar_multiplyd(Matrixd *m, int row, double factor){
	int i;
	if(m == NULL)
		return FAIL;
	if(m->rows <= row)
		return FAIL;
	for(i = 0; i < m->columns; i++)
		m->numbers[i][row] *= factor;
	return SUCCESS;
}

bool matequals(Matrix *m1, Matrix *m2){
	int i, j;
	if(m1 == NULL || m2 == NULL)
		return FAIL;
	if(m1->columns != m2->columns || m1->rows != m2->rows)
		return FAIL;
	for(i = 0; i < m1->columns; i++){
		for(j = 0; j < m1->rows; j++){
			if(bcneq(m1->numbers[i][j], m2->numbers[i][j]))
				return FAIL;
		}
	}
	return SUCCESS;
}

bool matequalsd(Matrixd *m1, Matrixd *m2){
	int i, j;
	if(m1 == NULL || m2 == NULL)
		return FAIL;
	if(m1->columns != m2->columns || m1->rows != m2->rows)
		return FAIL;
	for(i = 0; i < m1->columns; i++){
		for(j = 0; j < m1->rows; j++){
			if(m1->numbers[i][j] != m2->numbers[i][j])
				return FAIL;
		}
	}
	return SUCCESS;
}

bool clonemat(Matrix *m, Matrix *copy){
	int i, j;
	copy->rows = m->rows;
	copy->columns = m->columns;
	for(i = 0; i < m->columns; i++)
		for(j = 0; j < m->rows; j++)
			copy->numbers[i][j] = m->numbers[i][j];
	return true;
}

bool clonematd(Matrixd *m, Matrixd *copy){
	int i, j;
	copy->rows = m->rows;
	copy->columns = m->columns;
	for(i = 0; i < m->columns; i++)
		for(j = 0; j < m->rows; j++)
			copy->numbers[i][j] = m->numbers[i][j];
	return true;
}

bool transpose(Matrix *m, Matrix* trans){
	int i, j;
	if(m == NULL)
		return false;
	for(i = 0; i < trans->columns; i++){
		for(j = 0; j < trans->rows; j++)
			trans->numbers[i][j] = m->numbers[j][i];
	}
	return true;	
}

bool transposed(Matrixd *m, Matrixd* trans){
	int i, j;
	if(m == NULL)
		return false;
	for(i = 0; i < trans->columns; i++){
		for(j = 0; j < trans->rows; j++)
			trans->numbers[i][j] = m->numbers[j][i];
	}
	return true;	
}

/* m1 x m2  */
bool multiply(Matrix *m1, Matrix *m2, Matrix* product){
	Matrix trans;
	int i, j;
	if(m1 == NULL || m2 == NULL)
		return false;
	if(m1->columns != m2->rows)
		return false;
	product->rows = m1->rows;
	product->columns = m2->columns;
	transpose(m1, &trans);
	for(i = 0; i < product->columns; i++){
		for(j = 0; j < product->rows; j++){
			product->numbers[i][j] = matvector_multiply(trans.numbers[j], m2->numbers[i], m2->rows);
		}
	}
	return true;
}

bool matmultiplyd(Matrixd *m1, Matrixd *m2, Matrixd* product){
	Matrixd trans;
	int i, j;
	if(m1 == NULL || m2 == NULL)
		return false;
	if(m1->columns != m2->rows)
		return false;
	product->rows = m1->rows;
	product->columns = m2->columns;
	transposed(m1, &trans);
	for(i = 0; i < product->columns; i++){
		for(j = 0; j < product->rows; j++){
			product->numbers[i][j] = matvector_multiplyd(trans.numbers[j], m2->numbers[i], m2->rows);
		}
	}
	return true;
}

/* v1 x v2  -- simply a helper function -- computes dot product between two vectors*/
ComplexDouble matvector_multiply(ComplexDouble *col, ComplexDouble *row, int length){
	ComplexDouble sum;
	int i;
	sum.real = sum.imag = 0;
	for(i = 0; i < length; i++){
		sum = cadd(sum, cmul(col[i], row[i]));
	}
	return sum;
}

double matvector_multiplyd(double *col, double *row, int length){
	double sum;
	int i;
	sum = 0;
	for(i = 0; i < length; i++){
		sum += col[i] * row[i];
	}
	return sum;
}

/* m1 += m2  */
bool add(Matrix *m1, Matrix *m2){
	int i, j;
	if(m1 == NULL || m2 == NULL)
		return FAIL;
	if(m1->rows != m2->rows || m1->columns != m2->columns)
		return FAIL;
	for(i = 0; i < m1->columns; i++){
		for(j = 0; j < m1->rows; j++)
			m1->numbers[i][j] = cadd(m1->numbers[i][j], m2->numbers[i][j]);
	}
	return SUCCESS;
}

bool subtract(Matrix *m1, Matrix *m2){
	int i, j;
	if(m1 == NULL || m2 == NULL)
		return FAIL;
	if(m1->rows != m2->rows || m1->columns != m2->columns)
		return FAIL;
	for(i = 0; i < m1->columns; i++){
		for(j = 0; j < m1->rows; j++)
			m1->numbers[i][j] = csub(m1->numbers[i][j], m2->numbers[i][j]);
	}
	return SUCCESS;
}

/* v1 *= v2  */
void scalar_vector_multiplication(ComplexDouble factor, ComplexDouble *vector, int length){
	int i;
	for(i = 0; i < length; i++)
		vector[i] = cmul(vector[i], factor);
}

/* v1 += v2  */
void matvector_addition(ComplexDouble *v1, ComplexDouble *v2, int length){
	int i;
	for(i = 0; i < length; i++){
		v1[i] = cadd(v1[i], v2[i]);
	}
}

void matvector_subtraction(ComplexDouble *v1, ComplexDouble *v2, int length){
	int i;
	for(i = 0; i < length; i++){
		v1[i] = csub(v1[i], v2[i]);
	}
}

bool matdeterminantd(Matrixd *m, double* det){
	Matrixd copy;
	int i, j;
	double factor;
	if(m == NULL)
		return false;
	if(m->columns != m->rows)
		return false;
	clonematd(m, &copy);
	*det = 1;

	/* reduce each of the rows to get a lower triangle */	
	for(i = 0; i < copy.columns; i++){
		for(j = i + 1; j < copy.rows; j++){
			if(copy.numbers[i][i] == 0) {
				continue;
			}
			factor = copy.numbers[i][j]/(copy.numbers[i][i]);
			matreduced(&copy, i, j, factor);
		}
	}
	for(i = 0; i < copy.columns; i++)
		*det *= copy.numbers[i][i];
	return true;
}

bool matdeterminant(Matrix *m, ComplexDouble *det){
	Matrix copy;
	int i, j;
	ComplexDouble factor;
	if(m == NULL)
		return false;
	if(m->columns != m->rows)
		return false;
	clonemat(m, &copy);
	*det = makeComplex(1, 0);

	/* reduce each of the rows to get a lower triangle */	
	for(i = 0; i < copy.columns; i++){
		for(j = i + 1; j < copy.rows; j++){
			if(alm0(copy.numbers[i][i])) {
				continue;
			}
			factor = cdiv(copy.numbers[i][j], copy.numbers[i][i]);
			matreduce(&copy, i, j, factor);
		}
	}
	for(i = 0; i < copy.columns; i++)
		*det = cmul(*det, copy.numbers[i][i]);
	return true;
}

/* Return an array of all of the possible eigenvalues */
bool eigenvalues(Matrix *m, ComplexDouble *values){
	ComplexDouble factor;
	Matrix red;
	int i, j, l;
	if(m == NULL)
		return false;
	if(m->rows != m->columns)
		return false;
	clonemat(m, &red);
	/* reduce each of the rows to get a lower triangle */	
	for(i = 0; i < red.columns; i++){
		for(j = i + 1; j < red.rows; j++){
			if(alm0(red.numbers[i][i])){
				for(l = i + 1; l < red.rows; l++){
					if(!alm0(red.numbers[l][l])){
						matrow_swap(&red, i, l);
						break;
					}
				}
				continue;
			}
			factor = cdiv(red.numbers[i][j], red.numbers[i][i]);
			matreduce(&red, i, j, factor);
		}
	}
	for(i = 0; i < red.columns; i++)
		values[i] = red.numbers[i][i];
	return true;
}

