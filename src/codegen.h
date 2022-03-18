/* date = January 8th 2022 0:46 pm */

#ifndef CODEGEN_H
#define CODEGEN_H

// This is a bit of a macro hack to get the compiler to count
// the arguments passed to a function. This is so I wouldn't need 
// to do this manually. NOTE(ziv): This works only on the msvc compiler 
#define COUNT_ARGS(...) INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
#define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#define INTERNAL_EXPAND(a) a
#define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, count, ...) count

internal void internal_output(int num, ...);
#define output(...) internal_output(COUNT_ARGS(__VA_ARGS__), ##__VA_ARGS__)

/* 
internal void gen();
internal void gen_expr(Expr *expr);
internal void gen_op(Token operation); 
internal void gen_literal(Expr *expr);
 */

typedef long long Register; 


//////////////////////////////////

#endif //CODEGEN_H
