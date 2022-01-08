
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
    Assert(tokens_index < tokens_len); 
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
        report(token.loc.line, strcat(msg, " at "));
    }
}

internal void report(int line, char *msg) {
    char err[100];  // holding the error message
    char buff[100]; // holding the integer as a string
    
    strcat(err, msg); 
    strcat(err, itoa(line, buff, 10)); 
    fprintf(stderr, err);
    fprintf(stderr, "\n");
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


internal bool internal_match(int n, ...) {
    va_list kinds; 
    va_start(kinds, n);
    
    Token_Kind kind; 
    for (int i = 0; i < n; i++) {
        kind = va_arg(kinds, Token_Kind); 
        if (check(kind)) {
            advance(); 
            va_end(kinds);
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
    
    while (match(TK_BANG_EQUAL, TK_EQUAL_EQUAL)) {
        Token operator = previous(); 
        Expr *right = comparison(); 
        expr = init_binary(expr, operator, right);
    }
    
    return expr;
}

internal Expr *comparison() {
    Expr *expr = term(); 
    
    while (match(TK_GREATER, TK_LESS, TK_GREATER_EQUAL, TK_LESS_EQUAL)) {
        Token operation = previous(); 
        Expr *right = term(); 
        expr = init_binary(expr, operation, right);
        
    }
    return expr;
}

internal Expr *term() {
    Expr *expr = factor(); 
    
    while (match(TK_MINUS, TK_PLUS)) {
        Token operation = previous(); 
        Expr *right = factor(); 
        expr = init_binary(expr, operation, right);
    }
    
    return expr;
}

internal Expr *factor() {
    Expr *expr = unary(); 
    
    while (match(TK_SLASH, TK_STAR )) {
        Token operation = previous(); 
        Expr *right = factor(); 
        expr = init_binary(expr, operation, right);
    }
    
    return expr;
}

internal Expr *unary() {
    
    while (match(TK_BANG, TK_MINUS)) {
        Token operation = previous(); 
        Expr *right = unary(); 
        return init_unary(operation, right);
    }
    
    return primary();
}

internal Expr *primary() {
    if (match(TK_FALSE)) return init_literal(NULL, VALUE_FALSE);
    if (match(TK_TRUE)) return init_literal(NULL, VALUE_TRUE);
    if (match(TK_NIL)) return init_literal(NULL, VALUE_NIL);
    
    // number & string
    if (match(TK_NUMBER, TK_STRING)) { 
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
    
    if (match(TK_LPARAN, TK_RPARAN)) {
        Expr *expr = expression(); 
        consume(TK_RPARAN, "Expected ')' after expression"); 
        return init_grouping(expr); 
    }
    
    Assert(false); // we should never be getting here
    return NULL; 
}

//////////////////////////////////
// Debug printing of expressions
// NOTE(ziv): this is deprecated

void print_literal(Expr *expr) {
    Assert(expr->kind == EXPR_LITERAL);
    
    switch (expr->literal.kind) {
        
        case VALUE_FALSE: printf("false"); break;
        case VALUE_TRUE:  printf("true"); break;
        case VALUE_NIL:   printf("nil"); break;
        
        case VALUE_INTEGER: {
            u64 num = (u64)expr->literal.data; 
            printf("%I64d", num);
        } break;
        
        case VALUE_STRING: {
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
        case TK_GREATER_EQUAL: printf(">=");break;
        case TK_LESS_EQUAL:    printf("<=");break;
        case TK_BANG_EQUAL:    printf("!=");break;
        case TK_EQUAL_EQUAL:   printf("==");break;
        
    }
}

void print(Expr *expr) {
    Assert(expr->kind & (EXPR_GROUPING | EXPR_LITERAL | EXPR_UNARY | EXPR_BINARY));
    
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

//////////////////////////////////

internal Statement *init_expr_stmt(Expr *expr) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement)); 
    stmt->kind = STMT_EXPR; 
    stmt->print_expr = expr; 
    return stmt;
}

internal Statement *init_print_stmt(Expr *expr) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement)); 
    stmt->kind = STMT_PRINT_EXPR; 
    stmt->print_expr = expr; 
    return stmt;
}

internal Statement *print_stmt() {
    Expr *expr = expression(); 
    consume(TK_SEMI_COLON, "Expected ';' after a print statement");
    return init_print_stmt(expr);
}

internal Statement *expr_stmt() {
    Expr *expr = expression(); 
    consume(TK_SEMI_COLON, "Expected ';' after a statement");
    return init_expr_stmt(expr);
}

internal Statement *init_decl_stmt(Token name, Expr *expr) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement)); 
    stmt->kind = STMT_VAR_DECL; 
    stmt->var_decl.name = name; 
    stmt->var_decl.initializer = expr; 
    return stmt;
}

internal Statement *decloration() {
    Token name = consume(TK_IDENTIFIER, "Expedted variable name"); 
    
    Expr *expr = NULL; 
    if (match(TK_ASSIGN)) { 
        expr = expression(); 
        return init_decl_stmt(name, expr); 
    }
    consume(TK_SEMI_COLON, "Expected ';' after a variable decloration");
    return NULL;
}

internal Statement *statement() {
    if (match(TK_PRINT)) return print_stmt(); 
    if (match(TK_VAR)) return decloration(); 
    
    
    return expr_stmt();
}

//////////////////////////////////

internal Expr *parse_file() {
    
    // 
    // Parse expression
    //
    
    // NOTE(ziv): Currently I only support expressions
    Expr *result = expression(); 
    return result;
    
    
    //
    // go over the token list and parse statements
    //
    
    /*     
        while (tokens[i++].kind != TK_EOF) {
            statements[statements_index++] = statement(); 
        }
         */
    
}
