#ifndef LEXER_H
#define LEXER_H

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
    TK_PROC,    // proc
    TK_PRINT,   // print <------------- probably should remove this in the future as i see fit
    
    TK_TYPE_BEGIN, 
    TK_S8_TYPE, 
    TK_S16_TYPE, 
    TK_S32_TYPE, 
    
    TK_U8_TYPE, 
    TK_U16_TYPE, 
    TK_U32_TYPE, 
    
    TK_INT_TYPE, 
    TK_VOID_TYPE, 
    TK_STRING_TYPE,
    TK_BOOL_TYPE,
    TK_TYPE_END, 
    
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
    
    TK_NUMBER,  // e.g. 123
    TK_STRING,  // e.g. "abc"
    
    TK_DOUBLE_COLON,  // ::
    TK_RETURN_TYPE,   // ->
    TK_COMMA,      // , 
    TK_SEMI_COLON, // ;
    TK_COLON,      // :
    TK_RPARAN,     // (
    TK_LPARAN,     // )
    TK_RBRACE,     // {
    TK_LBRACE,     // }
    TK_IDENTIFIER, // e.g. this_is_someValidIdentifier123456789
    
    TK_EOF, 
    TK_COUNT,
};

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


static Token tokens[1024]; 
static unsigned int tokens_index;
static unsigned int tokens_len;

internal int keyword_cmp(char *str1, char *str2);



// keywords  TODO(ziv): maybe keep this up date
static char *tk_names[] = {
    
    [TK_IF]     = "if",
    [TK_ELSE]   = "else",
    [TK_WHILE]  = "while",
    [TK_RETURN] = "return",
    [TK_PRINT]  = "print",
    // ops
    [TK_PLUS]    = "+",
    [TK_MINUS]   = "-",
    [TK_BANG]    = "!",
    [TK_SLASH]   = "/ ",
    [TK_STAR]    = "*",
    [TK_GREATER] = ">",
    [TK_LESS]    = "<",
    [TK_ASSIGN]  = "= ",
    [TK_GREATER_EQUAL] = ">=",
    [TK_LESS_EQUAL]    = "<=",
    [TK_BANG_EQUAL]    = "!= ",
    [TK_EQUAL_EQUAL]   = "==",
    
    // literals
    [TK_FALSE]  = "false",
    [TK_TRUE]   = "true",
    [TK_NIL]    = "nil",
    
    // single character tokens that I don't know how to name
    [TK_SEMI_COLON] = ";",
    [TK_COMMA]  = ",",
    [TK_RPARAN] = "(",
    [TK_LPARAN] = ")",
    [TK_RBRACE] = "{",
    [TK_LBRACE] = "}",
    
}; 

#endif //LEXER_H
