# Gzar
My toy language which I will use to learn more about compilers. 

## What will the Language support? 
The language will support functions, variables, logical and mathematical operations on constants and variables. 
The language might support pointers (we shall see) and if I will have the time, even strings. 

## How to compile the Gzar compiler 
Make sure that you have visual studio 2019 C/C++ compiler installed and run the `build.bat` file from `cmd` or `x64 Native tools Command Prompt`. Note that the executable `gzar.exe` will pop in the `build` directory. 

## Code generation
The compiler will output unoptimized assembly(nasm) at first. Then at some point if I will feel like it, I will have a optimizer for the code generation. Also might transition to machine code directly. That is only if I have time. 
This will intern, make the compiler not dependent on any piece of software other than the microsoft linker. 

## What can it do? 

The following syntax can already be lexed and parsed correctly. 
It generates the appropriate AST but does nothing with it. 
```c
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
```