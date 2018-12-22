/*
 * Author Kevin He
 */


#ifndef UTIL__H
#define UTIL__H

#include <stdio.h>


/* Give some colors to the output */
#define NRM  "\x1B[0m"
#define RED  "\x1B[31m"
#define GREEN  "\x1B[32m"
#define YEL  "\x1B[33m"
#define BLUE  "\x1B[34m"
#define MAG  "\x1B[35m"
#define CYN  "\x1B[36m"
#define WHT  "\x1B[37m"
#define END "\033[0m"
/* Unicode */
#define UNICODE_LINE	"\xe2\x94\x80"
#define UNICODE_CHECK	"\xe2\x9c\x93"
#define UNICODE_X	    "x"


#define ebug(fmt,args...) printf("[%d] ERR "fmt,__LINE__,##args)
#define ibug(fmt,args...) printf(fmt,##args)
#define blue_bug(fmt,args...) printf(BLUE fmt END,##args)
#define red_bug(fmt,args...) printf(RED fmt END,##args)
#define green_bug(fmt,args...) printf(GREEN fmt END,##args)

#ifdef DEBUG
#define debug(fmt,args...) printf("[%d] "fmt,__LINE__,##args)
#else
#define debug(...) 
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif


#endif
