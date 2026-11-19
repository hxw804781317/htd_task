#ifndef _HTD_CONFIG_PROJECT_H__
#define _HTD_CONFIG_PROJECT_H__

#include "version.h"

#if defined(_MSC_VER)
#define _CRT_ERRNO_DEFINED  /* errno macro redefinition */
#define _CRT_DECLARE_NONSTDC_NAMES 0 /* avoid to conflict with the inherent STDC in VS */

/* disable some warning in MSC */
#pragma warning(disable:4273)   /* to ignore: warning C4273: inconsistent dll linkage */
#pragma warning(disable:4312)   /* to ignore: warning C4312: 'type cast'*/
#pragma warning(disable:4311)   /* to ignore: warning C4311: 'type cast' : pointer truncation from 'short *__w64 ' to 'long' */
#pragma warning(disable:4996)   /* to ignore: warning C4996: The POSIX name for this item is deprecated. */
#pragma warning(disable:4267)   /* to ignore: warning C4267: */
#pragma warning(disable:4244)   /* to ignore: warning C4244: */

#endif /* end of _MSC_VER */
#endif

