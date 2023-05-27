
#pragma pack(push, 1)
typedef struct Key_String_Bucket {
    char key;
    char *value;
    Token_Kind kind;
} Key_String_Bucket;
#pragma pack(pop)

internal inline Token lex_identifier(char *str) {

    // hash the given string
    char *temp = str;
    while (is_alphanumeric(*temp)) temp++;
    u64 len = temp - str;
	
	/// AUTO GENERATED
    static Key_String_Bucket keywords_hashes_tbl[32] = {
        { 64,  "true",   TK_TRUE },
		{ 0,   NULL,     0 },
		{ 98,  "return", TK_RETURN },
		{ 0,   NULL,     0 },
		{ 36,  "s8",     TK_S8_TYPE },
		{ 37,  "nil",    TK_NIL },
		{ 38,  "u8",     TK_U8_TYPE },
		{ 103, "string", TK_STRING_TYPE },
		{ 0,   NULL,     0 },
		{ 41,  "else",   TK_ELSE },
		{ 0,   NULL,     0 },
		{ 43,  "cast",   TK_CAST },
		{ 44,  "bool",   TK_BOOL_TYPE },
		{ 45,  "int",    TK_INT_TYPE },
		{ 78,  "while",  TK_WHILE },
		{ 0,   NULL,     0 },
		{ 48,  "false",  TK_FALSE },
		{ 81,  "s16",    TK_S16_TYPE },
		{ 0,   NULL,     0 },
		{ 83,  "u16",    TK_U16_TYPE },
		{ 20,  "if",     TK_IF },
		{ 0,   NULL,     0 },
		{ 0,   NULL,     0 },
		{ 0,   NULL,     0 },
		{ 0,   NULL,     0 },
		{ 57,  "s64",    TK_S64_TYPE },
		{ 0,   NULL,     0 },
		{ 59,  "u64",    TK_U64_TYPE },
		{ 60,  "void",   TK_VOID_TYPE },
		{ 61,  "s32",    TK_S32_TYPE },
		{ 0,   NULL,     0 },
		{ 63,  "u32",    TK_U32_TYPE },
    };

    static char buff_tbl[26] = {
        1, 2, 3, 15, 5, 11, 21, 8, 9, 10, 11, 12, 13, 16, 15, 17, 17, 18, 19, 20, 21, 21, 44, 24, 25,
    };
	///
	
    Token t = { 
		.str = { str, len },
		.kind = TK_IDENTIFIER
	};

	// compute hash
    u64 hash = 0;
    for (u64 i = 0; i < len; i++) {
        u64 index = ABS((str[i]-'a')) % 26;
        hash += buff_tbl[index];
    }

    // check in the keywords tbl whether my index is correct
    u64 index = hash & (32-1);
    Key_String_Bucket b = keywords_hashes_tbl[index];
    if (b.key == hash) {

        // confirm the check
        // TODO(ziv): make this sse
        int i = 0;
        for (; i < len && b.value[i]; i++) {
            if (str[i] != b.value[i]) {
                break;
            }
        }

        if (i == len) {
            t.kind = b.kind;
        }
    }

    return t;
}

////////////////////////////////
/// main lexing function

