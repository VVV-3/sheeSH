#include "sheesh.h"
#include "utils.h"
#include "cmds.h"

// Global Variables.
extern char USER_NAME[];
extern char MACHINE_NAME[];
extern char HOME_DIR[];
extern char PRE_DIR[];
extern bool FG_CONTROL;

extern struct process_data childProcesses[];
extern int child_process_count;


// Supplementary Functions

// Function to convert relative path tho absolute path.
char* expandPath(char* short_path)
{
    char* long_path = (char*) malloc((MAX_PATH_SIZE+1)*sizeof(char));
    sprintf(long_path, "%s%s", HOME_DIR, short_path + 1);
    return long_path;
}

// Function to find file permissions given file_stat.st_mode.
char* findFilePremissions(mode_t bits)
{
    char* permissions = (char*) malloc(11*sizeof(char));
    strcpy(permissions,"?---------");
    if      (S_ISREG(bits))  permissions[0] = '-';
    else if (S_ISDIR(bits))  permissions[0] = 'd';
    else if (S_ISLNK(bits))  permissions[0] = 'l';
    else if (S_ISBLK(bits))  permissions[0] = 'b';
    else if (S_ISCHR(bits))  permissions[0] = 'c';
    else if (S_ISSOCK(bits)) permissions[0] = 's';
    else if (S_ISFIFO(bits)) permissions[0] = 'f';

    if (bits & S_IRUSR) permissions[1] = 'r';
    if (bits & S_IWUSR) permissions[2] = 'w';
    if (bits & S_IXUSR) permissions[3] = 'x';
    if (bits & S_IRGRP) permissions[4] = 'r';
    if (bits & S_IWGRP) permissions[5] = 'w';
    if (bits & S_IXGRP) permissions[6] = 'x';
    if (bits & S_IROTH) permissions[7] = 'r';
    if (bits & S_IWOTH) permissions[8] = 'w';
    if (bits & S_IXOTH) permissions[9] = 'x';

    return permissions;
}

// Function to find file details given file stat, in long format.
char* findFileDetails(struct stat file_stat)
{
    char *output = (char*) malloc(2000*sizeof(char));
    strcpy(output,"");

    // Premissions
    strcat(output,findFilePremissions(file_stat.st_mode));

    // Links
    char *tp_string = (char*) malloc(100*sizeof(char));
    sprintf(tp_string," %ld ",file_stat.st_nlink);
    strcat(output,tp_string);

    // Username & Groupname.
    strcat(output, getpwuid(file_stat.st_uid)->pw_name);
    strcat(output," ");
    strcat(output, getgrgid(file_stat.st_gid)->gr_name);

    // File Size
    sprintf(tp_string," %lu ",file_stat.st_size);
    strcat(output,tp_string);

    // Time of modification.
    strcat(output, asctime(localtime(&(file_stat.st_mtime))));
    output[strlen(output) - 9] = '\0';
    strcat(output," ");

    return output;
}

// Function to concatenate command arguments
char* concatArgs(int argc, char* argv[])
{
    char* command = (char*) malloc((MAX_INPUT_SIZE+5)*sizeof(char));
    strcpy(command, argv[0]);
    for(int i=1; i<argc; i++)
    {
        strcat(command, " ");
        strcat(command, argv[i]);
    }
    return command;
}


//Command Functions

// Function to execute "cd" command.
void cd(int argc, char *argv[])
{
    if(argc > 2) handleErrors(-6);
    else
    {
        // Converting relative path to absolute.
        if(argc > 1 && argv[1][0] == '~') argv[1] = expandPath(argv[1]);

        char cur_dir[MAX_PATH_SIZE+1];
        if (getcwd(cur_dir, MAX_PATH_SIZE+1) == NULL) handleErrors(-5);
        char new_dir[MAX_PATH_SIZE+1] = "";

        if      (argc == 1)             strcpy(new_dir, HOME_DIR);
        else if (!strcmp(argv[1],"."))  strcpy(new_dir, cur_dir);
        else if (!strcmp(argv[1],"-"))  strcpy(new_dir, PRE_DIR);
        else                            strcpy(new_dir, argv[1]);

        strcpy(PRE_DIR, cur_dir);
        if(chdir(new_dir) == -1) handleErrors(-8);
    }
}

