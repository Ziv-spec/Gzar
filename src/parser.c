
internal void parse_file() {
    
    Program prog = {0}; 
    prog.decls = init_vec();
    prog.block.locals = init_vec(); 
    prog.block.statements = init_vec(); 
    global_block = prog.block;
    
    Token tk = peek(); 
    while (tk.kind != TK_EOF) {
        Statement *stmt = decloration();
        
        if (STMT_DECL_BEGIN < stmt->kind && stmt->kind < STMT_DECL_END) {
            vec_push(prog.decls, stmt);
        }
        else {
            // TODO(ziv): maybe print different errors for different statement kinds
            error(tk, "Expected a decloration in global scope");
        }
        
        tk = peek();
    }
    
}

//////////////////////////////////
/// Parsing of expressions 

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
        
        // if (type_equal(expr, right))
        
        
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
    if (match(TK_TRUE))  return init_literal(NULL, VALUE_TRUE);
    if (match(TK_NIL))   return init_literal(NULL, VALUE_NIL);
    
    // number & string
    if (match(TK_NUMBER, TK_STRING)) { 
        Token token = previous();
        Expr *result = NULL; 
        
        if (token.kind == TK_NUMBER) {
            u64 value = atoi(token.str); 
            result = init_literal((void *)value, VALUE_INTEGER); 
        }
        else {
            char *buff = (char *)malloc(token.len+1); 
            strncpy(buff, token.str, token.len);
            buff[token.len] = '\0';
            result = init_literal((void *)buff, VALUE_STRING); 
            
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
    
    char buff[100]; 
    Token not_literal = previous(); 
    if (match(TK_SEMI_COLON)) {
        sprintf(buff, "operation '%s' requires more than one operand", tk_names[not_literal.kind]);
        syntax_error(not_literal, buff); 
    }
    
    not_literal = peek(); 
    if (not_literal.kind == TK_EOF) {
        return NULL;
        //error(not_literal, "Unexpected end of file");
    }
    
    sprintf(buff, "illigal use of '%s'", tk_names[not_literal.kind]);
    syntax_error(not_literal, buff); 
    
#if DEBUG
    return NULL; 
#endif 
}

//////////////////////////////////
/// Initializations for expressions 

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

//////////// //////////////////////
/// helper functions for parsing statements

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

internal void add_decl(Scope block, Symbol *decl) {
    Assert(decl); 
    vec_push(block.locals, decl);
}

internal Symbol *init_symbol(Token name, Type *type, Expr *initializer) {
    Symbol *symbol = (Symbol *)malloc(sizeof(Symbol)); 
    symbol->name = name; 
    symbol->type = type; 
    symbol->initializer = initializer; 
    return symbol;
}

internal void scope_add_variable(Scope block, Token name, Type *type) {
    // TODO(ziv): change this to a map`
    Symbol *symbol = init_symbol(name, type, NULL); 
    add_decl(block, symbol); 
}

internal Type *symbol_exist(Scope block, Token name) {
    
    Vector *symbols = block.locals;
    for (int i = 0; i < symbols->index; i++) {
        Symbol *s = (Symbol *)symbols->data[i]; 
        
        Token known_var_name = s->name; // all variable names which have been declared
        if (known_var_name.len == name.len && 
            strncmp(name.str, known_var_name.str, known_var_name.len) == 0) {
            return s->type; 
        }
    }
    
    return NULL;
}

internal Type *local_exist(Token var_name) {
    Assert(var_name.kind == TK_IDENTIFIER);
    // TODO(ziv): Implement this using a hash map.
    
    Statement *block = get_curr_scope();
    if (block) {
        for (int scope_ind = scope_index-1; scope_ind >= 0; scope_ind--) {
            block = scopes[scope_ind]; 
            Assert(block->kind == STMT_SCOPE);
            
            Type *ty = symbol_exist(block->block, var_name); 
            if (ty) return ty; 
        }
    }
    
    return symbol_exist(global_block, var_name); 
}

//////////////////////////////////
/// Parsing of statements

internal Statement *scope(Statement *block) {
    push_scope(block);
    
    while (!is_at_end()) {
        if (match(TK_LBRACE)) {
            pop_scope(block);
            return block; // end
        }
        
        Statement *decl_stmt = decloration();
        vec_push(block->block.statements, decl_stmt); 
    }
    
    
    // file has unexpectedly ended 
    error(peek(), "Unexpected end of file");
#if DEBUG
    return NULL;
#endif 
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
    if (block) {
        scope_add_variable(block->block, name, type);
    }
    else {
        scope_add_variable(global_block, name, type);
    }
    
    return init_decl_stmt(name, type, expr);
}

internal Statement *function_decloration(Token name) {
    consume(TK_PROC, "Expected 'proc' keyword for function definition");
    
    Vector *args = arguments();
    
    consume(TK_RETURN_TYPE, "Expected '->' after arguments"); 
    Type *return_type = vtype();
    
    // TODO(ziv): enable the option for function prototypes
    consume(TK_RBRACE, "Expected beginning of block '{'");
    
    Statement *block =  init_scope();
    
    // add to the scope the function arguments 
    // so you would be able to use them inside 
    // the scope 
    for (int i = 0; i < args->index; i++) {
        add_decl(block->block, (Symbol *)args->data[i]);
    }
    
    // to allow the use of the function name inside the function I first create the function type and insert it into the global scope. Then I parse the block
    Type *ty = init_type(TYPE_FUNCTION, NULL, args, return_type); 
    add_decl(global_block, init_symbol(name, ty, NULL));  // add func decl to global scope
    
    block = scope(block);
    
    
    return init_func_decl_stmt(name, args, return_type, block);
}

internal Vector *arguments() {
    
    Vector *args = init_vec(); 
    
    consume(TK_LPARAN, "Expected '(' for arguments");
    
    while (!check(TK_RPARAN) && match(TK_IDENTIFIER)) {
        
        Token name = previous(); 
        consume(TK_COLON, "Expected ':' after variable name");
        Type *type = vtype(); 
        
        Expr *expr = NULL;
        if (match(TK_ASSIGN)) {
            expr = expression(); 
        }
        
        Symbol *symb = init_symbol(name, type, expr);
        vec_push(args, symb);
        
        if (!match(TK_COMMA)) {
            break;
        }
    } 
    
    consume(TK_RPARAN, "Expected ')' for end of arguments");
    
    return args;
}

internal Type *vtype() {
    
    // TODO(ziv): support struct/enum types
    
    static Atom_Kind tk_to_atom_type[] =  {
        [TK_S8_TYPE]  = TYPE_S8,
        [TK_S16_TYPE] = TYPE_S16,
        [TK_S32_TYPE] = TYPE_S32,
        
        [TK_S8_TYPE]  = TYPE_U8,
        [TK_U16_TYPE] = TYPE_U16,
        [TK_U32_TYPE] = TYPE_U32,
        
        [TK_VOID_TYPE]   = TYPE_VOID, 
        [TK_INT_TYPE]    = TYPE_INTEGER, 
        [TK_STRING_TYPE] = TYPE_STRING, 
        
        // Should this be a "atomic type"? or should it just be a special add-on pointer something type?
        [TK_STAR] = TYPE_POINTER, 
    };
    
    Token_Kind tk_kind = advance().kind;
    if (TK_TYPE_BEGIN <= tk_kind && tk_kind <= TK_TYPE_END) {
        // handle atomic type 
        Atom_Kind atom_kind = tk_to_atom_type[tk_kind];
        return init_type(atom_kind, NULL, NULL, NULL); 
    }
    else if (tk_kind == TK_STAR) {
        return init_type(TYPE_POINTER, vtype(), NULL, NULL);
    }
    
    // how about functions? 
    
    // TODO(ziv): add support for arrays, not only pointers
    
    error(peek(), "Expected type after decloration"); // TODO(ziv): maybe this should be more clear.
#ifdef DEBUG 
    return NULL;
#endif 
}

internal Statement *statement() {
    if (match(TK_PRINT)) return print_stmt();  // this needs more consideration
    if (match(TK_RETURN)) return return_stmt();
    if (match(TK_RBRACE)) return scope(init_scope()); 
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


//////////////////////////////////
/// Initializations for statements

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


internal Statement *init_func_decl_stmt(Token name, Vector *args, Type *return_type, Statement *sc) {
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
    stmt->block.statements = init_vec(); 
    stmt->block.locals = init_vec(); 
    return stmt;
}

internal Statement *init_return_stmt(Expr *expr) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement));
    stmt->kind = STMT_RETURN; 
    stmt->ret = expr; 
    return stmt;
}

