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
    TK_PROC,    // proc/*
    TK_PRINT,   // print <------------- probably should remove this in the future as i see fit
    
    // TYPES
    TK_S8,  // s8
    TK_S32, // s32
    TK_U8,  // u8
    TK_U32, // u32
    
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
    TK_DOUBLE_COLON,  // ::
    TK_OP_END,
    
    TK_LITERAL_BEGIN, 
    TK_NUMBER,  // e.g. 123
    TK_STRING,  // e.g. "abc"
    TK_LITERAL_END, 
    
    TK_RETURN_TYPE, // ->
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
    keywords[TK_PRINT]  = "print";
    keywords[TK_PROC]   = "proc";
    
    keywords[TK_S8]     = "s8";
    keywords[TK_S32]    = "s32";
    keywords[TK_U8]     = "u8";
    keywords[TK_U32]    = "u32";
}

static char *tk_names[TK_COUNT];

// this is kind of deprecated so idk about keeping it up-to date
internal void init_tk_names() { 
    
    // keywords  TODO(ziv): maybe keep this up date
    tk_names[TK_IF]     = "if";
    tk_names[TK_ELSE]   = "else";
    tk_names[TK_WHILE]  = "while";
    tk_names[TK_RETURN] = "return";
    tk_names[TK_PRINT]  = "print";
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
    tk_names[TK_COMMA]  = ",";
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


static Token tokens[1024]; 
static unsigned int tokens_index;
static unsigned int tokens_len;

// NOTE(ziv): DEPRECATED
//internal void debug_print_token(Token token); // prints a token in a index, value, type format.

////////////////////////////////

/* lexer function prototypes */



#endif //LEXER_H
