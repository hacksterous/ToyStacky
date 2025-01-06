
#include <stdio.h>
#include <ctype.h>
#include <string.h>

// Function to skip whitespace and return the new position
char* skip_whitespace(char* input) {
    while (isspace((unsigned char)*input)) {
        input++;
    }
    return input;
}

// Function to extract a double-quote delimited string
char* extract_string(char* input, char* output) {
    input = skip_whitespace(input);

    if (*input == '"') {
        *output++ = *input++;
        while (*input && *input != '"') {
            *output++ = *input++;
        }
        if (*input == '"') {
            *output++ = *input++;
        }
        *output = '\0';
    } else {
        *output = '\0';
    }

    return skip_whitespace(input);
}

// Function to check if a character is a valid digit
int is_digit(char c) {
    return isdigit((unsigned char)c);
}

// Function to extract a real number (REALNUM)
char* extract_realnum(char* input, char* output) {
    input = skip_whitespace(input);

    if (*input == '+' || *input == '-') {
        *output++ = *input++;
    }

    int has_digits = 0;
    while (is_digit(*input)) {
        has_digits = 1;
        *output++ = *input++;
    }

    if (*input == '.') {
        *output++ = *input++;
        while (is_digit(*input)) {
            has_digits = 1;
            *output++ = *input++;
        }
    }

    if ((*input == 'e' || *input == 'E') && has_digits) {
        *output++ = *input++;
        if (*input == '+' || *input == '-') {
            *output++ = *input++;
        }
        while (is_digit(*input)) {
            *output++ = *input++;
        }
    }

    *output = '\0';
    return skip_whitespace(input);
}

// Function to extract a bignum (BIGNUM)
char* extract_bignum(char* input, char* output) {
    input = skip_whitespace(input);

    if (*input == '"') {
        *output++ = *input++;
        while (*input && *input != '"') {
            *output++ = *input++;
        }
        if (*input == '"') {
            *output++ = *input++;
        }
        *output = '\0';
    } else {
        *output = '\0';
    }

    return skip_whitespace(input);
}

// Function to extract a complex number (COMPLEXNUM)
char* extract_complexnum(char* input, char* output) {
    input = skip_whitespace(input);

    if (*input == '(') {
        *output++ = *input++;
        input = extract_realnum(input, output);
        if (*output != '\0') {
            output += strlen(output);
            *output++ = ' ';
            input = extract_realnum(input, output);
            if (*output != '\0') {
                output += strlen(output);
                if (*input == ')') {
                    *output++ = *input++;
                } else {
                    printf("Error: Complex number not closed properly.\n");
                    *output = '\0';
                }
            } else {
                printf("Error: Invalid complex number format.\n");
                *output = '\0';
            }
        } else {
            printf("Error: Invalid complex number format.\n");
            *output = '\0';
        }
    } else {
        *output = '\0';
    }

    *output = '\0';
    return skip_whitespace(input);
}

// Function to extract a VECTOR token set
char* extract_vector(char* input, char* output) {
    char* next = input;
    int in_vector = 0;
    int vector_complete = 0;

    // Skip leading whitespace
    next = skip_whitespace(next);

    // Look for the VECTOR tokens
    while (*next && !vector_complete) {
        next = skip_whitespace(next);
        if (*next == '[') {
            if (in_vector) {
                // Nested vectors are not allowed in this grammar
                printf("Error: Nested vectors are not allowed.\n");
                return NULL;
            }
            in_vector = 1;
            strcat(output, "[");
            next++;
        } else if (*next == ']') {
            if (!in_vector) {
                printf("Error: Closing bracket without opening bracket.\n");
                return NULL;
            }
            vector_complete = 1;
            strcat(output, " ]");
            next++;
        } else {
            char temp_output[50];
            next = extract_realnum(next, temp_output);
            if (*temp_output != '\0') {
                strcat(output, " ");
                strcat(output, temp_output);
                continue;
            }
            next = extract_complexnum(next, temp_output);
            if (*temp_output != '\0') {
                strcat(output, " ");
                strcat(output, temp_output);
                continue;
            }
            next = extract_bignum(next, temp_output);
            if (*temp_output != '\0') {
                strcat(output, " ");
                strcat(output, temp_output);
                continue;
            }
            next = extract_string(next, temp_output);

			if (*temp_output == '\0') {
                // If none of the above, treat as invalid token and convert to string
                char invalid_token[50] = "\"";
                char* start = next;
                while (*next && !isspace((unsigned char)*next) && *next != ']' && *next != '[') {
                    next++;
                }
                strncat(invalid_token, start, next - start);
                strcat(invalid_token, "\"");
                strcpy(temp_output, invalid_token);
            }
            strcat(output, " ");
            strcat(output, temp_output);
        }
    }

    if (in_vector && !vector_complete) {
        printf("Error: Vector not closed properly.\n");
        return NULL;
    }

    return next;
}

