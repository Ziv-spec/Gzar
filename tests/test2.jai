
 //boring: string = "something boring"; 

//some other thing that I Had

// I can finally exit a process early
ExitProcess :: (val: int) -> int;
puts :: (s: string) -> int; 

something: string = "something\nlike\nthis\n"; 


exit :: (error_level: int) -> int {
    return ExitProcess(error_level);
}

main :: () -> int {
    
    result: int = 8 * 8;
    
    // exit(1);
    
    puts(something);
    
    
    return result;
}
