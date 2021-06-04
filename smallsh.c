#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int endLoop = 0; 		  	// Declare and initialize global int variable whose value
                 			// will determine when the program will stop running

int foregroundOnly = 0; 	// Declare and initialize global int variable whose value
                 			// will determine if commands can be run in the background

int runBackground = 0; 		// Declare and initialize global int variable whose value
                 			// will determine if commands will be run in the background

int argCount = 0;			// Declare and initialize global int variable
                 			// which will count the number of arguments given

int exitStatus = 0; 		// Declare and initialize global int variable whose value
                 			// represents the exit status of a process

// Change working directory of smallsh
void changeDirectory(char *args[]){  

	if(argCount == 1){ 			// If there are no arguments for cd command
        chdir(getenv("HOME")); 	// the working directory is changed to what is 
								// specified in the HOME environment variable
    }

    else{
		// Try to change working directory to path specified in first argument
		if (chdir(args[1]) == -1) {
			// If attempt fails, print error message
			printf("Invalid directory.\n");
			fflush(stdout);
		}
	}
}

// Exit smallsh after killing any and all
// processes or jobs that the shell started
void exitShell(){  

    endLoop = 1;
}

// Print exit status or the terminating signal
// of the last foreground process ran by smallsh
void displayStatus(int childStatus){  

	// Check if child process exited on its own
	if (WIFEXITED(childStatus)) {
		printf("Exit Value: %d\n", WEXITSTATUS(childStatus));
	} 
	
	// Child process was terminated by a signal
	else {
		printf("Terminated by Signal %d\n", WTERMSIG(childStatus));
	}
}

// Enter or exit foreground-only mode when SIGTSTP signal is caught
void handleSIGTSTP(){ 

    // If not in foreground-only mode, display informative message
	// and enter foreground-only mode
	if (foregroundOnly == 0) {
		char* message = "Entering foreground-only mode (& is now ignored)\n";
		write(1, message, 50);	// Can't use printf nor strlen since they are non-reentrant
		fflush(stdout);
		foregroundOnly = 1; 	// Enter foreground-only mode
	}

	// If already in foreground-only mode, display informative message
	// and exit foreground-only mode
	else {
		char* message = "Exiting foreground-only mode\n";
		write (1, message, 30);	// Can't use printf nor strlen since they are non-reentrant
		fflush(stdout);
		foregroundOnly = 0;		// Exit foreground-only mode
	}
}

// Get user input
void receiveCommand(int pid, char *args[], char inputFile[], char outputFile[]){ 

	int i = 0, j = 0;						// Declare counters
	runBackground = 0;						// Reset background checker
	argCount = 0;							// Reset argument counter
	char userInput[2048];					// Declare userInput string
	
	memset(userInput, '\0', 2048);			// Fully initialize userInput string
	memset(args, '\0', 512);				// Clear args string
	memset(inputFile, '\0', 256);			// Clear input file string
	memset(outputFile, '\0', 256);			// Clear output file string

	printf(": ");							// Prompt with ':'
	fflush(stdout);							// Flush output buffers
	fgets(userInput, 2048, stdin);			// Get user input
	fflush(stdout);							// Flush again just in case

	// Reprompt if userInput is blank or starts with '#'
	while (userInput[0] == '#' || sscanf(userInput, "%s") == -1){
		printf(": ");
		fflush(stdout);
		fgets(userInput, 2048, stdin);
		fflush(stdout);
	}

	// Remove trailing newline from userInput
	while (1){
		if(userInput[i] == '\n'){
			userInput[i] = '\0';			// Replace newline with null character
			i = 0;
			break;
		}

		i++;
	}

	char *token = strtok(userInput, " ");	// Tokenize userInput using a space as the delimiter
	while (token != NULL){

		// Check for '<' symbol
		// If found, copy next token into inputFile
		if (!strcmp(token, "<")){
			token = strtok(NULL, " ");
			if (token != NULL){
				strcpy(inputFile, token);
			}
		}

		// Check for '>' symbol
		// If found, copy next token into outputFile
		else if (!strcmp(token, ">")){
			token = strtok(NULL, " ");
			if (token != NULL){
				strcpy(outputFile, token);
			}
		}

		// Check for '&' symbol
		// If found, set runBackground to 1
		else if (!strcmp(token, "&")){
			runBackground = 1;
		}

		else{
			args[i] = strdup(token);		// Insert duplicated token into list
			i++;
		}

		token = strtok(NULL, " ");			// Next token		
	}
	
	i = 0;

	// Read command and all arguments if any
	while (args[i] != NULL){
		while (1){
			// Check if argument has been fully processed
			if(args[i][j] == '\0'){
				j = 0;	// Reset counter
				break;	// Exit inner loop
			}

			else if (args[i][j] == '$' && args[i][j+1] == '$'){
				args[i][j] = '\0';         							// Replace $ with null character								
				args[i][j+1] = '\0';								// Replace $ with null character
				memset(userInput, '\0', 2048); 						// Clear userInput array
				snprintf(userInput, 2048, "%s%d", args[i], pid);	// Concatenate '$$'-less argument with pid
                args[i] = strdup(userInput);						// Replace old argument with new one

				j = 0;	// Reset counter
				break;	// Exit inner loop
			}
			j++;
		}
		i++;
		argCount++;
	}	
}