internal bool lex_file(Token_Stream * restrict s) {

    char *restrict txt = s->start;
    u64 current_token = 0;

    Token t = {0};
    while (t.kind != TK_EOF) {

        //
        // skip whitespace and comments
        //

        //CLOCK_START(LEXER_TRASH);
        for (; *txt; txt++) {
            if (!(*txt == '\n' ||
				  *txt == ' '  ||
				  *txt == '\t' ||
				  *txt == '\0' ||
				  *txt == '\r')) {
                if (txt[0] == '/' && txt[1] == '/')  {
					// search for a comment, and ignore it
					for (; txt && *txt != '\n'; txt++);
				}
                else {
                    break;
                }
            }
        }
        //CLOCK_END(LEXER_TRASH);

        /*
                CLOCK_START(LEXER_ASCII);
                CLOCK_START(LEXER_DOUBLE_ASCII);
                CLOCK_START(LEXER_STRING);
                CLOCK_START(LEXER_NUMBER);
                CLOCK_START(LEXER_IDENTIFIER);
                 */

        //
        // lex a token
        //

        switch (*txt) {

            case '{': { t.kind = TK_RBRACE;     } break;
            case '}': { t.kind = TK_LBRACE;     } break;
            case ')': { t.kind = TK_RPARAN;     } break;
            case '(': { t.kind = TK_LPARAN;     } break;
            case ';': { t.kind = TK_SEMI_COLON; } break;
            case '+': { t.kind = TK_PLUS;       } break;
            case '*': { t.kind = TK_STAR;       } break;
            case '/': { t.kind = TK_SLASH;      } break;
            case ',': { t.kind = TK_COMMA;      } break;
            case '^': { t.kind = TK_XOR;        } break;
            case '%': { t.kind = TK_MODOLU;     } break;

            case '-': { t.kind = txt[1] == '>' ? TK_RETURN_TYPE : TK_MINUS;     } break;
            case ':': { t.kind = txt[1] == ':' ? TK_DOUBLE_COLON: TK_COLON;     } break;
            case '=': { t.kind = txt[1] == '=' ? TK_EQUAL_EQUAL : TK_ASSIGN;    } break;
            case '>': { t.kind = txt[1] == '=' ? TK_GREATER_EQUAL : TK_GREATER; } break;
            case '<': { t.kind = txt[1] == '=' ? TK_LESS_EQUAL : TK_LESS;       } break;
            case '!': { t.kind = txt[1] == '=' ? TK_BANG_EQUAL : TK_BANG;       } break;
            case '&': { t.kind = txt[1] == '&' ? TK_DOUBLE_AND : TK_AND;        } break;
            case '|': { t.kind = txt[1] == '|' ? TK_DOUBLE_OR : TK_OR;          } break;

            case '"': {
                t.kind = TK_STRING;
				char *start = ++txt;
				while (*txt != '"' && *txt != '\n') txt++;
                t.str = (String8){ start, txt++ - start };
                //CLOCK_END(LEXER_STRING);
            } break;

            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            {

                char *start = txt;
                if (!is_digit(*start)) {
                    return false;
                }
				
				while (is_digit(*txt)) txt++;
				
                if (is_alphanumeric(*txt)) {
                    String8 str = { start, txt-start };
                    fprintf(stderr, "Error: expected a number at %d\n ", get_line_number(s->start, str) );
                    return false;
                }

                t.kind = TK_NUMBER;
                t.str = (String8){ start, txt-start };
                //CLOCK_END(LEXER_NUMBER);
            } break;

            default: {

                if (is_alphanumeric(*txt)) {
                     t = lex_identifier(txt);
                    txt += t.str.size;
                }
                else if (txt[0] == '\0' || txt[1] == '\0') {
                    t.kind = TK_EOF;
                }
                else {
                    // I don't know what is this token.
                    fprintf(stderr, "Error: Unknown token at %d\n", get_line_number(s->start, (String8) { txt, 1 } ));
                    return false;
                }
                //CLOCK_END(LEXER_IDENTIFIER);

            } break;

        }

        // update the slice view for single and double character tokens
        if (t.kind < TK_ASCII) {
            t.str = (String8) { txt, 1 }; // token is a single character token
			txt++;
			//CLOCK_END(LEXER_ASCII);
        }
        else if (TK_ASCII < t.kind && t.kind < TK_DOUBLE_ASCII) {
            t.str = (String8) { txt, 2 }; // token is a double character token
			txt += 2;
			//CLOCK_END(LEXER_DOUBLE_ASCII);
        }


        CLOCK_START(LEXER_TK_TO_STREAM);

        //
        // Adding token to token stream
        //

        if (current_token >= s->capacity) {
            s->capacity *= 2;
            s->s = (Token *)realloc(s->s, sizeof(Token) * s->capacity);

            if (s->s == NULL) {
                fprintf(stderr, "Error: failed to allocate memory\n");
                return false;
            }
        }

        s->s[current_token++] = t;
        CLOCK_END(LEXER_TK_TO_STREAM);

    }
	
    return true;
}