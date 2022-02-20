


internal bool type_equal(Type *t1, Type *t2) {
    
    if (t1->kind != t2->kind) {
        return false; 
    }
    
    if (t1->subtype && t2->subtype) {
        return type_equal(t1->subtype, t2->subtype); 
    }
    else if (t1->kind == TYPE_FUNCTION && t2->kind == TYPE_FUNCTION) {
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
    
    return true;
}

/*
internal bool type_op_check(Type *type, Token operation) {
    
    if (type->kind) {
        
    }
    
    return true;
}
 */

internal Type *init_type(Atom_Kind kind, Type *subtype, Vector *symbols, Type *type) {
    Type *ty = (Type *)malloc(sizeof(Type)); 
    ty->kind = kind;
    ty->subtype = subtype;
    ty->symbols = symbols;
    ty->type = type;
    return ty;
}

