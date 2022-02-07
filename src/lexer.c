
internal bool match_character(char **txt, char c, int *len) {
    if ((*txt)[0] == c) {
        (*txt)++;
        (*len)++;
        return true;
    }
    return false;
}


internal bool get_next_token(Lexer *lexer) { 
    Location loc = lexer->loc;
    Token token = {0}; 
    
    char *txt = lexer->code + lexer->loc.index; // current character
    
    // 
    // skip whitespace and comments 
    // 
    
    for (; txt; txt++, loc.ch++) {
        
        if (*txt == '/') {
            if (*++txt == '/') { // skip comment
                while (*txt != '\n' && *txt != '\r' && *txt != '\0') { 
                    txt++; loc.ch++; 
                }
            }
        }
        if (*txt == '\n') {
            loc.line++;
            loc.ch = 0;
        }
        
        if (*txt != '\n' && *txt != ' ' && *txt != '\t') {
            break; 
        }
        
        
    }
    
    // 
    // lex a token 
    // 
    
    switch (*txt++) {
        
        case '{': { token.kind = TK_RBRACE; token.len = 1; } break; 
        case '}': { token.kind = TK_LBRACE; token.len = 1; } break; 
        case ')': { token.kind = TK_RPARAN; token.len = 1; } break; 
        case '(': { token.kind = TK_LPARAN; token.len = 1; } break; 
        case ';': { token.kind = TK_SEMI_COLON; token.len = 1; } break; 
        case '+': { token.kind = TK_PLUS;  token.len = 1; }  break; 
        case '*': { token.kind = TK_STAR;  token.len = 1; }  break; 
        case '/': { token.kind = TK_SLASH; token.len = 1; } break; 
        case ',': { token.kind = TK_COMMA; token.len = 1; } break; 
        
        case '-': { token.kind = match_character(&txt, '>', &token.len) ? TK_RETURN_TYPE: TK_MINUS; token.len++; } break;
        case ':': { token.kind = match_character(&txt, ':', &token.len) ? TK_DOUBLE_COLON: TK_COLON; token.len++; } break;
        case '=': { token.kind = match_character(&txt, '=', &token.len) ? TK_EQUAL_EQUAL : TK_ASSIGN; token.len++; } break;
        case '>': { token.kind = match_character(&txt, '=', &token.len) ? TK_GREATER_EQUAL : TK_GREATER; token.len++; } break; 
        case '<': { token.kind = match_character(&txt, '=', &token.len) ? TK_LESS_EQUAL : TK_LESS; token.len++; } break; 
        case '!': { token.kind = match_character(&txt, '=', &token.len) ? TK_BANG_EQUAL : TK_BANG; token.len++; } break; 
        
        case '\0': {
            token.kind = TK_EOF;
        } break;
        
        case '"': {
            
            token.kind = TK_STRING; 
            token.str  = txt;
            
            for (; *txt != '"' && *txt != '\n'; txt++); 
            token.len = (int)(txt-token.str);
            txt++; // eat '\"' character
            loc.ch+=3;
            
        } break;
        
        default: { 
            
            txt--;
            if (is_digit(*txt)) {
                token.kind = TK_NUMBER;
                token.str = txt;
                for (; is_digit(*txt); txt++, loc.ch++);
                token.len = (int)(txt-token.str); 
            }
            else if (is_alpha(*txt)) {
                
                //
                // Search if token is keyword
                //
                
                // This is map between the keywords and their string representation
                // I might use a hash map for the mapping in the future but for the time 
                // being it works so I will take it .
                static char *keywords[]  = {
                    [0]         = NULL,
                    [TK_IF]     = "if",
                    [TK_ELSE]   = "else",
                    [TK_WHILE]  = "while",
                    [TK_FALSE]  = "false",
                    [TK_TRUE]   = "true",
                    [TK_NIL]    = "nil",
                    [TK_RETURN] = "return",
                    [TK_PRINT]  = "print",
                    [TK_PROC]   = "proc",
                    
                    [TK_S8_TYPE]  = "s8",
                    [TK_S16_TYPE] = "s16",
                    [TK_S32_TYPE] = "s32",
                    [TK_U8_TYPE]  = "u8",
                    [TK_U16_TYPE] = "u16",
                    [TK_U32_TYPE] = "u32",
                    [TK_INT_TYPE] = "int",
                    [TK_STRING_TYPE] = "string",
                    [TK_VOID_TYPE]   = "void",
                    
                    // this is a bit of a hack though, I am unsure what would be better rn.
                    [TK_TYPE_BEGIN] = "\0", 
                    [TK_TYPE_END]   = "\0",
                    
                };
                
                bool found_keyword = false; 
                for (int i = 1; i < ArrayLength(keywords); i++) {
                    // if i don't put the result of the comparison into 'r' then 
                    // for some reason msvc bugs out and does not produce correct code here
                    // so I just do this
                    int r = keyword_cmp(txt, keywords[i]) == 0;
                    if (r) {
                        token.kind = i;
                        found_keyword = true;
                        break;
                    }
                }
                if (found_keyword) {
                    // because I do not effect txt in my_strcmp
                    // here I 'eat' that token
                    char *keyword = keywords[token.kind];
                    token.len = (int)strlen(keyword)-1;
                    while (*keyword++) txt++; 
                } 
                else {
                    
                    // token is a identifier
                    token.kind = TK_IDENTIFIER;
                    token.str = txt;  
                    for (; is_alphanumeric(*txt); txt++); 
                    token.len = (int)(txt-token.str); 
                }
            }
            else {
                // I don't know what is this token. 
                fprintf(stderr, "Error: Unknown character at %d\n", loc.line);
                return false;
            }
            
        } break;
        
        
    }
    
    loc.ch += token.len;
    loc.index = (int)(txt - lexer->code);  
    lexer->loc = loc; 
    token.loc = loc;
    
    lexer->token = token;
    return true;
}

internal bool top_next_token(Lexer *lexer) {
    Location loc = lexer->loc; 
    if (!get_next_token(lexer)) {
        return false;  
    }
    lexer->loc = loc; // move lexer to old location
    return true; 
}

internal bool lex_file(Lexer *lexer) {
    //
    // get a token list for the whole program
    //
    int i = 0;
    for (;  lexer->token.kind != TK_EOF; i++) {
        if (get_next_token(lexer)) {
            tokens[i] = lexer->token;
        }
        else {
            return false;
        }
    }
    tokens_len = i;
    
    return true;
}

internal int keyword_cmp(char *str1, char *str2) {
    char *temp1 = str1, *temp2 = str2; 
    while (*temp1 && *temp2 && *temp1 == *temp2) { temp1++; temp2++; }
    return is_alpha(*temp1) || *temp2 || *temp1 == *temp2;
}