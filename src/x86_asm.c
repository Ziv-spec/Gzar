internal void gen_translation_unit(Translation_Unit *tu, const char *filename); 
internal void stmt_gen(Translation_Unit *tu, Statement *func, Statement *stmt);
internal void expr_gen(Translation_Unit *tu, Type *func_ty, Expr *expr);
internal int  emit(const char *format, ...);
internal char *scratch_string_alloc(Translation_Unit *tu, char *str); 
internal Register scratch_alloc(int size); 
internal void scratch_free(Register r);
internal const char *scratch_name(Register r); 

// for creating labels
internal int label_create(); 
internal const char *label_name(int l); 

static char *fastcall_regs[] = {
    "rcx", "rdx", "r8", "r9"
}; 

static char *scratch_names_tbl[][7] = {
    { "bl",  "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" },
    { "bx",  "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" }, 
    { "ebx", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" }, 
    { "rbx", "r10",  "r11",  "r12",  "r13",  "r14",  "r15"  } 
};

static char *rax_reg_names[] = {
    [1] = "al",
    [2] = "ax", 
    [4] = "eax", 
    [8] = "rax"
}; 

internal const char *scratch_name(Register r) {
    
    int size = 0; 
    switch (r.size) {
        case 1: size = 0; break; 
        case 2: size = 1; break; 
        case 4: size = 2; break; 
        case 8: size = 3; break; 
        default: Assert(!"I should not be getting here");
    }
    
    return scratch_names_tbl[size][r.r];
}

static bool scratch_reg_tbl[7] = {0}; 

internal Register scratch_alloc(int size) {
    Register reg = { .size = size };
    
    while (scratch_reg_tbl[reg.r++]);
    scratch_reg_tbl[--reg.r] = true; 
    Assert(reg.r < ArrayLength(scratch_names_tbl[0]) && reg.size <= 8); 
    
    return reg; 
}

internal void scratch_free(Register reg) {
    scratch_reg_tbl[reg.r] = 0;
}

internal void scratch_free_all() {
    for (int r = 0; r < ArrayLength(scratch_names_tbl); r++) 
		scratch_reg_tbl[r] = 0;
}

internal char *symbol_gen(Translation_Unit *tu, Token t) {
    
    static char str[MAX_LEN];
    
    // the symbol is local 
    
    for (s64 j = tu->scopes->index-1; j >= 0 ; j--) {
        Statement *stmt = tu->scopes->data[j]; 
        
        if (stmt->kind == STMT_FUNCTION_DECL) {
            // I am at a function scope, I need to generate the appropriate reg names to use
            
            if (str8_compare(stmt->func.name.str, t.str) == 0) {
                snprintf(str, MAX(t.str.size, MAX_LEN), "%s", t.str.str);
            }
            
            Type *type = stmt->func.type;
            for (int i = 0; i < type->symbols->index; i++) {
                Symbol *symb = (Symbol *)type->symbols->data[i]; 
                if (str8_compare(symb->name.str, t.str) == 0) {
                    //param = [rbp + Z], Z = 16 + (param_num * 8) 
                    snprintf(str, MAX(symb->name.str.size, MAX_LEN), "[rbp+%d]", 16 + i * 8 );
                    return str; 
                }
            }
        }
        else if (stmt->kind == STMT_SCOPE) {
            // if i have found the symbol inside the current scope
            
            Block *block = &stmt->block;
            Symbol *symb = symbol_exist(block, t);
            if (symb) {
                
                if (j == 0) { // global scope
                    snprintf(str, MAX_LEN, symb->type->kind == TYPE_STRING ? "offset %s" : "%s", str8_to_cstring(t.str));
                    return str;
                }
                
                int symbol_offset = 0;
                Map_Iterator it = map_iterator(block->locals);
                while (map_next(&it)) {
                    
                    Symbol *value = it.value; 
                    String8 name = it.key; 
                    
                    if (str8_compare(symb->name.str, name) == 0) {
                        break;
                    }
                    
                    symbol_offset += get_type_size(value->type);
                    
                }
                
                //local = [rbp - Y], Y = (local_variable_usage -= size, return local_variable_usage) 
                snprintf(str, MAX(symb->name.str.size, MAX_LEN), "[rsp+%d]", (symbol_offset + block->offset) + 32);
                return str;
            }
            
        }
        
    }
    
    Assert(!"Why am I getting here?"); // this might suggest a bug in my semantic analyzer
    
    return str; 
}

internal char *scratch_string_alloc(Translation_Unit *tu, char *str) {
    // generates a string label for later use
    
    static char name[MAX_LEN];
    snprintf(name, MAX_LEN, "offset $%d", tu->unnamed_strings->index);
    vec_push(&tu->m, tu->unnamed_strings, str);
    
    return name;
}

internal int label_create() {
    static int label_index = 0; 
    return label_index++; 
}

internal const char *label_name(int l) {
    static char str[MAX_LEN];
    snprintf(str, MAX_LEN, "L%d", l);
    return str;
}


// this is a buffer onto which I put the resulting assembly code. 
static char _buff[16*1024];

int emit(const char *format, ...) {
    int done;
    static size_t offset = 0; 
    
    va_list arg;
    va_start(arg, format);
    done = vsprintf(_buff+offset, format, arg);
    va_end(arg);
    offset += strlen(_buff+offset);
    
    return done;
}


internal Register cast_register_gen(Register reg_from, int size_to, int sign) {
    
    int size_from = reg_from.size;
    if (size_to != size_from) {
        Register reg_to = (Register){ reg_from.r, size_to }; 
        
        if ((size_to == 8 || size_from == 8) && 
            (size_to == 4 || size_from == 4)) {
            emit("  mov %s, %s\n", scratch_name(reg_from), scratch_name(reg_from));
        }
        else if (size_to == 8 && size_from == 4) {
            // do nothing
        }
        else if (size_to > size_from){
            emit("  %s  %s, %s\n", sign ? "movsx" : "movzx", scratch_name(reg_to), scratch_name(reg_from));
        }
        
        return reg_to;
    }
    
    return reg_from;
}

internal void expr_gen(Translation_Unit *tu, Type *func_ty, Expr *expr) {
    if (!expr) return; 
    
	int size = get_type_size(expr->type); 
    
    switch (expr->kind) {
        
        case EXPR_LITERAL: {
            expr->reg = scratch_alloc(size); 
            
            switch (expr->literal.kind) {
                
                case TYPE_STRING: {
                    char *str = scratch_string_alloc(tu, expr->literal.data);
                    emit("  mov %s, %s\n", scratch_name(expr->reg), str);
                } break; 
                
                case TYPE_S8:
                case TYPE_U8:
                case TYPE_S16:
                case TYPE_U16:
                case TYPE_BOOL:
                case TYPE_U64:
                case TYPE_S64:
                case TYPE_POINTER: {
                    emit("  mov %s, %lld\n", scratch_name(expr->reg), (long long)expr->literal.data); 
                } break;
                
                default: {
                    Assert(!"Not implemented yet");
                } break;
                
            }
            
        } break;
        
        case EXPR_BINARY: {
            expr_gen(tu, func_ty, expr->left); 
            expr_gen(tu, func_ty, expr->right); 
            
            // implicit cast          
            int reg_size = get_type_size(expr->type);  // TODO(ziv): check whether I need this 
            if (reg_size != expr->left->reg.size) {
                expr->left->reg = cast_register_gen(expr->left->reg, reg_size, is_signed_integer(expr->left->type));
            }
            if (reg_size != expr->right->reg.size) {
                expr->right->reg = cast_register_gen(expr->right->reg, reg_size, is_signed_integer(expr->right->type)); 
            }
            
            const char *left_reg_name = scratch_name(expr->left->reg);
            const char *right_reg_name = scratch_name(expr->right->reg);
            
            Token operation = expr->operation;
            switch (operation.kind) {
                
                case TK_MINUS: 
                case TK_PLUS: 
                {
                    emit("  %s %s, %s\n", operation.kind == TK_PLUS ? "add" : "sub",
                         left_reg_name, right_reg_name); 
                    
                    expr->reg = expr->left->reg;
                    scratch_free(expr->right->reg);
                } break;
                
                case TK_MODOLU:
                case TK_SLASH: 
                case TK_STAR:
                {
                    static char *remainder_regs[] = {
                        [1] = "ah",
                        [2] = "dx",
                        [4] = "edx",
                        [8] = "rdx"
                    }; 
                    
                    expr->reg = scratch_alloc(size); 
                    emit("  push rdx\n");
                    emit("  xor rdx, rdx\n");
                    emit("  mov %s, %s\n", rax_reg_names[expr->left->reg.size], left_reg_name);
                    emit("  %s%s %s\n", 
                         is_signed_integer(expr->left->type) ? "i" : "", 
                         (operation.kind == TK_STAR) ? "mul" : "div",
                         right_reg_name);
                    emit("  mov %s, %s\n", 
                         scratch_name(expr->reg), 
                         (operation.kind == TK_MODOLU) ? remainder_regs[expr->left->reg.size] : rax_reg_names[expr->left->reg.size]); 
                    emit("  pop rdx\n");
                    
                    scratch_free(expr->right->reg);
                    scratch_free(expr->left->reg);
                } break; 
                
                case TK_LESS: 
                case TK_GREATER:
                case TK_EQUAL_EQUAL: 
                case TK_GREATER_EQUAL: 
                case TK_LESS_EQUAL: 
                case TK_BANG_EQUAL: 
                {
                    expr->reg = scratch_alloc(size);
                    emit("  cmp %s, %s\n", left_reg_name, right_reg_name); 
                    
                    char *op = NULL;
                    switch (operation.kind) {
                        case TK_LESS: op = "setl"; break;
                        case TK_GREATER: op = "setg"; break;
                        case TK_BANG_EQUAL:    op = "setne"; break;
                        case TK_EQUAL_EQUAL:   op = "sete"; break;
                        case TK_LESS_EQUAL:    op = "setle"; break;
                        case TK_GREATER_EQUAL: op = "setge"; break;
                        default: Assert(!"Should not be getting here"); break;
                    }
                    
                    emit("  %s al\n", op);
                    emit("  movzx rax, al\n");
                    emit("  mov %s, rax\n", scratch_name(expr->reg));
                    
                    scratch_free(expr->right->reg);
                    scratch_free(expr->left->reg);
                } break;
                
                
                case TK_AND: 
                case TK_XOR: 
                case TK_OR: 
                {
                    emit("  %s %s, %s\n", operation.kind == TK_AND ? "and" : operation.kind == TK_OR ? "or" : "xor" ,
                         left_reg_name, right_reg_name); 
                    
                    expr->reg = expr->left->reg;
                    scratch_free(expr->right->reg);
                } break;
                
                
                case TK_DOUBLE_AND:
                case TK_DOUBLE_OR: 
                {
                    emit("  %s %s, %s\n", operation.kind == TK_DOUBLE_AND ? "and" : "or",
                         left_reg_name, right_reg_name); 
                    
                    expr->reg = expr->left->reg;
                    scratch_free(expr->right->reg);
                } break; 
                
                default: {
                    Assert(!"Not implemented yet");
                }
                
            };
        } break; 
        
        case EXPR_LVAR: {
            expr->reg = scratch_alloc(size); 
            emit("  mov %s, %s\n", scratch_name(expr->reg), 
                 symbol_gen(tu, expr->lvar.name)); 
            
        } break;
        
        case EXPR_ASSIGN: {
            expr_gen(tu, func_ty, expr->assign.rvalue);
            emit("  mov %s, %s\n", symbol_gen(tu, expr->assign.lvar->lvar.name), scratch_name(expr->assign.rvalue->reg)); 
            expr->reg = expr->assign.rvalue->reg;
        } break; 
        
        case EXPR_CALL: {
            
            // Generating the results of the expressions
            Register regs[10] = {0};  // buffer to store the scratch registers from generated expression
            int reg_index = 0; 
            
            // generate expression for call arguments
            Vector *args = expr->call.args->args; 
            for (int i = 0; i < args->index; i++) {
                Expr *e = (Expr *)args->data[i];
                expr_gen(tu, func_ty, e); 
                regs[reg_index++] = (Register){ e->reg.r, 8 }; // saving scratch registers holding the values to the function call
            }
            
            // 
            // Moving the results into the correct place
            // that which follow the calling convention
            // aka first 4 into __fastcall regs, and the
            // other ones into the stack
            // 
            
            for (int i = 0; i < MIN(reg_index, 4); i++) {
                emit("  mov %s, %s\n", fastcall_regs[i], scratch_name(regs[i]));
                scratch_free(regs[i]);
            }
            
            for (int i = 4; i < reg_index; i++) {
                emit("  mov [rsp+%d], %s\n",i*8, scratch_name(regs[i]));
                scratch_free(regs[i]);
            }
            
            emit("  call %s\n", str8_to_cstring(expr->call.name->lvar.name.str));
            
            
            Symbol *func = local_exist(tu, expr->call.name->lvar.name); 
            Type *func_type = func->type;
            
            
            // If we don't expect a return value, don't put code to expect it.
            if (!(func_type->subtype->kind == TYPE_VOID)) {
                // move the result expected to be in `rax` into a scratch register
                expr->reg = scratch_alloc(8);
                emit("  mov %s, rax\n", scratch_name(expr->reg)); 
            }
            else {
                expr->reg.r = -1;  // aka do not expect a register from this node
            }
            
        } break; 
        
        case EXPR_UNARY: {
            expr_gen(tu, func_ty, expr->unary.right);
            
            switch (expr->unary.operation.kind) {
                
                case TK_MINUS: {
                    emit("  neg %s\n", scratch_name(expr->unary.right->reg));
                    expr->reg = expr->unary.right->reg;
                } break; 
                
                case TK_BANG: {
                    emit("  not %s\n", scratch_name(expr->unary.right->reg));
                    emit("  and %s, 1\n", scratch_name(expr->unary.right->reg)); 
                    expr->reg = expr->unary.right->reg;
                } break; 
                
                case TK_CAST: {
                    // Explicit cast
                    expr->reg = cast_register_gen(expr->unary.right->reg, size, is_signed_integer(expr->unary.right));
                    
                } break; 
                
                default: {
                    Assert(!"NOT IMPLEMENTED"); 
                } break; 
                
            }
            
        }
        
    }
    
}


static bool is_main = true; 

internal void stmt_gen(Translation_Unit *tu, Statement *func, Statement *stmt) {
    if (!stmt) return;
    
    switch (stmt->kind) {
        
        case STMT_VAR_DECL: {
            
            expr_gen(tu, func->func.type, stmt->var_decl.initializer);
            if (stmt->var_decl.initializer) {
                emit("  mov %s, %s\n", symbol_gen(tu, stmt->var_decl.name), scratch_name(stmt->var_decl.initializer->reg)); 
                scratch_free(stmt->var_decl.initializer->reg);
            }
            else {
                // give an initial value of 0
                emit("  mov qword ptr %s, 0\n", symbol_gen(tu, stmt->var_decl.name)); 
                
            }
            
        } break;
        
        case STMT_IF: {
            
            Expr *condition = stmt->if_else.condition;
            
            expr_gen(tu, func->func.type, condition);
            
            emit("  test %s, 1\n", scratch_name(condition->reg)); 
            
            scratch_free(condition->reg);
            
            int over_true_block = label_create();
            emit("  je  %s\n", label_name(over_true_block));
            
            Statement *true_block = stmt->if_else.true_block;
            stmt_gen(tu, func, true_block); 
            
            emit("%s:\n", label_name(over_true_block));
            
            Statement *false_block = stmt->if_else.false_block;
            if (false_block) {
                stmt_gen(tu, func, false_block); 
            }
            
        } break; 
        
        
        case STMT_WHILE: {
            
            int loop_beginning = label_create();
            emit("%s:\n", label_name(loop_beginning));
            // handle condition 
            
            Expr *condition = stmt->loop.condition;
            expr_gen(tu, func->func.type, condition);
            emit("  test %s, 1\n", scratch_name(condition->reg)); 
            scratch_free(condition->reg);
            
            // handle body
            int over_body = label_create();
            emit("  je  %s\n", label_name(over_body));
            Statement *block = stmt->loop.block;
            stmt_gen(tu, func, block); 
            emit("  jmp %s\n", label_name(loop_beginning));
            emit("%s:\n", label_name(over_body));
            
        } break;
        
        case STMT_SCOPE: {
            push_scope(tu, stmt);
            
            stmt->block.offset = tu->offset;
            
            // the block of statements I will actually need to travaverse
            for (int i = 0; i < stmt->block.statements->index; i++) {
                Statement *_stmt = (Statement *)stmt->block.statements->data[i]; 
                if (_stmt->kind == STMT_VAR_DECL) {
                    tu->offset += get_type_size(_stmt->var_decl.type);
                }
                stmt_gen(tu, func, _stmt);
            }
            
            pop_scope(tu);
        } break; 
        
        case STMT_FUNCTION_DECL: {
            
            // TODO(ziv): do a push and pop and something in here
            scratch_free_all(); // registers we used in another function should not matter here
            // so we clear their use data. 
            
            char *func_name = str8_to_cstring(stmt->func.name.str);
            
            // function prototype
            if (!stmt->func.sc) {
                emit("%s proto\n", func_name); 
                return;
            }
            
            // handling win64 calling convention: 
            //
            // caller
            //X = round_up(min(32, callee_usage * 8) + local_variable_usage, 16) + 8 
            //       ^                                      ^                      ^
            //    stack alignment                      not much to say,   remembmer 'push rbp'
            
            push_scope(tu, stmt); 
            
            Block block = stmt->func.sc->block;
            int local_variable_and_callee_usage = block.size;
            
            int stack_size = local_variable_and_callee_usage <= 0 ? 32 : (32 + 16 + local_variable_and_callee_usage ) & ~(16-1) + 8; 
            
            // prologe
            { 
                emit("%s%s\n", func_name, is_main ? ":" : " PROC"); 
                if (!is_main) emit("  push rbp\n");
                emit("  mov rbp, rsp\n");
                emit("  push rbx\n"); 
                emit("  push r10\n"); 
                emit("  push r11\n"); 
                emit("  push r12\n"); 
                emit("  push r13\n"); 
                emit("  push r14\n"); 
                emit("  push r15\n"); 
                emit("  sub rsp, %d\n", stack_size);  
            }
            
            // generate the saving of the __fastcall registers
            {
                Vector *symbols = stmt->func.type->symbols; 
                int argument_count = MIN(symbols->index, 4); // I need to save upto 4 registers
                for (int i = 0; i < argument_count; i++) {
                    emit("  mov [rbp+%d], %s\n", 16+i*8, fastcall_regs[i]);
                }
            }
            
            // generating the body
            {
                stmt_gen(tu, func, stmt->func.sc);
            }
            
            // epilogue
            {
                emit("%s_epilogue:\n", func_name);
                emit("  add rsp, %d\n", stack_size); 
                emit("  pop r15\n"); 
                emit("  pop r14\n"); 
                emit("  pop r13\n"); 
                emit("  pop r12\n"); 
                emit("  pop r11\n"); 
                emit("  pop r10\n"); 
                emit("  pop rbx\n"); 
                if (!is_main)        emit("  pop rbp\n");
                emit("  ret\n");
                if (!is_main)        emit("%s ENDP\n", func_name); 
            }
            
            tu->offset = 0;
            pop_scope(tu);
            
        } break;
        
        case STMT_EXPR: {
            expr_gen(tu, func->func.type, stmt->expr); 
            
            if (stmt->expr->reg.r > 0) {
                scratch_free(stmt->expr->reg);
            }
            
        } break; 
        
        case STMT_RETURN: {
            
            // function return stmt
            expr_gen(tu, func->func.type, stmt->expr);
            
            char *inst = is_signed_integer(stmt->expr) ? "movsx" : "movzx"; 
            
            if (stmt->expr->reg.size == 8) {
                emit("  mov rax, %s\n", scratch_name(stmt->expr->reg));
            }
            else if (stmt->expr->reg.size == 4) {
                emit("  mov eax, %s\n", scratch_name(stmt->expr->reg));
            }
            else {
                emit("  %s rax, %s\n", inst, scratch_name(stmt->expr->reg));
            }
            
            emit("  jmp %s_epilogue\n", str8_to_cstring(func->func.name.str));
            scratch_free(stmt->expr->reg);
        } break; 
        
        
        default: {
            Assert(!"Not implemented yet");
        } break; 
    }
    
}

// 
// Set of functions for generating globals in the .data segment
// 

internal void gen_translation_unit(Translation_Unit *tu, const char *filename) {
    if (!tu) return; 
    
    // 
    // Generate the globals inside the .data segment
    // for example the global variables and public function 
    // interfaces will get generated in this phase
    // 
    
    Vector *decls = tu->decls;
    Statement *block_stmt = tu->scopes->data[0]; 
    Block *block = &block_stmt->block;
    
    Assert(0 < decls->index);
    
    
    // 
    // Now we generate the code for each of the functions 
    // at the order that we see them. 
    // 
    {
        emit("\n\n"); 
        emit(".code\n");
        
        // skip non function declorations
        Statement *stmt = (Statement *)decls->data[0]; 
        int i;
        for (i = 1; i < decls->index && stmt->kind != STMT_FUNCTION_DECL; i++) {
            stmt = (Statement *)decls->data[i];
        }
        
        const char *entry_point_name = "main"; 
        for (; i <= decls->index; i++) {
            emit("\n");
            
            is_main = str8_compare(stmt->func.name.str, str8_lit(entry_point_name)) == 0;
			
            stmt_gen(tu, stmt, stmt);
            
            // skip non function declorations
            stmt = (Statement *)decls->data[i]; 
            
            while (i < decls->index && stmt->kind != STMT_FUNCTION_DECL) {
                stmt = (Statement *)decls->data[++i];
            }
            
        }
        
    }
    
    
    Map *symbols_map = block->locals; 
    
    // generate code for the data segment 
    {
        emit("\n\n"); 
        if (symbols_map->count > 0)
            emit(".data\n");
        
        for (int i = 0; i < tu->unnamed_strings->index; i++) {
            emit("$%d db \"%s\", 0\n", i,  (char *)tu->unnamed_strings->data[i]); 
        }
        
        char buff[MAX_LEN];
        s64 buff_cursor = 0; 
        
        Map_Iterator it = map_iterator(symbols_map); 
        while (map_next(&it)) {
            Symbol *symb = (Symbol *)it.value;
            
            // generate the appropriate global data with this
            switch (symb->type->kind) {
                
                case TYPE_STRING: {
                    // TODO(ziv): ouput the bytes representing the string not a string idk
                    emit("%s db \"%s\", 0\n", 
                         str8_to_cstring(symb->name.str),
                         (char *)symb->initializer->literal.data);
                } break; 
                
                // TODO(ziv): add more literal types with different sizes
                case TYPE_BOOL:
                case TYPE_S64: { 
                    if ((long long)symb->initializer) {
                        emit("%s dq %lld\n", 
                             str8_to_cstring(symb->name.str), (long long)symb->initializer->literal.data);
                    }
                    else {
                        emit("%s dq %lld\n", 
                             str8_to_cstring(symb->name.str), 0);
                    }
                    
                    
                } break; 
                
                case TYPE_FUNCTION: break;
                
                default: {
                    Assert(!"Not implemented yet");
                } break;
                
            }
        }
        
        // entry point is main
        strcpy(&buff[buff_cursor], "PUBLIC main");
        buff_cursor += (int)strlen("PUBLIC main");
        strcpy(&buff[buff_cursor++], "\n");
        // output public function declorations
        emit("\n");
        strcpy(&buff[buff_cursor++], "\0");
        emit(buff); 
    }
    
    emit("END\n");
    
    // 
    // open the first file and read it's contents
    // 
    
    FILE *file = fopen(filename,  "w+"); 
    if (!file) {
        fprintf(stderr, "Error: failed to open file '%s' to write output assembly to\n", filename);
    }
    
    fprintf(file, _buff);
    
    fclose(file);
}