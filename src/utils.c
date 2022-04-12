#include "sheesh.h"
#include "utils.h"
#include "cmds.h"

// Global Variables
extern char USER_NAME[];
extern char MACHINE_NAME[];
extern char HOME_DIR[];
extern char PRE_DIR[];
extern bool FG_CONTROL;

extern struct process_data childProcesses[];
extern int child_process_count;

// Function to handle completion of child bg processes.
void childProcessSignalHandler(int sigNum)
{
    pid_t pid;
    int status;
    bool process_terminated = false;

    // Checking to see if a child process can be reaped
    while( (pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        bool found = false;
        status = WIFEXITED(status);
        for(int i=0; i<=child_process_count; i++)
        {
            if(!found)
            {
                if(childProcesses[i].pid == pid)
                {
                    printf("\n%s with pid %d exited %s.\n", childProcesses[i].pname, pid, status != 0 ? "normally" : "abnormally");
                    found = true; process_terminated = true;
                }
            }
            else childProcesses[i-1] = childProcesses[i];
        }
        if(found) child_process_count--;
    }

    // Displaying new prompt using strerr stream if a child process finishes.
    if(process_terminated) displayPrompt();
}

// Function to handle completion of ctrl+c.
void ctrlCSignalHandler(int sigNum)
{
    printf("\n");
    if(FG_CONTROL) displayPrompt();
}

// Function to handle completion of ctrl+z.
void ctrlZSignalHandler(int sigNum)
{
    printf("\n");
    if(FG_CONTROL) displayPrompt();
}

// Function to sort child process
void sortChildProcesses()
{
    int k = 0;
    for(int i=0; i< child_process_count; i++)
    {
        k = i;
        for(int j=i+1; j<child_process_count; j++) if( strcmp(childProcesses[k].pname, childProcesses[j].pname) > 0 ) k=j;
        struct process_data tp;
        tp                = childProcesses[k];
        childProcesses[k] = childProcesses[i];
        childProcesses[i] =                tp;
    }
}

// Function to handle setting up signal handlers.
void handleSignal(int sigNum, void (*handler)(int))
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handler;
    action.sa_flags = SA_RESTART;
    sigemptyset(&action.sa_mask);
    sigaction(sigNum, &action, NULL);
}

// initializing all the basic info needed for shell to function.
void initInfo()
{
    // setting up username and machine name.
    struct passwd *USER_INFO = getpwuid(getuid());
    strcpy(USER_NAME, USER_INFO->pw_name);
    if(USER_NAME == NULL) handleErrors(-1);
    if(gethostname(MACHINE_NAME, MAX_NAME_SIZE) == -1) handleErrors(-2);
    if(getcwd(HOME_DIR, MAX_PATH_SIZE) == NULL) handleErrors(-3);
    strcpy(PRE_DIR, HOME_DIR);

    // setting up the signal handlers for checking when a bg process finishes.
    for(int i=0; i<=MAX_CHILD_PROCESS_COUNT; i++) { childProcesses[i].pid=0; strcpy(childProcesses[i].pname,""); }
    handleSignal(17, childProcessSignalHandler);
    handleSignal( 2, ctrlCSignalHandler);
    handleSignal(20, ctrlZSignalHandler);


    // checking to see if shell history file exists, if not one is created.
    FILE* shell_history;
    shell_history = fopen("/var/tmp/sheesh_history.txt", "r");
    if (shell_history == NULL) shell_history = fopen("/var/tmp/sheesh_history.txt", "w+");
    fclose(shell_history);

    // showing welcome message.
    printf("\033[01;33mWelcome to sheeSH !\033[0m\n");
}

// Function to display the promp message.
void displayPrompt()
{
    char cur_dir[MAX_NAME_SIZE];
    if (getcwd(cur_dir, MAX_PATH_SIZE) == NULL) handleErrors(-4);

    int home_dir_len = strlen(HOME_DIR);
    int cur_dir_len = strlen(cur_dir);

    // Converting absolute path to relative path.
    if (strncmp(HOME_DIR, cur_dir, home_dir_len) == 0)
    {
        char tmp_path[MAX_PATH_SIZE];
        strcpy(tmp_path,cur_dir);
        cur_dir[0] = '~';
        for(int i = home_dir_len; i <= cur_dir_len; i++) cur_dir[ i - home_dir_len + 1] = tmp_path[i];
    }

    fprintf(stdout,"\033[1;36m<%s@%s\033[0m:\033[01;33m%s\033[1;36m>\033[0m ",USER_NAME, MACHINE_NAME, cur_dir);
    fflush(stdout);
}

