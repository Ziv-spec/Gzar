#ifndef LEXER_H
#define LEXER_H

#define MAX_LEN 255

typedef enum Token_Kind Token_Kind; 

enum Token_Kind {
    
    TK_PLUS       = '+',
    TK_MINUS      = '-',
    TK_BANG       = '!',
    TK_SLASH      = '/',
    TK_STAR       = '*',
    TK_GREATER    = '>',
    TK_LESS       = '<',
    TK_ASSIGN     = '=',
    TK_COMMA      = ',',
    TK_COLON      = ':',
    TK_RPARAN     = '(',
    TK_LPARAN     = ')',
    TK_RBRACE     = '{',
    TK_LBRACE     = '}', 
    TK_SEMI_COLON = ';',
    TK_AND        = '&', 
    TK_OR         = '|', 
    TK_XOR        = '^', 
    TK_MODOLU     = '%', 
    TK_ASCII = 256,
    
    TK_GREATER_EQUAL = '>' + 320, // >=
    TK_LESS_EQUAL    = '<' + 320, // <=
    TK_BANG_EQUAL    = '!' + 320, // != 
    TK_EQUAL_EQUAL   = '=' + 320, // ==
    TK_DOUBLE_COLON  = ':' + 320, // ::
    TK_RETURN_TYPE   = '-' + 320, // ->
    TK_DOUBLE_AND    = '&' + 320, // &&
    TK_DOUBLE_OR     = '|' + 320, // ||
    
    TK_DOUBLE_ASCII  = 256 + 320, 
    
    TK_IDENTIFIER, // e.g. this_is_someValidIdentifier123456789
    TK_NUMBER,  // e.g. 123
    TK_STRING,  // e.g. "abc"
    
    TK_KEYWORD_BEGIN,
    TK_IF,      // if 
    TK_ELSE,    // else
    TK_WHILE,   // while 
    TK_FALSE,   // false
    TK_TRUE,    // true
    TK_NIL,     // nil
    TK_RETURN,  // return
    TK_CAST,    // cast 
    
    TK_TYPE_BEGIN, 
    TK_S8_TYPE, 
    TK_S16_TYPE, 
    TK_S32_TYPE, 
    TK_S64_TYPE, 
    
    TK_U8_TYPE, 
    TK_U16_TYPE, 
    TK_U32_TYPE, 
    TK_U64_TYPE, 
    
    TK_INT_TYPE, 
    TK_VOID_TYPE, 
    TK_STRING_TYPE,
    TK_BOOL_TYPE,
    TK_TYPE_END, 
    TK_KEYWORD_END,
    
    TK_EOF, 
    TK_COUNT,
};

typedef struct Token Token; 
struct Token { 
    String8 str;     // the tokens string representation
    Token_Kind kind; // token kind
};

typedef struct Token_Stream Token_Stream; 
struct Token_Stream {
    char *start;  // start of the file from which tokens are generated 
    Token *s;     // array of all tokens
    int current;  // current token 
    int capacity; // size of the array
}; 

////////////////////////////////
internal bool lex_file(Token_Stream *s); 

#endif //LEXER_H
