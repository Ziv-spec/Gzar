#ifndef PARSER_H
#define PARSER_H

typedef struct Parser Parser; 

struct Parser {
    Lexer *lexer; 
    
    unsigned int token_index; 
    Token tokens[1000]; // this will probably grow 
    
}; 

#define is_token_op(tk) (TK_OP_BEGIN < tk.kind && tk.kind < TK_OP_END)
#define is_token_operand(tk) (TK_LITERAL_BEGIN < tk.kind && tk.kind < TK_LITERAL_END)

#define is_kind_op(kind) (TK_OP_BEGIN < kind && kind < TK_OP_END)
#define is_kind_operand(kind) (TK_LITERAL_BEGIN < kind && kind < TK_LITERAL_END)
#define is_kind_op_or_operand(kind) (is_kind_op(kind) || is_kind_operand(kind))

#define NULL_TOKEN (Token){ 0 }; 

#if 0
void parse_file(Parser *parser) {
    Lexer *lexer = parser->lexer; 
    
    Token op;
    Token operand1; 
    Token operand2; 
    
    
    // right now, I will be accepting 
    // the following format (until I change it) 
    
    ////////////////////////////////
    // op operand operand
    ////////////////////////////////
    
    do {
        
        if (!get_next_token(lexer))  goto err; 
        if (is_kind_op(lexer->token.kind)) {
            
            if (lexer->token.kind == TK_PLUS) {
                
            }
        }
        
        if (!get_next_token(lexer))  goto err; 
        operand1 = lexer->token;
        
        if (!get_next_token(lexer))  goto err; 
        operand2 = lexer->token;
        
        
        
    } while (lexer->token.kind != TK_EOF);
    
    err: 
    // TODO(ziv): Handle errors incoming from the parser
    
}
#endif 


// TODO(ziv): Think more about the design in here because for the time 
// being the way that I am doing is kind of bad (I'm converting the reuslt 
// back to begin a token instead of ... not doing that).
Token parse_plus(Parser *parser) {
    
    Token operand1 = parser->tokens[++parser->token_index];
    char buff[MAX_LEN] = {0};
    int a, b; 
    if (is_kind_operand(operand1.kind) && operand1.kind == TK_INTEGER) {
        a = atoi(slicecpy(buff, MAX_LEN, operand1.s, operand1.len)); 
    }
    else {
        fprintf(stderr, "Error: must recieve integer type\n"); 
        return NULL_TOKEN;
    }
    
    Token operand2 = parser->tokens[++parser->token_index];
    if (is_kind_operand(operand2.kind) && operand2.kind == TK_INTEGER) {
        b= atoi(slicecpy(buff, MAX_LEN, operand2.s, operand2.len)); 
    }
    else {
        fprintf(stderr, "Error: must recieve integer type\n"); 
        return NULL_TOKEN;
    }
    
    return (Token){ parser->lexer->loc, TK_INTEGER, 1, itoa(a+b, buff, 10) }; 
}

void sim_file(Parser *parser) {
    Lexer *lexer = parser->lexer; 
    
    // 
    // fill the stack with tokens
    // 
    
    do {
        if (!get_next_token(lexer) && lexer->token.kind != TK_EOF)  goto err; 
        parser->tokens[parser->token_index++] = lexer->token;;
    } while (lexer->token.kind != TK_EOF);
    
    // 
    // sim on the stack 
    // 
    
    parser->token_index = 0;
    Token token = parser->tokens[parser->token_index];
    while (token.kind != TK_EOF) {
        
        if (!is_kind_operand(token.kind)) {
            fprintf(stderr, "Error: must begin with a var\n");
            goto err; 
        }
        
        switch (token.kind) {
            
            case TK_PLUS: {
                parser->tokens[parser->token_index] = parse_plus(parser); 
                parser->token_index--;
            } break;
            
            case TK_MINUS: {
                //parse_minus(parser); 
            } break;
            
            case TK_DUMP: {
                // parse_dump(parser); 
            } break;
        }
        
        token = parser->tokens[++parser->token_index];
    }
    
    err:
    
    return;
}


#endif //PARSER_H
