#ifndef MYTIME_H
#define MYTIME_H

//#define PLATFORM_LINUX

#ifdef PLATFORM_LINUX
#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#include <stropts.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#define LONGLONG long
long GetTickCount();
int _kbhit();
#else
#include "time.h"
#include <conio.h>    /*ÒýÓÃ¿ØÖÆÌ¨ÊäÈëÊä³öº¯Êý¿â*/
#include "Timer.h"//显示程序运行时间 所需头文件 
#endif

#endif
