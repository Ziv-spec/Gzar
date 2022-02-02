
//something : s8 = 234;

main :: proc () -> s32 {
    
    	8 * 8;   // normal expression  
	a1: u32; // decloration 
	b1: s8 = 2==2;  // decloration with initializer
	a1 = 3;         // assignment expression 
    
	// recursive scopes
	{ 
		b: u32 = 10; 
		b = 10 * 2 + 9;
	}
	
    a1 = "sdf";
    
    return a1+b1;
}
