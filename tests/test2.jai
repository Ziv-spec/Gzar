
 //boring: string = "something boring"; 

_add :: proc (a: int, b: int) -> int {
    return a+b;
}

some_other_function :: proc(a: int, b: int, c: int, d: int) -> int {
    return b*c - a*d;
}

main :: proc (something: int) -> int {
    result: int = _add(1, 3); 
    return result;
}
