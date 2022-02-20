/* date = February 3rd 2022 2:04 pm */

#ifndef TYPE_H
#define TYPE_H



// This is a list of atomic types (aka the most basic types that the langauges supports) 
// on them you can perform atomic operations, which I will need to typecheck. 

typedef enum Atom_Kind Atom_Kind;
enum Atom_Kind{
    
    TYPE_INTEGER = 1 << 0, // s64? 
    TYPE_VOID    = 1 << 1, 
    TYPE_STRING  = 1 << 2, 
    
    // TODO(ziv): expand this to 64bit integers (as this will be used by defualt)
    
    TYPE_S8  = 1 << 3, 
    TYPE_S16 = 1 << 4, 
    TYPE_S32 = 1 << 5 , 
    
    TYPE_U8  = 1 << 6,
    TYPE_U16 = 1 << 7, 
    TYPE_U32 = 1 << 8, 
    
    TYPE_POINTER  = 1 << 9,
    TYPE_FUNCTION = 1 << 10,
    TYPE_ARRAY    = 1 << 11,
}; 

typedef struct Type Type; 
struct Type {
    Atom_Kind kind;   // atomic type 
    Type   *subtype;  // extension to the atomic type
    Vector *symbols;  // array of symbols
    Type   *type;     // can function as a return type, or as a list of types
};

/* 
internal Type *init_type(Token type_token); 
internal bool type_equal(Type *t1, Type t2);
internal bool type_op_check(Type *type, Token operation);
internal Token_Kind find_exact_type(Token token);
 */


#endif //TYPE_H
