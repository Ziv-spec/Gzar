
// quick way of checking whether the type is atomic
// like interger | string | void | ... 
#define is_atomic(t) ((t->kind) < TYPE_ATOMIC_STUB)

#define is_pointer(t) (t->kind == TYPE_POINTER)
#define is_boolean(t) (t->kind == TYPE_BOOL)
#define is_integer(t) ((t->kind) < TYPE_UNSIGNED_INTEGER_STUB)
#define is_signed_integer(t) ((t->kind) < TYPE_INTEGER_STUB)
#define is_unsigned_integer(t) (TYPE_INTEGER_STUB < (t->kind) && (t->kind) < TYPE_UNSIGNED_INTEGER_STUB)


inline int type_kind_to_atom(Type_Kind kind) {
    
    // This is a hack for getting the log2 of a number
    // which results in the correct index for the atom
    // inside the `types_tbl`
    
#if 1 // this is only useful if I want the types to be 2^n for creating a bitfield
    f32 something = kind; 
    s32 index = (*(s32 *)&something >> 23) - 127;
#else
    s32 index = kind; 
#endif 
    return index; 
}

internal Type *get_atom(Type_Kind kind) {
    Assert(kind < TYPE_ATOMIC_STUB);
    s32 index = type_kind_to_atom(kind);
    return &types_tbl[index]; 
}

internal int get_argument_count(Expr *arguments) {
    int count = 0;
    
    for (; arguments && arguments->kind == EXPR_ARGUMENTS; count++ ,arguments = arguments->args.next);
    
    return count;
}

internal char *type_to_string(Type *ty) {
    
    if (!ty) return "";
    
    static char *type_to_string_tbl[] = {
        [ATOM_S8]  = "s8",
        [ATOM_S16] = "s16",
        [ATOM_S32] = "s32",
        [ATOM_S64] = "s64",
        
        [ATOM_U8]  = "u8",
        [ATOM_U16] = "u16",
        [ATOM_U32] = "u32",
        [ATOM_U64] = "u64",
        
        [ATOM_VOID]   = "void",
        [ATOM_STRING] = "string",
        [ATOM_BOOL]   = "bool",
        
        [ATOM_UNKNOWN]  = "unknown",
        
        [ATOM_POINTER]  = "*",
        [ATOM_ARRAY]    = "[]",
    };
    
    char buff[MAX_LEN] = {0}; 
    
    while (ty) {
        strcat(buff, type_to_string_tbl[type_kind_to_atom(ty->kind)]); 
        ty = ty->subtype;
    }
    
    return _strdup(buff);
}

internal char *format_types(char *msg, Type *t1, Type *t2) {
    char buff[MAX_LEN]; 
    char *t1_str = type_to_string(t1);
    char *t2_str = type_to_string(t2);
    if (t1 && t2) {
        sprintf(buff, msg, t1_str, t2_str); 
    }
    else if (t1 && !t2) {
        sprintf(buff, msg, t1_str); 
    }
    else {
        return msg;
    }
    return _strdup(buff);
}

internal void type_error(Token line, Type *t1, Type *t2, char *msg) {
    char buff[MAX_LEN] = {0}; 
    char err[MAX_LEN] = {0}; 
    
    char *result = format_types(msg, t1, t2);
    strcat(err, result); 
    strcat(err, " at "); 
    
    strcat(err, _itoa(line.loc.line, buff, 10)); 
    strcat(err, "\n");
    fprintf(stderr, err);
}

internal bool type_equal(Type *t1, Type *t2) {
    if (t1 == t2) {
        return true; 
    }
    
    if (t1->kind != t2->kind) {
        return false; 
    }
    
    while (t1 && t2 && t1->kind == t2->kind) {
        
        // test whether the kind is of integer kind (s8/16/.. u8/16/..)
        if (is_atomic(t1)) {
            Assert((t1->subtype || t2->subtype) == false);
            return true;
        }
        else if (t1->kind == TYPE_FUNCTION) {
            if (t1->symbols->index == t1->symbols->index) {
                for (int i = 0; i < t1->symbols->index; i++) {
                    
                    Symbol *s1 = (Symbol *)t1->symbols->data[i]; 
                    Symbol *s2 = (Symbol *)t2->symbols->data[i]; 
                    
                    if (!type_equal(s1->type, s2->type)) {
                        return false; 
                    }
                }
            }
        }
        else if (t1->kind == TYPE_ARRAY) {
            // TODO(ziv): check whether I want to have array types which 
            // can encode the following: [2]int aka a array of two integers
            // or I can leave the size unknown so you would only be able to 
            // do '[]int' and so on... without the array size
            //
            // If I end up wanting to compare the sizes this is where that 
            // should be done.
        }
        
        t1 = t1->subtype; 
        t2 = t2->subtype; 
    }
    
    if (t1 || t2) {
        return false; 
    } 
    
    return true; 
}

