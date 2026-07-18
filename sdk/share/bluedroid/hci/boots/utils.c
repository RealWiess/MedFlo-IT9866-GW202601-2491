#include <stdio.h>
#include "utils.h"

void print_hex_dump(const char *label, const void *buf, size_t len) {
    const unsigned char *data = (const unsigned char *)buf;
    printf("%s\n", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}