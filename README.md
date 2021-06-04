# smallsh
A small shell implementation written in C that: 

- Provides a prompt (:) for running commands
- Handles blank lines and comments, which are lines beginning with the # character
- Provides expansion for the variable $$
- Executes 3 commands "exit", "cd", and "status" via code built into the shell
- Executes other commands by creating new processes using a function from the exec family of functions
- Supports input and output redirection
- Supports running commands in foreground and background processes
- Implements custom handlers for 2 signals, SIGINT and SIGTSTP

# Instructions
To compile the smallsh.c file into an executable, 
run the following command in the terminal: "gcc --std=gnu99 -o smallsh smallsh.c"

To run the executable file, 
run the following command in the terminal: "./smallsh"
