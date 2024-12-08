

#ifndef MY_SHELL_H
#define MY_SHELL_H

#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

#define DANABOL_TOK_BUFSIZE 64
#define DANABOL_TOK_DELIM " \t\r\n\a"
#define DANABOL_RL_BUFSIZE 1024
#define BLOCK_SIZE 16384  // 16 KB
#define MICROSECONDS 1000000
#define NUMERAL_SYSTEM 10
#define LINE_SIZE 256
extern const char* const BuiltinStr[];

extern int DanabolHelp(void);
extern int DanabolExit(void);

extern int DanabolLaunch(char** args);

extern int DanabolExecute(char** args);

extern int DanabolLoop(void);

extern int DanabolIoLatRead(char** argv);
extern int DanabolLinReg(char** argv);
extern int DanabolBoth(char** argv);

#endif