// Function to note down new inputs into the shell history.
void noteHistory(char* input)
{
    if(input[0] == '\n') return;

    // Reading from history.
    char history[21*MAX_INPUT_SIZE];
    FILE* shell_history;
    shell_history = fopen("/var/tmp/sheesh_history.txt", "r");
    fread(history, 1, 21*MAX_INPUT_SIZE, shell_history);
    fclose(shell_history);

    int i, history_length = strlen(history);
    for(i=strlen(history)-2; i>=0 ; i--) if(history[i] == '\n') break;
    if(!strcmp(history+i+1,input)) return;

    // Adding input to history.
    int first_cmd = -1;
    int no_of_cmd = 0;
    strcat(history, input);
    history_length = strlen(history);
    for(i=0; i< history_length; i++)
    {
        if(history[i] == '\n')
        {
            if(first_cmd == -1) first_cmd = i+1;
            no_of_cmd++;
        }
    }
    if(no_of_cmd <= 20) first_cmd = 0;

    // Writing to history.
    shell_history = fopen("/var/tmp/sheesh_history.txt", "w+");
    fwrite(history + first_cmd, 1, 21*MAX_INPUT_SIZE, shell_history);
    fclose(shell_history);
}

// Function to handle input from the user.
void handleInput(char* input)
{
    noteHistory(input);
    
    // Replace all tabs(\t) with white space and newline(\n) with ;
    int input_length = strlen(input);
    for(int i=0; i<input_length; i++)
        if      (input[i] == '\t') input[i] = ' ';
        else if (input[i] == '\n') input[i] = ';';

    // Tokenize the given input into valid commands by using ; as delimiter.
    int no_of_commands = 0;
    char* raw_commands[MAX_INPUT_SIZE];
    raw_commands[no_of_commands] = strtok(input, ";");
    while(raw_commands[no_of_commands] != NULL) raw_commands[ ++no_of_commands ] = strtok(NULL, ";");

    // Tokenize each raw command to remove white spaces
    // and send each tokeized command to handleCommand to be executed sequentially.
    for(int i=0; i<no_of_commands; i++)
    {
        int no_of_words = 0;
        char* command[MAX_NO_WORDS_IN_COMMAND] = {NULL};
        command[0] = strtok(raw_commands[i], " ");
        while( command[no_of_words] != NULL) command[++no_of_words] = strtok(NULL, " ");
        if(no_of_words > 0) handleCommand(no_of_words, command);
    }
}

// Function to execute Command
void execCommand(int argc, char* argv[])
{
    if(argv[0] != NULL)
    {
        if      (!strcmp(argv[0],"exit"))     exit(0);
        else if (!strcmp(argv[0],"clear"))    clearTerminal();
        else if (!strcmp(argv[0],"cd"))       cd(argc, argv);
        else if (!strcmp(argv[0],"pwd"))      pwd();
        else if (!strcmp(argv[0],"echo"))     echo(argc, argv);
        else if (!strcmp(argv[0],"ls"))       ls(argc, argv);
        else if (!strcmp(argv[0],"repeat"))   repeat(argc, argv);
        else if (!strcmp(argv[0],"pinfo"))    pinfo(argc, argv);
        else if (!strcmp(argv[0],"history"))  history(argc, argv);
        else if (!strcmp(argv[0],"jobs"))     jobs(argc, argv);
        else if (!strcmp(argv[0],"sig"))      sig(argc, argv);
        else if (!strcmp(argv[0],"bg"))       bg(argc, argv);
        else if (!strcmp(argv[0],"fg"))       fg(argc, argv);
        else if (!strcmp(argv[0],"replay"))   replay(argc, argv);
        else                                  fgExecute(argc, argv);
    }
}

// Function to handle Redirection.
void handleRedirection(int argc, char* argv[])
{
    int stdin_fd,stdout_fd;
    int ipfd,opfd;
    int redir = 0;
    for(int i=0; i<argc; i++)
    {
        if(!strcmp(argv[i],"<"))
        {
            redir+=1;
            stdin_fd = dup(STDIN_FD);
            ipfd = open(argv[i+1], O_RDONLY);
            if (ipfd < 0) {close(stdin_fd); handleErrors(-17); return;}
            if( dup2(ipfd,STDIN_FD) == -1) {handleErrors(-18); return;}
            argv[i] = NULL; 
        }
        else if(!strcmp(argv[i],">"))
        {
            redir+=2;
            stdout_fd = dup(STDOUT_FD);
            opfd = open(argv[i+1], O_CREAT | O_WRONLY | O_TRUNC, PERMS);
            if (opfd < STDOUT_FD) {close(stdout_fd); handleErrors(-17); return;}
            if( dup2(opfd,STDOUT_FD) == -1) {handleErrors(-18); return;}
            argv[i] = NULL;
        }
        else if(!strcmp(argv[i],">>"))
        {
            redir+=2;
            stdout_fd = dup(STDOUT_FD);
            opfd = open(argv[i+1], O_CREAT | O_WRONLY | O_APPEND, PERMS);
            if (opfd < 0) {close(stdout_fd); handleErrors(-17); return;}
            if( dup2(opfd,STDOUT_FD) == -1) {handleErrors(-18); return;}
            argv[i] = NULL;
        }
    }
    for(int i=0; i<argc; i++) {if(argv[i] == NULL) {argc = i;break;}}
    
    // Executing the command.
    execCommand(argc, argv);

    // Resetting stdio streams after redirection.
    if(redir%2 == 1) { close(ipfd); dup2(stdin_fd,0); }
    if(redir   >  1) { close(opfd); dup2(stdout_fd,1);}
}

