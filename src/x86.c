
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
internal char *symbol_gen(Token t) {
    
    Statement *block_stmt = get_curr_scope();
    Scope block = block_stmt->block;
    
    
    // the symbol is local 
    Vector *locals = block.locals; 
    int offset_to_symbol = 0; 
    
    // generate the appropriate offset for the symbol
    for (int i = 0; i < locals->index; i++){
        Symbol *s = (Symbol *)locals->data[i];
        offset_to_symbol += get_type_size(s->type);
        
        if (strncmp(s->name.str, t.str, MIN(s->name.len, t.len)) == 0) {
            break; 
        }
    }
    
    
    
    if (offset_to_symbol == 0 && symbol_exist(global_block, t)) {
        return slice_to_str(t.str, t.len);
    }
    
    
    static char str[MAX_LEN];
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


internal void expr_gen(Type *func_ty, Expr *expr) {
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
            expr_gen(func_ty, expr->right); 
            expr_gen(func_ty, expr->left); 
            
            Token operation = expr->operation;
            switch (operation.kind) {
                
                case TK_MINUS: 
                case TK_PLUS: 
                {
                    emit("  %s %s, %s\n", operation.kind == TK_PLUS ? "add" : "sub",
                         scratch_name(expr->left->reg), scratch_name(expr->right->reg)); 
                    
                    expr->reg = expr->left->reg;
                    scratch_free(expr->left->reg);
                } break;
                
                // TODO(ziv): make division work
                // case TK_SLASH: 
                case TK_STAR:
                {
                    
                    expr->reg = scratch_alloc(); 
                    emit("  push rdx\n");
                    emit("  mov rax, %s\n", scratch_name(expr->right->reg));
                    emit("  mul %s\n", scratch_name(expr->left->reg)); 
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
                if (strncmp(expr->lvar.name.str, symb->name.str, MIN(expr->lvar.name.len, symb->name.len)) == 0) {
                    found_index = i; 
                    break;
                }
            }
            
            emit("  mov %s, %s\n", scratch_name(expr->reg), 
                 (found_index == -1) ? symbol_gen(expr->lvar.name) : fastcall_regs[found_index]); 
            
        } break;
        
        case EXPR_ASSIGN: {
            expr_gen(func_ty, expr->assign.rvalue);
            emit("  mov %s, %s\n", symbol_gen(expr->assign.lvar->lvar.name), scratch_name(expr->assign.rvalue->reg)); 
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
            
            Expr *args = expr->call.args; 
            for (; args && args->kind == EXPR_ARGUMENTS; args = args->args.next) {
                Expr *lvar = args->args.lvar; 
                expr_gen(func_ty, lvar); 
                regs[reg_index++] = lvar->reg; 
            }
            
            if (args) {
                // detect the type 
                expr_gen(func_ty, args); 
                regs[reg_index++] = args->reg; 
            }
            
            
            // 
            // Moving the results into the correct registers 
            // that which follow the calling convention 
            // 
            
            for (int i = 0; i < reg_index; i++) {
                emit("  mov %s, %s\n", fastcall_alloc(), scratch_name(regs[i]));
            }
            
            for (int i = 0; i < reg_index; i++) {
                emit("  push %s\n", scratch_name(regs[i])); 
            }
            
            fastcall_free();
            
            emit("  call %s\n", slice_to_str(expr->call.name->lvar.name.str,
                                             expr->call.name->lvar.name.len));
            
            // restore the stack 
            for (int i = 0; i < reg_index; i++) {
                emit("  pop %s\n", scratch_name(regs[i])); 
            }
            
            // move the result expected to be in `rax` into a scratch register
            expr->reg = scratch_alloc();
            emit("  mov %s, rax\n", scratch_name(expr->reg)); 
            
            
        } break; 
        
    }
    
}



static bool is_main = true; 

