
// I use the recursive decent algorithem for parsing my language. 
// For each translation unit I have, I parse it's content and 
// generate a AST - Abstract Syntax Tree which for the time being 
// I use it as a IR - Immediate representation for generating code from
// This is not perfect but will suffice for the time being.

internal void parse_translation_unit(Translation_Unit *tu, Token_Stream *restrict s) {
    
    // TODO(ziv): put this in a function? 
    
    // init translation unit
    tu->s = s; 
    tu->decls = init_vec(); 
    tu->unnamed_strings = init_vec();
    tu->scopes = init_vec();
    tu->offset = 0; 
    
    vec_push(tu->scopes, init_scope());
    
    // 
    // Parsing of global declorations
    //
    
    while (peek(tu).kind != TK_EOF) {
        Statement *stmt = parse_decloration(tu);
        
        if (STMT_DECL_BEGIN < stmt->kind && stmt->kind < STMT_DECL_END)
            vec_push(tu->decls, stmt);
        else
            parse_error(tu, peek(tu), "Expected a decloration in global scope");
    }
    
}

//////////////////////////////////
/// Parsing of expressions 

internal Expr *parse_expression(Translation_Unit* tu) {
    return parse_assignment(tu); 
}

internal Expr *parse_assignment(Translation_Unit* tu) {
    Expr *expr = parse_logical(tu);
    if (match(tu, TK_ASSIGN)) {
        return init_assignment(expr, parse_logical(tu));
    }
    return expr;
}

internal Expr *parse_logical(Translation_Unit* tu) {
    Expr *expr = parse_equality(tu); 
    
    while (match(tu, TK_DOUBLE_AND, TK_DOUBLE_OR)) {
        Token operator = previous(tu); 
        Expr *right = parse_equality(tu); 
        expr = init_binary(expr, operator, right);
    }
    
    return expr;
}

internal Expr *parse_equality(Translation_Unit* tu) {
    Expr *expr = parse_bitwise(tu); 
    
    while (match(tu, TK_BANG_EQUAL, TK_EQUAL_EQUAL)) {
        Token operator = previous(tu); 
        Expr *right = parse_bitwise(tu); 
        expr = init_binary(expr, operator, right);
    }
    
    return expr;
}

internal Expr *parse_bitwise(Translation_Unit* tu) {
    Expr *expr = parse_comparison(tu); 
    
    while (match(tu, TK_OR, TK_XOR, TK_AND)) {
        Token operator = previous(tu); 
        Expr *right = parse_comparison(tu); 
        expr = init_binary(expr, operator, right);
    }
    
    return expr;
}

internal Expr *parse_comparison(Translation_Unit* tu) {
    Expr *expr = parse_term(tu); 
    
    while (match(tu, TK_GREATER, TK_LESS, TK_GREATER_EQUAL, TK_LESS_EQUAL)) {
        Token operation = previous(tu); 
        Expr *right = parse_term(tu); 
        expr = init_binary(expr, operation, right);
        
    }
    return expr;
}

internal Expr *parse_term(Translation_Unit* tu) {
    Expr *expr = parse_factor(tu); 
    
    while (match(tu, TK_MINUS, TK_PLUS)) {
        Token operation = previous(tu); 
        Expr *right = parse_factor(tu); 
        expr = init_binary(expr, operation, right);
    }
    
    return expr;
}

internal Expr *parse_factor(Translation_Unit* tu) {
    Expr *expr = parse_unary(tu); 
    
    while (match(tu, TK_SLASH, TK_STAR)) {
        Token operation = previous(tu); 
        Expr *right = parse_factor(tu); 
        
        expr = init_binary(expr, operation, right);
    }
    
    return expr;
}

internal Expr *parse_unary(Translation_Unit* tu) {
    
    if (match(tu, TK_CAST)) {
        Token operation = previous(tu);
        consume(tu, TK_LPARAN, "Expected '(' at beginning of cast"); 
        Type *ty = parse_type(tu); 
        consume(tu, TK_RPARAN, "Expected ')' at end of cast"); 
        Expr *right = parse_unary(tu); 
        
        return init_unary(operation, ty, right); 
    }
    
    if (match(tu, TK_BANG, TK_MINUS)) {
        Token operation = previous(tu);
        Expr *right = parse_unary(tu);
        
        return init_unary(operation, NULL, right);
    }
    
    return parse_call(tu);
}

