#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "clexer.h"

#include "stb_c_lexer.h"

extern void parse_program(stb_lexer *lexer, FILE *out);

void write_fasm_header(FILE *out) {
    fprintf(out,
        "format PE64 console\n"
        "entry main\n\n"
        "include 'asm/INCLUDE/WIN64A.inc'\n\n"
        "section '.data' data readable writeable\n"
    );
}

void write_fasm_imports(FILE *out) {
    fprintf(out, "section '.idata' import data readable\n");

    fprintf(out, "    library msvcrt, 'msvcrt.dll', kernel32, 'kernel32.dll'\n");
    fprintf(out, "    import msvcrt, printf, 'printf', strlen, 'strlen'\n");
    fprintf(out, "    import kernel32, ExitProcess, 'ExitProcess'\n");
}

char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size + 1);

    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char *argv[]) {
    char *source = read_file(argv[1]);
    if (!source) return 1;

    FILE *out = fopen("out.asm", "w");

    char *string_store = malloc(0x10000);
    stb_lexer lex;
    stb_c_lexer_init(&lex, source, source + strlen(source), string_store, 0x10000);

    write_fasm_header(out);
    parse_program(&lex, out);
    fprintf(out, "section '.text' code readable executable\n");
    write_code = true;
    str_pc = 0;
    float_pc = 0;
    stb_c_lexer_init(&lex, source, source + strlen(source), string_store, 0x10000);
    parse_program(&lex, out);
    write_fasm_imports(out);
    fclose(out);
    int status = system(".\\asm\\FASM.EXE out.asm >nul 2>&1");

    if (status != 0) {
        fprintf(stderr, "FASM failed to assemble the file.\n");
        fclose(out);
        free(string_store);
        free(source);
        return 1;
    }

    printf("FASM assembled successfully. Output: out.exe (or .com)\n");
    free(string_store);
    free(source);
    return 0;
}