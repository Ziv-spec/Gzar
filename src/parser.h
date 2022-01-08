#ifndef PARSER_H
#define PARSER_H

// 
// expression -> literal 
//               | unary
//               | binary 
//               | grouping
// 
// literal -> NUMBER | STRING | "true" | "false" | "nil" 
// grouping -> "(" expression ")"
// unary -> ("-" | "!") expression
// binary -> expression operator expression 
// operator -> "==" | "!=" | ">=" | "<=" | "<" |
//             ">" | "+" | "-" | "*" | "/"

typedef struct Expr Expr; 
typedef struct Grouping Grouping; 
typedef struct Unary Unary; 
typedef struct Literal Literal; 
typedef enum Value_Kind Value_Kind; 
typedef enum Expr_Kind Expr_Kind; 
typedef Expr Binary; 

void gen(Expr *expr);

enum Value_Kind {
    VALUE_INTEGER, 
    VALUE_STRING, 
    VALUE_FALSE, 
    VALUE_TRUE, 
    VALUE_NIL, 
}; 

enum Expr_Kind {
    EXPR_GROUPING = 1, 
    EXPR_LITERAL, 
    EXPR_UNARY, 
    EXPR_BINARY, 
}; 

struct Expr {
    Expr_Kind kind; 
    
    union {
        struct Grouping {
            Expr *expr; 
        } grouping;
        
        struct Literal {
            Value_Kind kind;
            void *data; 
        } literal; 
        
        struct Unary {
            Token operation; 
            Expr *right; 
        } unary;
        
        // binary expression which the most commonly used
        // out of all of the types of expressions, so I 
        // left out the name to indicate this is what I 
        // assume it to be most of the time. 
        struct {
            Expr *left; 
            Token operation; 
            Expr *right; 
        }; 
    }; 
    
};

typedef enum Statement_Kind Statement_Kind; 
typedef struct Statement Statement; 

enum Statement_Kind {
    STMT_EXPR, 
    STMT_PRINT_EXPR, 
    STMT_VAR_DECL,
}; 

struct Statement {
    Statement_Kind kind; 
    
    union {
        
        // for the time being I will allow the print
        // function to be a stand alone expression
        Expr *print_expr; // TODO(ziv): rethink the name
        
        Expr *expr;
        
        struct {
            Token name; 
            Expr *initializer;
        } var_decl; 
        
    };
};

// NOTE(ziv): I will not bother with dynamic
// arrays this will be left for the final 
// design to implement.

static Statement *statements[10]; 
static unsigned int statements_index = 0;


/* initializers for the different types of expressions */ 
internal Expr *init_binary(Expr *left, Token operation, Expr *right); 
internal Expr *init_unary(Token operation, Expr *right); 
internal Expr *init_literal(void *data, Value_Kind kind); 
internal Expr *init_grouping(Expr *expr); 

/* helper functions */
internal int   is_at_end(); 
internal Token peek(); 
internal Token advance(); 
internal Token previous(); 
internal bool  check(Token_Kind kind); 
internal bool  internal_match(int n, ...);
internal void  report(int line, char *msg); 
internal void  error(Token token, char *msg); 
internal Token consume(Token_Kind kind, char *msg);

#define match(...) internal_match(COUNT_ARGS(__VA_ARGS__), ##__VA_ARGS__)

/* resolving expressions */
internal Expr *primary(); 
internal Expr *unary();
internal Expr *factor(); 
internal Expr *term();
internal Expr *comparison(); 
internal Expr *equality(); 
internal Expr *expression();

internal Expr *parse_file();


#endif //PARSER_H
