#ifndef _COMPAT_H
#define _COMPAT_H

#if(defined __cplusplus)
extern "C"
{
#endif

#ifdef WINCE
char *strerror(int errno);
#endif

#ifndef WINCE
#define DEBUGMSG(cond,printf_exp) ((void)0)
#endif

#if(defined __cplusplus)
}
#endif

#endif /* _COMAPT_H */