internal Expr *parse_call(Translation_Unit* tu) {
    Expr *expr = parse_primary(tu);
    
    if (expr && expr->kind == EXPR_LVAR && match(tu, TK_LPARAN)) {
        Expr *args = parse_arguments(tu); 
        if (args) {
            consume(tu, TK_RPARAN, "Expected ')' in a function call"); 
        }
        return init_call(expr, args); 
    }
    
    return expr;
}

internal Expr *parse_arguments(Translation_Unit* tu) {
    
    // arguments
    Vector *args = init_vec(); 
    Expr *expr = NULL; 
    while (!check(tu, TK_RPARAN)) {
        
        expr = parse_expression(tu);
        vec_push(args, expr);
        
        if (!match(tu, TK_COMMA)) {
            break;
        }
    } 
    
    return init_arguments(args);
}

internal Expr *parse_primary(Translation_Unit* tu) {
    if (match(tu, TK_FALSE)) return init_literal(0, TYPE_BOOL);
    if (match(tu, TK_TRUE))  return init_literal((void *)1, TYPE_BOOL);
    if (match(tu, TK_NIL))   return init_literal(NULL, TYPE_UNKNOWN);
    
    if (match(tu, TK_NUMBER, TK_STRING)) { 
        Token token = previous(tu);
        Expr *result = NULL; 
        
        if (token.kind == TK_NUMBER) {
            u64 value = atoi(str8_to_cstring(token.str)); 
            result = init_literal((void *)value, TYPE_S64); 
        }
        else {
            result = init_literal((void *)str8_to_cstring(token.str), TYPE_STRING); 
        }
        
        return result;
    }
    
    if (match(tu, TK_IDENTIFIER)) {
        Token var_name = previous(tu);
        return init_lvar(var_name);
    }
    
    if (match(tu, TK_LPARAN, TK_RPARAN)) {
        Expr *expr = parse_expression(tu); 
        consume(tu, TK_RPARAN, "Expected ')' after expression"); 
        return init_grouping(expr); 
    }
    
    char buff[100]; 
    Token not_literal = previous(tu); 
    if (match(tu, TK_SEMI_COLON)) {
        //Assert(!"check whether this works correctly");
        sprintf(buff, "Can not have empty expressions");
        syntax_error(tu, not_literal, buff); 
        return NULL;
    }
    
    not_literal = peek(tu); 
    if (not_literal.kind == TK_EOF) {
        parse_error(tu, not_literal, "Unexpected end of file");
        return NULL;
    }
    
    
    char *temp = NULL; 
    if (not_literal.kind < TK_DOUBLE_ASCII) {
        sprintf(buff, "operation '%c' requires two operands", not_literal.kind);
        syntax_error(tu, not_literal, buff); 
        return NULL;
    }
    temp = str8_to_cstring(not_literal.str); 
    
    sprintf(buff, "illigal use of '%s'", temp);
    syntax_error(tu, not_literal, buff); 
    
    return NULL; 
}

//////////////////////////////////
/// Initializations for expressions 

internal Expr *init_binary(Expr *left, Token operation, Expr *right) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr));
    result_expr->kind = EXPR_BINARY; 
    result_expr->type = NULL;
    result_expr->left = left; 
    result_expr->operation = operation; 
    result_expr->right = right;
    return result_expr;
}

internal Expr *init_unary(Token operation, Type *type, Expr *right) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr));
    result_expr->kind = EXPR_UNARY; 
    result_expr->type = NULL;
    result_expr->unary.operation = operation;
    result_expr->unary.right = right; 
    result_expr->unary.type = type; 
    return result_expr;
}

internal Expr *init_literal(void *data, Type_Kind kind) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr));
    result_expr->kind = EXPR_LITERAL; 
    result_expr->type = NULL;
    result_expr->literal.kind = kind; 
    result_expr->literal.data = data; 
    return result_expr;
}

internal Expr *init_grouping(Expr *expr) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr));
    result_expr->kind = EXPR_GROUPING;  
    result_expr->type = NULL;
    result_expr->grouping.expr = expr; 
    return result_expr;
}

