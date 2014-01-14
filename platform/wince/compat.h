#ifndef _COMPAT_H
#define _COMPAT_H

#if(defined __cplusplus)
extern "C"
{
#endif

char *strerror(int errno);

#if(defined __cplusplus)
}
#endif

#endif /* _COMAPT_H */
