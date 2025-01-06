#include "../bigint.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main() {
    char hex_str[260];
    bigint_t base, exp, mod;
	bigint_t result, x;
    const char *base_str = "24"; //"344543632412321435436435234329897837254625761715";
    const char *exp_str  = "8"; //"989873872536586779485732874576486487246";
    const char *mod_str  = "7"; //"343298792139812387198470932750932468322";

	bigint_from_str(&base, base_str);
	bigint_from_str(&exp, exp_str);
	bigint_from_str(&mod, mod_str);

	printf("%s\n", base_str);
	bigint_print(&base);
	printf("%s\n", exp_str);
	bigint_print(&exp);
	printf("%s\n", mod_str);
	bigint_print(&mod);

	printf("\n");
    bigint_gcd (&base, &mod, &result, &x);
    printf("Modular Exponentiation Result: ");
	bigint_print(&result);
	bigint_print(&x);

	bigint_t sh, div;
	bigint_from_str(&sh, "20000000000000");
    printf("sh bigint_print: ");
	bigint_print(&sh);
	printf("\n");
	bigint_from_uint32(&div, (uint32_t)4294967295UL); //2**32 - 1
	bigint_from_str(&div, "4294967295"); //2**32 - 1
    printf("div bigint loop: ");
	//bigint_print(&div);
	for (int i = 0; i < sh.length; i++) {
		printf("%u ", sh.data[i]);
	}
	printf("\n");
	bigint_div(&sh, &div, &result);
    printf("result bigint loop: ");
	for (int i = 0; i < result.length; i++) {
		printf("%u ", result.data[i]);
	}
	printf("\n");

    return 0;

}
