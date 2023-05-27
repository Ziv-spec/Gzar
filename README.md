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
/*
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

 Language spec
 *  - at least one
 ?  - 0 or more times
 |  - expands the possible space
 () - groups
 "" - string which represents the
      combination of charatcters
      needed for a certain token
      in the language

 program -> decloration*

 block -> "{" statement* "}"

 decloration -> variable_decl
     | function_decl
     | statement

 statement -> expression
     | while
     | if
     | block
     | decloration
     | return

 expression -> expression
     | call
     | unary
     | binary
     | grouping
     | arguments
     | assignment
     | literal

 literal -> number | string | "true" | "false" | "nil"
 cast -> "cast" "(" type ")" expression
 call -> identifier "(" arguments ")"
 unary -> ("-" | "!" | cast ) expression
 grouping -> "(" expression ")"
 binary -> expression operator expression
 operator -> "==" | "!=" | ">=" | "<=" | "<" |
            ">" | "+" | "-" | "*" | "/" | "&" |
            "|" | "^" | "&&" | "||" | "%"

 number -> "0-9"*
 string -> """ any character between the " are accepted  """
 variable_decl -> identifier ":" type ( ("=" expression) | ";" )
 function_decl -> identifier "::" grouping "->" return_type block

 identifier -> "A-Z" | "a-z" | "_"

 type -> "*"? ( "u64" | "u32" | "u8" |
                "s64" | "s32" | "s8" |
                "int" | "string" | "bool" |
                "*void")

 return_type type | "void"

 if -> ("if" expression block ) | ("if" expression block "else" block )
 while -> "while" expression block
*/