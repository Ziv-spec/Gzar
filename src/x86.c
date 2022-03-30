
internal Register scratch_alloc(); 
internal void scratch_free(Register r);
internal const char *scratch_name(Register r); 

// for creating labels
internal int labal_create(); 
internal const char *labal_name(int l); 


static char *fastcall_regs[] = {
    "rcx", "rdx", "r8", "r9"
}; 

static char *scratch_names_tbl[] = { 
    "rbx", "r10", "r11", "r12", "r13", "r14", "r15" 
};

internal const char *scratch_name(Register r) {
    return scratch_names_tbl[r];
}


static bool scratch_reg_tbl[ArrayLength(scratch_names_tbl)] = {0}; 

internal Register scratch_alloc() {
    Register r = 0; 
    while (scratch_reg_tbl[r++]);
    scratch_reg_tbl[r-1] = true; 
    
    Assert(r-1 < ArrayLength(scratch_names_tbl)); 
    
    return r-1; 
}

static int fastcall_index = 0; 

internal char *fastcall_alloc() {
    return fastcall_regs[fastcall_index++]; 
}

internal void fastcall_free() {
    fastcall_index = 0;
}

internal void scratch_free(Register reg) {
    scratch_reg_tbl[reg] = 0;
}


internal void scratch_free_all() {
    for (int r = 0; r < ArrayLength(scratch_names_tbl); r++) {
        scratch_reg_tbl[r] = 0;
        
    }
}

/*
This function generates a local symbol e.g. 
s: int = 0;  would get turned into a mov rbx, [rbp+8]
so the variable 's' is assisiated with the offset 8
from the base pointer 'rbp'. 

For this reason I need to know the order and size of each 
of the other symbols inside a given block. This can be solved
in a more elegant way naturally by using a hash map which 
I will do in the future. That map will contain the allocated 
mapping for all symbols inside a given block

*/ 
internal char *symbol_gen(Translation_Unit *tu, Token t) {
    
    static char str[MAX_LEN];
    
    Block *block = get_curr_scope(tu);
    
    // the symbol is local 
    Map *locals = block->locals; 
    int offset_to_symbol = 0; 
    
    for (s64 i = tu->block_stack.index-1; i >= 0; i--) {
        Block *b = tu->block_stack.blocks[i]; 
        Symbol *symb = symbol_exist(b, t); 
        if (symb && i == 0) {
            
            
            if (symb->type->kind == TYPE_STRING) {
                snprintf(str, MAX_LEN, "offset %s", str8_to_cstring(t.str));
            }
            else {
                snprintf(str, MAX_LEN, "%s", str8_to_cstring(t.str));
            }
            return str;
        }
    }
    
    // generate the appropriate offset for the symbol
    for (int i = 0; i < locals->capacity; i++){
        Bucket b = locals->buckets[i]; 
        if (b.value) {
            Symbol *symb = (Symbol *)b.value; 
            offset_to_symbol += get_type_size(symb->type); 
            
            if (str8_compare(symb->name.str, t.str) == 0) {
                break;
            }
        }
    }
    
    
    snprintf(str, MAX_LEN, "[rbp+%d]", offset_to_symbol);
    return str; 
}

internal char *scratch_string_alloc(char *str) {
    // generates a string label for later use
    return str;
}


static FILE *output_file;

int emit(const char *format, ...)
{
    va_list arg;
    int done;
    
    va_start(arg, format);
    done = vfprintf(output_file, format, arg);
    va_end(arg);
    
    return done;
}


