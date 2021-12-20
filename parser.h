#ifndef PARSER_H
#define PARSER_H

#define is_token_op(tk) (TK_OP_BEGIN < tk.kind && tk.kind < TK_OP_END)
#define is_token_operand(tk) (TK_LITERAL_BEGIN < tk.kind && tk.kind < TK_LITERAL_END)

#define is_kind_op(kind) (TK_OP_BEGIN < kind && kind < TK_OP_END)
#define is_kind_operand(kind) (TK_LITERAL_BEGIN < kind && kind < TK_LITERAL_END)
#define is_kind_op_or_operand(kind) (is_kind_op(kind) || is_kind_operand(kind))

#if 0

/* 
#define NULL_TOKEN (Token){ 0 }; 
#define NULL_EXPR  (Ast_Expr){ 0 }; 
 */

// How this should look like: 
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

enum Value_Kind {
    VALUE_INTEGER, 
    VALUE_STRING, 
}; 

enum Expr_Kind {
    EXPR_GROUPING, 
    EXPR_LITERAL, 
    EXPR_UNARY, 
    EXPR, 
    
}; 

struct Expr {
    Expr_Kind kind; 
    
    union {
        struct Grouping {
            Expr *expression; 
        } grouping;
        
        struct Literal {
            Value_Kind kind;
            char *value; // probably should be more specific here 
        } literal; 
        
        struct Unary {
            Token operator; 
            Expr *right; 
        } unary;
        
        // This is what the actual expression should contain (as a regular expression)
        struct {
            Expr *left; 
            Token *operation; 
            Expr *right; 
        }; 
    }; 
    
}; 






#else /* =============================================================== */

typedef struct Parser Parser; 
typedef struct Ast_Expr Ast_Expr; 
typedef struct Ast_Value Ast_Value;
typedef enum Ast_Kind Ast_Kind;
typedef struct Ast_Dump Ast_Dump; 

enum Ast_Kind {
    AST_EXPR, 
    AST_INTEGER, 
};

struct Ast_Value { 
    Ast_Kind kind; 
    union {
        Ast_Expr *expr;
        char *integer; 
    };
}; 

struct Ast_Expr {
    Token_Kind op; // the operation that is needed to be executed on the right and left operands
    Ast_Value right; 
    Ast_Value left; 
}; 

// TODO(ziv): think about this some more 
struct Ast_Dump {
    Ast_Kind kind; 
    union {
        char *integer;
        Ast_Expr *expr; // the expression we want to dump
    }; 
}; 


struct Parser {
    Lexer *lexer; 
    Ast_Expr *expr; // for the time being it is going to be a add
}; 


char *to_literal(Token token) {
    Assert(is_kind_operand(token.kind));
    
    // TODO(ziv): extend this to something that handles more 
    // than just a integer
    Assert(token.kind == TK_INTEGER);
    
    char *result = (char *)malloc((token.len+1) * sizeof(char));
    strncpy(result, token.s, token.len); 
    result[token.len] = '\0';
    
    return result;
}

Ast_Expr *parse_expr(Parser *parser) {
    Lexer *lexer = parser->lexer; 
    
    Ast_Expr *expr = (Ast_Expr *)malloc(sizeof(Ast_Expr)); 
    if (!expr) {
        fprintf(stderr, "Error: out of memory\n");
        return NULL;
    }
    
    if (get_next_token(lexer)) {
        
        switch (lexer->token.kind) {
            
            default: {
                if (is_kind_operand(lexer->token.kind)) {
                    fprintf(stderr, "Error: syntax error, expected to begin with a operation but got: '%s'\n", tk_names[lexer->token.kind]);
                    return NULL;
                }
                else {
                    Assert(!"Token not impelemented!!!"); 
                }
            } break; 
            
            case TK_EOF: {
                return NULL;
            }break; 
            
            
            //
            // handling ops
            //
            
            case TK_PLUS: 
            case TK_MINUS: 
            {
                expr->op = lexer->token.kind;
                
                if (top_next_token(lexer)) {
                    if (is_kind_op(lexer->token.kind)) {
                        expr->left.kind = AST_EXPR;
                        expr->left.expr = parse_expr(parser); 
                    }
                    else if (is_kind_operand(lexer->token.kind)) {
                        Assert(lexer->token.kind == TK_INTEGER); 
                        get_next_token(lexer);
                        
                        expr->left.kind = AST_INTEGER;
                        expr->left.integer = to_literal(lexer->token); 
                        
                    }
                    else {
                        fprintf(stderr, "Error: expected operation or literal got '%s'\n", tk_names[lexer->token.kind]); 
                    }
                }
                
                
                if (top_next_token(lexer)) {
                    if (is_kind_op(lexer->token.kind)) {
                        expr->right.kind = AST_EXPR;
                        expr->right.expr = parse_expr(parser); 
                    }
                    else if (is_kind_operand(lexer->token.kind)) {
                        Assert(lexer->token.kind == TK_INTEGER); 
                        get_next_token(lexer);
                        
                        expr->right.kind = AST_INTEGER;
                        expr->right.integer = to_literal(lexer->token); 
                        
                    }
                    else {
                        fprintf(stderr, "Error: expected operation or literal got '%s'\n", tk_names[lexer->token.kind]); 
                    }
                }
                
            } break;
            
        }
    }
    
    return expr;
}


bool parse_file(Parser *parser) {
    Lexer *lexer = parser->lexer; 
    
    Ast_Expr add_node = {0};
    
    // 
    // Create the ast parse tree
    // 
    Ast_Expr *expr; 
    while (top_next_token(lexer)) {
        
        switch (lexer->token.kind) {
            
            default: {
                if (is_kind_operand(lexer->token.kind)) {
                    fprintf(stderr, "Error: syntax error, must begin with a operation\n");
                    return false;
                }
                else {
                    Assert(!"Token impel needs to be done"); 
                }
            } break; 
            
            case TK_MINUS:
            case TK_PLUS: {
                expr = parse_expr(parser);
                
                parser->expr = expr; 
                
            } break; 
            
            
            case TK_DUMP: {
                //parse_dump(parser);
            } break;
            
            case TK_EOF: {
                // do nothing
                goto end;
            }break; 
            
        }
    }
    
    end: 
    return true; 
}


int value_to_int(Ast_Value *value) {
    return atoi(value->integer); 
}

int sim_add(int right, int left) {
    return left+right; 
}

int sim_minus(int right, int left) {
    return left-right; 
}



int sim_expr(Ast_Expr *expr) {
    Assert(expr); // i'm expecting a expression
    
    int right, left, result = 0;
    
    if (expr->right.kind == AST_EXPR) {
        right = sim_expr(expr->right.expr);
    }
    else if (expr->right.kind == AST_INTEGER) {
        right = atoi(expr->right.integer);
    }
    
    
    if (expr->left.kind == AST_EXPR) {
        left = sim_expr(expr->left.expr);
    }
    else if (expr->left.kind == AST_INTEGER) {
        left = atoi(expr->left.integer);
    }
    
    
    switch (expr->op) {
        
        case TK_PLUS: {
            result = sim_add(right, left);
        } break;
        
        case TK_MINUS: {
            result = sim_minus(right, left);
        } break;
        
    }
    
    return result; 
}

void sim_file(Parser *parser) {
    
    if (!parse_file(parser)) {
        // parsing the file has gone wrong 
        return; 
    }
    
    // 
    // Go over the parse tree and execute the commands 
    // 
    
    int sim_result = sim_expr(parser->expr); 
    printf("result = %d\n", sim_result);
    //int sim_result = sim_add(&parser->expr); 
    
}
#endif 

#endif //PARSER_H
