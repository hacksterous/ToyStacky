#include "../../arduino-pico/ToyStacky/TS-core-tokenize.h"

int main() {
    char input[] = "23@ 0@ ab: (1 2 3";//"// 12: 2 @ f@ if el fi {@! [(1 1) (2 2)1]@dup -1.001e-3l - \"example string\" @@ ab (4 5) \"incompletestring \"[1.3 2.5]";
    char output[50];
    char* next = input;

    while (*next) {
        next = tokenize(next, output);
        if (*output) {
            printf("Token: %s\n", output);
        }
    }

    return 0;
}
