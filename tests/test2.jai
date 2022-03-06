


a : int = 234; // global variables 

main :: proc (something: int, something_else: int) -> int {
    return something + something_else; 
}

swap :: proc (a: int, b: int) -> void {
    temp: int; 
    
    temp = a; 
    a = b; 
    b = temp;
    
    
    main(a, b, temp());
    
    
}
