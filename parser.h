#ifndef PARSER_H
#define PARSER_H

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
    VALUE_FALSE, 
    VALUE_TRUE, 
    VALUE_NIL, 
}; 

enum Expr_Kind {
    EXPR_GROUPING, 
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
        
        // To be more vorbose, I use a union
        // instead of a 'void *' type. 
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


static Token tokens[1024]; 
static unsigned int tokens_index;
static unsigned int tokens_len;

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
internal bool  match(Token_Kind *kinds, int n); 
internal void  report(int line, char *msg); 
internal void  error(Token token, char *msg); 
internal Token consume(Token_Kind kind, char *msg);

/* resolving expressions */
internal Expr *primary(); 
internal Expr *unary();
internal Expr *factor(); 
internal Expr *term();
internal Expr *comparison(); 
internal Expr *equality(); 
internal Expr *expression();

internal int   parse_file(Lexer *lexer);

//////////////////////////////////
// Implementation

internal Expr *init_binary(Expr *left, Token operation, Expr *right) {
    Expr *binary = (Expr *)malloc(sizeof(Expr));
    binary->kind = EXPR_BINARY; 
    binary->left = left; 
    binary->operation = operation; 
    binary->right = right;
    return binary;
}

internal Expr *init_unary(Token operation, Expr *right) {
    Expr *unar = (Expr *)malloc(sizeof(Expr));
    unar->kind = EXPR_UNARY; 
    unar->unary.operation = operation; 
    unar->unary.right = right; 
    return unar;
}

internal Expr *init_literal(void *data, Value_Kind kind) {
    Expr *literal = (Expr *)malloc(sizeof(Expr));
    literal->kind = EXPR_LITERAL; 
    literal->literal.kind = kind; 
    literal->literal.data = data; 
    return literal;
}

internal Expr *init_grouping(Expr *expr){
    Expr *grouping = (Expr *)malloc(sizeof(Expr));
    grouping->kind = EXPR_GROUPING;  
    grouping->grouping.expr = expr; 
    return grouping;
}

//////////////////////////////////

internal int is_at_end() {
    Assert(tokens_index > tokens_len); // TODO(ziv): check whether I need this
    return tokens[tokens_index].kind == TK_EOF;
}

internal Token peek() {
    return tokens[tokens_index];
}

internal Token advance() {
    if (!is_at_end()) tokens_index++; 
    return tokens[tokens_index];
}

internal Token previous() {
    return tokens[tokens_index-1];
}

internal Token consume(Token_Kind kind, char *msg) {
    if (check(kind)) return advance(); 
    error(peek(), msg); 
    return (Token){ 0 };
}

internal void error(Token token, char *msg) {
    char buff[255];
    
    if (token.kind == TK_EOF) {
        strcat(buff, " at end");
        strcat(buff, msg);
        report(token.loc.line, buff); 
    }
    else {
        report(token.loc.line, strcat(" at ", msg));
    }
}

internal void report(int line, char *msg) {
    char err[255];  // holding the error message
    char buff[255]; // holding the integer as a string
    
    strcat(err, msg); 
    strcat(err, itoa(line, buff,10)); 
    fprintf(stdout, err);
    fprintf(stdout, "\n");
    exit(-1); // NOTE(ziv): for the time being, when 
    // the parser is reporting a error for the use 
    // it is not going to continue finding more erorrs. 
    // This is done to simplify the amounts of things 
    // that I need to think about. That will change in 
    // the future as adding this feature will be easier 
    // when I have statements.
}

internal bool check(Token_Kind kind) { 
    if (is_at_end()) return false; 
    return peek().kind == kind; 
}

internal bool match(Token_Kind *kinds, int n) {
    for (int i = 0; i < n; i++) {
        if (check(kinds[i])) {
            advance(); 
            return true;
        }
    }
    return false; 
}

//////////////////////////////////

internal Expr *expression() {
    return equality(); 
}

internal Expr *equality() {
    Expr *expr = comparison(); 
    
    Token_Kind ops[] = { TK_BANG_EQUAL, TK_EQUAL_EQUAL }; 
    while (match(ops, 2)) {
        Token operator = previous(); 
        Expr *right = comparison(); 
        expr = init_binary(expr, operator, right);
    }
    
    return expr;
}

internal Expr *comparison() {
    Expr *expr = term(); 
    
    Token_Kind ops[] = { TK_GREATER, TK_LESS }; 
    while (match(ops, 2)) {
        Token operation = previous(); 
        Expr *right = term(); 
        expr = init_binary(expr, operation, right);
        
    }
    return expr;
}

internal Expr *term() {
    Expr *expr = factor(); 
    
    Token_Kind ops[] = { TK_MINUS, TK_PLUS }; 
    while (match(ops, 2)) {
        Token operation = previous(); 
        Expr *right = factor(); 
        expr = init_binary(expr, operation, right);
    }
    
    return expr;
}

internal Expr *factor() {
    Expr *expr = unary(); 
    
    Token_Kind ops[] = { TK_SLASH, TK_STAR }; 
    while (match(ops, 2)) {
        Token operation = previous(); 
        Expr *right = factor(); 
        expr = init_binary(expr, operation, right);
    }
    
    return expr;
}

internal Expr *unary() {
    
    Token_Kind ops[] = { TK_BANG, TK_MINUS }; 
    while (match(ops, 2)) {
        Token operation = previous(); 
        Expr *right = unary(); 
        return init_unary(operation, right);
    }
    
    return primary();
}

internal Expr *primary() {
    Token_Kind literal_types[] = { TK_FALSE, TK_TRUE, TK_NIL, TK_NUMBER, TK_STRING }; 
    if (match(literal_types+0, 1)) return init_literal(NULL, VALUE_FALSE);
    if (match(literal_types+1, 1)) return init_literal(NULL, VALUE_TRUE);
    if (match(literal_types+2, 1)) return init_literal(NULL, VALUE_NIL);
    
    // number & string
    if (match(literal_types+3, 2)) { 
        Token token = previous();
        Expr *result = NULL; 
        
        switch (token.kind) {
            case TK_NUMBER: {
                u64 value = atoi(token.str); 
                result = init_literal((void *)value, VALUE_INTEGER); 
            } break; 
            
            case TK_STRING: {
                char *buff = (char *)malloc(token.len+1); 
                strncpy(buff, token.str, token.len);
                buff[token.len] = '\0';
                result = init_literal((void *)buff, VALUE_STRING); 
            } break; 
            
        }
        
        return result;
    }
    
    Token_Kind parans[] = { TK_LPARAN, TK_RPARAN }; 
    if (match(parans, 2)) {
        Expr *expr = expression(); 
        consume(TK_RPARAN, "Expected ')' after expression"); 
        return init_grouping(expr); 
    }
    
    Assert(false); // we should never be getting here
    return NULL; 
}

//////////////////////////////////


void print_literal(Expr *expr) {
    Assert(expr->kind == EXPR_LITERAL);
    
    switch (expr->literal.kind) {
        
        case VALUE_TRUE: printf("true"); break;
        case VALUE_FALSE: printf("false"); break;
        case VALUE_NIL: printf("nil"); break;
        
        case VALUE_INTEGER: {
            u64 num = (u64)expr->literal.data; 
            printf("%I64d", num);
        } break;
        
        case VALUE_STRING: {
            char buff[255]; 
            printf((char *)expr->literal.data);
        } break;
        
    }
    
}

void print_operation(Token operation) {
    switch (operation.kind) {
        
        case TK_PLUS:  printf("+"); break;
        case TK_MINUS: printf("-"); break;
        case TK_BANG:  printf("!");break;
        case TK_SLASH: printf("/"); break;
        case TK_STAR:  printf("*");break;
        case TK_GREATER: printf(">");break;
        case TK_LESS:    printf("<");break;
        case TK_ASSIGN:  printf("="); break;
        case TK_GREATER_EQUAL: printf(">");break;
        case TK_LESS_EQUAL:    printf("<");break;
        case TK_BANG_EQUAL:    printf("!");break;
        case TK_EQUAL_EQUAL:   printf("=");break;
        
    }
    
}

void print(Expr *expr) {
    Assert(expr->kind == EXPR_BINARY);
    
    switch(expr->kind) {
        case EXPR_BINARY: {
            printf("(");
            print_operation(expr->operation); printf(" ");
            print(expr->left); printf(" ");
            print(expr->right); 
            printf(")");
        } break;
        
        case EXPR_UNARY: {
            printf("(");
            print_operation(expr->unary.operation); printf(" ");
            print(expr->unary.right);
            printf(")");
        } break;
        
        case EXPR_GROUPING: {
            print(expr->grouping.expr);
        } break;
        
        case EXPR_LITERAL: {
            print_literal(expr);
        } break;
        
        
    }
    
}

internal int parse_file(Lexer *lexer) {
    
    //
    // get a token list for the whole program
    //
    int i = 0;
    for (;  lexer->token.kind != TK_EOF; i++) {
        if (get_next_token(lexer)) {
            tokens[i] = lexer->token;
        }
        else {
            return -1; 
        }
    }
    tokens_len = i;
    
    // actually parse a expression using the 
    // token list that I have created before
    Expr *result = expression(); // this will parse a single expression
    
    print(result); // pretty pring for debugging purposes
    
    return 1;
}

#endif //PARSER_H
