




/* 
internal bool type_equal(Type *t1, Type t2) {
    
}

internal bool type_op_check(Type *type, Token operation) {
    
    if (type->kind) {
        
    }
    
    return true;
}
 */

internal Type *init_type(Token type_token) {
    
    // just a fast map from the token type to the actual type (which I will of course will expand upon I think) because of future considerations which might turn out to be useless but this is a pretty fast conversion so I hope that it does not matter :).
    
    static Type_Kind map_token_to_type[] =  {
        
        [TK_S8_TYPE]  = TYPE_S8,
        [TK_S16_TYPE] = TYPE_S16,
        [TK_S32_TYPE] = TYPE_S32,
        
        [TK_S8_TYPE]  = TYPE_U8,
        [TK_U16_TYPE] = TYPE_U16,
        [TK_U32_TYPE] = TYPE_U32,
        
        [TYPE_VOID]      = TK_VOID_TYPE, 
        [TK_INT_TYPE]    = TYPE_INTEGER, 
        [TK_STRING_TYPE] = TYPE_STRING, 
    };
    
    Type *type = (Type *)malloc(sizeof(Type)); 
    type->kind = map_token_to_type[type_token.kind]; 
    return type;
}
