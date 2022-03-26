
 //boring: string = "something boring"; 

_add :: (a: int, b: int) -> int {
    return a+b;
}

some_other_function :: (a: int, b: int, c: int, d: int) -> int {
    return b*c - a*d;
}

main :: () -> int {
    result: int = some_other_function(1, 2, 3, 4); 
    return result;
}
