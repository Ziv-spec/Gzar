/* date = February 3rd 2022 2:04 pm */

#ifndef TYPE_H
#define TYPE_H

typedef enum Type_Kind Type_Kind;
typedef enum Atom_Kind Atom_Kind;
typedef struct Type Type; 

// NOTE(ziv): The `Type` in the compiler works in the following way: 
// If you have a atomic type, then the kind will be ATOMIC and the subtype 
// and symbols will be NULL. 
// If you have a pointer to some other type then you will use the subtype 
// field to extend the type further. e.g. ****int
// If you have a function `Type` then it shall be represented as a ATOMIC
// function type and have a subtype as it's return type and the symbols 
// as the input arguments for the function with their order.

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

// a table for getting more frequently used types
static Type types_tbl[] = {
    [ATOM_S8]  = { TYPE_S8,  NULL, NULL }, 
    [ATOM_S16] = { TYPE_S16, NULL, NULL }, 
    [ATOM_S32] = { TYPE_S32, NULL, NULL }, 
    [ATOM_S64] = { TYPE_S64, NULL, NULL }, 
    
    [ATOM_U8]  = { TYPE_U8,  NULL, NULL }, 
    [ATOM_U16] = { TYPE_U16, NULL, NULL }, 
    [ATOM_U32] = { TYPE_U32, NULL, NULL }, 
    [ATOM_U64] = { TYPE_U64, NULL, NULL }, 
    
    [ATOM_UNKNOWN]   = { TYPE_UNKNOWN,NULL, NULL }, 
    [ATOM_VOID]   = { TYPE_VOID,   NULL, NULL }, 
    [ATOM_STRING] = { TYPE_STRING, NULL, NULL }, 
    [ATOM_BOOL]   = { TYPE_BOOL,   NULL, NULL }, 
    
    // NOTE(ziv): From here the types do not have a 1-1 relationship with the types.
    // Also, adding more types means that this table will get largeer and might get
    // evicted from the cash more often. This means that I will probably try to 
    // minize the size of this table instead of maximize it's usefulness. 
    
    
    // [ATOM_VOID_POINTER] = { TYPE_POINTER, &types_tbl[ATOM_VOID], NULL }, // *void 
    
}; 

// A table used for checking the types on which a operation can operate on

// maybe have some global variable for the time being (which would get inside the Translation Unit thingy 
// which will represent the type index into the types list which will have you know the actual types and things 


#endif //TYPE_H
