
// quick way of checking whether the type is atomic
// like interger | string | void | ... 
#define is_atomic(t) ((t->kind) < TYPE_ATOMIC_STUB)

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
    Assert(t1 && t2);
    
    if (t1->kind != t2->kind) {
        return false; 
    }
    
    while (t1 && t2) {
        
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
    
    Assert(t1 || t2); // NOTE(ziv): This shouldn't really happen
    // but if for some reason it does then uncomment the code
    //  below. it will actually check for that instead of just asserting
    /*
    if (t1 || t2) {
        return false; 
    } 
    */
    
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
        
        // TODO(ziv): add more rules for implicit casting
        
    }
    
    if (dest->kind == TYPE_POINTER) {
        if (dest->subtype->kind == TYPE_VOID) {
            // I should be able to cast any pointer into a void*
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
            // the only problem is the NULL type in which case I should get a unknown 
            // type of some sort. this type can cast to any other type but should remain
            // anonymous until it is casted to some type explicitly or implicitly
            return get_atom(expr->literal.kind);
        } break;
        
        case EXPR_LVAR: {
            
            Type *type = local_exist(expr->lvar.name); 
            type = type ? type : symbol_exist(global_block, expr->lvar.name);
            
            Assert(type); // I should be getting a type here 
            // if I don't that means I probably have a bug somewhere 
            
            return type; 
            
        } break;
        
        case EXPR_ASSIGN: { // should I put this into the binary thingy? 
            
            Type *rhs = sema_expr(expr->assign.lvar);
            Type *lhs = sema_expr(expr->assign.rvalue);
            
            if (!rhs || !lhs) {
                return NULL;
            }
            
            if (!type_equal(rhs, lhs)) {
                error(expr->lvar.name, "Unexpected incompatible types '%s' != '%s' on assignment");
                return NULL; 
            }
            
        } break; 
        
        case EXPR_BINARY: {
            Type *rhs = sema_expr(expr->right);
            Type *lhs = sema_expr(expr->left);
            
            if (!rhs || !lhs) {
                return NULL;
            }
            
            /*             
                        if (!type_compatible(rhs, lhs)) {
                            // TODO(ziv): errors for failing the implicit type casting 
                        }
                         */
            
            // implicit upcast of integer type 
            if ((is_signed_integer(rhs) && is_signed_integer(lhs)) ||
                (is_unsigned_integer(rhs) && is_unsigned_integer(lhs))) {
                result = (rhs->kind > lhs->kind) ? rhs : lhs;
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
                    if (!type_equal(rhs, lhs)) {
                        error(expr->operation, "Unexpected incompatible types '%s' != '%s'");
                        return NULL;
                    }
                    
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
            
            Token_Kind op = expr->operation.kind; 
            Assert(TK_OP_BEGIN < op && op < TK_OP_END); 
            
            switch (op) {
                
                case TK_MINUS: {
                    if (is_integer(rhs) || is_unsigned_integer(rhs)) {
                        return rhs; 
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
            
            default: {
                Assert(false); // I should never be getting here
            }
            
        }
    }
    
    return NULL;
}


internal bool sema_block(Statement *block);

internal bool sema_var_decl(Statement *var_decl) {
    Assert(var_decl->kind == STMT_VAR_DECL);
    
    Type *t1 = var_decl->var_decl.type; 
    Type *t2 = sema_expr(var_decl->var_decl.initializer); 
    if (type_equal(t1, t2)) {
        var_decl->var_decl.type = t1;
        return true;
    }
    
    Assert(false);
    return false;
}

internal bool sema_statement(Statement *stmt) {
    
    switch (stmt->kind) {
        
        case STMT_EXPR: {
            Type *ty = sema_expr(stmt->expr);
            if (!ty) {
                fprintf(stderr, "bad boy!!! not types matched bullshit");
                return false;
            }
            
        } break; 
        
        case STMT_FUNCTION_DECL: {
            
            Vector *args = stmt->func.type->symbols; 
            for (int i = 0; i < args->index; i++) {
                Statement *var_decl = (Statement *)args->data[i]; 
                
                sema_statement(var_decl); 
                
                
            }
            
            Statement *block = stmt->func.sc;
            return sema_block(block); 
            
        } break; 
        
        
        case STMT_VAR_DECL: {
            Expr *expr = stmt->var_decl.initializer; 
            Type *ty = sema_expr(expr);
            if (!(ty && stmt->var_decl.type) || !type_equal(ty, stmt->var_decl.type)) {
                return false; 
            }
            
        } break;
        
        default: {
            
            Assert(false); // We should not be getting here
        }; 
        
    }
    
    return true;
}

internal bool sema_block(Statement *block) {
    Assert(block->kind == STMT_SCOPE); 
    
    push_scope(block); 
    
    bool success = true;
    Vector *statements = block->block.statements;
    for (int i = 0; i < statements->index; i++) { 
        Statement *stmt = (Statement *)block->block.statements->data[i]; 
        success = (!success) ? success : sema_statement(stmt);
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


/*

a : int = 234; // global variables 

swap :: proc (a: int, b: int) -> void {
    temp: int; 
    
    temp = a; 
    a = b; 
    b = temp;
    
}

main :: proc (s : u8) -> s32 {
    
    8* 8;   // normal expression  
	a1: u32; // decloration 
	b1: s8 = 2==2;  // decloration with initializer
	a1 = 3;         // assignment expression 
    
    b: int = 234 * 3;
    
    
    c : u32; 
    // recursive scopes
    { 
        b: u32 = 10; // redefinition of a variable
        b = b + 10 * 2;
        c = a1 + b; // using variables from a outer scope
    }
    
// function calls
main(a, b, temp());

    return a1+b1;
}

*/