// Function to execute "pwd" command.
void pwd()
{
    char cur_dir[MAX_NAME_SIZE];
    if (getcwd(cur_dir, MAX_PATH_SIZE) == NULL) handleErrors(-5);
    printf("%s\n",cur_dir);
}

// Function to execute "echo" command.
void echo(int argc, char *argv[])
{
    for(int i=1; i<argc; i++) printf("%s ",argv[i]);
    printf("\n");
}

// Function to execute "ls" command.
void ls(int argc, char *argv[])
{
    bool flag_a = false;
    bool flag_l = false;
    int no_of_inputs = 0;
    char* inputs[MAX_NO_WORDS_IN_COMMAND] = {NULL};

    // Handling Flags.
    for(int i=1; i<argc; i++)
    {
        if(argv[i][0] == '-')
        {
            int flag_len = strlen(argv[i]);
            for(int j=1;j<flag_len;j++)
                if      (argv[i][j] == 'a') flag_a = true;
                else if (argv[i][j] == 'l') flag_l = true;
        }
        else inputs[no_of_inputs++] = argv[i];
    }

    char tp[1] = ".";
    if (no_of_inputs == 0) inputs[no_of_inputs++] = tp;
    for(int i=0; i<no_of_inputs; i++)
    {
        if(i>0) printf("\n");
        if(no_of_inputs > 1) printf("%s:\n",inputs[i]);

        DIR *dir_stream;
        if(inputs[i][0] == '~') inputs[i] = expandPath(inputs[i]);
        dir_stream = opendir(inputs[i]);
        if(dir_stream == NULL) handleErrors(-9);
        else
        {
            // Reading files and sub directories in the directory.
            struct dirent *sub_dir = readdir(dir_stream);
            int total = 0;
            char *output = (char*) malloc(5000*sizeof(char));
            strcpy(output,"");
            while(sub_dir != NULL)
            {
                // Long format as -l flag is active.
                if( flag_l && (flag_a || sub_dir->d_name[0] != '.') )
                {
                    char *file_path = (char*) malloc(MAX_PATH_SIZE*sizeof(char));
                    sprintf(file_path,"%s/%s", inputs[i], sub_dir->d_name);
                    struct stat file_stat;
                    if(lstat(file_path, &file_stat)) handleErrors(-13);
                    else
                    {
                        total += file_stat.st_blocks;
                        strcat(output, findFileDetails(file_stat));                        
                        strcat(output,sub_dir->d_name);
                        strcat(output,"\n");
                    }
                    free(file_path);
                }
                // Short format
                else if(flag_a || sub_dir->d_name[0] != '.')
                {
                    strcat(output, sub_dir->d_name);
                    strcat(output, "  ");
                }    

                sub_dir = readdir(dir_stream);
            }

            // Printing output
            if(flag_l) printf("total %d\n",total/2);
            else strcat(output,"\n");
            printf("%s",output);
            free(output);
        }
        closedir(dir_stream);
    }
}

// Function to execute "repeat" command.
void repeat(int argc, char *argv[])
{
    if (argc < 3) handleErrors(-12);
    else
    {
        int repeat_count = atoi(argv[1]);
        for(int i=0; i< repeat_count; i++) handleCommand(argc-2, &argv[2]);
    }
}

