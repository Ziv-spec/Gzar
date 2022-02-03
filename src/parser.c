
internal void parse_file() {
    
    Token tk = peek(); 
    Statement *stmt = decloration();
    
    if (stmt->kind != STMT_FUNCTION_DECL)
    {
        error(tk, "Expected a function definition");
    }
    
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
        
        Token var_name = previous();
        if (!local_exist(var_name)) {
            char buff[100]; 
            char *name = slice_to_str(var_name.str, (unsigned int)var_name.len);
            sprintf(buff, "Error: can not use '%s' it has never been declared", name);
            
            error(var_name, buff); 
        }
        
        return init_lvar(var_name);
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

internal Statement *get_curr_scope() {
    if (scope_index > 0) {
        return scopes[scope_index-1]; 
    }
    return NULL;
}

internal Statement *next_scop() {
    return scopes[++scope_index];
}

internal void push_scope(Statement *block) {
    scopes[scope_index++] = block;
}

internal Statement *pop_scope() {
    if (scope_index <= 0) {
        return scopes[scope_index];
    }
    return scopes[--scope_index];
}

internal void add_local(Statement *block, Local local) {
    // TODO(ziv): give the locals some initial capacity? so that I could get rid of this branch
    if (block->scope.capacity <= block->scope.local_index || block->scope.capacity == 0) {
        block->scope.capacity = block->scope.capacity ? block->scope.capacity : 1;
        block->scope.capacity *= 2;
        block->scope.locals = (Local *)realloc(block->scope.locals, sizeof(Local)*block->scope.capacity);
    }
    
    block->scope.locals[block->scope.local_index++] = local;
    
}

internal void scope_add_variable(Statement *block, Token name, Type *type) {
    Local local = (Local){ name, type }; 
    add_local(block, local); 
}

internal Type *local_exist(Token var_name) {
    Assert(var_name.kind == TK_IDENTIFIER);
    // TODO(ziv): Implement this using a hash map.
    
    Statement *block = get_curr_scope();
    if (!block) {
        // TODO(ziv): NOTE IMPORTANT probably should return global scope here IKD I need to think about this more but (and implement a global scope but ikd which I guess would be in the program structure which I will have it be global such that changes to it will be easy to impelmenet
        return NULL;
    }
    
    Scope s = block->scope; 
    
    for (unsigned int i = 0; i < s.local_index; i++) {
        Local l = s.locals[i]; 
        Token known_var_name = l.name; // all variable names which have been declared
        if (known_var_name.len == var_name.len && 
            strncmp(var_name.str, known_var_name.str, known_var_name.len) == 0) {
            return l.type; 
        }
    }
    return NULL;
}

internal void add_arg(Args **args, Local local) {
    Args *temp_args = *args; 
    
    if (temp_args == NULL) {
        temp_args = (Args *)malloc(sizeof(Args));
        temp_args->capacity =  2;
        temp_args->local_index = 0;
        temp_args->locals = (Local *)realloc(NULL, sizeof(Local)*temp_args->capacity);
    }
    else if (temp_args->capacity <= temp_args->local_index) {
        temp_args->capacity = temp_args->capacity ? temp_args->capacity : 1;
        temp_args->capacity *= 2;
        temp_args->locals = (Local *)realloc(temp_args->locals, sizeof(Local)*temp_args->capacity);
    }
    
    temp_args->locals[temp_args->local_index++] = local;
    *args = temp_args;
}

internal Statement *scope() {
    Statement *stmt = init_scope();
    push_scope(stmt);
    
    while (!is_at_end()) {
        if (match(TK_LBRACE)) {
            return pop_scope();
        }
        
        Statement *decl_stmt = decloration();
        vec_push(stmt->scope.statements, decl_stmt); 
    }
    
    consume(TK_LBRACE, "Expected ending of scope '}' ");
    return stmt;
}

internal Statement *decloration() {
    
    if (match(TK_IDENTIFIER)) 
    {
        // variable decloration / expression / function decloration 
        Token name = previous(); 
        if (match(TK_COLON))        return variable_decloration(name); 
        if (match(TK_DOUBLE_COLON)) return function_decloration(name); 
        
        back_one();
    }
    
    return statement();
}

internal Statement *variable_decloration(Token name) {
    Type *type = vtype(); 
    
    Expr *expr = NULL;
    if (match(TK_ASSIGN)) {
        expr = expression(); 
    }
    
    consume(TK_SEMI_COLON, "Expected ';' after a variable decloration");
    
    // Add variable name to local scope
    Statement *block = get_curr_scope();
    if (!block) {
        // TODO(ziv): change this such that you would be able to declare things 
        // at a global scope and not only local. 
        error(name, "Cannot declare variable at global scope");
    }
    scope_add_variable(block, name, type);
    
    return init_decl_stmt(name, type, expr);
}


internal Statement *function_decloration(Token name) {
    consume(TK_PROC, "Expected 'proc' keyword for function definition");
    Args *in_args = arguments();
    
    consume(TK_RETURN_TYPE, "Expected '->' after arguments"); 
    Type *return_type = vtype();
    
    consume(TK_RBRACE, "Expected beginning of block '{'");
    Statement *sc = scope(); 
    
    return init_func_decl_stmt(name, in_args, return_type, sc);
}

internal Args *arguments() {
    Local local = {0};
    
    Args *args = NULL;
    
    // @nocheckin
    consume(TK_LPARAN, "Expected '(' for arguments");
    
    while (!check(TK_RPARAN)) {
        
        if (match(TK_IDENTIFIER)) {
            Token name = previous();
            consume(TK_COLON, "Expected ':' after argument name");
            Type *type = vtype(); 
            
            local.name = name; 
            local.type = type; 
            add_arg(&args, local); 
            
            if (!match(TK_COMMA)) {
                break;
            }
            
        }
        
    } 
    
    consume(TK_RPARAN, "Expected ')' for end of arguments");
    
    
    return args;
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

internal Statement *statement() {
    if (match(TK_PRINT)) return print_stmt();  // this needs more consideration
    if (match(TK_RETURN)) return return_stmt();
    if (match(TK_RBRACE)) return scope(); 
    return expr_stmt();
}

internal Statement *return_stmt() {
    Expr *expr = expression();
    consume(TK_SEMI_COLON, "Expected ';' after expression");
    return init_return_stmt(expr);
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


internal Statement *init_func_decl_stmt(Token name, Args *args, Type *return_type, Statement *sc) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement)); 
    stmt->kind = STMT_FUNCTION_DECL; 
    stmt->func.name = name;
    stmt->func.args = args;
    stmt->func.return_type = return_type;
    stmt->func.sc = sc;
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

internal Statement *init_scope() {
    Statement *stmt = (Statement *)malloc(sizeof(Statement)); 
    stmt->kind = STMT_SCOPE;
    stmt->scope.statements = init_vec(); 
    stmt->scope.local_index = 0; 
    stmt->scope.capacity = 0; 
    stmt->scope.locals = NULL; 
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
        report(token.loc.line, token.loc.ch - token.len, buff); 
    }
    else {
        strcat(buff, msg); 
        report(token.loc.line, token.loc.ch - token.len, strcat(buff, " at "));
    }
}

