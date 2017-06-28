/*
*Name: assert.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef ASSERT_H
#define ASSERT_H

extern void printf(const char *fmt, ...);

#define assert(c) \
do \
{ \
	if(!(c)) \
	{ \
		printf("assert(%s)failed!@%s of %s:%d.\n", \
			#c, __func__, __FILE__, __LINE__);\
		asm volatile("\t b .": : :"memory"); \
	} \
}while(0);


#endif


