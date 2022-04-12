#ifndef UTILS_H
#define UTILS_H

void initInfo();

void displayPrompt();

void handleInput(char* input);

void handleCommand(int no_of_words, char** command);

void clearTerminal();

void handleErrors(int error_code);

void sortChildProcesses();

#endif