internal Expr *init_assignment(Expr *left_var, Expr *rvalue) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr));
    result_expr->kind = EXPR_ASSIGN;  
    result_expr->type = NULL;
    result_expr->assign.lvar = left_var;  
    result_expr->assign.rvalue = rvalue; 
    return result_expr;
}

internal Expr *init_lvar(Token name) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr)); 
    result_expr->kind = EXPR_LVAR;
    result_expr->type = NULL;
    result_expr->lvar.name = name;
    return result_expr;
}

internal Expr *init_arguments(Vector *args) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr)); 
    result_expr->kind = EXPR_ARGUMENTS;
    result_expr->args = args;
    return result_expr;
}

internal Expr *init_call(Expr *name, Expr *args) {
    Expr *result_expr = (Expr *)malloc(sizeof(Expr)); 
    result_expr->kind = EXPR_CALL;
    result_expr->call.name = name;
    result_expr->call.args = args;
    result_expr->type = NULL;
    return result_expr;
}

//////////////////////////////////
/// Helper functions for symbols

internal Statement *get_curr_scope(Translation_Unit *tu) {
    if (1 < tu->scopes->index) {
        return tu->scopes->data[tu->scopes->index-1]; 
    }
    return tu->scopes->data[0];  // global scope
}

internal bool is_at_global_block(Translation_Unit *tu) {
    return tu->scopes->index == 1;
}

internal void push_scope(Translation_Unit *tu, Statement *block) {
    vec_push(tu->scopes, block);
}

internal Statement *pop_scope(Translation_Unit *tu) {
    return vec_pop(tu->scopes);
}

internal bool add_symbol(Block *block, Symbol *decl) {
    Assert(decl && block); 
    bool success = map_set(block->locals, decl->name.str, decl);
    Assert(success); 
    return success; 
}

internal Symbol *symbol_exist(Block *block, Token name) {
    return map_peek(block->locals, name.str); 
}

internal Symbol *local_exist(Translation_Unit *tu, Token var_name) {
    Assert(var_name.kind == TK_IDENTIFIER);
    
    if (!is_at_global_block(tu)) {
        
        // from bottom to top scopes, check whether the symbol exists in it
        for (size_t i = tu->scopes->index-1; 0 < i; i--) {
            
            Statement *stmt = (Statement *)tu->scopes->data[i];
            if (stmt->kind == STMT_SCOPE) {
                Symbol *symb = symbol_exist(&stmt->block, var_name); 
                if (symb)  return symb;
                
            }
            else if (stmt->kind == STMT_FUNCTION_DECL) {
                if (str8_compare(var_name.str, stmt->func.name.str) == 0) {
                    return init_symbol(stmt->func.name, stmt->func.type, NULL);
                }
                
                Vector *symbols = stmt->func.type->symbols; 
                for (int j = 0; j < symbols->index; j++) {
                    Symbol *symb = symbols->data[j]; 
                    if (str8_compare(symb->name.str, var_name.str) == 0)
                        return symb;
                    
                }
            }
            else {
                Assert(!"Should not be getting here");
            }
            
            //return NULL;
        }
        
    }
    
    // last search in the global scope
    return symbol_exist(&((Statement *)tu->scopes->data[0])->block, var_name);
}

//////////////////////////////////
/// Parsing of statements

internal Statement *parse_scope_stmt(Translation_Unit* tu) {
    Statement *block = init_scope();
    
    push_scope(tu, block);
    
    while (!is_at_end(tu)) {
        if (match(tu, TK_LBRACE)) {
            pop_scope(tu);
            return block; // end
        }
        
        Statement *stmt = parse_decloration(tu);
        vec_push(block->block.statements, stmt); 
    }
    
    parse_error(tu, peek(tu), "Unexpected end of file");
    return NULL;
}

internal void parse_syncronize(Translation_Unit *tu) {
    advance(tu);
    
    while (!is_at_end(tu)) {
        if (previous(tu).kind == TK_SEMI_COLON) return;
        
        switch (peek(tu).kind) {
            case TK_IF:
            case TK_ELSE:
            case TK_WHILE:
            case TK_RETURN:
            return;
        }
        
        advance(tu);
    }
}