internal void stmt_gen(Statement *func, Statement *stmt) {
    if (!stmt) return;
    
    switch (stmt->kind) {
        
        case STMT_VAR_DECL: {
            
            expr_gen(func->func.type, stmt->var_decl.initializer);
            emit("  mov %s, %s\n", symbol_gen(stmt->var_decl.name), scratch_name(stmt->var_decl.initializer->reg)); 
            scratch_free(stmt->var_decl.initializer->reg);
        } break;
        
        case STMT_SCOPE: {
            push_scope(stmt);
            
            // the block of statements I will actually need to travaverse
            for (int i = 0; i < stmt->block.statements->index; i++) {
                Statement *_stmt = (Statement *)stmt->block.statements->data[i]; 
                stmt_gen(func, _stmt);
            }
            
            pop_scope();
        } break; 
        
        case STMT_FUNCTION_DECL: {
            
            //
            // Generation of the function:
            //
            
            char *func_name = slice_to_str(stmt->func.name.str, stmt->func.name.len);
            Type *type = stmt->func.type; type;
            Scope block = stmt->func.sc->block; 
            
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
                
                bool ignore = false; 
                for (int i = 0; i < block.locals->index; i++){
                    Symbol *s = (Symbol *)block.locals->data[i];
                    
                    for (int j = 0; j < type->symbols->index; j++) {
                        Symbol *function_symbol = (Symbol *)type->symbols->data[j]; 
                        if (function_symbol->name.len == s->name.len && 
                            strncmp(s->name.str, function_symbol->name.str, MIN(s->name.len, function_symbol->name.len)) == 0) {
                            ignore = true; 
                            break;
                        }
                    }
                    
                    if (!ignore) {
                        stack_size += get_type_size(s->type);
                    }
                    ignore = false;
                }
                
                if (stack_size > 0) {
                    emit("  sub rsp, %d\n", stack_size);  
                }
            }
            
            // generating the body
            stmt_gen(func, stmt->func.sc);
            
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
            
            
        } break;
        
        case STMT_EXPR: {
            expr_gen(func->func.type, stmt->expr); 
            scratch_free(stmt->expr->reg);
        } break; 
        
        case STMT_RETURN: {
            
            // function return stmt
            expr_gen(func->func.type, stmt->expr);
            emit("  mov rax, %s\n", scratch_name(stmt->expr->reg));
            emit("  jmp %s_epilogue\n", slice_to_str(func->func.name.str, func->func.name.len));
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

internal void program_gen(Program *prog) {
    if (!prog) return; 
    
    output_file = stdout;
    
    // 
    // Generate the globals inside the .data segment
    // for example the global variables and public function 
    // interfaces will get generated in this phase
    // 
    
    Vector *decls = prog->decls;
    Scope block = prog->block; 
    
    Assert(0 < decls->index);
    
    Vector *symbols = block.locals; 
    
    emit("\n\n"); 
    if (symbols->index > 0)
        emit(".data\n");
    
    char buff[MAX_LEN];
    int buff_cursor = 0; 
    
    
    for (int i = 0; i < symbols->index; i++) {
        Symbol *symb = (Symbol *)symbols->data[i]; 
        
        // generate the appropriate global data with this
        switch (symb->type->kind) {
            
            case TYPE_STRING: {
                emit("%s db \"%s\", 0\n", 
                     slice_to_str(symb->name.str, symb->name.len), 
                     (char *)symb->initializer->literal.data);
            } break; 
            
            // TODO(ziv): add more literal types with different sizes
            case TYPE_BOOL:
            case TYPE_S64: { 
                emit("%s dq %lld\n", 
                     slice_to_str(symb->name.str, symb->name.len),
                     (long long)symb->initializer->literal.data);
            } break; 
            
            case TYPE_FUNCTION: {
                // TODO(ziv): Maybe if I have semantics for declaring functions as private 
                // then I would not need to do this decloration as pulic or whatever
                
                if (is_main) {
                    // write to a buff
                    strcpy(&buff[buff_cursor], "PUBLIC ");
                    buff_cursor += (int)strlen("PUBLIC ");
                    strncpy(&buff[buff_cursor], symb->name.str, symb->name.len);
                    buff_cursor += symb->name.len;
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
    
    
    
    // 
    // Now we generate the code for each of the functions 
    // at the order that we see them. 
    // 
    
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
        
        is_main = strncmp(stmt->func.name.str, entry_point_name,
                          MIN(stmt->func.name.len, strlen(entry_point_name))) == 0;
        
        stmt_gen(stmt, stmt);
        
        // skip non function declorations
        stmt = (Statement *)decls->data[i]; 
        for (; i < decls->index && stmt->kind != STMT_FUNCTION_DECL; i++) {
            stmt = (Statement *)decls->data[i];
        }
        
    }
    
    emit("END\n");
    
}