#ifndef __common_define_20180302155347_h__
#define __common_define_20180302155347_h__

#include <stdbool.h>
#include <inttypes.h>

#define logError   printf
#define logDebug   printf
#define logInfo    printf

#define get_current_time() time(NULL)

#ifndef byte
#define byte signed char
#endif

#ifndef ubyte
#define ubyte unsigned char
#endif

#define STRERROR(no) (strerror(no) != NULL ? strerror(no) : "Unkown error")
#define MEM_ALIGN(x)  (((x) + 7) & (~7))


#endif // __common_define_20180302155347_h__
