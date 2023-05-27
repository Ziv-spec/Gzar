#ifndef PARSER_H
#define PARSER_H

// NOTE(ziv): The parser uses three main types for generating the Ast which will represent
// It's structure. The Translation Unit is a unit which describes a file. The Expression 
// which describes a expression, and a Statement. 
// For the expressions I might have done a pretty poor job with the different kind and the 
// seperation of these different kinds. But the idea still stays the same, for different kinds 
// of expressions you will have to use the appropriate name to get the correct data out of 
// these structures. 
//
// For example a expression can be described as "1+b-c*5" or some_function_name(var1, ...)
// where "b" and "c" are expressions themselves, so are the arguments to the function call
// The expression can easily be drawn as a tree of `Expr` nodes with some nodes describing 
// some operations on some other nodes while others don't. 
// As for statements, you can have expressions be statements, return statement, block, if 
// and so on.. 
// The tree which is generated is usually called a Ast - Abstract Syntax Tree. Do note 
// that you can have different representations for the underlaying language but Ast seems 
// to be the most popular and well understood so I use it as the structure to represent 
// my language in. 

typedef enum Expr_Kind Expr_Kind; 
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

typedef struct Expr Expr; 
struct Expr {
    Expr_Kind kind; 
    Type *type;
    Register reg;  // I should remove this
    
    union {
        
        struct Literal {
            Type_Kind kind;
            void *data; 
        } literal; 
        
        struct Unary {
            Token operation; 
            Type *type;
            Expr *right; 
        } unary;
        
        struct Assignement {
            Expr *lvar;
            Expr *rvalue;
        } assign;
        
        struct Left_Variable {
            Token name; 
        } lvar; 
        
        Vector *args; 
        
        struct Call {
            Expr *name; // lvar
            Expr *args; // Args
        } call; 
        
        
        // TODO(ziv): add subscrip expression 
        // My thoughts after reading this todo a couple weeks later: first I need to implement integer and calling functions before I even think about implementing 
        // arrays (thought that would be cool)
        
        
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
enum Statement_Kind {
    STMT_EXPR, 
    STMT_RETURN,
    STMT_SCOPE,
    
    STMT_IF,  // change to if_else
    STMT_WHILE, // change to loop
    
    STMT_DECL_BEGIN, 
    STMT_VAR_DECL,
    STMT_FUNCTION_DECL,
    STMT_DECL_END, 
}; 

typedef struct Symbol Symbol; 
struct Symbol {
    Token name; 
    Type *type;
    Expr *initializer;
}; 

typedef struct Block Block;
struct Block {
    Map *locals;        // a hashtable of `Symbol` types to store the locals declared in a block
    Vector *statements; // a list of statements inside the scope 
    
    // the following is needed for code generation
    int size;   // the total size of the block
    int offset; // the offset from the beginning of the function
};

typedef struct Statement Statement; 
struct Statement {
    Statement_Kind kind; 
    
    union {
        
        Expr *expr;
        Expr *ret;
        Symbol var_decl; 
        
        struct Function {
            Token name; 
            Type *type;
            Statement *sc; 
        } func;
        
        struct {
            Token position; // this is just used for reporting errors to the user (maybe I could put it somewhere else but meh)
            Expr *condition;
            Statement *true_block; 
            Statement *false_block; 
        } if_else; 
        
        struct {
            Token position; // same as the if_else thingy up ^
            Expr *condition; 
            Statement *block;
        } loop;
        
        Block block;
    };
};

typedef struct Translation_Unit Translation_Unit; 
struct Translation_Unit {
    Token_Stream *restrict s; // Stream of tokens from the lexer
    Vector *unnamed_strings;  // constnat strings which don't have a name (used in the x86 gen)
    Vector *decls;
    
    // anything that defines a scope can be in here. e.g. functions/blocks
    Vector *scopes; 
    int offset; 
    
