

a : int = 234; // global variables 

swap :: proc (a: int, b: int) -> void {
    temp: int; 
    
    temp = a; 
    a = b; 
    b = temp;
    
}

main :: proc (s : u8) -> s32 {
    
    8 * 8;   // normal expression  
	a1: u32; // decloration 
	b1: s8 = 2==2;  // decloration with initializer
	a1 = 3;         // assignment expression 
    
    b: int = 234-3;
    
    c : u32; 
    // recursive scopes
    { 
        b: u32 = 10; // redefinition of a variable
        b = b + 10 * 2;
        c = a1 + b; // using variables from a outer scope
    }
    
    e: *int; // support for arbitrary pointer types
    
    return a1+b1;
}