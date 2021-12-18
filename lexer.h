#ifndef LEXER_H
#define LEXER_H

/* lexer function prototypes */

#define MAX_LEN 255

typedef enum Token_Kind Token_Kind; 
typedef struct Token Token; 
typedef struct Lexer Lexer; 

enum Token_Kind {
    TK_EOF, 
    
    TK_OP_BEGIN,
    TK_PLUS,    // + 
    TK_MINUS,   // - 
    TK_DUMP,    // . 
    TK_OP_END,
    
    TK_LITERAL_BEGIN, 
    TK_INTEGER, // e.g. 123
    TK_LITERAL_END, 
    
    TK_COUNT,
};

char *tk_names[TK_COUNT];

internal void init_tk_names() { 
    // ops
    tk_names[TK_PLUS]    = "+";
    tk_names[TK_MINUS]   = "-";
    tk_names[TK_DUMP]    = ".";
    // literals
    tk_names[TK_INTEGER] = "int";
}

struct Token { 
    Location loc;    // location of the token
    Token_Kind kind; // token kind
    
    // NOTE(ziv): Token string for error purposes
    int len;         // length of string 
    char *s;         // string representation. 
};

struct Lexer {
    Location loc; // location of the tokenizing process
    char *code;   // source code of the main file
    Token token;  // the current token
};



////////////////////////////////

int __token_index = 0;
internal void debug_print_token(Token token); // prints a token in a index, value, type format.

////////////////////////////////



/* implementation */

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
            else {
                loc.index--;
            }
        }
        if (c == '\n') {
            loc.line++;
        }
        
        if (c != '\n' && c != ' ' && c != '\t') {
            break; 
        }
    }
    
    loc.index--; // needed because we exit the loop on 'c' but the index has moved pass 'c'
    
    
    // 
    // lex everything else 
    // 
    
    // TODO(ziv): Handle errors incoming from lexing errors
    switch (c) {
        default: { 
            // currently I do nothing :)
        } break;
        
        
        case '+':  {
            token.kind = TK_PLUS; 
        } break; 
        
        case '-':  {
            token.kind = TK_MINUS; 
        } break; 
        
        case '.': {
            token.kind = TK_DUMP;
        } break;
        
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
            // TODO(ziv): maybe I should support hex values 
            token.kind = TK_INTEGER;
            
            token.s = lexer->code + loc.index;
            c = lexer->code[loc.index];
            while ('0' <= c && c <= '9') {
                c = lexer->code[++loc.index];
                loc.ch++;
            }
            
            if (c != ' ' && c != '\n' && c != '\t') { 
                // 'token number' must end with a space or newline or tab 
                fprintf(stderr, "Error: a number cannot end with '%c'\n", c);
                return false;
            }
            
            token.len = lexer->code + loc.index - token.s; 
        } break; 
        
        case '\0': {
            token.kind = TK_EOF;
        }
    }
    
    loc.index++;
    lexer->loc = loc; 
    token.loc = loc;
    
#if DEBUG
    //debug_print_token(token);
#endif 
    
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

internal void debug_print_token(Token token) {
    
    char buff[255] = {0}; // temp buffer for string minipulation
    
    // NOTE(ziv): some hacky way of printing the values that I get from this :) 
    if (token.s)  slicecpy(buff, 255, token.s, token.len);
    
    if (TK_OP_BEGIN <= token.kind && token.kind <= TK_OP_END) {
        printf("%d\t%s\t%s\n", __token_index++, 
               tk_names[token.kind], 
               tk_names[token.kind]);
    }
    else if (TK_LITERAL_BEGIN <= token.kind && token.kind <= TK_LITERAL_END) {
        if (token.s) printf("%d\t%s\t%s\n", __token_index++, buff, tk_names[token.kind]);
    }
}


#endif //LEXER_H
