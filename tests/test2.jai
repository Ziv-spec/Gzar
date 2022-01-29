{
    8 * 8;   // normal expression  
	a1: u32; // decloration 
	b1: s8 = 2==2;  // decloration with initializer
	a1 = 3;         // assignment expression 
	
	// recursive scopes
	{ 
		b: u32 = 10; 
		b = 10 * 2 + 9;
	}
    
	return a1+b1;
}
