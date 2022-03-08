
// quick way of checking whether the type is atomic
// like interger | string | void | ... 
#define is_atomic(t) ((t->kind) < TYPE_ATOMIC_STUB)

#define is_pointer(t) (t->kind == TYPE_POINTER)
#define is_boolean(t) (t->kind == TYPE_BOOL)
#define is_integer(t) ((t->kind) < TYPE_UNSIGNED_INTEGER_STUB)
#define is_signed_integer(t) ((t->kind) < TYPE_INTEGER_STUB)
#define is_unsigned_integer(t) (TYPE_INTEGER_STUB < (t->kind) && (t->kind) < TYPE_UNSIGNED_INTEGER_STUB)

internal Type *get_atom(Type_Kind kind) {
    Assert(kind < TYPE_ATOMIC_STUB);
    
    // This is a hack for getting the log2 of a number
    // which results in the correct index for the atom
    // inside the `types_tbl`
    
#if 1 // this is only useful if I want the types to be 2^n for creating a bitfield
    f32 something = kind; 
    s32 index = (*(s32 *)&something >> 23) - 127;
#else
    s32 index = kind; 
#endif 
    return &types_tbl[index]; 
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

/* 
internal bool type_compatible(Type *dest, Type *src) {
    
    if (type_equal(dest, src)) {
        return true; 
    }
    
    if (dest->kind != src->kind) {
        if ((is_unsigned_integer(dest) && is_unsigned_integer(src)) || (is_integer(dest) && is_integer(src))) {
            return true;
        }
    }

}
 */

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
                error(expr->assign.lvar->lvar.name, "Unexpected incompatible types '%%s' != '%%s' on assignment");
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
                        error(expr->operation, "Unexpected incompatible types '%%s' != '%%s'");
                    return NULL;
                }
                result = rhs; 
            }
            
            
            // Make sure that only valid types will continue 
            if (!is_integer(result) &&
                !is_boolean(result) &&
                !is_pointer(result)) {
                error(expr->operation, "Incompatible type for operation"); 
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
                            error(expr->operation, "Unexpected '+' operation on two pointer types");
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
                    error(expr->unary.operation, "Incompatible type for '-' operation");
                } break;
                
                case TK_STAR: {
                    if (rhs->kind == TYPE_POINTER) {
                        return rhs; 
                    }
                    error(expr->unary.operation, "Incompatible type, must be pointer for dereference operation");
                    
                } break; 
                
                case TK_BANG:
                {
                    if (rhs->kind == TYPE_BOOL) {
                        return rhs; 
                    }
                    error(expr->unary.operation, "Incompatible type, must be boolean for operation");
                } break; 
                
            } break; 
            
            case EXPR_GROUPING: {
                return sema_expr(expr->grouping.expr); 
            } break;
            
            case EXPR_CALL: {
                Assert(!"This is not implemented yet");
            } break; 
            
            default: {
                Assert(!"I should never be getting here"); 
            }
            
            
            
        }
        
    }
    
    return NULL;
}


internal bool sema_block(Statement *block);

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
                    error(symb->name, "Illigal use of type `void`");
                    return false;
                }
            }
            
            Statement *block = stmt->func.sc;
            return sema_block(block); 
            
        } break; 
        
        case STMT_VAR_DECL: {
            Statement *var_decl = stmt;
            Assert(var_decl->kind == STMT_VAR_DECL);
            
            Type *t1 = var_decl->var_decl.type; 
            Type *t2 = sema_expr(var_decl->var_decl.initializer); 
            
            if (!t1) {
                if (t2) {
                    // type is infered
                    var_decl->var_decl.type = t1;
                    return true;
                }
                return false;
            }
            
            if (t1->kind == TYPE_VOID) {
                error(var_decl->var_decl.name, "Illigal use of type `void`");
                return false;
            }
            
            if (t2) {
                if (type_equal(t1, t2)) {
                    var_decl->var_decl.type = t1;
                    return true;
                }
                
                error(var_decl->var_decl.name, "Types are not equal something");
                return false;
            }
            return true;
            
        } break;
        
        case STMT_SCOPE: {
            return sema_block(stmt);
        }
        
        default: {
            Assert(!"We should not be getting here");
        }; 
        
    }
    
    return true;
}

internal bool sema_block(Statement *block) {
    Assert(block->kind == STMT_SCOPE); 
    
    push_scope(block); 
    
    bool temp, success = true;
    Vector *statements = block->block.statements;
    for (int i = 0; i < statements->index; i++) { 
        Statement *stmt = (Statement *)block->block.statements->data[i]; 
        temp = sema_statement(stmt);
        success = (!success) ? success : temp;
    }
    
    pop_scope(block); 
    return success; 
}

internal void sema_file(Program *prog) {
    
    
    for (int i = 0; i < prog->decls->index; i++) {
        Statement *stmt = (Statement *)prog->decls->data[i]; 
        sema_statement(stmt);
        
        // not sure what to do with this but okay
    }
}

