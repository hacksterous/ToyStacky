#ifndef __TS_CORE_LLIST__
#define __TS_CORE_LLIST__
typedef struct Cnode {
	ComplexDouble num;
	struct Cnode* next;
} Cnode;


ComplexDouble nrpolysolve(unsigned int n, Cnode* poly, ComplexDouble trial);
bool polysolve(Machine* vm);

#endif