// Function to execute "history" command.
void pinfo(int argc, char *argv[])
{
    // Getting pid.
    pid_t pid;
    if(argc > 1) pid = atoi(argv[1]);
    else         pid = getpid();

    // Opening stat file.
    char file_path[25];
    sprintf(file_path,"/proc/%d/stat", pid);
    char file_stats_str[1000] = {'\0'};
    FILE* proc;
    proc = fopen(file_path, "r");
    if( proc == NULL) handleErrors(-13);
    else
    {
        // Reading file and breaking it into arguments.
        fread(file_stats_str, 1000, 1, proc);
        fclose(proc);
        char* file_stats[100] = {NULL}; int i=0;
        file_stats[i] = strtok(file_stats_str, " ");
        while(file_stats[i] != NULL) file_stats[++i] = strtok(NULL, " ");

        // Printing Process Info.
        printf("pid -- %d\n",pid);
        printf("Process Status -- %s%s\n", file_stats[2], pid == getpid() ? "+":"");
        printf("memory -- %s\n",file_stats[22]);

        // Finding executable path and converitng into relative format.
        sprintf(file_path, "/proc/%d/exe", (int)pid);
        char exe_path[MAX_PATH_SIZE + 1];
        memset(exe_path, 0, MAX_PATH_SIZE);
        int readStat = readlink(file_path, exe_path, PATH_MAX);
        int home_dir_len = strlen(HOME_DIR);
        int exe_path_len = strlen(exe_path);
        if (strncmp(HOME_DIR, exe_path, home_dir_len) == 0)
        {
            char tmp_path[MAX_PATH_SIZE];
            strcpy(tmp_path,exe_path);
            exe_path[0] = '~';
            for(int i = home_dir_len; i <= exe_path_len; i++) exe_path[ i - home_dir_len + 1] = tmp_path[i];
        }
        printf("Executable Path -- %s\n", readStat == -1 ? "does not exist" : exe_path);
    }
}

// Function to execute "history" command.
void history(int argc, char *argv[])
{
    // Reading from file into history.
    char history[21*MAX_INPUT_SIZE];
    FILE* shell_history;
    shell_history = fopen("/var/tmp/sheesh_history.txt", "r");
    fread(history, 1, 21*MAX_INPUT_SIZE, shell_history);
    fclose(shell_history);

    // Printing nessasary lines.
    if (argc == 1) printf("%s",history);
    else if(atoi(argv[1]) > 20) handleErrors(-14);
    else
    {
        int i, no_of_cmds = atoi(argv[1]);
        for(i=strlen(history) - 1; i>=0; i--)
        {
            if(history[i] == '\n') no_of_cmds--;
            if(no_of_cmds < 0) break;
        }
        printf("%s",history+i+1);
    }
}

// Function to execute "jobs" command.
void jobs(int argc, char *argv[])
{
    bool flag_r = false;
    bool flag_s = false;

    // Handling Flags.
    for(int i=1; i<argc; i++)
    {
        if(argv[i][0] == '-')
        {
            int flag_len = strlen(argv[i]);
            for(int j=1;j<flag_len;j++)
                if      (argv[i][j] == 'r') flag_r = true;
                else if (argv[i][j] == 's') flag_s = true;
        }
    }
    if( !flag_r && !flag_s) { flag_r = true; flag_s = true;}

    char file_path[25];
    char stat_string[100];
    for(int i=0; i<child_process_count; i++)
    {
        sprintf(file_path, "/proc/%d/stat", childProcesses[i].pid );
        FILE* proc_stat = fopen(file_path, "r");
        if( proc_stat == NULL) handleErrors(-13);
        else
        {
            fread(stat_string, 1, 100, proc_stat);
            fclose(proc_stat);

            char* tp;
            tp = strtok(stat_string, " ");
            tp = strtok(NULL, " ");
            tp = strtok(NULL, " ");
            char run_state = tp[0];
            if      (run_state == 'S' && flag_r) printf("[%d] Running %s [%d]\n", i+1, childProcesses[i].pname, childProcesses[i].pid);
            else if (run_state == 'T' && flag_s) printf("[%d] Stopped %s [%d]\n", i+1, childProcesses[i].pname, childProcesses[i].pid);
        }
    }
}

// Function to execute "sig" command.
void sig(int argc, char *argv[])
{
    if( argc < 3) { handleErrors(-12); return;}

    int job_no, signal_no;
    job_no = atoi(argv[1]);
    if( job_no > child_process_count) { handleErrors(-16); return;}
    signal_no = atoi(argv[2]);
    kill(childProcesses[job_no-1].pid, signal_no);
}

