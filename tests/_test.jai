
puts :: (val: string) -> void; // extrenal function puts

main :: () -> int {
    
	result: bool = false; 
    
	answer1: int = fib_recursive(10);
	answer2: int = fib_iterative(10); 
    
	if !(answer1 == answer2 && answer1 == 55) {
		return 0;
	}
	
	answer1 = fib_recursive(5); 		
	answer2 = fib_iterative(5); 
    
	if !(answer1 == answer2 && answer1 == 5) {
		return 0; 
	}
    
	puts("correct answer");
    
	return 1;
}

fib_recursive :: (index: int) -> int {
	if index < 2 {
		return index;
	}
    
	return fib_recursive(index-1) + fib_recursive(index-2);
}

fib_iterative :: (index: int) -> int {
	a: int = 0; 
	b: int = 1; 
	temp: int = 0;
    
	i: int = 0; 
	while i < index {
		temp = a + b; 
		a = b; 
		b = temp; 
		i = i + 1; 
	}
    
	return a;
}






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