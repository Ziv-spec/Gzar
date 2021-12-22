#ifndef LEXER_H
#define LEXER_H

/* lexer function prototypes */

#define MAX_LEN 255

typedef enum Token_Kind Token_Kind; 
typedef struct Token Token; 
typedef struct Lexer Lexer; 

enum Token_Kind {
    // NOTE(ziv): keywords are required to 
    // be at the beginning here.
    TK_KEYWORD_BEGIN,
    TK_IF,      // if 
    TK_ELSE,    // else
    TK_WHILE,   // while 
    TK_FALSE,   // false
    TK_TRUE,    // true
    TK_NIL,     // nil
    TK_RETURN,  // return
    // TODO(ziv): add types maybe??
    TK_KEYWORD_END,
    
    TK_OP_BEGIN,
    TK_PLUS,    // + 
    TK_MINUS,   // - 
    TK_BANG,    // !
    TK_SLASH,   // / 
    TK_STAR,    // *
    TK_GREATER, // >
    TK_LESS,    // <
    TK_ASSIGN,  // = 
    TK_GREATER_EQUAL, // >=
    TK_LESS_EQUAL,    // <=
    TK_BANG_EQUAL,    // != 
    TK_EQUAL_EQUAL,   // ==
    TK_OP_END,
    
    TK_LITERAL_BEGIN, 
    TK_NUMBER,  // e.g. 123
    TK_STRING,  // e.g. "abc"
    TK_LITERAL_END, 
    
    TK_SEMI_COLON, // ;
    TK_RPARAN,     // (
    TK_LPARAN,     // )
    TK_RBRACE,     // {
    TK_LBRACE,     // }
    TK_IDENTIFIER, // e.g. this_is_someValidIdentifier123456789
    
    TK_EOF, 
    TK_COUNT,
};

static char *keywords[TK_KEYWORD_END]; 

internal void init_keywords() {
    keywords[0]         = NULL;
    keywords[TK_IF]     = "if";
    keywords[TK_ELSE]   = "else";
    keywords[TK_WHILE]  = "while";
    keywords[TK_FALSE]  = "false";
    keywords[TK_TRUE]   = "true";
    keywords[TK_NIL]    = "nil";
    keywords[TK_RETURN] = "return";
}

static char *tk_names[TK_COUNT];

internal void init_tk_names() { 
    
    // keywords 
    tk_names[TK_IF]     = "if";
    tk_names[TK_ELSE]   = "else";
    tk_names[TK_WHILE]  = "while";
    tk_names[TK_RETURN] = "return";
    
    // ops
    tk_names[TK_PLUS]    = "+";
    tk_names[TK_MINUS]   = "-";
    tk_names[TK_BANG]    = "!";
    tk_names[TK_SLASH]   = "/ ";
    tk_names[TK_STAR]    = "*";
    tk_names[TK_GREATER] = ">";
    tk_names[TK_LESS]    = "<";
    tk_names[TK_ASSIGN]  = "= ";
    tk_names[TK_GREATER_EQUAL] = ">=";
    tk_names[TK_LESS_EQUAL]    = "<=";
    tk_names[TK_BANG_EQUAL]    = "!= ";
    tk_names[TK_EQUAL_EQUAL]   = "==";
    
    // literals
    tk_names[TK_NUMBER] = "int";
    tk_names[TK_STRING] = "string";
    tk_names[TK_FALSE]  = "false";
    tk_names[TK_TRUE]   = "true";
    tk_names[TK_NIL]    = "nil";
    
    // single character tokens that I don't know how to name
    tk_names[TK_SEMI_COLON] = ";";
    tk_names[TK_RPARAN] = "(";
    tk_names[TK_LPARAN] = ")";
    tk_names[TK_RBRACE] = "{";
    tk_names[TK_LBRACE] = "}";
    
}

struct Token { 
    // NOTE(ziv): This location of the token will 
    // be removed in future versions of this compiler
    // as it is rarely used and it slows down (probably)
    // the compiler (due to increased size of the tokens
    // inside the cash line). 
    // 
    // Honestly as proven inside the zig compiler 
    // I can just use a pointer to the first instance 
    // of the token, and the token kind and not worry 
    // about any thing else at all. 
    // Location can be calcualted when needed at a much
    // better way. The length of the token might be 
    // important. That I shall see in the future
    // (though maybe half of the tokens are just 
    // 1 character long, and some more that I don't 
    // care about their size).
    Location loc;    // location of the token
    Token_Kind kind; // token kind
    
    // NOTE(ziv): Token string for error purposes
    int len;         // length of string 
    char *str;       // string representation. 
};

struct Lexer {
    Location loc; // location of the tokenizing process
    char *code;   // source code of the main file
    Token token;  // the current token
};



////////////////////////////////

int __token_index = 0;
//internal void debug_print_token(Token token); // prints a token in a index, value, type format.

////////////////////////////////



/* implementation */

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
            c = lexer->code[loc.index];
            while (c != '"') {
                c = lexer->code[++loc.index];
                loc.ch++;
            }
            token.len = lexer->code + loc.index - token.str; 
        }
        
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
                bool found_keyword = false; 
                for (int i = 1; i < ArrayLength(keywords); i++) {
                    if (my_strcmp(lexer->code + loc.index, keywords[i])) {
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

/* 
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


#endif //LEXER_H
