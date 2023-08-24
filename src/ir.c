
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

typedef struct IR_Inst IR_Inst; 
struct IR_Inst { 
	int op; 
	IR_Operand *a, *b, *c; 
	IR_Inst *next;
}; 

typedef struct {
	M_Pool *m;
	IR_Inst *instructions;
	IR_Operand *operands;
	
	int reg_name; // keeps the name of the last gpr
} IR_Buffer; 

internal IR_Inst *ir_buffer_push_inst(IR_Buffer *buf, int op, IR_Operand *a, IR_Operand *b, IR_Operand *c) {
	
	IR_Inst *instruction = pool_alloc(buf->m, sizeof(IR_Inst)); 
	Assert(instruction && "failed to allocate instruction");
	*instruction = (IR_Inst){ op, a, b, c };
	return instruction;
}

#define ir_node3(buf, op, a, b, c) ir_buffer_push_inst(buf, op, a, b, c)
#define ir_node2(buf, op, a, b)    ir_buffer_push_inst(buf, op, a, b, 0)
#define ir_node1(buf, op, a)       ir_buffer_push_inst(buf, op  a, 0, 0)
#define ir_node0(buf, op)          ir_buffer_push_inst(buf, op, 0, 0, 0)

internal IR_Operand *ir_operand(IR_Buffer *buf, int kind, unsigned long long val) {
	
	IR_Operand *operand = pool_alloc(buf->m, sizeof(IR_Operand)); 
	Assert(operand && "failed to allocate operand");
	 *operand = (IR_Operand){ kind, val };
	return operand; 
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
		
		default: Assert(!"Not Implemented"); 
		
	}
	
	return NULL;
}

// This converts the ast to a IR form. Quite interestingly, a side effect of how this 
// is done, made it also a linearlizer. This is of course what is needed from my IR.
// but I never quite expected it to be this specific way. At least not initially.
internal Vector *convert_ast_to_ir(Translation_Unit *tu, IR_Buffer *ir_buff) {
	
	Vector *functions = vec_init(); 
	Vector *decls = tu->decls;
	for (int i = 0; i < decls->index; i++) {
		Statement *decl_stmt = decls->data[i];
		if (decl_stmt->kind != STMT_FUNCTION_DECL || !decl_stmt->func.sc) continue;
		
		Statement *scope = decl_stmt->func.sc;
		Assert(scope->kind == STMT_SCOPE);
		
			IR_Inst *entrypoint = ir_node0(ir_buff, 0); // dumy node
		IR_Inst *temp = entrypoint;
		
		Vector *statements = scope->block.statements;
		for (int stmt_idx = 0; stmt_idx < statements->index; stmt_idx++) {
			Statement *stmt = statements->data[stmt_idx]; 
			switch (stmt->kind) {
				
				case STMT_RETURN: {
					IR_Inst *expression = convert_expr_to_ir(ir_buff, stmt->ret, temp);
					temp = ir_node0(ir_buff, IR_RET); 
					expression->next = temp;
				} break;
				
				default: Assert(!"Not implemented");
			}
		}
		
		vec_push(&tu->m, functions, entrypoint);
	}
	
	return functions;
}

internal void print_ir(Vector *ir_functions) {
	 
	for (int i = 0; i < ir_functions->index; i++) {
		printf("func%d\n", i); 
		IR_Inst *func_entry = ir_functions->data[i]; 
		
		IR_Inst *inst = func_entry;
		while (inst = inst->next) { 
			switch (inst->op) { 
				case IR_LOADC: printf("LOADC \tr%d %llu\n", inst->a->reg, inst->b->imm);  break;
				
				case IR_ADD: 
				case IR_SUB: {
					printf("%s \tr%d r%d r%d\n", inst->op == IR_ADD ? "ADD" : "SUB", 
						   inst->a->reg, inst->b->reg, inst->c->reg);
				} break; 
				
				case IR_RET: { 
					printf("RET\n"); 
				} break; 
			}
		}
		
		
	}
	
}