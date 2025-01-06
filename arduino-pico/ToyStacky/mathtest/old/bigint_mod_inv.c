bool bigint_mod_inv(const bigint_t *x, const bigint_t *mod, bigint_t *res) {
    bigint_t t, new_t, r, new_r, quotient, temp1, temp2;

    // Initialize bigints
    memset(&t, 0, sizeof(t));
    memset(&new_t, 0, sizeof(new_t));
    memset(&r, 0, sizeof(r));
    memset(&new_r, 0, sizeof(new_r));
    memset(&quotient, 0, sizeof(quotient));
    memset(&temp1, 0, sizeof(temp1));
    memset(&temp2, 0, sizeof(temp2));

    t.data[0] = 0;
    t.length = 1;
    new_t.data[0] = 1;
    new_t.length = 1;

    memcpy(&r, mod, sizeof(bigint_t));
    memcpy(&new_r, x, sizeof(bigint_t));

    while (!bigint_is_zero(&new_r)) {
        bigint_div(&r, &new_r, &quotient);
        bigint_mul(&quotient, &new_r, &temp1);
        bigint_sub(&r, &temp1, &temp2);
        memcpy(&r, &new_r, sizeof(bigint_t));
        memcpy(&new_r, &temp2, sizeof(bigint_t));

        bigint_mul(&quotient, &new_t, &temp1);
        bigint_sub(&t, &temp1, &temp2);
        memcpy(&t, &new_t, sizeof(bigint_t));
        memcpy(&new_t, &temp2, sizeof(bigint_t));
    }

    if (bigint_cmp(&r, &temp1) > 1) {
        return false; // No inverse exists
    }

    if (t.negative) {
        bigint_add(&t, mod, res);
    } else {
        memcpy(res, &t, sizeof(bigint_t));
    }

    return true;
}

int main() {
    // Example usage of modular exponentiation
    bigint_t base, exp, mod, result;
    const char *base_str = "0x123456789ABCDEF";
    const char *exp_str = "0xFEDCBA987654321";
    const char *mod_str = "0x1000000000000001";

    bigint_from_hex(base_str, &base);
    bigint_from_hex(exp_str, &exp);
    bigint_from_hex(mod_str, &mod);

    bigint_mod_exp(&base, &exp, &mod, &result);

    char hex_str[260];
    bigint_to_hex(&result, hex_str);
    printf("Modular Exponentiation Result: %s\n", hex_str);

    // Example usage of modular inversion
    bigint_t x, mod_inv_result;
    const char *x_str = "0x123456789ABCDEF";

    bigint_from_hex(x_str, &x);
    bigint_from_hex(mod_str, &mod);

    if (bigint_mod_inv(&x, &mod, &mod_inv_result)) {
        bigint_to_hex(&mod_inv_result, hex_str);
        printf("Modular Inversion Result: %s\n", hex_str);
    } else {
        printf("Modular inversion does not exist.\n");
    }

    return 0;
}
