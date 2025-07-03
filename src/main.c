#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

const char *keywords[] = {
    "fn",
};

const char* CLEX_to_tokenstr[] = {
    [CLEX_eof]         = "<eof>",
    //[CLEX_parse_error] = "Parse Error",
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

bool expect_clex(stb_lexer *lexer, int expected)
{
    if (!stb_c_lexer_get_token(lexer))
    {
        printf("Unexpected end of input, expected token %d\n", expected);
        return false;
    }
    if (lexer->token != expected)
    {
        if (lexer->token == CLEX_parse_error) {
            printf("Parse ERROR: expected %s, but got token '%s'\n", CLEX_to_tokenstr[expected], lexer->string);
        } else {
            printf("Syntax ERROR: expected %s, but got %s '%s'\n", CLEX_to_tokenstr[expected], CLEX_to_tokenstr[lexer->token], lexer->string);
        }
        return false;
    }
    return true;
}

static void parse_token(stb_lexer *lexer, FILE *c_file)

{
    printf("\n");
    switch (lexer->token)
    {
    case CLEX_id:

        for (int key = 0; key < sizeof(keywords) / (sizeof(keywords[0])); key++)
        {
            if (!strcmp(keywords[key], lexer->string))
            {

                if (!strcmp(lexer->string, "fn"))
                {
                    fprintf(c_file, "int ");
                    if (expect_clex(lexer, CLEX_id)) {
                        fprintf(c_file, "%s", lexer->string);
                        //TODO: args

                        expect_clex(lexer, '(');
                        expect_clex(lexer, ')');
                        fprintf(c_file, "()");
                    }
                }
            }
        }
        break;
    case CLEX_eq:
        printf("==");
        break;
    case CLEX_noteq:
        printf("!=");
        break;
    case CLEX_lesseq:
        printf("<=");
        break;
    case CLEX_greatereq:
        printf(">=");
        break;
    case CLEX_andand:
        printf("&&");
        break;
    case CLEX_oror:
        printf("||");
        break;
    case CLEX_shl:
        printf("<<");
        break;
    case CLEX_shr:
        printf(">>");
        break;
    case CLEX_plusplus:
        printf("++");
        break;
    case CLEX_minusminus:
        printf("--");
        break;
    case CLEX_arrow:
        printf("->");
        break;
    case CLEX_andeq:
        printf("&=");
        break;
    case CLEX_oreq:
        printf("|=");
        break;
    case CLEX_xoreq:
        printf("^=");
        break;
    case CLEX_pluseq:
        printf("+=");
        break;
    case CLEX_minuseq:
        printf("-=");
        break;
    case CLEX_muleq:
        printf("*=");
        break;
    case CLEX_diveq:
        printf("/=");
        break;
    case CLEX_modeq:
        printf("%%=");
        break;
    case CLEX_shleq:
        printf("<<=");
        break;
    case CLEX_shreq:
        printf(">>=");
        break;
    case CLEX_eqarrow:
        printf("=>");
        break;
    case CLEX_dqstring:
        printf("\"%s\"", lexer->string);
        break;
    case CLEX_sqstring:
        printf("'\"%s\"'", lexer->string);
        break;
    case CLEX_charlit:
        printf("'%s'", lexer->string);
        break;
    case CLEX_intlit:
        printf("#%g", lexer->real_number);
        break;
    case CLEX_floatlit:
        printf("%g", lexer->real_number);
        break;
    default:
        if (lexer->token >= 0 && lexer->token < 256)
            printf("%c", (int)lexer->token);
        else
        {
            printf("<<<UNKNOWN TOKEN %ld >>>\n", lexer->token);
        }
        break;
    }
}

char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror("open");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size + 1);
    if (!buf)
    {
        perror("malloc");
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';

    fclose(f);
    return buf;
}

int main(void)
{
    char *source = read_file("hello_world.gl");
    FILE *outputC = fopen("gl2C.cdump", "w");

    // Initialize lexer with source string
    stb_lexer lex;
    stb_c_lexer_init(&lex, source, source + strlen(source), (char *)malloc(0x10000), 0x10000);
    printf("Tokens:\n");
    while (stb_c_lexer_get_token(&lex))
    {
        parse_token(&lex, outputC);
    }
    free(source);

    return 0;
}