internal Statement *parse_decloration(Translation_Unit* tu) {
    
    Statement *stmt = NULL;
    if (match(tu, TK_IDENTIFIER)) {
        Token name = previous(tu); 
        if (match(tu, TK_COLON))             stmt = parse_variable_decloration(tu, name); 
        else if (match(tu, TK_DOUBLE_COLON)) stmt = parse_function_decloration(tu, name);
        
        if (stmt) {
            return stmt; 
        }
        
        back_one(tu); 
    }
    
    if (stmt = parse_statement(tu)) {
        return stmt; 
    }
    
    parse_syncronize(tu); 
    
    return NULL;
}

internal Statement *parse_variable_decloration(Translation_Unit* tu, Token name) {
    
    Type *type = parse_type(tu); 
    
    Expr *expr = NULL;
    if (match(tu, TK_ASSIGN)) {
        expr = parse_expression(tu); 
    }
    
    consume(tu, TK_SEMI_COLON, "Expected ';' after a variable decloration");
    
    Symbol *symb = init_symbol(name, type, expr);
    
    // add symbol to scope
    {
        Statement *block = get_curr_scope(tu);
        add_symbol(&block->block, symb);
    }
    
    return init_var_decl_stmt(symb);
}

internal Statement *parse_function_decloration(Translation_Unit* tu, Token name) {
    
    // arguments
    Vector *args = init_vec(); 
    consume(tu, TK_LPARAN, "Expected '(' for arguments");
    while (!check(tu, TK_RPARAN) && match(tu, TK_IDENTIFIER)) {
        Token var_name = previous(tu); 
        consume(tu, TK_COLON, "Expected ':' after variable name");
        Type *type = parse_type(tu); 
        
        Expr *expr = NULL;
        if (match(tu, TK_ASSIGN)) {
            expr = parse_expression(tu); 
        }
        
        Symbol *symb = init_symbol(var_name, type, expr);
        vec_push(args, symb);
        
        if (!match(tu, TK_COMMA)) {
            break;
        }
    } 
    
    // function type 
    consume(tu, TK_RPARAN, "Expected ')' for end of arguments");
    consume(tu, TK_RETURN_TYPE, "Expected '->' after arguments"); 
    
    Type *return_type = parse_type(tu);
    Type *ty = init_type(TYPE_FUNCTION, return_type, args);
    
    // function body/fuction prototype
    Statement *block_stmt = NULL;
    if (match(tu, TK_RBRACE)) { // function body
        block_stmt = parse_scope_stmt(tu);
    }
    else { // function prototype
        consume(tu, TK_SEMI_COLON, "Expected '{' for function body or  ';' for prototype");
    }
    
    Statement *curr_block = get_curr_scope(tu);
    Assert(curr_block->kind == STMT_SCOPE); 
    
    add_symbol(&curr_block->block, init_symbol(name, ty, NULL));
    
    return init_func_decl_stmt(name, ty, block_stmt);
}

internal Type *parse_type(Translation_Unit *tu) {
    
    // TODO(ziv): support struct/enum types
    
    // NOTE(ziv): The function type creation does not happen here 
    // because doing it that way would restrict me in a couple of 
    // major ways. So instead I am doing it in the 'function_decloration'
    // This means that this function can only parse 'simple' types. 
    
    static Type_Kind tk_to_atom_type[] =  {
        [TK_S8_TYPE]  = TYPE_S8,
        [TK_S16_TYPE] = TYPE_S16,
        [TK_S32_TYPE] = TYPE_S32,
        [TK_S64_TYPE] = TYPE_S64,
        
        [TK_U8_TYPE]  = TYPE_U8,
        [TK_U16_TYPE] = TYPE_U16,
        [TK_U32_TYPE] = TYPE_U32,
        [TK_U64_TYPE] = TYPE_U32,
        
        [TK_VOID_TYPE]   = TYPE_VOID, 
        [TK_STRING_TYPE] = TYPE_STRING, 
        [TK_BOOL_TYPE]   = TYPE_BOOL, 
        [TK_INT_TYPE]    = TYPE_S64, 
        [TK_STAR]        = TYPE_POINTER, 
    };
    
    Token_Kind tk_kind = advance(tu).kind;
    if (TK_TYPE_BEGIN <= tk_kind && tk_kind <= TK_TYPE_END) {
        return get_atom(tk_to_atom_type[tk_kind]); 
    }
    else if (tk_kind == TK_STAR) {
        return init_type(TYPE_POINTER, parse_type(tu), NULL);
    }
    
    // TODO(ziv): add support for arrays, not only pointers
    
    parse_error(tu, peek(tu), "Expected type after decloration"); 
    
    
    return NULL;
}