internal int get_type_size(Type *type) {
    int size = 0; 
    
    if (is_integer(type)) {
        
        switch (type->kind) {
            case TYPE_S8: case TYPE_U8:   { size = 1; }
            case TYPE_S16: case TYPE_U16: { size = 2; }
            case TYPE_S32: case TYPE_U32: { size = 4; }
            case TYPE_S64: case TYPE_U64: { size = 8; }
        }
        
    }
    else if (type->kind == TYPE_POINTER || type->kind == TYPE_ARRAY) {
        size = 8;
    }
    else {
        Assert(!"Unable to get the size of this type"); 
    }
    
    return size; 
}

internal Type *get_compatible_type(Type *dest, Type *src) {
    
    if ((is_integer(dest) && is_integer(src)) || 
        (is_unsigned_integer(dest) && is_unsigned_integer(src))) {
        return dest->kind > src->kind ? dest : src;
    }
    
    // You can convert any pointer type to a void* type implicitly *I think*
    if (dest->kind == TYPE_POINTER && src->kind == TYPE_POINTER) {
        if (dest->subtype->kind == TYPE_VOID) {
            return dest; 
        }
    }
    
    return NULL;
}

internal Type *init_type(Type_Kind kind, Type *subtype, Vector *symbols) {
    Type *ty = (Type *)malloc(sizeof(Type)); 
    ty->kind = kind;
    ty->subtype = subtype;
    ty->symbols = symbols;
    return ty;
}

internal Type *implicit_cast(Type *t1, Type *t2) {
    
    // implicit upcast of integer type 
    if ((is_signed_integer(t1) && is_signed_integer(t2)) ||
        (is_unsigned_integer(t1) && is_unsigned_integer(t2))) {
        return (t1->kind > t2->kind) ? t1 : t2;
    }
    
    if ((t1->kind == TYPE_POINTER && t2->kind == TYPE_UNKNOWN) || 
        (t2->kind == TYPE_POINTER && t1->kind == TYPE_UNKNOWN)) {
        return t1->kind == TYPE_POINTER ? t1 : t2;
    }
    
    
    return NULL; 
}

