
internal Token lex_identifier(char *str) {
    
    const static char keywords[][16] = {
        "bool",
        "else",
        "false",
        "if",
        "int",
        "nil",
        "return",
        "s16",
        "s32",
        "s64",
        "s8",
        "string",
        "true",
        "u16",
        "u32",
        "u64",
        "u8",
        "void",
        "while",
    };
    
    // NOTE(ziv): this is a conversion from index to token table. 
    // I could have done this in a much better way but you know.. 
    // it's late and I don't care.. 
    const static int keywords_to_tk_kind_tbl[] = {
        [1]  = TK_BOOL_TYPE,
        [2]  = TK_ELSE, 
        [3]  = TK_FALSE,
        [4]  = TK_IF, 
        [5]  = TK_INT_TYPE, 
        [6]  = TK_NIL, 
        [7]  = TK_RETURN, 
        [8]  = TK_S16_TYPE, 
        [9]  = TK_S32_TYPE, 
        [10] = TK_S64_TYPE, 
        [11] = TK_S8_TYPE, 
        [12] = TK_STRING_TYPE, 
        [13] = TK_TRUE, 
        [14] = TK_U16_TYPE, 
        [15] = TK_U32_TYPE, 
        [16] = TK_U64_TYPE, 
        [17] = TK_U8_TYPE, 
        [18] = TK_VOID_TYPE, 
        [19] = TK_WHILE, 
    }; 
    
    Token t = {0}; 
    int i = 0;
    for (; i < ArrayLength(keywords); i++) {
        if (strncmp(keywords[i], str, strlen(keywords[i])) == 0) {
            break;
        }
    }
    
    if (i < ArrayLength(keywords)) {
        t.kind = keywords_to_tk_kind_tbl[i+1];
        t.str = (String8){ str, strlen(keywords[i]) };
    }
    else {
        // lex a identifier
        char *start = str; 
        for (; is_alphanumeric(*str); str++);
        t.kind = TK_IDENTIFIER; 
        t.str = (String8) { start, str-start };
    }
    return t;
}

////////////////////////////////
/// main lexing function

internal bool lex_file(Token_Stream *s) {
    
    char *txt = s->start;
    int current_token = 0;
    
    Token t = {0}; 
    while (t.kind != TK_EOF) {
        //t.str = (String8){0};
        
        // 
        // skip whitespace and comments 
        // 
        
        for (; *txt; txt++) {
            if (!(*txt == '\n' || *txt == ' ' || *txt == '\t' || *txt == '\0' || *txt == '\r')) {
                if (txt[0] == '/' && txt[1] == '/') {
                    for (; txt && *txt != '\n'; txt++);
                }
                else {
                    break;
                }
            }
        }
        
        // 
        // lex a token 
        // 
        
        switch (*txt++) {
            
            case '{': { t.kind = TK_RBRACE; } break; 
            case '}': { t.kind = TK_LBRACE; } break; 
            case ')': { t.kind = TK_RPARAN; } break; 
            case '(': { t.kind = TK_LPARAN; } break; 
            case ';': { t.kind = TK_SEMI_COLON; } break; 
            case '+': { t.kind = TK_PLUS;   }  break; 
            case '*': { t.kind = TK_STAR;   }  break; 
            case '/': { t.kind = TK_SLASH;  } break; 
            case ',': { t.kind = TK_COMMA;  } break; 
            case '^': { t.kind = TK_XOR;  } break; 
            
            case '-': { t.kind = txt[0] == '>' ? TK_RETURN_TYPE : TK_MINUS; } break; 
            case ':': { t.kind = txt[0] == ':' ? TK_DOUBLE_COLON: TK_COLON; } break;
            case '=': { t.kind = txt[0] == '=' ? TK_EQUAL_EQUAL : TK_ASSIGN; } break;
            case '>': { t.kind = txt[0] == '=' ? TK_GREATER_EQUAL : TK_GREATER; } break; 
            case '<': { t.kind = txt[0] == '=' ? TK_LESS_EQUAL : TK_LESS; } break; 
            case '!': { t.kind = txt[0] == '=' ? TK_BANG_EQUAL : TK_BANG; } break; 
            
            case '&': { t.kind = txt[0] == '&' ? TK_DOUBLE_AND : TK_AND; } break; 
            case '|': { t.kind = txt[0] == '|' ? TK_DOUBLE_OR : TK_OR; } break; 
            
            case '"': {
                t.kind = TK_STRING; 
                char *start = txt; 
                while (*txt != '"' && *txt != '\n') txt++; 
                t.str = (String8){ start, txt-start };
                txt++; // eat '\"' character
            } break;
            
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            {
                
                char *start = txt-1;
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
                
            } break;
            
            default: { 
                
                txt--;
                if (is_alphanumeric(*txt)) {
                    t = lex_identifier(txt);
                    txt += t.str.size;
                }
                else if (txt[0] == '\0' || txt[1] == '\0') {
                    t.kind = TK_EOF;
                }
                else {
                    // I don't know what is this token. 
                    fprintf(stderr, "Error: Unknown character at %d\n", get_line_number(s->start, (String8) { txt, 1 } ));
                    return false; 
                }
                
            } break;
            
        }
        
        // update the slice view for single and double character tokens
        if (t.kind < TK_ASCII) {
            t.str = (String8) { txt-1, 1 }; // token is a single character token
        }
        else if (TK_ASCII < t.kind && t.kind < TK_DOUBLE_ASCII) {
            t.str = (String8) { txt-1, 2 }; // token is a double character token
            txt++; 
        }
        
        
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
        
    }
    
    return true; 
}