#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "clexer.h"

#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"

enum VarType {
    TYPE_STR,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_TOKEN,
};

struct Variable {
    int type;
    char *name;
    char* Str;
    int Int;
    int Token;
    float Float;
};

int float_pc = 0;
int str_pc = 0;
bool write_code = false;
char *functions[2048];
struct Variable *variables[2048];
int var_count = 0;
int func_count = 0;

#define PRINT_ERR(fmt, ...) { printf("\x1b[1;31m"); printf("Error: "); printf("\x1b[0m"); printf(fmt, ##__VA_ARGS__); }
#define PRINT_ERR2(fmt, ...) printf("\x1b[1;31m"); printf("Error: "); printf("\x1b[0m"); printf(fmt, ##__VA_ARGS__);

const char* CLEX_to_tokenstr[] = {
    [CLEX_eof]         = "<eof>",
    [CLEX_intlit]      = "Integer",
    [CLEX_floatlit]    = "Float",
    [CLEX_id]          = "Identifier",
    [CLEX_dqstring]    = "String",
    [CLEX_sqstring]    = "String",
    [CLEX_charlit]     = "String",
    [CLEX_eq]          = "Operator",
    [CLEX_noteq]       = "Operator",
    [CLEX_lesseq]      = "Operator",
    [CLEX_greatereq]   = "Operator",
    [CLEX_andand]      = "Operator",
    [CLEX_oror]        = "Operator",
    [CLEX_shl]         = "Operator",
    [CLEX_shr]         = "Operator",
    [CLEX_plusplus]    = "Operator",
    [CLEX_minusminus]  = "Operator",
    [CLEX_pluseq]      = "Operator",
    [CLEX_minuseq]     = "Operator",
    [CLEX_muleq]       = "Operator",
    [CLEX_diveq]       = "Operator",
    [CLEX_modeq]       = "Operator",
    [CLEX_andeq]       = "Operator",
    [CLEX_oreq]        = "Operator",
    [CLEX_xoreq]       = "Operator",
    [CLEX_arrow]       = "Operator",
    [CLEX_eqarrow]     = "Operator",
    [CLEX_shleq]       = "Operator",
    [CLEX_shreq]       = "Operator",
};

char* escape_string(const char* input) {
    if (!input) return NULL;
    size_t len = strlen(input);
    size_t max_len = len * 2 + 1;
    char* escaped = malloc(max_len);
    if (!escaped) return NULL;

    char* dst = escaped;
    for (const char* src = input; *src; src++) {
        switch (*src) {
            case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
            case '\t': *dst++ = '\\'; *dst++ = 't'; break;
            case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
            case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
            case '\"': *dst++ = '\\'; *dst++ = '\"'; break;
            case '\'': *dst++ = '\\'; *dst++ = '\''; break;
            default:
                *dst++ = *src;
        }
    }
    *dst = '\0';
    return escaped;
}

bool expect_clex(stb_lexer *lexer, int expected) {
    if (!stb_c_lexer_get_token(lexer)) {
        PRINT_ERR("unexpected end of input, expected token %s\n", CLEX_to_tokenstr[expected]);
        return false;
    }
    if (lexer->token != expected) {
        const char *got = (lexer->token >= 0 && lexer->token < 256)
                          ? (char[]){ (char)lexer->token, '\0' }
                          : lexer->string;
        PRINT_ERR("expected token %s, got '%s'\n", CLEX_to_tokenstr[expected], got);
        return false;
    }
    return true;
}

void parse_return(stb_lexer *lexer, FILE *out, const char* func_name) {
    stb_c_lexer_get_token(lexer);

    int retVal = 0;

    switch(lexer->token)    {
        case CLEX_intlit:
            retVal = lexer->int_number;
            break;
        //supporting only ints currently
        /*case CLEX_dqstring:
            retval = &lexer->string;
            break;*/ 
        case CLEX_id:
            if (strcmp(lexer->string, "true") == 0) {
                retVal = 1;
            } else if (strcmp(lexer->string, "false") == 0 || strcmp(lexer->string, "NULL") == 0) {
                retVal = 0;
            }
            break;
    }
    fprintf(out, "    return %d;\n", (retVal)); 
}

