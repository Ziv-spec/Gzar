
internal bool match_character(char **txt, char c) {
    if ((*txt)[0] == c) {
        (*txt)++;
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
        }
        
        if (*txt != '\n' && *txt != ' ' && *txt != '\t') {
            break; 
        }
        
        
    }
    
    // 
    // lex a token 
    // 
    
    switch (*txt++) {
        
        case '{': { token.kind = TK_RBRACE; } break; 
        case '}': { token.kind = TK_LBRACE; } break; 
        case ')': { token.kind = TK_RPARAN; } break; 
        case '(': { token.kind = TK_LPARAN; } break; 
        case ';': { token.kind = TK_SEMI_COLON; } break; 
        case ':': { token.kind = TK_COLON; } break; 
        case '+': { token.kind = TK_PLUS;  }  break; 
        case '-': { token.kind = TK_MINUS; }  break;  
        case '*': { token.kind = TK_STAR;  }  break; 
        case '/': { token.kind = TK_SLASH; } break; 
        
        case '=': { token.kind = match_character(&txt, '=') ? TK_EQUAL_EQUAL : TK_ASSIGN; } break;
        case '>': { token.kind = match_character(&txt, '=') ? TK_GREATER_EQUAL : TK_GREATER; } break; 
        case '<': { token.kind = match_character(&txt, '=') ? TK_LESS_EQUAL : TK_LESS; } break; 
        case '!': { token.kind = match_character(&txt, '=') ? TK_BANG_EQUAL : TK_BANG; } break; 
        
        case '\0': {
            token.kind = TK_EOF;
        } break;
        
        case '"': {
            
            token.kind = TK_STRING; 
            token.str  = txt;
            
            for (; *txt != '"' && *txt != '\n'; txt++, loc.ch++); 
            token.len = (int)(txt-token.str);
            
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
                
                bool found_keyword = false; 
                for (int i = 1; i < ArrayLength(keywords); i++) {
                    // if i don't put the result of the comparison into 'r' then 
                    // for some reason msvc bugs out and does not produce correct code here
                    // so I just do this
                    int r = my_strcmp(txt, keywords[i]) == 0;
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
                    while (*keyword++) txt++; 
                    
                } 
                else {
                    
                    // token is a identifier
                    token.kind = TK_IDENTIFIER;
                    token.str = txt;  
                    for (; is_alphanumeric(*txt); txt++, loc.ch++); 
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



/* NOTE(ziv): DEPRECATED
internal void debug_print_token(Token token) { 
    
    char buff[255] = {0}; // temp buffer for string minipulation
    
    // NOTE(ziv): some hacky way of printing the values that I get from this :) 
    if (token.str)  slicecpy(buff, 255, token.str, token.len);
    
    if (TK_OP_BEGIN <= token.kind && token.kind <= TK_OP_END) {
        printf("%d\t%s\t%s\n", __token_index++, 
               tk_names[token.kind], 
               tk_names[token.kind]);
    }
    else if (TK_LITERAL_BEGIN <= token.kind && token.kind <= TK_LITERAL_END) {
        if (token.str) printf("%d\t%s\t%s\n", __token_index++, buff, tk_names[token.kind]);
    }
}
 */