internal void expr_gen(Translation_Unit *tu, Type *func_ty, Expr *expr) {
    if (!expr) return; 
    
    switch (expr->kind) {
        
        case EXPR_LITERAL: {
            expr->reg = scratch_alloc(); 
            
            switch (expr->literal.kind) {
                
                case TYPE_STRING: {
                    char *str = scratch_string_alloc(expr->literal.data);
                    emit("  mov %s, %s\n", scratch_name(expr->reg), str);
                } break; 
                
                // TODO(ziv): add more literal types 
                case TYPE_BOOL:
                case TYPE_S64: { 
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
            
            Token operation = expr->operation;
            switch (operation.kind) {
                
                case TK_MINUS: 
                case TK_PLUS: 
                {
                    emit("  %s %s, %s\n", operation.kind == TK_PLUS ? "add" : "sub",
                         scratch_name(expr->left->reg), scratch_name(expr->right->reg)); 
                    
                    expr->reg = expr->left->reg;
                    scratch_free(expr->right->reg);
                } break;
                
                // TODO(ziv): make division work
                // case TK_SLASH: 
                case TK_SLASH: 
                case TK_STAR:
                {
                    
                    expr->reg = scratch_alloc(); 
                    emit("  push rdx\n");
                    emit("  xor rdx, rdx\n");
                    emit("  mov rax, %s\n", scratch_name(expr->left->reg));
                    emit("  %s %s\n", (operation.kind == TK_STAR) ? "imul" : "idiv", scratch_name(expr->right->reg)); 
                    emit("  mov %s, rax\n", scratch_name(expr->reg)); 
                    emit("  pop rdx\n");
                    
                    scratch_free(expr->right->reg);
                    scratch_free(expr->left->reg);
                } break; 
                
                
                
                default: {
                    Assert(!"Not implemented yet");
                }
                
            };
            
            
        } break; 
        
        case EXPR_LVAR: {
            expr->reg = scratch_alloc(); 
            
            int found_index = -1; 
            for (int i = 0; i < func_ty->symbols->index; i++) {
                Symbol *symb = (Symbol *)func_ty->symbols->data[i]; 
                if (str8_compare(symb->name.str, expr->lvar.name.str) == 0) {
                    found_index = i; 
                    break;
                }
            }
            
            emit("  mov %s, %s\n", scratch_name(expr->reg), 
                 (found_index == -1) ? symbol_gen(tu, expr->lvar.name) : fastcall_regs[found_index]); 
            
        } break;
        
        case EXPR_ASSIGN: {
            expr_gen(tu, func_ty, expr->assign.rvalue);
            emit("  mov %s, %s\n", symbol_gen(tu, expr->assign.lvar->lvar.name), scratch_name(expr->assign.rvalue->reg)); 
            expr->reg = expr->assign.rvalue->reg;
            
        } break; 
        
        case EXPR_CALL: {
            
            // generate all of the expressions 
            // take their results and put into the correct calling convention rules 
            // push the regs, 
            // call the function name
            // then pop and restore them later 
            // take the result in rax 
            // move it into a new scratch reg 
            
            
            //
            // Generating the results of the expressions
            //
            
            Register regs[10] = {0}; // TODO(ziv): I should change the way this works 
            int reg_index = 0; 
            
            // generate expression for call arguments
            Vector *args = expr->call.args->args; 
            for (int i = 0; i < args->index; i++) {
                Expr *e = (Expr *)args->data[i];
                expr_gen(tu, func_ty, e); 
                regs[reg_index++] = e->reg; // saving their scratch registers
            }
            
            // 
            // Moving the results into the correct registers 
            // that which follow the calling convention 
            // 
            
            for (int i = 0; i < reg_index; i++) {
                emit("  mov %s, %s\n", fastcall_alloc(), scratch_name(regs[i]));
            }
            
            reg_index = 0;
            for (int r = 0; r < ArrayLength(scratch_reg_tbl); r++) {
                if (scratch_reg_tbl[r]) {
                    regs[reg_index++] = r;
                }
            }
            
            for (int i = reg_index-1; i >= 0; i--) {
                emit("  push %s\n", scratch_name(regs[i])); 
            }
            
            fastcall_free();
            
            emit("  call %s\n", str8_to_cstring(expr->call.name->lvar.name.str));
            
            // restore the stack 
            for (int i = 0; i < reg_index; i++) {
                emit("  pop %s\n", scratch_name(regs[i])); 
            }
            
            // move the result expected to be in `rax` into a scratch register
            expr->reg = scratch_alloc();
            emit("  mov %s, rax\n", scratch_name(expr->reg)); 
            
            
        } break; 
        
        case EXPR_GROUPING: {
            expr_gen(tu, func_ty, expr->grouping.expr); 
            expr->reg = expr->grouping.expr->reg;
        } break;
        
        case EXPR_UNARY: {
            expr_gen(tu, func_ty, expr->unary.right);
            
            switch (expr->unary.operation.kind) {
                
                case TK_MINUS: {
                    emit("  neg %s\n", scratch_name(expr->unary.right->reg));
                    expr->reg = expr->unary.right->reg;
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
            emit("  mov %s, %s\n", symbol_gen(tu, stmt->var_decl.name), scratch_name(stmt->var_decl.initializer->reg)); 
            scratch_free(stmt->var_decl.initializer->reg);
        } break;
        
        case STMT_SCOPE: {
            push_scope(tu, stmt);
            
            // the block of statements I will actually need to travaverse
            for (int i = 0; i < stmt->block.statements->index; i++) {
                Statement *_stmt = (Statement *)stmt->block.statements->data[i]; 
                stmt_gen(tu, func, _stmt);
            }
            
            pop_scope(tu);
        } break; 
        
        case STMT_FUNCTION_DECL: {
            
            //
            // Generation of the function:
            //
            
            scratch_free_all(); // we don't want to depend on registers from prefiouse functions
            
            char *func_name = str8_to_cstring(stmt->func.name.str);
            Type *type = stmt->func.type; type;
            
            // regular function
            if (stmt->func.sc) {
                Block block = stmt->func.sc->block; 
                
                // prologe
                { 
                    emit("%s%s\n", func_name, is_main ? ":" : " PROC"); 
                    if (!is_main) {
                        emit("  push rsp\n");
                    }
                    emit("  mov rbp, rsp\n");
                }
                
                // setting up the stack
                int stack_size = 0; 
                { 
                    
                    // calc how many bytes do all variable declorations take up
                    
                    Map *locals = block.locals; 
                    Map_Iterator mit = map_iterator(locals); 
                    
                    // calc the size of the input arguments to a function 
                    // and then subtract this size from the total 
                    
                    // TODO(ziv): check whether I want to have this limited to 4 arguments 
                    
                    int input_arguments_size = 0; 
                    Vector *symbols = type->symbols; 
                    for (int i = 0; i < symbols->index; i++) {
                        Symbol *symb = (Symbol *)symbols->data[i]; 
                        input_arguments_size += get_type_size(symb->type); 
                    }
                    
                    int total_size = 0; 
                    while (map_next(&mit)) {
                        Symbol *symb = (Symbol *)mit.value; 
                        total_size += get_type_size(symb->type); 
                    }
                    
                    stack_size = total_size - input_arguments_size; 
                    
                    if (stack_size > 0) {
                        emit("  sub rsp, %d\n", stack_size);  
                    }
                }
                
                // generating the body
                stmt_gen(tu, func, stmt->func.sc);
                
                // epilogue
                {
                    emit("%s_epilogue:\n", func_name);
                    if (stack_size > 0) {
                        emit("  add rsp, %d\n", stack_size); 
                    }
                    if (!is_main) {
                        emit("  pop rsp\n");
                    }
                    emit("  ret\n");
                    if (!is_main) {
                        emit("%s ENDP\n", func_name); 
                    }
                }
            }
            else {
                emit("%s proto\n", func_name); 
                // function prototype.
            }
            
            
        } break;
        
        case STMT_EXPR: {
            expr_gen(tu, func->func.type, stmt->expr); 
            scratch_free(stmt->expr->reg);
        } break; 
        
        case STMT_RETURN: {
            
            // function return stmt
            expr_gen(tu, func->func.type, stmt->expr);
            emit("  mov rax, %s\n", scratch_name(stmt->expr->reg));
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

internal void x86gen_translation_unit(Translation_Unit *tu) {
    if (!tu) return; 
    
    output_file = stdout; // this is subject to change
    
    // 
    // Generate the globals inside the .data segment
    // for example the global variables and public function 
    // interfaces will get generated in this phase
    // 
    
    Vector *decls = tu->decls;
    Block *block = tu->block_stack.blocks[0]; 
    
    Assert(0 < decls->index);
    
    Map *symbols_map = block->locals; 
    
    // generate code for the data segment 
    {
        emit("\n\n"); 
        if (symbols_map->count > 0)
            emit(".data\n");
        
        char buff[MAX_LEN];
        s64 buff_cursor = 0; 
        
        Map_Iterator it = map_iterator(symbols_map); 
        
        while (map_next(&it)) {
            Symbol *symb = (Symbol *)it.value;
            
            // generate the appropriate global data with this
            switch (symb->type->kind) {
                
                case TYPE_STRING: {
                    emit("%s db \"%s\", 0\n", 
                         str8_to_cstring(symb->name.str),
                         (char *)symb->initializer->literal.data);
                } break; 
                
                // TODO(ziv): add more literal types with different sizes
                case TYPE_BOOL:
                case TYPE_S64: { 
                    emit("%s dq %lld\n", 
                         str8_to_cstring(symb->name.str),
                         (long long)symb->initializer->literal.data);
                } break; 
                
                case TYPE_FUNCTION: {
                    // TODO(ziv): Maybe if I have semantics for declaring functions as private 
                    // then I would not need to do this decloration as pulic or whatever
                    
                    if (is_main) {
                        // write to a buff
                        strcpy(&buff[buff_cursor], "PUBLIC ");
                        buff_cursor += (int)strlen("PUBLIC ");
                        strncpy(&buff[buff_cursor], symb->name.str.str, symb->name.str.size);
                        buff_cursor += symb->name.str.size;
                        //strcpy(&buff[buff_cursor], " PROC");
                        //buff_cursor += (int)strlen(" PROC");
                        strcpy(&buff[buff_cursor++], "\n");
                    }
                    
                    
                } break;
                
                default: {
                    Assert(!"Not implemented yet");
                } break;
                
            }
        }
        
        // output public function declorations
        emit("\n");
        strcpy(&buff[buff_cursor++], "\0");
        emit(buff); 
    }
    
    
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
            
            String8 s = str8_lit(entry_point_name);
            is_main = str8_compare(stmt->func.name.str, s) == 0;
            
            stmt_gen(tu, stmt, stmt);
            
            // skip non function declorations
            stmt = (Statement *)decls->data[i]; 
            
            while (i < decls->index && stmt->kind != STMT_FUNCTION_DECL) {
                stmt = (Statement *)decls->data[++i];
            }
            
        }
        
        emit("END\n");
    }
    
    
}