void parse_call(stb_lexer *lexer, FILE *out) {
    const char *fn_name = strdup(lexer->string);

    if (!expect_clex(lexer, '(')) {
        free((void *)fn_name);
        return;
    }

    fprintf(out, "    %s(", fn_name);
    free((void *)fn_name);

    bool first = true;

    while (true) {
        if (!stb_c_lexer_get_token(lexer)) {
            PRINT_ERR("unexpected end of input in function call\n");
            return;
        }

        if (lexer->token == ')') {
            break; // end
        }

        if (!first) {
            fprintf(out, ", ");
        }
        first = false;

        switch (lexer->token) {
            case CLEX_intlit:
                fprintf(out, "%ld", lexer->int_number);
                break;
            case CLEX_floatlit:
                fprintf(out, "%f", lexer->real_number);
                break;
            case CLEX_dqstring: {
                char *escaped = escape_string(lexer->string);
                fprintf(out, "\"%s\"", escaped);
                free(escaped);
                break;
            }
            case CLEX_id:
                if (strcmp(lexer->string, "true") == 0) {
                    fprintf(out, "1");
                } else if (strcmp(lexer->string, "false") == 0 || strcmp(lexer->string, "NULL") == 0) {
                    fprintf(out, "0");
                } else {
                    fprintf(out, "%s", lexer->string);
                }
                break;
            default:
                PRINT_ERR("unexpected token in function call argument\n");
                return;
        }

        if (!stb_c_lexer_get_token(lexer)) {
            PRINT_ERR("expected ',' or ')' after function parameter\n");
            return;
        }

        if (lexer->token == ')') {
            break;
        } else if (lexer->token != ',') {
            PRINT_ERR("expected ',' between function parameters\n");
            return;
        }

        // next arg
    }

    fprintf(out, ");\n");
}

extern void parse_variable(stb_lexer *lexer, FILE *out, bool global);

void parse_function(stb_lexer *lexer, FILE *out) {
    // expect function name after 'fn'
    if (!expect_clex(lexer, CLEX_id)) return;
    fprintf(out, "int %s() {\n", lexer->string);
    char* func_name = strdup(lexer->string);
    functions[func_count++] = func_name;

    // expect '('
    if (!expect_clex(lexer, '(')) return;
    // expect ')'
    if (!expect_clex(lexer, ')')) return;

    while (true) {
        if (!expect_clex(lexer, CLEX_id)) return;

        if (strcmp(lexer->string, "return") == 0) {
            parse_return(lexer, out, func_name);
        } else if (strcmp(lexer->string, "end") == 0) {
            fprintf(out, "}\n");
            break;
        } else if (strcmp(lexer->string, "gvar") == 0) {
            parse_variable(lexer, out, true);
        } else if (strcmp(lexer->string, "gvar") == 0) {
            parse_variable(lexer, out, false);
        } else {
            parse_call(lexer, out);
        }
    }
}

void parse_variable(stb_lexer *lexer, FILE *out, bool global) {
    if (!expect_clex(lexer, CLEX_id)) return;
    char *variable_name = strdup(lexer->string);
    if (!expect_clex(lexer, '=')) return;
    if (!stb_c_lexer_get_token(lexer)) return;

    if (global) {
        switch(lexer->token) {
            case CLEX_intlit:
                fprintf(out, "int %s = %ld;\n", variable_name, lexer->int_number);
                break;
            case CLEX_floatlit:;
                fprintf(out, "float %s = %f;\n", variable_name, lexer->real_number);
                break;
            case CLEX_dqstring:
                fprintf(out, "char *%s = \"%s\";\n", variable_name, escape_string(lexer->string));
                break;
            case CLEX_id:
                if (strcmp(lexer->string, "false") == 0 || strcmp(lexer->string, "true") == 0) {
                    fprintf(out, "bool %s = %s;\n", variable_name, lexer->string);
                } else if (strcmp(lexer->string, "null") == 0 || strcmp(lexer->string, "nil") == 0) {
                    fprintf(out, "int *%s = NULL;\n", variable_name);
                } else {
                    fprintf(out, "int %s = ", variable_name, lexer->real_number);
                    parse_call(lexer, out);
                }
                break;
            default:
                break;
        }
    } else {

    }
}

void parse_program(stb_lexer *lexer, FILE *out) {
    while (stb_c_lexer_get_token(lexer)) {
        if (lexer->token == CLEX_id) {
            if (strcmp(lexer->string, "fn") == 0) parse_function(lexer, out);
            if (strcmp(lexer->string, "gvar") == 0) parse_variable(lexer, out, true);
            if (strcmp(lexer->string, "svar") == 0) parse_variable(lexer, out, false);
        }
    }
}