
internal void parse_file() {
    
    // 
    // Parse expression
    //
    
    // NOTE(ziv): Currently I only support expressions
    /*     
    Expr *result = expression(); 
    return result;
         */
    
    
    //
    // go over the token list and parse statements
    //
    
    /*     
         */
    while (!is_at_end()) {
        statements[statements_index++] = statement(); 
    }
    statements[statements_index] = NULL;
    
}

//////////////////////////////////


internal Expr *expression() {
    return assignement(); 
}
internal Expr *assignement() {
    Expr *expr = equality(); 
    if (match(TK_ASSIGN)) {
        return init_assignement(expr, equality());
    }
    return expr;
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
    
    if (match(TK_IDENTIFIER)) {
        
        Assert(false); // I have not yet figured this out. After I do, I pormise to delete this. For real this time. @nocheckin
        Expr *node = local_exist(previous());
        if (node) {
            // This is a operation on a varaible name. Not decloration.
            return init_lvar(previous(), node->left_variable.offset); 
        }
        
        // decloration of a variable
        
        node = init_lvar(previous(), next_offset());
        
        add_locals(node);
        return node; 
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

internal Statement *return_stmt() {
    Expr *expr = expression();
    consume(TK_SEMI_COLON, "Expected ';' after expression");
    return init_return_stmt(expr);
}

internal Statement *statement() {
    if (match(TK_PRINT)) return print_stmt();  // this needs more consideration
    if (match(TK_IDENTIFIER)) return decloration(); 
    if (match(TK_RETURN)) return return_stmt();
    
    return expr_stmt();
}

internal Type *vtype() {
    // TODO(ziv): have some array for all known types (this is mainly for struct/enum support)
    // and check inside that array if this type matches one of those types 
    if (match(TK_S8, TK_U8, TK_S32, TK_U32)) {
        return init_type(previous()); 
    }
    
    error(peek(), "Expected type in a variable decloration");
    return NULL;
}

internal Statement *decloration() {
    // declorations syntax
    // 
    // identifier : type = initializer_expression; 
    //      or 
    // identifier : type; 
    //
    
    Token name = previous(); 
    
    Statement *stmt = NULL; 
    
    if (match(TK_COLON)) {
        // variable decloration 
        Type *type = vtype(); 
        
        Expr *expr = NULL;
        if (match(TK_ASSIGN)) {
            expr = expression(); 
        }
        consume(TK_SEMI_COLON, "Expected ';' after a variable decloration");
        stmt = init_decl_stmt(name, type, expr);
        add_locals(init_lvar(name, next_offset())); // add token name to locals 
    }
    else { 
        // assignment to a variable
        back_one();
        stmt = expr_stmt();
    }
    
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


internal Statement *init_decl_stmt(Token name, Type *type, Expr *initializer) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement)); 
    stmt->kind = STMT_VAR_DECL; 
    stmt->var_decl.type = type; 
    stmt->var_decl.name = name; 
    stmt->var_decl.initializer = initializer; 
    return stmt;
}

internal Statement *init_return_stmt(Expr *expr) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement));
    stmt->kind = STMT_RETURN; 
    stmt->ret = expr; 
    return stmt;
}

internal Type *init_type(Token type_token) {
    Type *type = (Type *)malloc(sizeof(Type)); 
    type->kind = type_token.kind; 
    return type;
}

//////////////////////////////////

internal void back_one() {
    tokens_index--;
}

internal int is_at_end() {
    Assert(tokens_index < tokens_len); 
    return tokens[tokens_index].kind == TK_EOF;
}

internal Token peek() {
    return tokens[tokens_index];
}

internal Token advance() {
    if (!is_at_end()) tokens_index++; 
    return tokens[tokens_index-1];
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
    char buff[255] = {0};
    
    if (token.kind == TK_EOF) {
        strcat(buff, msg);
        strcat(buff, " at end ");
        report(token.loc.line, buff); 
    }
    else {
        report(token.loc.line, strcat(msg, " at "));
    }
}

internal void report(int line, char *msg) {
    char err[100] = {0};  // holding the error message
    char buff[100] = {0}; // holding the integer as a string
    
    strcat(err, msg); 
    strcat(err, itoa(line, buff, 10)); 
    fprintf(stderr, err);
    fprintf(stderr, "\n");
    
    Assert(false);
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
    va_end(kinds);
    return false; 
}

internal Expr *local_exist(Token token) {
    Assert(token.kind == TK_IDENTIFIER);
    
    for (int i = 0; i < locals_index; i++) {
        Expr *lvar = locals[i]; 
        Token name = lvar->left_variable.name;
        if (name.len == token.len && strncmp(token.str, name.str, name.len) == 0) {
            return lvar;
        }
    }
    return NULL;
}

internal void add_locals(Expr *lvar) {
    locals[locals_index++] = lvar;
}

//////////////////////////////////

internal Expr *init_binary(Expr *left, Token operation, Expr *right) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr));
    result_expr->kind = EXPR_BINARY; 
    result_expr->left = left; 
    result_expr->operation = operation; 
    result_expr->right = right;
    return result_expr;
}

internal Expr *init_unary(Token operation, Expr *right) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr));
    result_expr->kind = EXPR_UNARY; 
    result_expr->unary.operation = operation; 
    result_expr->unary.right = right; 
    return result_expr;
}

internal Expr *init_literal(void *data, Value_Kind kind) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr));
    result_expr->kind = EXPR_LITERAL; 
    result_expr->literal.kind = kind; 
    result_expr->literal.data = data; 
    return result_expr;
}

internal Expr *init_grouping(Expr *expr) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr));
    result_expr->kind = EXPR_GROUPING;  
    result_expr->grouping.expr = expr; 
    return result_expr;
}

internal Expr *init_assignement(Expr *left_var, Expr *rvalue) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr));
    result_expr->kind = EXPR_ASSIGN;  
    result_expr->assign.left_variable = left_var;  
    result_expr->assign.rvalue = rvalue; 
    return result_expr;
}

//////////////////////////////////

internal int next_offset() {
    global_next_offset += 4;
    return global_next_offset; 
}

internal Expr *init_lvar(Token name, int offset) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr)); 
    result_expr->kind = EXPR_LVAR;
    result_expr->left_variable.name = name;
    result_expr->left_variable.offset = offset; 
    return result_expr;
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
