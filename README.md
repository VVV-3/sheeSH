# SheeSH
<!-- Sinatraa says hello -->

### Usage
1. make
2. ./sheesh

### File Structure
    * All source code for the shell is contained inside the src directory.
    * sheesh.h contains the config variables for the shell. ( i.e, shell name, max input size, etc)
    * sheesh.c is the main file, initializing the shell and starting the infinte while loop to handle user inputs.
    * utils.c contains all the utility function needed for the shell to run, i.e, function like displayPrompt() to show the prompt on the terminal, handleInput() to handle input given by the user, etc. utils.h contails all the headers of functions in utils.c
    * cmds.c contains all the functions needed to execute shell commands, i.e, cd, pwd, echo, ls, repeat, pinfo, history. Also functions to execute foreground and background processes.

### Working of the shell
    * The main function starts the shell with startSheesh()
    * The shell the calls initInfo() to initalize the info and functions needed for the shell to function properly.
    * The main while loop is then started and the prompt is displayed, waiting for the user's input.
    * The raw input is sent to handleInput() which first adds the input to history and then splits it into raw commands, formats it for tabs and spaces and sends the command to handleCommand().
    * handleCommand() first iterates through the command to execute bg processes and then using if else conditions call the appropriate command function.
    * The handleErrors() function handles all possible error scenarios which may occur.

### Functioning
    * cd <directory path>                       - Changes directory to the given path. Changes to shell home if no directory is provided
    * pwd                                       - Shows the absolute path of the current working directory 
    * echo                                      - Prints the message to the terminal. Handled single and double quotes.
    * ls [al] <directory path>                  - Handles all variations of ls with a and l flags and directory path
    * repeat [n] <command>                      - Repeats the given command n times.
    * pinfo [pid]                               - Prints process related info (pid, Process Status {R, S, S+, Z}, memory and Executable Path) about given pid. Prints process related info of shell program if no pid is provided
    * exit                                      - Exits the shell
    * history [n]                               - Prints history of n (max 20) commands. Prints history of maximum 10 commands if n is not provided
    * jobs                                      - prints list of all currently running background jobs along with their pid and their current state
    * sig <jobNumber> <signalNumber>            - sends the corresponding signal to the specific bg process
    * fg <jobNumber>                            - brings a running or a stopped background job with a given job number to background
    * bg <jobNumber>                            - changes a stopped background job to a running background job
    * overkill                                  - kills all background process at once
    * CTRL-Z                                    - changes the status of currently running job to stop, and pushes it in background process
    * CTRL-C                                    - cause a SIGINT signal to be sent to the current foreground job of this shell.
    * input-output redirection functionality    - replicates (almost) what bash does
    * piping functionality                      - replicates (almost) what bash does
    * All other commands are implemented using execvp. 
    * Background Processing (can be run with '&') is handled for commands executed through execvp