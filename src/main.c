#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "clexer.h"

#include "stb_c_lexer.h"

extern void parse_program(stb_lexer *lexer, FILE *out);

void write_c_header(FILE *out) {
    fprintf(out,
        "#include <stdio.h>\n"
        "#include <stdint.h>\n"
        "#include <stdbool.h>\n"
        "#include <stdlib.h>\n"
        "#include <time.h>\n"
        "#include <string.h>\n\n"

        "#define println printf\n"
    );
}

#if defined(_WIN32)
    #include <direct.h>
    #define mkdir_crossp(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define mkdir_crossp(path) mkdir(path, 0755)
#endif

int make_dir(const char *path) {
    int result = mkdir_crossp(path);
    if (result == 0 || errno == EEXIST) {
        return 0;
    }
    return -1;
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
    make_dir("out");
    FILE *out = fopen("out/out.c", "w");
    char *string_store = malloc(0x10000);
    stb_lexer lex;
    stb_c_lexer_init(&lex, source, source + strlen(source), string_store, 0x10000);

    write_c_header(out);
    parse_program(&lex, out);
    fclose(out);
    /*int status = */system("gcc out/out.c -o out/out.exe");
    free(string_store);
    free(source);
    return 0;
}