
enum {
	IRK_LOAD, 
	
	IRK_ADD,
	IRK_SUB,
	
	IRK_BRH,  // branch
	IRK_CBRH, // conditional branch
	
	IRK_RET,
} IR_Kind;

typedef struct { 
	int op;
	int idx; // index to jump to
	int *a;
	int *b;
	int *c;
} IR;

typedef struct { 
	IR *instructions;
	int count; 
	int capacity;
} IR_Buffer; 

typedef struct {
	int begin, end;
} IR_Slice; 

static int last_register;
static int registers[0x100];

static int last_data; 
static int datas[0x100]; 

internal int *ir_register() { 
	registers[last_register] = last_register;
	return &registers[last_register++]; 
}
internal int *ir_data(int x) {
	datas[last_data] = x; 
	return &datas[last_data++]; 
}

internal int ir_buffer_push3(IR_Buffer *buf, int op, int idx, int *a, int *b, int *c) {
	
	if (buf->count >= buf->capacity) {
		buf->capacity = (buf->capacity + 1) * 2;
		buf->instructions = realloc(buf->instructions, buf->capacity*sizeof(IR));
		Assert(buf->instructions && "Failed to allocate memory");
	}
	
	buf->instructions[buf->count++] = (IR){ op, idx, a, b, c };
	return buf->count - 1;
}

#define ir_push3(buf, op, a, b, c) ir_buffer_push3(buf, op, 0, a, b, c)
#define ir_push2(buf, op, a, b)    ir_buffer_push3(buf, op, 0, a, b, 0)
#define ir_push1(buf, op, a)       ir_buffer_push3(buf, op  0, a, 0, 0)
#define ir_push0(buf, op)          ir_buffer_push3(buf, op, 0, 0, 0, 0)

#define ir_push2m(buf, idx, op, a, b)    ir_buffer_push3(buf, op, idx, a, b, 0)
#define ir_push1m(buf, idx, op, a)       ir_buffer_push3(buf, op  idx, a, 0, 0)
#define ir_push0m(buf, idx, op)          ir_buffer_push3(buf, op, idx, 0, 0, 0)

internal int convert_expr_to_ir(IR_Buffer *ir_buff, Expr *expr, int **output) { 
	if (!expr) return 0; 
	
	switch(expr->kind) {
		case EXPR_BINARY: { 
			int *ol = NULL, *or = NULL;
			convert_expr_to_ir(ir_buff, expr->left,  &ol);
			convert_expr_to_ir(ir_buff, expr->right, &or);
			
			int op = 0;
			switch (expr->operation.kind) {
                case TK_MINUS: op = IRK_SUB; break; 
                case TK_PLUS:  op = IRK_ADD; break;
				default: Assert(!"Not implemented");
			}
			*output = ir_register();
			return ir_push3(ir_buff, op, *output, ol, or);
		} break;
		
		case EXPR_LITERAL: {
			int *reg = ir_register(); *output = reg; 
			return ir_push2(ir_buff, IRK_LOAD, reg, ir_data((int)(size_t)expr->literal.data)); 
		} break;
		
	}
	
	return 0;
}

internal int convert_function_to_ir(IR_Buffer *ir_buff, IR_Slice *slice, Statement *function) {
	Assert(function->kind == STMT_FUNCTION_DECL);
	Statement *scope = function->func.sc;
	Assert(scope->kind = STMT_SCOPE);
	Vector *statements = scope->block.statements;
	
	slice->begin = ir_buff->count; 
	for (int i = 0; i < statements->index; i++) {
		Statement *stmt = statements->data[i]; 
		switch (stmt->kind) {
			case STMT_RETURN: { 
				int *o;
				int ir = convert_expr_to_ir(ir_buff, stmt->ret, &o);
				ir = ir_push0(ir_buff, IRK_RET); 
			} break;
			default: Assert(!"Not implemented");
		}
	}
	slice->end = ir_buff->count;
	 
	return slice->end - slice->begin;
}

internal Vector *convert_ast_to_ir(Translation_Unit *tu, IR_Buffer *ir_buff) {
	
	Vector *ir_functions = vec_init();
	Vector *decls = tu->decls;
	for (int i = 0; i < decls->index; i++) {
		Statement *stmt = decls->data[i];
		if (stmt->kind != STMT_FUNCTION_DECL || !stmt->func.sc) continue;
		
		// TODO(ziv): @Incomplete push the ir graph
		IR_Slice *slice = pool_alloc(&tu->m, sizeof(IR_Slice));
		if (!convert_function_to_ir(ir_buff, slice, stmt)) break;
		vec_push(&tu->m, ir_functions, slice);
		
	}
	return ir_functions;
}

internal void print_ir(IR_Buffer *ir_buff, Vector *ir_functions) {
	
	for (int i = 0; i < ir_functions->index; i++) {
		printf("func%d\n", i); 
		IR_Slice *slice = ir_functions->data[i]; 
		
		IR *arr = &ir_buff->instructions[slice->begin];
		int count = slice->end - slice->begin; 
		
		for (int j = 0; j < count; j++) {
			IR ir = arr[j];
			
			switch (ir.op) {
				
				case IRK_LOAD: {
				printf("LOAD r%d %d\n", *ir.a, *ir.b);
				} break;
				
				case IRK_ADD: 
				case IRK_SUB: {
					printf("%s r%d r%d r%d\n", ir.op == IRK_ADD ? "ADD" : "SUB", *ir.a, *ir.b, *ir.c);
				} break; 
				
				case IRK_RET: { 
					printf("RET\n"); 
				} break; 
			}
			
			
		}
		
	}
	
}