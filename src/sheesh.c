#include "sheesh.h"
#include "utils.h"
#include "cmds.h"

// Global Variables.
char USER_NAME[MAX_NAME_SIZE+1];
char MACHINE_NAME[MAX_NAME_SIZE+1];
char HOME_DIR[MAX_PATH_SIZE+1];
char PRE_DIR[MAX_PATH_SIZE+1];
bool FG_CONTROL = true;

struct process_data childProcesses[MAX_CHILD_PROCESS_COUNT + 5];
int child_process_count = 0;

// Shell.
void startSheesh()
{
    // Clearing terminal and initializing shell details.
    clearTerminal();
    initInfo();

    while(1)
    {
        // Showing Prompt.
        displayPrompt();

        // Getting input from user and execute it.
        char input_string[MAX_INPUT_SIZE];
        if (fgets(input_string, MAX_INPUT_SIZE, stdin) == 0) break;
        else handleInput(input_string);
    }
}

// Main Function.
int main()
{
    startSheesh();
    return 0;
}
