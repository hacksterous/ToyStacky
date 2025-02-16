typedef struct Cnode {
	ComplexDouble num;
	struct Cnode* next;
} Cnode;

//insert a node at tail
Cnode* insertnode(Cnode* headp, ComplexDouble c) {
	// Create a new node
	Cnode* new_node = (Cnode*)malloc(sizeof(Cnode));
	if (new_node == NULL) {
		printf("Memory allocation failed\n");
		return headp;
	}
	new_node->num = c;
	new_node->next = NULL;

	// If the list is empty, the new node becomes the head
	if (headp == NULL) {
		return new_node;
	}

	// Traverse to the end of the list
	Cnode* current = headp;
	while (current->next != NULL) {
		current = current->next;
	}

	// Insert the new node at the end
	current->next = new_node;
	return headp;
}

//delete tail node
Cnode* deletenode(Cnode* headp) {
	// If the list is empty, return NULL
	if (headp == NULL) {
		return NULL;
	}

	// If the list has only one node, delete it and return NULL
	if (headp->next == NULL) {
		free(headp);
		return NULL;
	}

	// Traverse to the second-last node
	Cnode* current = headp;
	while (current->next->next != NULL) {
		current = current->next;
	}

	// Delete the tail node
	free(current->next);
	current->next = NULL;
	return headp;
}

ComplexDouble fnll (unsigned int n, Cnode* poly, ComplexDouble x){
	//polynomial of degree n
	ComplexDouble res = makeComplex(0, 0);
	int i = 0;
	Cnode* current = poly;
	while (current != NULL) {
		ComplexDouble power = makeComplex((long double)(n - i), 0);
		ComplexDouble factor = cpower(x, power);
		//printf("-------fnll-------\n");
		//printf("current->num = %Lg + i * %Lg\n", current->num.real, current->num.imag);
		//printf("x = %Lg + i * %Lg\n", x.real, x.imag);
		//printf("n = %d, i = %d\n", n, i);
		//printf("power = %Lg + i * %Lg\n", power.real, power.imag);
		//printf("factor = %Lg + i * %Lg\n", factor.real, factor.imag);
		res = cadd(res, cmul(current->num, factor));
		current = current->next;
		i++;
	}
	return res;
}

ComplexDouble dfnll (unsigned int n, Cnode* poly, ComplexDouble x){
	//polynomial of degree n, diff(poly) is a poly of degree (n - 1)
	ComplexDouble res = makeComplex(0, 0);
	int i = 0;
	Cnode* current = poly;
	while (current->next != NULL) {
		ComplexDouble power = makeComplex((long double)(n - 1 - i), 0);
		ComplexDouble coeff = makeComplex((long double)(n - i), 0);
		ComplexDouble factor = cpower(x, power);
		//printf("-------dfnll-------\n");
		//printf("current->num = %Lg + i * %Lg\n", current->num.real, current->num.imag);
		//printf("x = %Lg + i * %Lg\n", x.real, x.imag);
		//printf("n = %d, i = %d\n", n, i);
		//printf("power = %Lg + i * %Lg\n", power.real, power.imag);
		//printf("factor = %Lg + i * %Lg\n", factor.real, factor.imag);
		res = cadd(res, cmul(current->num, cmul(coeff, factor)));
		current = current->next;
		i++;
	}
	return res;
}

ComplexDouble fn (unsigned int n, ComplexDouble* polyp, ComplexDouble x){
	//polynomial of degree n
	ComplexDouble res = makeComplex(0, 0);
	for (unsigned int i = 0; i <= n; i++) {
		res = cadd(res, cmul(polyp[i], cpower(x, makeComplex((long double)(n - i), 0))));
	}
	return res;
}

ComplexDouble dfn (unsigned int n, ComplexDouble* polyp, ComplexDouble x){
	//polynomial of degree n, diff(poly) is a poly of degree (n - 1)
	ComplexDouble res = makeComplex(0, 0);
	for (unsigned int i = 0; i < n; i++) {
		res = cadd(res, cmul(polyp[i], cmul(makeComplex((long double)(n - i), 0), cpower(x, makeComplex((long double)(n - 1 - i), 0)))));
	}
	return res;
}

void printc(ComplexDouble c){
	printf("re = %Lg, im = %Lg\n", c.real, c.imag);
}

void printl(Cnode* headp) {
	Cnode* current = headp;
	while (current != NULL) {
		printf("(%Lf, %Lf) -> ", current->num.real, current->num.imag);
		current = current->next;
	}
	printf("\n");
}

bool searchnode(Cnode* headp, ComplexDouble c) {
	Cnode* current = headp;
	while (current != NULL) {
		long double diffr = fabsl(c.real - current->num.real);
		long double diffi = fabsl(c.imag - current->num.imag);
		//printf("searchnode got differences %Lg and %Lg\n", diffr, diffi);
		if (alm0double(diffr) && alm0double(diffi)) {
			//printf("searchnode returning true %Lg and %Lg\n", diffr, diffi);
			return true;
		}
		current = current->next;
	}
	//printf("searchnode returning false\n");
	return false;
}

bool catnode(Cnode* headp, char* str, uint8_t precision, char* notationStr) {
	Cnode* current = headp;
	strcpy(str, "[");
	char temp[SHORT_STRING_SIZE];
	bool success = true;
	while (current != NULL) {
		success = complexToString(current->num, temp, precision, notationStr) && success;
		strcat(str, temp);
		strcat(str, " ");
		current = current->next;
	}
	unsigned int len = strlen(str);
	str[len - 1] = ']'; //clobber the last space
	//printf("catnode: returning str = %s\n", str);
	return success;
}

//Newton-Raphson
ComplexDouble nrpolysolve(unsigned int n, Cnode* poly, ComplexDouble trial) {
	ComplexDouble x = trial;
	//ComplexDouble g = fnll(n, poly, x);
	ComplexDouble h = dfnll(n, poly, x);
	//printf("--- nrpolysolve n = %d g = %Lg + i * %Lg and h = %Lg + i * %Lg\n", n, g.real, g.imag, h.real, h.imag);

	h = cdiv(fnll(n, poly, x), h);

	int i = 0;
	while (abso(h) > DOUBLE_EPS) {
		h = dfnll(n, poly, x);
		h = cdiv(fnll(n, poly, x), h);
		if (i > 500) break;
		// x(i+1) = x(i) - f(x) / f'(x)
		x = csub(x,h);
		//printf("iter solution = %Lg + i * %Lg in iter %d h = %Lg + i * %Lg\n", x.real, x.imag, i, h.real, h.imag);
		i++;
	}
 
	if ((i > 500) && (abso(h) > DOUBLE_EPS))
		errno = 11001;
	//else 
		//printf("one solution = %.14Lg + i * %.14Lg in iter %d h = %.14Lg\n", x.real, x.imag, i, abso(h));
	if (fabsl(x.imag/x.real) < NRPOLYSOLV_EPS) return makeComplex(x.real, 0);
	else if (fabsl(x.real/x.imag) < NRPOLYSOLV_EPS) return makeComplex(0, x.imag);
	return x;
}

