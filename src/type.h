/* date = February 3rd 2022 2:04 pm */

#ifndef TYPE_H
#define TYPE_H

typedef enum Type_Kind Type_Kind;


// This is a list of atomic types (aka the most basic types that the langauges supports) 
// on them you can perform atomic operations, which I will need to typecheck. 

enum Type_Kind {
    
    TYPE_INTEGER, // TODO(ziv): determine the size of integer probably should be 64 bit but I should see whetehr I am capable of actually using 64bit registers in the ml64 assembler or I will just stick to nasm which has served me quite well for the time being. 
    
    TYPE_VOID, 
    
    TYPE_STRING, 
    
    TYPE_S8,
    TYPE_S16, 
    TYPE_S32, 
    
    TYPE_U8,
    TYPE_U16, 
    TYPE_U32, 
    
}; 

typedef struct Type Type; 
struct Type {
    Type_Kind kind; // atomic type 
    Type *sub_type; // extension to the atomic type
    
    
};

internal Type *init_type(Token type_token); 

internal bool type_equal(Type *t1, Type t2);
internal bool type_op_check(Type *type, Token operation);

internal Token_Kind find_exact_type(Token token);


#endif //TYPE_H
