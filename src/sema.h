/* date = February 3rd 2022 2:04 pm */

#ifndef SEMA_H
#define SEMA_H

typedef enum Type_Kind Type_Kind;
typedef enum Atom_Kind Atom_Kind;

// NOTE(ziv): The `Type` in the compiler works in the following way: 
// If you have a atomic type, then the kind will be ATOMIC and the subtype 
// and symbols will be NULL. 
// If you have a pointer to some other type then you will use the subtype 
// field to extend the type further. e.g. ****int
// If you have a function `Type` then it shall be represented as a ATOMIC
// function type and have a subtype as it's return type and the symbols 
// as the input arguments for the function with their order.

typedef struct Type Type; 
struct Type {
    Type_Kind kind;  // atomic type
    Type   *subtype; // extension to the atomic (for things like `*int` and so on...)
    Vector *symbols; // array of symbols (for function input arguments) 
};

enum Type_Kind {
    
    TYPE_S8  = 1 << 0,
    TYPE_S16 = 1 << 1,
    TYPE_S32 = 1 << 2,
    TYPE_S64 = 1 << 3,
    
    TYPE_INTEGER_STUB, 
    
    TYPE_U8  = 1 << 4,
    TYPE_U16 = 1 << 5,
    TYPE_U32 = 1 << 6,
    TYPE_U64 = 1 << 7,
    
    TYPE_UNSIGNED_INTEGER_STUB, 
    
    TYPE_VOID   = 1 << 8,
    TYPE_STRING = 1 << 9,
    TYPE_BOOL   = 1 << 10,
    
    TYPE_UNKNOWN  = 1 << 11,
    TYPE_ATOMIC_STUB, 
    
    TYPE_POINTER  = 1 << 12,
    TYPE_FUNCTION = 1 << 13,
    TYPE_ARRAY    = 1 << 14,
}; 

enum Atom_Kind {
    ATOM_S8 = 0,
    ATOM_S16,
    ATOM_S32,
    ATOM_S64,
    
    ATOM_U8,
    ATOM_U16,
    ATOM_U32,
    ATOM_U64,
    
    ATOM_VOID,
    ATOM_STRING,
    ATOM_BOOL,
    
    ATOM_UNKNOWN,
    // ATOM_ATOMIC_TYPES_STUB,
    
    ATOM_POINTER,
    ATOM_ARRAY,
    
    ATOM_VOID_POINTER,
    
}; 

// Not really needed function declorations

typedef struct Translation_Unit Translation_Unit; 
typedef struct Expr Expr; 
typedef struct Statement Statement; 

internal inline s64 type_kind_to_atom(Type_Kind kind);
internal Type *get_atom(Type_Kind kind);
internal char *type_to_string(Type *ty); 
internal char *format_types(char *msg, Type *t1, Type *t2);
internal void type_error(Translation_Unit *tu, Token line, Type *t1, Type *t2, char *msg); 
internal bool type_equal(Type *t1, Type *t2);
internal int get_type_size(Type *type);
internal Type *get_compatible_type(Type *dest, Type *src);
internal Type *implicit_cast(Type *t1, Type *t2);
internal Type *sema_expr(Translation_Unit *tu, Expr *expr);
internal bool sema_statement(Translation_Unit *tu, Statement *stmt);
internal bool sema_translation_unit(Translation_Unit *tu);

#endif //SEMA_H
