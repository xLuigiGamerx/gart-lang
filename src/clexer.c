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

char *escape_string(const char *src) {
    size_t len = strlen(src);
    char *escaped = malloc(len * 4 + 1); // worst case: every char becomes \xNN
    char *dst = escaped;

    while (*src) {
        if (*src == '\\') {
            src++;
            switch (*src) {
                case 'n': *dst++ = 10; break;
                case 't': *dst++ = 9;  break;
                case 'r': *dst++ = 13; break;
                case '\\': *dst++ = '\\'; break;
                case '"': *dst++ = '"'; break;
                default: *dst++ = *src; break;
            }
        } else {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
    return escaped;
}

bool expect_clex(stb_lexer *lexer, int expected) {
    if (!stb_c_lexer_get_token(lexer)) {
        printf("Syntax ERROR: unexpected end of input, expected token %s\n", CLEX_to_tokenstr[expected]);
        return false;
    }
    if (lexer->token != expected) {
        const char *got = (lexer->token >= 0 && lexer->token < 256)
                          ? (char[]){ (char)lexer->token, '\0' }
                          : lexer->string;
        printf("Syntax ERROR: expected token %s, got '%s'\n", CLEX_to_tokenstr[expected], got);
        return false;
    }
    return true;
}

void parse_return(stb_lexer *lexer, FILE *out) {
    stb_c_lexer_get_token(lexer);
    if (write_code){
        if (lexer->token == CLEX_intlit) {
            fprintf(out, "    mov eax, %d\n", (int)lexer->int_number);
        } else if (lexer->token == CLEX_floatlit) {
            fprintf(out, "    movsd xmm0, [float_val_%d]\n", float_pc++);
        } else if (lexer->token == CLEX_dqstring) {
            fprintf(out, "    lea rcx, [str_val_%d]\n", str_pc++);
        }
        fprintf(out, "    call [ExitProcess]\n");
    } else {
        switch (lexer->token) {
            case CLEX_intlit:
                break;
            case CLEX_floatlit:
                fprintf(out, "    float_val_%d dq %f\n", float_pc++, lexer->real_number);
                break;
            case CLEX_dqstring:
                char *escaped = escape_string(lexer->string);
                fprintf(out, "    str_val_%d db ", str_pc);
                for (char *c = escaped; *c; ++c)
                    fprintf(out, "%d,", (unsigned char)*c);
                fprintf(out, "0\n");
                free(escaped);
                str_pc++;
                break;
            default:
                printf("Syntax ERROR: unexpected return value '%s'\n", CLEX_to_tokenstr[lexer->token]);
                break;
        }
    }
  
}

void parse_call(stb_lexer *lexer, FILE *out) {
    char func_name[256];
    strncpy(func_name, lexer->string, sizeof(func_name) - 1);
    func_name[sizeof(func_name) - 1] = 0;
    bool custom_func = false;

    for (int func = 0; func < 2048; func++) {
        if (functions[func]) {
            if (strcmp(functions[func], func_name) == 0) {
                custom_func = true;
            }
        }
    }

    if (!expect_clex(lexer, '(')) return;

    if (!stb_c_lexer_get_token(lexer)) {
        printf("Syntax ERROR: unexpected end of input in function call param\n");
        return;
    }

    if (write_code) {
        if (lexer->token == CLEX_dqstring) {
            fprintf(out, "    lea rcx, [str_val_%d]\n", str_pc++);
        } else if (lexer->token == CLEX_intlit) {
            fprintf(out, "    mov rcx, %d\n", (int)lexer->int_number);
        } else if (lexer->token == CLEX_floatlit) {
            fprintf(out, "    movsd xmm0, [float_val_%d]\n", float_pc++);
        } else if (lexer->token == CLEX_id) {
            bool isVariable = false;
            struct Variable *custom_var = NULL;
            for (int var = 0; var < 2048; var++) {
                if (variables[var] != NULL) {
                    if (strcmp(variables[var]->name, lexer->string) == 0) {
                        isVariable = true;
                        custom_var = variables[var];
                    }
                }
            }
            if (!isVariable) {
                printf("Syntax ERROR: undefined variable '%s'\n", lexer->string);
                return;
            }
            switch (custom_var->type) {
                case TYPE_INT:
                case TYPE_TOKEN:
                    fprintf(out, "    mov rcx, [%s]\n", custom_var->name);
                    break;
                case TYPE_FLOAT:
                    fprintf(out, "    movsd xmm0, [%s]\n", custom_var->name);
                    break;
                case TYPE_STR:
                    fprintf(out, "    lea rcx, [%s]\n", custom_var->name);
                    break;
                default:
                    printf("Syntax ERROR: unexpected variable type '%d'\n", custom_var->type);
                    break;
            }
        } else {
            if (lexer->token == ')') {
                if (custom_func) {
                    fprintf(out, "    call %s\n", func_name); 
                } else {
                    fprintf(out, "    call [%s]\n", func_name);
                }
                return;
            }
            printf("Syntax ERROR: unexpected parameter type '%s'\n", CLEX_to_tokenstr[lexer->token]);
            return;
        }
        // call func 
        for (int func = 0; func < 2048; func++) {
            if (functions[func]) {
                if (strcmp(functions[func], func_name) == 0) {
                    fprintf(out, "    call %s\n", func_name); 
                }
            }
        }
        if (custom_func) {
            fprintf(out, "    call %s\n", func_name); 
        } else {
            fprintf(out, "    call [%s]\n", func_name);
        }
    } else {
        switch (lexer->token) {
            case CLEX_dqstring:
                char *escaped = escape_string(lexer->string);
                fprintf(out, "    str_val_%d db ", str_pc);
                for (char *c = escaped; *c; ++c)
                    fprintf(out, "%d,", (unsigned char)*c);
                fprintf(out, "0\n");
                free(escaped);
                str_pc++;
                break;
            case CLEX_intlit:
                //
                break;
            case CLEX_floatlit:
                fprintf(out, "    float_val_%d dq %f\n", float_pc++, lexer->real_number);
                break;
            case CLEX_id:
                //
                break;
            default:
                if (lexer->token == ')') {
                    return;
                }
                printf("Syntax ERROR: unexpected parameter type '%s'\n", CLEX_to_tokenstr[lexer->token]);
                return;
        }
    }

    if (!expect_clex(lexer, ')')) return;
}

void parse_function(stb_lexer *lexer, FILE *out) {
    // expect function name after 'fn'
    if (!expect_clex(lexer, CLEX_id)) return;
    if (write_code) fprintf(out, "%s:\n", lexer->string);
    functions[func_count++] = strdup(lexer->string);

    // expect '('
    if (!expect_clex(lexer, '(')) return;
    // expect ')'
    if (!expect_clex(lexer, ')')) return;

    while (true) {
        if (!expect_clex(lexer, CLEX_id)) return;

        if (strcmp(lexer->string, "return") == 0) {
            parse_return(lexer, out);
        } else if (strcmp(lexer->string, "end") == 0) {
            break;
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
    variables[var_count] = malloc(sizeof(struct Variable));
    memset(variables[var_count], 0, sizeof(struct Variable));
    variables[var_count]->name = variable_name;

    int intVar = 0;
    double floatVar = 0.0;
    char* strVar;
    int tokenVar = false;
    if (!write_code) {
        if (global) {
            switch (lexer->token) {
                case CLEX_intlit:
                    intVar = lexer->int_number;
                    variables[var_count]->type = TYPE_INT;
                    variables[var_count]->Int = intVar;
                    var_count++;
                    fprintf(out, "    %s dq %d\n", variable_name, intVar);
                    break;
                case CLEX_floatlit:
                    floatVar = lexer->real_number;
                    variables[var_count]->type = TYPE_FLOAT;
                    variables[var_count]->Float = floatVar;
                    var_count++;
                    fprintf(out, "    %s dq %f\n", variable_name, floatVar);
                    break;
                case CLEX_dqstring:
                    strVar = lexer->string;
                    variables[var_count]->type = TYPE_STR;
                    variables[var_count]->Str = strVar;
                    var_count++;
                    char *escaped = escape_string(strVar);
                    fprintf(out, "    %s db ", variable_name);
                    for (char *c = escaped; *c; ++c)
                        fprintf(out, "%d,", (unsigned char)*c);
                    fprintf(out, "0\n");
                    free(escaped);
                    break;
                case CLEX_id:
                    if (strcmp(lexer->string, "true")) tokenVar = true; 
                    else if (strcmp(lexer->string, "false")) tokenVar = false;
                    else if (strcmp(lexer->string, "NULL")) tokenVar = 0;
                    else {
                        printf("Syntax ERROR: unexpected bool/token value '%s'\n", lexer->string);
                        return;
                    }
                    fprintf(out, "    %s dq %d\n", variable_name, tokenVar);
                    variables[var_count]->type = TYPE_TOKEN;
                    variables[var_count]->Token = tokenVar;
                    var_count++;
                    break;
                default:
                    printf("Syntax ERROR: unexpected token '%s'\n", CLEX_to_tokenstr[lexer->token]);
                    break;
            }
        } else {
        }
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