// Execute command according to user input
void executeCommand(char *args[], char inputFile[], char outputFile[], struct sigaction sigAct){ 

	int input, output;

	/* Built-in commands */
	if (!strcmp(args[0], "cd")){
		changeDirectory(args);
	}
	else if (!strcmp(args[0], "exit")){
		exitShell();
	}
	else if (!strcmp(args[0], "status")){
		displayStatus(exitStatus);
	}

	/* Non-Built commands */
	else{
		// Try to create a child process
		pid_t childPid = fork();
		switch (childPid){
			// Print error message if the fork was unsuccessful
			case -1:
				perror("Attempt to fork has failed...");
				exit(1);
			
			// The child should execute the code in this branch
			case 0:

				if (runBackground == 0 || foregroundOnly){
					// Foreground children will now terminate 
					// themselves upon receiving a SIGINT
					sigAct.sa_handler = SIG_DFL;
					sigaction(SIGINT, &sigAct, NULL);
				}
				

				// Check if input file given
				if (inputFile[0]){
					// Try to open specified input file for reading only
					input = open(inputFile, O_RDONLY);

					// Print error message if opening is unsuccessful
					if (input == -1) {
						printf("Failed to open %s\n", inputFile);
						fflush(stdout);
						exit(1);
					}

					dup2(input, 0);								// Redirect stdin to input file
					fcntl(input, F_SETFD, FD_CLOEXEC);			// Close file upon execvp()
				}

				// Check if output file given
				if (outputFile[0]){
					// Try to open specified output file for writing only
					// File will be created if it does not exist and truncated otherwise
					output = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);

					// Print error message if opening is unsuccessful
					if (output == -1) {
						printf("Failed to open %s\n", inputFile);
						fflush(stdout);
						exit(1);
					}

					dup2(output, 1);							// Redirect stdout to output file
					fcntl(output, F_SETFD, FD_CLOEXEC);			// Close file upon execvp()
				}

				// Try to execvp() non-built command
				// Print error message if execvp() is unsuccessful
				if (execvp(args[0], args)) {
					printf("%s: No such file or directory\n", args[0]);
					fflush(stdout);
					exit(1);
				}
			
			// The parent should execute the code in this branch
			default:
				if (runBackground && !foregroundOnly){
					waitpid(childPid, &exitStatus, WNOHANG);
					printf("Background pid is %d\n", childPid);
					fflush(stdout);
				}
				else{
					waitpid(childPid, &exitStatus, 0);
				}
			
			// Kill any and all zombies	
			while ((childPid = waitpid(-1, &exitStatus, WNOHANG)) > 0) {
				printf("Child %d terminated\n", childPid);
				displayStatus(exitStatus);
				fflush(stdout);
			}
		}
	}
}

// Run smallsh
int main(){

	int pid = getpid();							// Get and store process ID of smallsh
	char *cmdArgs[512]; 						// Store a list of command arguments
	char inputFileName[256];					// Declare input file to be redirected via stdin
	char outputFileName[256];					// Declare output file to be redirected via stdout

	memset(cmdArgs, '\0', 512);					// Fully initialize cmdArgs string
	memset(inputFileName, '\0', 256);			// Fully initialize input file string
	memset(outputFileName, '\0', 256);			// Fully initialize output file string
    
    // Handle interrupt signal 
	struct sigaction SIGINT_ACTION = {0};       // Declare and initialize a sigaction struct as empty
	SIGINT_ACTION.sa_handler = SIG_IGN;         // Ignore SIGINT
	sigfillset(&SIGINT_ACTION.sa_mask);         // Block all signals while signal handler is executing
	SIGINT_ACTION.sa_flags = 0;                 // Do not set any flags
	sigaction(SIGINT, &SIGINT_ACTION, NULL);    // Register signal handling function for SIGINT

	// Designate handleSIGTSTP() as the signal handling function for SIGTSTP 
	struct sigaction SIGTSTP_ACTION = {0};      // Declare and initialize a sigaction struct as empty
	SIGTSTP_ACTION.sa_handler = handleSIGTSTP;  // Call handleSIGTSTP() when SIGTSTP is caught
	sigfillset(&SIGTSTP_ACTION.sa_mask);        // Block all signals while signal handler is executing
	SIGTSTP_ACTION.sa_flags = 0;                // Do not set any flags
	sigaction(SIGTSTP, &SIGTSTP_ACTION, NULL);  // Register signal handling function for SIGTSTP
	
	// Loop only ends when endLoop != 0
    while (endLoop == 0){ 						
		receiveCommand(pid, cmdArgs, inputFileName, outputFileName);
		executeCommand(cmdArgs, inputFileName, outputFileName, SIGINT_ACTION);
    }
	
    return 0;
}