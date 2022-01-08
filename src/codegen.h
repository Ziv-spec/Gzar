/* date = January 8th 2022 0:46 pm */

#ifndef CODEGEN_H
#define CODEGEN_H

#define COUNT_ARGS(...) INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
#define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#define INTERNAL_EXPAND(a) a
#define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, count, ...) count


#define output(...) internal_output(COUNT_ARGS(__VA_ARGS__), ##__VA_ARGS__)

internal void internal_output(int num, ...) {
    va_list oplist; 
    
    va_start(oplist, num);
    
    char *asmline = NULL;
    for (int i = 0; i < num; i++) {
        asmline = va_arg(oplist, char*); 
        printf("\t"); printf(asmline); printf("\n");
    }
    
    va_end(oplist);
}


internal void gen_op(Token operation); 
internal void gen(Expr *expr);
internal void gen_literal(Expr *expr);
internal void gen_body(Expr *expr);

//////////////////////////////////

#endif //CODEGEN_H
