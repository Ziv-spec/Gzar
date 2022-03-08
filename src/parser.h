#ifndef PARSER_H
#define PARSER_H

typedef struct Expr Expr; 
typedef enum Expr_Kind Expr_Kind; 
typedef Expr Binary; 

enum Expr_Kind {
    EXPR_GROUPING, 
    EXPR_LITERAL, 
    EXPR_UNARY, 
    EXPR_BINARY, 
    EXPR_ASSIGN, 
    EXPR_LVAR,
    EXPR_CALL, 
    EXPR_ARGUMENTS,
}; 

// TODO(ziv): maybe simplify this model by not using named unions? 
// for the time being I don't see this as a major thing that I need to do 
// but I will have to see whether this adds clarity or not over time. 
struct Expr {
    Expr_Kind kind; 
    Type *type;
    
    union {
        struct Grouping {
            Expr *expr; 
        } grouping;
        
        struct Literal {
            Type_Kind kind;
            void *data; 
        } literal; 
        
        struct Unary {
            Token operation; 
            Expr *right; 
        } unary;
        
        struct Assignement {
            Expr *lvar;
            Expr *rvalue;
        } assign;
        
        struct Left_Variable {
            Token name; 
        } lvar; 
        
        
        // TODO(ziv): add subscrip expression 
        // My thoughts after reading this todo a couple weeks later: first I need to implement integer and calling functions before I even think about implementing 
        // arrays (thought that would be cool)
        
        struct Args {
            Expr *lvar; // current arguemnt
            Expr *next; // next arguemnt (linked list)
        } args; 
        
        struct Call {
            Expr *name; 
            Expr *args; 
        } call; 
        
        
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
typedef struct Symbol Symbol; 
typedef struct Scope Scope;

enum Statement_Kind {
    STMT_EXPR, 
    STMT_PRINT_EXPR, 
    STMT_RETURN,
    STMT_SCOPE,
    
    STMT_DECL_BEGIN, 
    STMT_VAR_DECL,
    STMT_FUNCTION_DECL,
    STMT_DECL_END, 
}; 

struct Symbol {
    Token name; 
    Type *type;
    Expr *initializer;
}; 

struct Scope {
    Vector *locals;
    Vector *statements;
};

struct Statement {
    Statement_Kind kind; 
    
    union {
        
        union {
            
            // for the time being I will allow the print
            // function to be a stand alone expression
            Expr *print_expr; 
            Expr *expr;
            Expr *ret;
            
            struct {
                Token name; 
                Type *type;
                Expr *initializer;
            } var_decl; 
            
            struct Function {
                Token name; 
                Type *type;
                Statement *sc; 
            } func;
            
            Scope block;
            
        };
        
    };
};


typedef struct Program Program; 
struct Program {
    Vector *decls; 
    Scope block;
};

static Scope global_block;


/* initializers for the different types of expressions */ 
internal Expr *init_binary(Expr *left, Token operation, Expr *right); 
internal Expr *init_unary(Token operation, Expr *right); 
internal Expr *init_literal(void *data, Type_Kind kind); 
internal Expr *init_grouping(Expr *expr); 
internal Expr *init_assignement(Expr *lvalue, Expr *rvalue);

////////////////////////////////

internal Expr *init_lvar(Token name);

static Statement *scopes[100];
static unsigned int scope_index; 

internal Statement *get_curr_scope();
internal Statement *next_scop();
internal void push_scope(Statement *block);
internal Statement *pop_scope();

internal void add_decl(Scope block, Symbol *decl);
internal void scope_add_variable(Scope block, Token name, Type *type);

////////////////////////////////


/* initializers for the different statements */ 
internal Statement *init_expr_stmt(Expr *expr); 
internal Statement *init_print_stmt(Expr *expr);
internal Statement *init_decl_stmt(Token name, Type *type, Expr *initializer);
internal Statement *init_return_stmt(Expr *expr);
internal Statement *init_scope(); 
internal Statement *init_func_decl_stmt(Token name, Type *ty, Statement *sc);


/* helper functions */
internal void  back_one();
internal int   is_at_end(); 
internal Token peek(); 
internal Token advance(); 
internal Token previous(); 
internal bool  check(Token_Kind kind); 
internal bool  internal_match(int n, ...);
internal void  report(int line, int ch, char *msg); 
internal void  error(Token token, char *msg); 
internal void  syntax_error(Token token, const char *err);
internal Token consume(Token_Kind kind, char *msg);

internal Type *local_exist(Token var_name);

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
internal Statement *scope();
internal Statement *decloration();
internal Statement *variable_decloration(Token name);
internal Statement *function_decloration(Token name);
internal Statement *statement();
internal Statement *print_stmt();
internal Statement *expr_stmt();
internal Statement *return_stmt();

/* other */
internal Vector *function_arguments();
internal Type   *parse_type();
internal Type   *symbol_exist(Scope block, Token name); 

internal void parse_file();


#endif //PARSER_H
