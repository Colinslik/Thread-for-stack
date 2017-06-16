#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

//sleep for win32 and linux
#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS__) || defined(__TOS_WIN__)

#include <windows.h>

void delay(unsigned long ms)
{
	Sleep(ms);
}

#else  /* presume POSIX */

#include <unistd.h>
#include <stdlib.h>

void delay(unsigned long ms)
{
	usleep(ms * 1000);
}

#endif 

//Exception handle 2

bool consoleHandler(int signal){
//#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS__) || defined(__TOS_WIN__)
//	if (signal == CTRL_C_EVENT){
//#else
	if (signal == SIGINT){
//#endif
		printf("Signal caught2\n");
		//exit(0);
	}
	printf("signal:%d  SIGINT: %d\n",signal, SIGINT);
	exit(1);
}

//Exception handle 1

void CtrlHandler(int sig){
	printf("Signal caught\n");
	consoleHandler(sig);
}

#endif //__SAMPLE_H__

//pair struct
typedef struct pair{
	char* key;
	char* value;
	struct pair *next;
} my_pair;
