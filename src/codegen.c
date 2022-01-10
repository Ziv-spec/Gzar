
/* 
Currently the way I will compile my code to machine code is going to be 
using a "Stack Machine". This will produce horrable code but it is 
 extremely simple to program and make sure works correctly. 
*/



internal void gen() {
    
    // 
    // Generating the program
    //
    
    printf("segment .text\n");
    printf("global _start\n");
    printf("_start:\n");
    output("push ebp");
    output("mov ebp, esp");
    output("sub esp, 200");
    
    Statement *stmt = NULL;
    for (int i = 0; i < statements_index; i++) {
        stmt = statements[i];
        if (stmt->kind == STMT_EXPR) {
            gen_body(stmt->expr);
            output("pop eax ; discart result\n");
        }
        else if (stmt->kind == STMT_RETURN) {
            gen_body(stmt->expr); 
            output(
                   "pop eax",
                   "mov esp, ebp",
                   "pop ebp",
                   "ret"
                   ); 
        }
    }
    
    // this is reduendent
    output("", "; THIS IS NOT NEEDED IF YOU HAVE A RETURN STATEMENT");
    output(
           "mov esp, ebp",
           "pop ebp",
           "ret"
           );
    
}

internal void gen_lval(Expr *expr) {
    if (expr->kind != EXPR_LVAR) Assert(!"inccorect expression given to `gen_lval` function");
    
    char buff[15];
    sprintf(buff, "sub eax, %d", expr->left_variable.offset);
    
    output(
           "mov eax, ebp",
           buff, 
           "push eax"
           ); 
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
        
        case EXPR_LVAR: {
            gen_lval(expr);
            output(
                   "pop eax", 
                   "mov eax, [eax]", 
                   "push eax"
                   );
        } break;
        
        case EXPR_ASSIGN: {
            gen_lval(expr->assign.left_variable);
            gen_body(expr->assign.rvalue);
            
            output(
                   "pop edi",
                   "pop eax",
                   "mov [eax], edi",
                   "push edi"
                   );
        } break; 
        
        default: {
            Assert(!"Error: Not implemented");
        } break;
    }
    
    
}

internal void gen_op(Token operation) {
    switch (operation.kind) {
        
        case TK_PLUS:  output("; PLUS",   "add eax, edi");  break;
        case TK_MINUS: output("; MINUS",  "sub eax, edi");  break;
        case TK_STAR:  output("; MUL",    "imul eax, edi"); break;
        case TK_SLASH: output(
                              "; DIVIDE", 
                              "xor edx, edx", // TODO(ziv): maybe different instruction? 
                              "idiv edi"
                              ); break;
        case TK_BANG:  output(
                              "; NOT", 
                              "test eax, eax",
                              "sete al", 
                              "movzx eax, al"
                              ); break;
        case TK_GREATER:       output(
                                      "; GREATER", 
                                      "cmp eax, edi", 
                                      "setg al",
                                      "movzx eax, al"
                                      ); break;
        case TK_LESS:          output(
                                      "; LESS", 
                                      "cmp eax, edi", 
                                      "setl al", 
                                      "movzx eax, al"
                                      ); break;
        case TK_GREATER_EQUAL: output(
                                      "; GREATER EQUAL",
                                      "cmp eax, edi", 
                                      "setge al",
                                      "movzx eax, al"
                                      ); break;
        case TK_LESS_EQUAL:    output(
                                      "; LESS EQUAL",
                                      "cmp eax, edi", 
                                      "setle al", 
                                      "movzx eax, al"
                                      ); break;
        case TK_BANG_EQUAL:    output(
                                      "; NOT EQUAL",
                                      "cmp eax, edi", 
                                      "setne al", 
                                      "movzx eax, al"
                                      ); break;
        case TK_EQUAL_EQUAL:   output(
                                      "; EQUAL",
                                      "cmp eax, edi", 
                                      "sete al",
                                      "movzx eax, al"
                                      ); break;
        
        default: {
            fprintf(stderr, "Error: unsupported operation\n");
            Assert( "Error: unsupported operation");
            
        }
        
    }
    
    output("push eax");
    printf("\n");
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
            Assert( "Error: unsupported operation");
            exit(1); 
        }
    }
}

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
