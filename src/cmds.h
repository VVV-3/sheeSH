#ifndef CMDS_H
#define CMDS_H

void cd(int argc, char *argv[]);

void pwd();

void echo(int argc, char *argv[]);

void ls(int argc, char *argv[]);

void repeat(int argc, char *argv[]);

void pinfo(int argc, char *argv[]);

void history(int argc, char *argv[]);

void jobs(int argc, char *argv[]);

void sig(int argc, char *argv[]);

void bg(int argc, char *argv[]);

void fg(int argc, char *argv[]);

void replay(int argc, char *argv[]);

void fgExecute(int argc, char *argv[]);

void bgExecute(int argc, char *argv[]);

#endif