#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIRST_LEN 24
#define LAST_LEN  32


// to Complile:
// gcc split3.c -o split
// Usage: ./split [string]



int split_string_3(const char *input, char **first24, char **middle, char **last32) {
    if (!input || !first24 || !middle || !last32) return 0;

    size_t n = strlen(input);

    // We require at least 24+32 characters to have a valid first and last segment.
    if (n < (size_t)(FIRST_LEN + LAST_LEN)) return 0;

    size_t mid_len = n - (size_t)(FIRST_LEN + LAST_LEN);

    *first24 = (char *)malloc(FIRST_LEN + 1);
    *middle  = (char *)malloc(mid_len + 1);
    *last32  = (char *)malloc(LAST_LEN + 1);

    if (!*first24 || !*middle || !*last32) {
        free(*first24); free(*middle); free(*last32);
        *first24 = *middle = *last32 = NULL;
        return 0;
    }

    // first 24
    memcpy(*first24, input, FIRST_LEN);
    (*first24)[FIRST_LEN] = '\0';

    // middle (everything between)
    memcpy(*middle, input + FIRST_LEN, mid_len);
    (*middle)[mid_len] = '\0';

    // last 32
    memcpy(*last32, input + (n - LAST_LEN), LAST_LEN);
    (*last32)[LAST_LEN] = '\0';

    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s \"your_string_here\"\n", argv[0]);
        return 1;
    }

    const char *input = argv[1];

    char *first24 = NULL, *middle = NULL, *last32 = NULL;

    if (!split_string_3(input, &first24, &middle, &last32)) {
        fprintf(stderr, "Error: input must be at least %d characters long.\n", FIRST_LEN + LAST_LEN);
        return 1;
    }

    printf("First 24 : \"%s\"\n", first24);
    printf("Middle   : \"%s\"\n", middle);
    printf("Last 32  : \"%s\"\n", last32);

    free(first24);
    free(middle);
    free(last32);

    return 0;
}