internal void report(int line, int ch, char *msg) {
    char err[100] = {0};  // holding the error message
    char buff[100] = {0}; // holding the integer as a string
    
    fprintf(stderr, "\n");
    strcat(err, msg); 
    strcat(err, _itoa(line, buff, 10)); 
    fprintf(stderr, err);
    fprintf(stderr, "\n");
    
    // rewind 'code' to line 
    for (int l = 0; *code && line-2 > l; code++) {
        if (*code == '\n') {
            l++;
        }
    }
    
    int count = 0; 
    if (line > 1) {
        char *temp = code; 
        for (; *temp && *temp != '\n'; temp++, count++);
        temp++;
        for (; *temp && *temp != '\n'; temp++, count++);
    }
    else {
        // special case for when the error is at line 1
        // NOTE(ziv): @notchecked
        char *temp = code; 
        for (; *temp && *temp != '\n'; temp++, count++);
    }
    
    strncpy(buff, code, count+1);
    buff[count+1] = '\0';
    fprintf(stderr, buff);
    fprintf(stderr, "\n");
    fprintf(stderr, "%*c", ch,'^');
    
    fprintf(stderr, "\n");
    
    
    
    Assert(false);
    exit(-1); // NOTE(ziv): for the time being, when 
    // the parser is reporting a error for the use 
    // it is not going to continue finding more erorrs. 
    // This is done to simplify the amounts of things 
    // that I need to think about. 
    // I might change this when I have the will.. :)
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
    result_expr->assign.lvar = left_var;  
    result_expr->assign.rvalue = rvalue; 
    return result_expr;
}

internal Expr *init_lvar(Token name) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr)); 
    result_expr->kind = EXPR_LVAR;
    result_expr->lvar.name = name;
    return result_expr;
}

//////////////////////////////////



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