internal Statement *parse_statement(Translation_Unit* tu) {
    if (match(tu, TK_RETURN)) return parse_return_stmt(tu);
    if (match(tu, TK_RBRACE)) return parse_scope_stmt(tu); 
    if (match(tu, TK_IF)) return parse_if_stmt(tu); 
    if (match(tu, TK_WHILE)) return parse_while_stmt(tu); 
    
    return parse_expr_stmt(tu);
}

internal Statement *parse_return_stmt(Translation_Unit* tu) {
    Expr *expr = parse_expression(tu);
    consume(tu, TK_SEMI_COLON, "Expected ';' after expression");
    return init_return_stmt(expr);
}

internal Statement *parse_expr_stmt(Translation_Unit* tu) {
    Expr *expr = parse_expression(tu); 
    consume(tu, TK_SEMI_COLON, "Expected ';' after a statement");
    return init_expr_stmt(expr);
}

internal Statement *init_if_stmt(Expr *condition, Statement *true_block,  Statement *false_block) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement));
    stmt->kind = STMT_IF; 
    stmt->if_else.condition = condition; 
    stmt->if_else.true_block = true_block; 
    stmt->if_else.false_block = false_block; 
    return stmt;
}

internal Statement *init_while_stmt(Expr *condition, Statement *block) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement));
    stmt->kind = STMT_WHILE; 
    stmt->loop.condition = condition; 
    stmt->loop.block = block; 
    return stmt;
}

internal Statement *parse_if_stmt(Translation_Unit *tu) {
    Expr *condition = parse_expression(tu); 
    
    consume(tu, TK_RBRACE, "Expected '{' for beginning of block"); 
    Statement *true_block = parse_scope_stmt(tu); 
    
    Statement *false_block = NULL;
    if (match(tu, TK_ELSE)) {
        consume(tu, TK_RBRACE, "Expected '{' for beginning of block"); 
        false_block = parse_scope_stmt(tu); 
    }
    
    return init_if_stmt(condition, true_block, false_block); 
}

internal Statement *parse_while_stmt(Translation_Unit *tu) {
    
    Expr *condition = parse_expression(tu); 
    
    consume(tu, TK_RBRACE, "Expected beginning of block '{' for while statement");
    Statement *block = parse_scope_stmt(tu); 
    
    return init_while_stmt(condition, block); 
}




//////////////////////////////////
/// Initializations for statements

internal Statement *init_expr_stmt(Expr *expr) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement)); 
    stmt->kind = STMT_EXPR; 
    stmt->expr = expr; 
    return stmt;
}

internal Statement *init_func_decl_stmt(Token name, Type *ty, Statement *sc) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement)); 
    stmt->kind = STMT_FUNCTION_DECL; 
    stmt->func.name = name;
    stmt->func.type = ty;
    stmt->func.sc   = sc;
    return stmt;
}

internal Statement *init_var_decl_stmt(Symbol *symb) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement)); 
    stmt->kind = STMT_VAR_DECL; 
    stmt->var_decl.type = symb->type; 
    stmt->var_decl.name = symb->name; 
    stmt->var_decl.initializer = symb->initializer; 
    return stmt;
}

internal Statement *init_scope() {
    Statement *stmt = (Statement *)malloc(sizeof(Statement)); 
    stmt->kind = STMT_SCOPE;
    stmt->block.statements = init_vec(); 
    stmt->block.locals = init_map(sizeof(Symbol)); 
    stmt->block.size = 0; 
    stmt->block.offset = 0; 
    return stmt;
}

internal Statement *init_return_stmt(Expr *expr) {
    Statement *stmt = (Statement *)malloc(sizeof(Statement));
    stmt->kind = STMT_RETURN; 
    stmt->ret = expr; 
    return stmt;
}

