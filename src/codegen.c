
/* 
Currently the way I will compile my code to machine code is going to be 
using a "Stack Machine". This will produce horrable code but it is 
 extremely simple to program and make sure works correctly. 
*/

#include <stdarg.h> 

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

internal void gen_op(Token operation) {
    switch (operation.kind) {
        
        case TK_PLUS:  output("; PLUS",   "add eax, edi");  break;
        case TK_MINUS: output("; MINUS",  "sub eax, edi");  break;
        case TK_STAR:  output("; MUL",    "imul eax, edi"); break;
        case TK_SLASH: output("; DIVIDE", 
                              "xor edx, edx", 
                              "idiv edi"); break;
        
        case TK_BANG:  output("; NOT", 
                              "test eax, eax",
                              "sete al", 
                              "movzx eax, al"); break;
        /*
        case TK_ASSIGN:  printf("="); break;
        */
        
        case TK_GREATER:       output("; GREATER", 
                                      "cmp eax, edi", 
                                      "setg al",
                                      "movzx eax, al"); break;
        case TK_LESS:          output("; LESS", 
                                      "cmp eax, edi", 
                                      "setl al", 
                                      "movzx eax, al");break;
        case TK_GREATER_EQUAL: output("; GREATER EQUAL",
                                      "cmp eax, edi", 
                                      "setge al",
                                      "movzx eax, al"); break;
        case TK_LESS_EQUAL:    output("; LESS EQUAL",
                                      "cmp eax, edi", 
                                      "setle al", 
                                      "movzx eax, al"); break;
        case TK_BANG_EQUAL:    output("; NOT EQUAL",
                                      "cmp eax, edi", 
                                      "setne al", 
                                      "movzx eax, al"); break;
        case TK_EQUAL_EQUAL:   output("; EQUAL",
                                      "cmp eax, edi", 
                                      "sete al",
                                      "movzx eax, al"); break;
    }
}

internal void gen_literal(Expr *expr) {
    Assert(expr->kind == EXPR_LITERAL);
    
    switch (expr->literal.kind) {
        
        //case VALUE_FALSE: printf("false"); break;
        //case VALUE_TRUE:  printf("true"); break;
        //case VALUE_NIL:   printf("nil"); break;
        
        case VALUE_INTEGER: {
            u64 num = (u64)expr->literal.data; 
            printf("\tpush %I64d\n", num);
        } break;
        
        //case VALUE_STRING: {
        //printf((char *)expr->literal.data);
        //} break;
        
        default: {
            fprintf(stderr, "unsupported literal given\n");
            exit(1); 
        }
    }
}


internal void gen_body(Expr *expr) {
    
    switch(expr->kind) {
        case EXPR_BINARY: {
            gen_body(expr->left); 
            gen_body(expr->right);
            printf("\n");
            output("pop edi");
            output("pop eax");
            gen_op(expr->operation);
            output("push eax");
            printf("\n");
        } break;
        
        case EXPR_UNARY: {
            gen_body(expr->unary.right);
            output("pop eax");
            gen_op(expr->unary.operation);
            output("push eax");
        } break;
        
        case EXPR_GROUPING: {
            gen_body(expr->grouping.expr);
        } break;
        
        case EXPR_LITERAL: {
            gen_literal(expr);
        } break;
        
    }
}

internal void gen(Expr *expr) {
    
    printf("segment .text\n");
    printf("global _start\n");
    printf("_start:\n");
    gen_body(expr);
    printf("\tpop eax\n");
    printf("\tret\n");
    
}
