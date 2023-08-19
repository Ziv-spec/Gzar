
enum {
	IR_LOAD, 
	IR_LOADC, // load const
	
	IR_ADD,
	IR_SUB,
	
	IR_BRH,  // branch
	IR_CBRH, // conditional branch
	
	IR_RET,
} IR_Op;

enum { 
	IR_OPERAND_IMMEDIATE, 
	IR_OPERAND_REGISTER,
} IR_Operand_Kind;

typedef struct { 
	int kind;
	union {
	 u64 imm; 
	int reg;
	};
} IR_Operand;

typedef struct { 
	int op; 
	IR_Operand *a, *b, *c; 
	struct IR_Inst *next;
} IR_Inst; 

typedef struct {
	IR_Inst *instructions; // instructions buffer
	int inst_cnt; // instruction count 
	int inst_cap; // buffer capacity
	
	IR_Operand *operands;
	int operand_cnt;
	int operand_cap;
	
	int reg_name; 
	
} IR_Buffer; 

internal IR_Inst *ir_buffer_push_inst(IR_Buffer *buf, int op, IR_Operand *a, IR_Operand *b, IR_Operand *c) {
	if (buf->inst_cnt >= buf->inst_cap) {
		buf->inst_cap= (buf->inst_cap+ 1) * 2;
		buf->instructions = realloc(buf, buf->inst_cap*sizeof(IR_Inst));
		Assert(buf->instructions && "Failed to allocate memory");
	}
	
	buf->instructions[buf->inst_cnt] = (IR_Inst){ op, a, b, c };
	return &buf->instructions[buf->inst_cnt++];
}

#define ir_node3(buf, op, a, b, c) ir_buffer_push_inst(buf, op, a, b, c)
#define ir_node2(buf, op, a, b)    ir_buffer_push_inst(buf, op, a, b, 0)
#define ir_node1(buf, op, a)       ir_buffer_push_inst(buf, op  a, 0, 0)
#define ir_node0(buf, op)          ir_buffer_push_inst(buf, op, 0, 0, 0)

internal IR_Operand *ir_operand(IR_Buffer *buf, int kind, unsigned long long val) {
	if (buf->operand_cnt >= buf->operand_cap) {
		buf->operand_cap= (buf->operand_cap+ 1) * 2;
		buf->operands = realloc(buf, buf->operand_cap*sizeof(IR_Inst));
		Assert(buf->operands && "Failed to allocate memory");
	}
	
	buf->operands[buf->operand_cnt] = (IR_Operand){ kind, val };
	return &buf->operands[buf->operand_cnt++];
}

#define ir_immediate(buf, v) ir_operand(buf, IR_OPERAND_IMMEDIATE, v) 
#define ir_register(buf)     ir_operand(buf, IR_OPERAND_REGISTER, buf->reg_name++) 


internal IR_Inst *convert_expr_to_ir(IR_Buffer *ir_buff, Expr *expr, IR_Inst *last) { 
	if (!expr) return 0; 
	
	switch(expr->kind) {
		case EXPR_BINARY: { 
			IR_Inst *lhs = convert_expr_to_ir(ir_buff, expr->left, last);
			IR_Inst *rhs = convert_expr_to_ir(ir_buff, expr->right, lhs);
			
			int op = 0;
			switch (expr->operation.kind) {
                case TK_MINUS: op = IR_SUB; break; 
                case TK_PLUS:  op = IR_ADD; break;
				default: Assert(!"Not implemented");
			}
			
			IR_Inst *result = ir_node3(ir_buff, op, ir_register(ir_buff), lhs->a, rhs->a);
			rhs->next = result; // connect result to the graph
			return result; 
		} break;
		
		case EXPR_LITERAL: {
			// connect nodde to graph 
			last->next = ir_node2(ir_buff, IR_LOADC,
								  ir_register(ir_buff), 
								  ir_immediate(ir_buff, (size_t)expr->literal.data));
			return last->next;
		} break;
		
	}
	
	return 0;
}

internal IR_Inst *convert_ast_to_ir(Translation_Unit *tu, IR_Buffer *ir_buff) {
	
	Vector *decls = tu->decls;
	for (int i = 0; i < decls->index; i++) {
		Statement *stmt = decls->data[i];
		if (stmt->kind != STMT_FUNCTION_DECL || !stmt->func.sc) continue;
		
		Statement *scope = stmt->func.sc;
		Assert(scope->kind == STMT_SCOPE);
		
			IR_Inst *entrypoint = ir_node0(ir_buff, 0); // dumy node
		IR_Inst *temp = entrypoint;
		
		Vector *statements = scope->block.statements;
		for (int stmt_idx = 0; i < statements->index; stmt_idx++) {
			Statement *stmti = statements->data[stmt_idx]; 
			switch (stmti->kind) {
				case STMT_RETURN: {
					IR_Inst *expression = convert_expr_to_ir(ir_buff, stmt->ret, temp);
					temp = ir_node0(ir_buff, IR_RET); 
					expression->next = temp;
				} break;
				
				default: Assert(!"Not implemented");
			}
		}
		
	}
	
	return NULL;
}

#if 0 
internal void print_ir(IR_Buffer *ir_buff, Vector *ir_functions) {
	
	for (int i = 0; i < ir_functions->index; i++) {
		printf("func%d\n", i); 
		IR_Slice *slice = ir_functions->data[i]; 
		
		IR *arr = &ir_buff->instructions[slice->begin];
		int count = slice->end - slice->begin; 
		
		for (int j = 0; j < count; j++) {
			IR ir = arr[j];
			
			switch (ir.op) {
				
				case IR_LOAD: {
				printf("LOAD r%d %d\n", *ir.a, *ir.b);
				} break;
				
				case IR_ADD: 
				case IR_SUB: {
					printf("%s r%d r%d r%d\n", ir.op == IRK_ADD ? "ADD" : "SUB", *ir.a, *ir.b, *ir.c);
				} break; 
				
				case IR_RET: { 
					printf("RET\n"); 
				} break; 
			}
			
			
		}
		
	}
	
}
#endif