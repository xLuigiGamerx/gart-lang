#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

char *string = "Length of \"Hello\" is %u";
int length =     strlen("Hello");
int main() {
    printf(string, length);
    return 0;
}