// Function to handle Pipes.
void handlePipes(int no_of_words, char* command[])
{
    int pipe_count = 0;
    for(int i=0; i<no_of_words; i++) if(!strcmp(command[i],"|")) pipe_count++;

    if (pipe_count == 0) handleRedirection(no_of_words, command);
    else
    {
        int fds[2*pipe_count+1];
        for(int i=0; i<pipe_count; i++) if(pipe(fds + 2*i) == -1) handleErrors(-19);

        int pid;
        int cur_word = 0;
        int argc = 0;
        char* argv[MAX_NO_WORDS_IN_COMMAND + 5] = {NULL};

        for(int i=0; i< pipe_count + 1; i++)
        {
            for(; cur_word<no_of_words; cur_word++)
            {
                if(!strcmp(command[cur_word],"|")) {cur_word++;break;}
                argv[argc++] = command[cur_word];
            }

            pid = fork();
            if(pid == 0)
            {

                // skipping last command after final pipe.
                if(i < pipe_count) dup2(fds[2*i + 1], STDOUT_FD);

                // skipping first command b4 initial pipe.
                if(i > 0) dup2(fds[2*(i - 1) + 0], STDIN_FD);

                for(int j=0; j<2*pipe_count; j++) close(fds[j]);

                handleRedirection(argc, argv);

                exit(0);
            }

            for(int j=0; j<=argc; j++) argv[j] = NULL;
            argc=0;
        }

        for(int j=0; j<2*pipe_count; j++) close(fds[j]);
        for(int i=0; i< pipe_count + 1; i++) wait(NULL);
    }
}

// Function to handle a given command from the user input.
void handleCommand(int no_of_words, char* command[])
{
    int argc = 0;
    char* argv[MAX_NO_WORDS_IN_COMMAND + 5] = {NULL};
    
    // Executing all background processes in a given command.
    for(int i=0; i<no_of_words; i++)
    {
        if(command[i][0] == '&' && argv[0] != NULL)
        {
            bgExecute(argc, argv);
            for(int i=0; i<argc; i++) argv[i] = NULL;
            argc = 0;
        }
        else argv[argc++] = command[i];
    }

    // // Handling Pipes.
    handlePipes(argc, argv);
}

// Function to clear terminal.
void clearTerminal()
{
    printf("\e[1;1H\e[2J");
}

// Function to handle all error senarios.
void handleErrors(int error_code)
{
    if(error_code != 0)
    {
        fprintf(stderr, "Error: ");
        if      (error_code == -1)  fprintf(stderr, "Could not retrieve Username. Unable to launch shell.");
        else if (error_code == -2)  fprintf(stderr, "Could not retrieve Machine Name. Unable to launch shell.");
        else if (error_code == -3)  fprintf(stderr, "Could not retrieve Current Directory. Unable to launch shell.");
        else if (error_code == -4)  fprintf(stderr, "Could not retrieve Current Directory. Exiting Shell");
        else if (error_code == -5)  fprintf(stderr, "Could not retrieve Present Working Directory.");
        else if (error_code == -6)  fprintf(stderr, "Invalid number of arguments provided, cd takes in atmost 1 argument.");
        else if (error_code == -7)  fprintf(stderr, "Cannot change directory as Max path size set will be exceeded.");
        else if (error_code == -8)  fprintf(stderr, "No such file or directory.");
        else if (error_code == -9)  fprintf(stderr, "Cannot open this directory stream.");
        else if (error_code == -10) fprintf(stderr, "Could not create a child process.");
        else if (error_code == -11) fprintf(stderr, "Could not create new background process as limit set for active bg processes reached.");
        else if (error_code == -12) fprintf(stderr, "Invalid number of arguments provided");
        else if (error_code == -13) fprintf(stderr, "Could not retrieve stat file.");
        else if (error_code == -14) fprintf(stderr, "Invalid argument, the max number that can be provided is 20");
        else if (error_code == -15) perror("error");
        else if (error_code == -16) fprintf(stderr, "Invalid argument, job number does not exist.");
        else if (error_code == -17) fprintf(stderr, "Could not open file stream for redirection.");
        else if (error_code == -18) fprintf(stderr, "Could not redirect. Exiting shell.");
        else if (error_code == -19) fprintf(stderr, "Could not create pipe. Exiting shell.");
        else if (error_code == -20) fprintf(stderr, "Invalid interval or period entered.");
        fprintf(stderr, "\n");
        
        if(error_code > -4 || error_code == -15|| (error_code < -17 && error_code > -20) ) exit(0);
    }
}
