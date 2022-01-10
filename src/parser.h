#ifndef PARSER_H
#define PARSER_H

typedef struct Expr Expr; 
typedef enum Value_Kind Value_Kind; 
typedef enum Expr_Kind Expr_Kind; 
typedef Expr Binary; 

enum Value_Kind {
    VALUE_INTEGER, 
    VALUE_STRING, 
    VALUE_FALSE, 
    VALUE_TRUE, 
    VALUE_NIL, 
}; 

enum Expr_Kind {
    EXPR_GROUPING, 
    EXPR_LITERAL, 
    EXPR_UNARY, 
    EXPR_BINARY, 
    EXPR_ASSIGN, 
    EXPR_LVAR, 
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
        
        struct Assignement {
            Expr *left_variable;
            Expr *rvalue;
        } assign;
        
        struct Left_Variable{
            Token name; // name of the variable
            int offset; // offset in the stack
        } left_variable; 
        
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
    STMT_RETURN,
}; 

struct Statement {
    Statement_Kind kind; 
    
    union {
        
        // for the time being I will allow the print
        // function to be a stand alone expression
        Expr *print_expr; // TODO(ziv): rethink the name
        Expr *ret;
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
internal Expr *init_assignement(Expr *lvalue, Expr *rvalue);


//////////////////////////////// =============================
// TODO(ziv): @nochecking this is bound to change in design. for the time being I will use this.
internal Expr *init_lvar(Token name, int offset);
internal int next_offset();
static int global_next_offset = 0; 

// this is for lvars
static Expr *locals[20]; 
static int locals_index; 

internal Expr *local_exist(Token token);
internal void add_locals(Expr *lvar);

//////////////////////////////// =============================


/* initializers for the different statements */ 
internal Statement *init_expr_stmt(Expr *expr); 
internal Statement *init_print_stmt(Expr *expr);
internal Statement *init_decl_stmt(Token name, Expr *expr);
internal Statement *init_return_stmt(Expr *expr);

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
internal Expr *expression();
internal Expr *assignement();
internal Expr *equality(); 
internal Expr *comparison(); 
internal Expr *term();
internal Expr *factor(); 
internal Expr *unary();
internal Expr *primary(); 

/* resolving statements */
internal Statement *statement();
internal Statement *decloration();
internal Statement *print_stmt();
internal Statement *expr_stmt();


internal void parse_file();


#endif //PARSER_H