    // memory pool from which most allocation will happen on
    M_Pool m; 
    
};

/* TODO(ziv): rethink this!!!! please!!!!
typedef struct Program Program; 
struct Program {
    Vector *decls; 
    Block block;
};
*/


/* helper functions */
internal inline void  back_one(Translation_Unit *tu);
internal inline int   is_at_end(Translation_Unit *tu); 
internal inline Token peek(Translation_Unit *tu); 
internal inline Token advance(Translation_Unit *tu); 
internal inline Token previous(Translation_Unit *tu); 
internal inline bool  check(Translation_Unit *tu, Token_Kind kind); 
internal inline Token consume(Translation_Unit *tu, Token_Kind kind, char *msg);
internal bool  internal_match(Translation_Unit *tu, int n, ...);
internal void  report(Translation_Unit *tu, int line, int ch, char *msg); 
internal void  parse_error(Translation_Unit *tu, Token token, char *msg); 
internal void  syntax_error(Translation_Unit *tu,Token token, const char *err);

#define match(l, ...) internal_match(l, COUNT_ARGS(__VA_ARGS__), ##__VA_ARGS__)

////////////////////////////////

internal bool is_at_global_block(Translation_Unit *tu); 
internal Statement *get_curr_scope(Translation_Unit *tu);
internal void push_scope(Translation_Unit *tu, Statement *block);
internal Statement *pop_scope(Translation_Unit *tu);
internal bool add_symbol(Block *block, Symbol *decl); /* adds a symbol decloration to the block */ 
internal Symbol *symbol_exist(Block *block, Token name); /* returns the symbol found inside a block */
internal Symbol *local_exist(Translation_Unit *tu, Token var_name); /* returns the symbol found inside the translation unit */ 
internal void parse_syncronize(Translation_Unit *tu);
////////////////////////////////

/* resolving statements */
internal void parse_translation_unit(Translation_Unit *tu, Token_Stream *restrict s);
internal Statement *parse_scope_stmt(Translation_Unit* tu);
internal Statement *parse_if_stmt(Translation_Unit *tu); 
internal Statement *parse_while_stmt(Translation_Unit *tu); 
internal Statement *parse_decloration(Translation_Unit* tu);
internal Statement *parse_variable_decloration(Translation_Unit* tu, Token name);
internal Statement *parse_function_decloration(Translation_Unit* tu, Token name);
internal Statement *parse_statement(Translation_Unit* tu);
internal Statement *parse_expr_stmt(Translation_Unit* tu);
internal Statement *parse_return_stmt(Translation_Unit* tu);

internal Type      *parse_type(Translation_Unit *tu);

/* resolving expressions */
internal Expr *parse_expression(Translation_Unit* tu);
internal Expr *parse_assignment(Translation_Unit* tu);
internal Expr *parse_logical(Translation_Unit* tu);
internal Expr *parse_bitwise(Translation_Unit* tu);
internal Expr *parse_equality(Translation_Unit* tu); 
internal Expr *parse_comparison(Translation_Unit* tu); 
internal Expr *parse_term(Translation_Unit* tu);
internal Expr *parse_factor(Translation_Unit* tu); 
internal Expr *parse_unary(Translation_Unit* tu);
internal Expr *parse_call(Translation_Unit* tu); 
internal Expr *parse_arguments(Translation_Unit* tu);
internal Expr *parse_primary(Translation_Unit* tu); 

////////////////////////////////

/* initializers for the different statements */ 
internal Statement *init_scope(M_Pool *mem); 
internal Statement *init_expr_stmt(M_Pool *mem, Expr *expr); 
internal Statement *init_return_stmt(M_Pool *mem, Expr *expr);
internal Statement *init_var_decl_stmt(M_Pool *mem, Symbol *symb);
internal Statement *init_func_decl_stmt(M_Pool *mem, Token name, Type *ty, Statement *sc);
internal Statement *init_if_stmt(M_Pool *mem, Expr *condition, Token position, Statement *true_block,  Statement *false_block);
internal Statement *init_while_stmt(M_Pool *mem, Expr *condition, Statement *block);

internal Type      *init_type(M_Pool *mem, Type_Kind kind, Type *subtype, Vector *symbols);
internal Symbol    *init_symbol(M_Pool *mem, Token name, Type *type, Expr *initializer);

/* initializers for the different types of expressions */ 
internal Expr *init_binary(M_Pool *mem, Expr *left, Token operation, Expr *right); 
internal Expr *init_unary(M_Pool *mem, Token operation, Type *type, Expr *right); 
internal Expr *init_literal(M_Pool *mem, void *data, Type_Kind kind); 
internal Expr *init_grouping(M_Pool *mem, Expr *expr); 
internal Expr *init_assignment(M_Pool *mem, Expr *lvalue, Expr *rvalue);
internal Expr *init_call(M_Pool *mem, Expr *name, Expr *args); 
internal Expr *init_arguments(M_Pool *mem, Vector *args); 
internal Expr *init_lvar(M_Pool *mem, Token name);

#endif //PARSER_H