internal Type *sema_expr(Expr *expr) {
    if (!expr) return NULL;
    
    // recursively go and typecheck the expression 
    // and then return the type of the resulting expression
    // do note that in this phase if it failes it just returns
    // a NULL type. This will signal that there has been an 
    // error. This is done so in the future I would be able 
    // to continue compilation even if I have found  such of
    // an error. Though with the current design I do not allow 
    // for said behaviour. 
    
    Type *result = NULL; 
    
    switch (expr->kind) {
        
        
        
        case EXPR_LITERAL: {
            // NOTE(ziv): a null literal gets the ATOM_UNKNOWN type
            // which means that it's type is not yet known and will be 
            // decided in a later stage
            // if there are no hints leading to a implicit type cast 
            // then the type will be a void* type. 
            return expr->type = get_atom(expr->literal.kind);
        } break;
        
        
        
        case EXPR_LVAR: {
            Type *type = local_exist(expr->lvar.name); 
            type = type ? type : symbol_exist(global_block, expr->lvar.name);
            
            Assert(type); // I should be getting a type here 
            // if I don't that means I probably have a bug somewhere 
            
            return expr->type = type; 
        } break;
        
        
        
        case EXPR_ASSIGN: { // should I put this into the binary thingy? 
            Type *rhs = sema_expr(expr->assign.lvar);
            if (!rhs) return NULL; 
            Type *lhs = sema_expr(expr->assign.rvalue);
            if (!lhs) return NULL; 
            
            // If I have nil literal, I want to implicitly cast it to the 
            // type onto which I want to assign the nil to. 
            Expr *lit = expr->assign.rvalue; 
            if (lit->kind == EXPR_LITERAL) {
                if (lit->literal.kind == TYPE_UNKNOWN && lit->literal.data == 0 && rhs->kind == TYPE_POINTER) {
                    return expr->type = rhs;
                }
            }
            
            result = implicit_cast(rhs, lhs); 
            if (result) {
                return expr->type = result;
            }
            
            if (!type_equal(rhs, lhs)) {
                type_error(expr->assign.lvar->lvar.name, lhs, rhs, "Unexpected incompatible types '%s' != '%s' on assignment");
                return NULL; 
            }
            
        } break; 
        
        
        
        case EXPR_BINARY: {
            Type *rhs = sema_expr(expr->right);
            if (!rhs) return NULL; 
            Type *lhs = sema_expr(expr->left);
            if (!lhs) return NULL; 
            
            result = implicit_cast(rhs, lhs);
            if (!result) {
                if (!type_equal(rhs, lhs)) {
                    if (is_integer(rhs) && is_integer(lhs))
                        fprintf(stderr, "Unable to implicitly cast between unsigned and signed integer types");
                    else
                        type_error(expr->operation, lhs, rhs, "Unexpected incompatible types `%s` != `%s`");
                    return NULL;
                }
                result = rhs; 
            }
            
            
            // Make sure that only valid types will continue 
            if (!is_integer(result) &&
                !is_boolean(result) &&
                !is_pointer(result)) {
                type_error(expr->operation, NULL, NULL, "Incompatible type for operation"); 
                return NULL; 
            }
            
            Token_Kind op = expr->operation.kind; 
            Assert(TK_OP_BEGIN < op && op < TK_OP_END); 
            switch (op) {
                
                case TK_PLUS: 
                case TK_MINUS:
                case TK_SLASH:
                case TK_STAR: 
                {
                    if ((op == TK_MINUS || op == TK_PLUS)  && 
                        (rhs->kind == TYPE_POINTER && lhs->kind == TYPE_POINTER)) {
                        
                        if (op == TK_MINUS) {
                            // ptr - ptr = ptrdiff (of type s64)
                            return &types_tbl[ATOM_S64];
                        }
                        else {
                            type_error(expr->operation, NULL, NULL, "Can not add two pointer types");
                        }
                    }
                    
                    return result;
                    
                } break; 
                
                case TK_GREATER: 
                case TK_LESS: 
                case TK_GREATER_EQUAL:
                case TK_BANG_EQUAL:
                case TK_EQUAL_EQUAL: 
                case TK_LESS_EQUAL: 
                {
                    // TYPE_UNKOWN special case (for implicit type casting
                    expr->type = result;
                    return &types_tbl[ATOM_BOOL];
                } break; 
                
                default: {
                    // we should not be getting here 
                    Assert(false);
                    return NULL;
                }; 
                
            }
            
        } break;
        
        
        
        case EXPR_UNARY: {
            
            Type *rhs = sema_expr(expr->unary.right);
            
            Token_Kind op = expr->unary.operation.kind; 
            Assert(TK_OP_BEGIN < op && op < TK_OP_END); 
            
            switch (op) {
                
                case TK_MINUS: {
                    if (is_integer(rhs) || is_unsigned_integer(rhs)) {
                        return expr->type = rhs;
                    }
                    type_error(expr->unary.operation, NULL, NULL, "Incompatible type for '-' operation");
                } break;
                
                case TK_STAR: {
                    if (rhs->kind == TYPE_POINTER) {
                        return rhs; 
                    }
                    type_error(expr->unary.operation, rhs, NULL,  "Incompatible type, must be pointer for dereference operation");
                    
                } break; 
                
                case TK_BANG:
                {
                    if (rhs->kind == TYPE_BOOL) {
                        return rhs; 
                    }
                    type_error(expr->unary.operation, rhs, NULL,  "Incompatible type, must be boolean for operation");
                } break; 
            } 
            
        } break;
        
        
        
        case EXPR_CALL: {
            
            Type *function_type = local_exist(expr->call.name->lvar.name);
            Expr *args = expr->call.args;
            Vector *symbols = function_type->symbols; 
            
            //
            // check whether the argument count matches
            //
            
            int call_args_count = get_argument_count(args);
            int func_args_count = symbols->index; 
            if (call_args_count != func_args_count) {
                char *msg = "Argument count missmatch when calling a function %d != %d";
                char *buff = (char *)malloc(strlen(msg) * sizeof(char)); 
                
                sprintf(buff, msg, func_args_count, call_args_count); 
                type_error(expr->call.name->lvar.name, NULL, NULL, buff);
            }
            
            //
            // check whether type of the function signiture matches 
            // the arugments passed to the call node
            //
            
            for (int i = 0; i < symbols->index && args; i++) { 
                Symbol *symb = (Symbol *)symbols->data[i];
                Type *func_arg = symb->type; 
                
                Type *call_arg = (args->kind == EXPR_ARGUMENTS) ? sema_expr(args->args.lvar) : sema_expr(args); 
                
                if (!type_equal(func_arg, call_arg)) {
                    type_error(expr->call.name->lvar.name, func_arg, call_arg, "Unexpected incompatible types `%s` != `%s`");
                }
            }
            
            return function_type->subtype; // return type of the function signiture
            
        } break; 
        
        
        
        case EXPR_GROUPING: {
            return sema_expr(expr->grouping.expr); 
        } break;
        
        
        
        default: {
            Assert(!"I should never be getting here"); 
        }
        
    }
    
    return NULL;
}


