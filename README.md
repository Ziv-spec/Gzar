# Gzar
My toy language which I will use to learn more about compilers. 

## What will the Language support? 
The language will support functions, variables, logical and mathematical operations on constants and variables. 
The language might support pointers (we shall see) and if I will have the time, even strings. 

## How to compile the Gzar compiler 
Make sure that you have visual studio 2019 C/C++ compiler installed and run the build file from any command line you have.

## Running the compiler
You need to run the compiler from the x64 native command prompt for VS 
which will initialize the enviorment for the ml64 assembler that my compiler
uses, and the linker.

## Code generation
The compiler will output unoptimized assembly masm syntax

## What can it do? 

```c
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

```