internal Symbol *init_symbol(Token name, Type *type, Expr *initializer) {
    Symbol *symbol = (Symbol *)malloc(sizeof(Symbol)); 
    symbol->name = name; 
    symbol->type = type; 
    symbol->initializer = initializer; 
    return symbol;
}

internal Type *init_type(Type_Kind kind, Type *subtype, Vector *symbols) {
    Type *ty = (Type *)malloc(sizeof(Type)); 
    ty->kind = kind;
    ty->subtype = subtype;
    ty->symbols = symbols;
    return ty;
}

//////////////////////////////////
/// General helper functions 

internal void back_one(Translation_Unit *tu) {
    tu->s->current--;
}

internal int is_at_end(Translation_Unit *tu) {
    Assert(tu->s->current < tu->s->capacity); 
    return tu->s->s[tu->s->current].kind == TK_EOF;
}

internal Token peek(Translation_Unit *tu) {
    return tu->s->s[tu->s->current];
}

internal Token advance(Translation_Unit *tu) {
    if (!is_at_end(tu)) tu->s->current++; 
    return tu->s->s[tu->s->current-1];
}

internal Token previous(Translation_Unit *tu) {
    return tu->s->s[tu->s->current-1];
}

internal Token consume(Translation_Unit *tu, Token_Kind kind, char *msg) {
    if (check(tu, kind)) return advance(tu); 
    syntax_error(tu, peek(tu), msg);
    
    return (Token){0};
}

internal void parse_error(Translation_Unit *tu, Token token, char *msg) {
    char buff[255] = {0};
    
    int line = get_line_number(tu->s->start, token.str); 
    int ch   = get_character_number(tu->s->start, token.str);
    
    strcat(buff, msg);
    report(tu, line, ch, (token.kind == TK_EOF) ? strcat(buff, " at end ") : strcat(buff, " at ")); 
}

internal void syntax_error(Translation_Unit *tu, Token token, const char *err) {
    char buff[100]; 
    sprintf(buff, "Syntax error: %s", err); 
    parse_error(tu, token, buff); 
}

internal void report(Translation_Unit *tu, int line, int ch, char *msg) {
    
    // NOTE(ziv): This is really bad and I should refactor this probably
    // but this is not important so I don't care for the time being.
    
    char err[100] = {0};  // holding the error message
    char buff[100] = {0}; // holding the integer as a string
    
    fprintf(stderr, "\n");
    strcat(err, msg); 
    strcat(err, _itoa(line, buff, 10)); 
    //strcat(err, ":");
    //strcat(err, _itoa(ch, buff, 10)); 
    fprintf(stderr, err);
    fprintf(stderr, "\n");
    
    
    // rewind to the line 
    char *start = tu->s->start; 
    for (int l = 0; *start && line-1 > l; start++) {
        if (*start == '\n') {
            l++;
        }
    }
    
    int count = 0; 
    
    char *temp = tu->s->start; 
    for (; *temp && *temp != '\n'; temp++, count++);
    fprintf(stderr, str8_to_cstring((String8){tu->s->start, count+1}));
    
    fprintf(stderr, "%*c", ch,'^');
    fprintf(stderr, "\n");
    
    // NOTE(ziv): for the time being, when 
    // the parser is reporting a error for the use 
    // it is not going to continue finding more erorrs. 
    // This is done to simplify the amounts of things 
    // that I need to think about. 
    // I might change this when I have the will.. :)
    
    //__debugbreak(); // for the debugger  
    //exit(-1);       // exit the application
}

internal bool check(Translation_Unit *tu, Token_Kind kind) { 
    if (is_at_end(tu)) return false; 
    return peek(tu).kind == kind; 
}


internal bool internal_match(Translation_Unit *tu, int n, ...) {
    va_list kinds; 
    va_start(kinds, n);
    
    Token_Kind kind; 
    for (int i = 0; i < n; i++) {
        kind = va_arg(kinds, Token_Kind); 
        if (check(tu, kind)) {
            advance(tu); 
            va_end(kinds);
            return true;
        }
    }
    va_end(kinds);
    return false; 
}

