
internal bool match_character(Lexer *lexer, Location *loc, char c) {
    if (lexer->code[loc->index+1] == '=') {
        loc->index++; 
        return true;
    }
    return false;
}


internal bool get_next_token(Lexer *lexer) { 
    Location loc = lexer->loc;
    Token token = {0}; 
    char c;
    
    // 
    // skip whitespace and comments 
    // 
    
    while (1) {
        c = lexer->code[loc.index++];
        loc.ch++;
        
        if (c == '/') {
            if (lexer->code[loc.index] == '/') { // skip comment
                while (c != '\n' || c != '\r')
                    c = lexer->code[loc.index++];
            }
        }
        if (c == '\n') {
            loc.line++;
        }
        
        if (c != '\n' && c != ' ' && c != '\t') {
            break; 
        }
    }
    
    loc.index--; // needed because we exit the loop on 'c' but the index has moved passt 'c'
    
    
    // 
    // lex everything else 
    // 
    
    // TODO(ziv): Handle errors incoming from lexing errors
    switch (c) {
        
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
        
        case '=': { token.kind = match_character(lexer, &loc, '=') ? TK_EQUAL_EQUAL : TK_ASSIGN; } break;
        case '>': { token.kind = match_character(lexer, &loc, '=') ? TK_GREATER_EQUAL : TK_GREATER; } break; 
        case '<': { token.kind = match_character(lexer, &loc, '=') ? TK_LESS_EQUAL : TK_LESS; } break; 
        case '!': { token.kind = match_character(lexer, &loc, '=') ? TK_BANG_EQUAL : TK_BANG; } break; 
        
        case '\0': {
            token.kind = TK_EOF;
        } break;
        
        case '"': {
            token.kind = TK_STRING; 
            token.str = lexer->code + loc.index;
            c = lexer->code[++loc.index];
            while (c != '"') {
                c = lexer->code[++loc.index];
                loc.ch++;
            }
            token.len = lexer->code + loc.index - token.str;
        } break;
        
        default: { 
            // TODO(ziv): make sure that this is actually is working
            if (is_digit(c)) {
                token.kind = TK_NUMBER;
                token.str = lexer->code + loc.index;
                while (is_digit(lexer->code[loc.index])) {
                    loc.ch++; loc.index++;
                }
                loc.index--;
                token.len = lexer->code + loc.index - token.str; 
            }
            else if (is_alpha(c)) {
                
                //
                // Search if token is keyword
                //
                
                bool found_keyword = false; 
                for (int i = 1; i < ArrayLength(keywords); i++) {
                    // if i don't put the result of the comparison into 'r' then 
                    // for some reason msvc bugs out and does not produce correct code here
                    // so I just do this
                    int r = my_strcmp(lexer->code + loc.index, keywords[i]) == 0;
                    if (r) {
                        token.kind = i;
                        found_keyword = true;
                        break;
                    }
                }
                if (found_keyword) {
                    char *keyword = keywords[token.kind];
                    while (*keyword++) loc.index++; 
                    
                } 
                else {
                    
                    // token is a identifier
                    token.kind = TK_IDENTIFIER;
                    token.str = lexer->code + loc.index;
                    while (is_alphanumeric(lexer->code[loc.index])) {
                        loc.ch++; loc.index++;
                    }
                    token.len = lexer->code + loc.index - token.str; 
                }
            }
            else {
                // I don't know what is this token. 
                fprintf(stderr, "Error: Unknown character at %d\n", loc.line);
                return false;
            }
            
        } break;
        
        
    }
    
    loc.index++;
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

