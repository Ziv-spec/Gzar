// external function puts
puts :: (str: string) -> int; 

main :: () -> int {
	i: int = 0; // there is also support for &, |, ^, +, -, *, /
    
	while (i < 5) {
		puts("something");
		i = i + 1; 
	}
    
    {
        something: int = 0; // recursive scopes
    }
    
    var: bool = i > 5;
    if var && true { // you can try to use the || operator insted 
        return 0;
    }
    
    return i;
}