//////////////////////////////////
/// General helper functions 

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
    syntax_error(peek(), msg);
    
#ifdef DEBUG
    return (Token){0};
#endif 
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

internal void syntax_error(Token token, const char *err) {
    char buff[100]; 
    sprintf(buff, "Syntax error: %s", err); 
    error(token, buff); 
}

internal void report(int line, int ch, char *msg) {
    char err[100] = {0};  // holding the error message
    char buff[100] = {0}; // holding the integer as a string
    
    fprintf(stderr, "\n");
    strcat(err, msg); 
    strcat(err, _itoa(line, buff, 10)); 
    //strcat(err, ":");
    //strcat(err, _itoa(ch, buff, 10)); 
    fprintf(stderr, err);
    fprintf(stderr, "\n");
    
    // rewind 'code' to line 
    for (int l = 0; *code && line-1 > l; code++) {
        if (*code == '\n') {
            l++;
        }
    }
    
    int count = 0; 
    
    char *temp = code; 
    for (; *temp && *temp != '\n'; temp++, count++);
    
    strncpy(buff, code, count+1);
    buff[count+1] = '\0';
    fprintf(stderr, buff);
    fprintf(stderr, "%*c", ch,'^');
    fprintf(stderr, "\n");
    
    // NOTE(ziv): for the time being, when 
    // the parser is reporting a error for the use 
    // it is not going to continue finding more erorrs. 
    // This is done to simplify the amounts of things 
    // that I need to think about. 
    // I might change this when I have the will.. :)
    
    Assert(false);
    exit(-1); 
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