internal bool sema_statement(Statement *stmt) {
    
    switch (stmt->kind) {
        
        
        case STMT_RETURN:
        case STMT_EXPR: {
            // NOTE(ziv): This is not an error the `expr` is just another name for the ret
            Type *ty = sema_expr(stmt->expr);
            return ty ? true : false; 
        } break; 
        
        
        
        case STMT_FUNCTION_DECL: {
            
            Vector *args = stmt->func.type->symbols; 
            for (int i = 0; i < args->index; i++) {
                Symbol *symb = (Symbol *)args->data[i]; 
                Assert(symb->initializer == NULL); // I do not support default values
                if (symb->type->kind == TYPE_VOID) {
                    type_error(symb->name, NULL, NULL, "Illigal use of type `void`");
                    return false;
                }
            }
            
            Statement *block = stmt->func.sc;
            return sema_statement(block); 
            
        } break; 
        
        
        
        case STMT_VAR_DECL: {
            Type *lhs = stmt->var_decl.type; 
            Type *rhs = sema_expr(stmt->var_decl.initializer); 
            
            if (!lhs) {
                if (rhs) {
                    // type is infered
                    stmt->var_decl.type = lhs;
                    return true;
                }
                return false;
            }
            
            if (lhs->kind == TYPE_VOID) {
                type_error(stmt->var_decl.name, NULL, NULL, "Illigal use of type `void`");
                return false;
            }
            
            if (rhs) {
                if (type_equal(lhs, rhs) || (rhs->kind == TYPE_UNKNOWN && lhs->kind == TYPE_POINTER)) {
                    stmt->var_decl.initializer->type = lhs; 
                    stmt->var_decl.type = lhs;
                    return true;
                }
                
                type_error(stmt->var_decl.name, lhs, rhs, "Types do not match `%s` != `%s`");
                return false;
            }
            return true;
            
        } break;
        
        
        
        case STMT_SCOPE: {
            push_scope(stmt); 
            
            bool temp, success = true;
            Vector *statements = stmt->block.statements;
            for (int i = 0; i < statements->index; i++) { 
                Statement *block_stmt = (Statement *)stmt->block.statements->data[i]; 
                temp = sema_statement(block_stmt);
                success = (!success) ? success : temp;
            }
            
            pop_scope(stmt); 
            return success;
        }
        
        
        
        default: {
            Assert(!"We should not be getting here");
        }; 
        
    }
    
    return true;
}

internal void sema_file(Program *prog) {
    
    for (int i = 0; i < prog->decls->index; i++) {
        Statement *stmt = (Statement *)prog->decls->data[i]; 
        sema_statement(stmt);
    }
    
}