// Function to execute "bg" command.
void bg(int argc, char *argv[])
{
    if( argc < 2) { handleErrors(-12); return;}

    int job_no;
    job_no = atoi(argv[1]);
    if( job_no > child_process_count) { handleErrors(-16); return;}
    kill(childProcesses[job_no-1].pid, 18);
}

// Function to execute "fg" command.
void fg(int argc, char *argv[])
{
    bg(argc, argv);

    struct process_data tp = childProcesses[ atoi(argv[1])-1 ];
    for(int i=0, found=0; i<child_process_count; i++)
        if(found) childProcesses[i-1] = childProcesses[i];
        else if(tp.pid == childProcesses[i].pid)  found=1;
    child_process_count--;
    FG_CONTROL = false;
    
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    tcsetpgrp(STDIN_FD, getpgid(tp.pid));
    int status;
    if(waitpid(tp.pid, &status, WUNTRACED) > 0)
    {
        if(WIFSTOPPED(status))
        {
            childProcesses[child_process_count].pid = tp.pid;
            strcpy(childProcesses[child_process_count].pname, tp.pname);
            child_process_count++;
            sortChildProcesses();
        }
    }
    tcsetpgrp(STDIN_FD, getpgid(0));
    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);

    FG_CONTROL = true;
}

// Function to execute "replay" command.
void replay(int argc, char *argv[])
{
    if( argc < 7) { handleErrors(-12); return;}
    int command=0,period=0,interval=0;
    for(int i=1; i<argc; i++)
    {
        if      (!strcmp(argv[i],"-command"))  { command = i+1;              argv[i]=NULL; }
        else if (!strcmp(argv[i],"-period"))   { period= atoi(argv[i+1]);    argv[i]=NULL; }
        else if (!strcmp(argv[i],"-interval")) { interval= atoi(argv[i+1]);  argv[i]=NULL; }
    }
    argc=0;
    for(int i=command;argv[i]!=NULL;i++) argc++;

    if(!period || !interval) { handleErrors(-20); return;}
    int i=0;
    while(i<=period)
    {
        handleCommand(argc,argv+command);
        if(period - i > interval) sleep(interval);
        else                      sleep(period - i);
        i+=interval;
    }
}

// Function to execute foreground process.
void fgExecute(int argc, char *argv[])
{   
    // Converting realtive path to absolute.
    if(argv[0][0] == '~') argv[0] = expandPath(argv[0]);

    // Creating a child process.
    pid_t pid = fork();
    
    if (pid < 0) { handleErrors(-10); return; }
    
    // Foreground control lost.
    FG_CONTROL = false;

    if (pid == 0)
    {
        if (execvp(argv[0], argv)) handleErrors(-15);
    }
    else
    {

        // Waiting till child process finishes.
        int status;
        if(waitpid(pid, &status, WUNTRACED) > 0)
        {
            // handling ctrl Z.
            if(WIFSTOPPED(status))
            {
                childProcesses[child_process_count].pid = pid;
                strcpy(childProcesses[child_process_count].pname, concatArgs(argc, argv));
                child_process_count++;
                sortChildProcesses();
            }
        }

        FG_CONTROL = true;
    }
}

// Function to execute background processes.
void bgExecute(int argc, char *argv[])
{   
    // Converting realtive path to absolute.
    if(argv[0][0] == '~') argv[0] = expandPath(argv[0]);

    if( child_process_count >= MAX_CHILD_PROCESS_COUNT) handleErrors(-11);
    else
    {
        // Creating a child process.
        pid_t pid = fork();
        if (pid < 0) handleErrors(-10);
        else if (pid == 0)
        {
            setpgid(0, 0);
            if (execvp(argv[0], argv)) handleErrors(-15);
        }
        else
        {
            // Printing the child process id and adding pid and pname to the child process array.
            printf("%d\n", pid);
            childProcesses[child_process_count].pid = pid;
            strcpy(childProcesses[child_process_count].pname, concatArgs(argc, argv));
            child_process_count++;
            sortChildProcesses();
        }
    }   
}
