#include "../bigint.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main() {
    bigint_t bigint;
    char hex_str[260];

	// Convert hex string back to bigint
	const char *hex_input = "h123456789abcdef1234567888563421431231";
	if (bigint_from_hex(hex_input, &bigint)) {
	    printf("Converted bigint from hex %s\n", hex_input);
	    bigint_print(&bigint);
	} else {
	    printf("Invalid hex string.\n");
	}
	
	char *decstr = "3247329846872531872763821693245703294732469328451542145198598";
	printf("Decimal: %s\n", decstr);
	if (!bigint_from_str (&bigint, decstr)) 
		printf ("Bad decimal big integer.\n");
	else {
	  printf("Converted bigint from decimal str %s\n", decstr);
	  bigint_print(&bigint);
	  bigint_to_hex(&bigint, hex_str);
	  printf("Hexadecimal: %s\n", hex_str);
	}

    const char *base_str = "h123456789abcdef";
    const char *exp_str = "hfedcba987654321";
    bigint_t exp;

	printf("Converted bigint from hex %s\n", base_str);
    bigint_from_hex(base_str, &bigint);
	bigint_print(&bigint);
	printf("Converted bigint from hex %s\n", exp_str);
    bigint_from_hex(exp_str, &exp);
	bigint_print(&exp);

    bigint_to_hex(&exp, hex_str);
	printf("Converted bigint exp to hex %s\n", hex_str);

    const char *x_str = "h123456789abcdef";
	printf("Converted bigint from hex %s\n", x_str);
    bigint_from_hex(x_str, &exp);
	bigint_print(&exp);
    bigint_to_hex(&exp, hex_str);
	printf("Converted bigint exp to hex %s\n", hex_str);

    return 0;

}
