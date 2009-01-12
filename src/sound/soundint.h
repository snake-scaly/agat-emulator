#ifndef SOUNDINT_H
#define SOUNDINT_H

#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"


struct SOUNDPARAMS
{
	HWND	w;
	int 	freq;
	int	buflen;
};


#define SOUND_TOGGLE (-1)
struct SOUNDPROC
{
	void* (*sound_init)(struct SOUNDPARAMS*par);
	void  (*sound_term)(void*p);
	int   (*sound_data)(void*p, int d, long t, long f);
	void  (*sound_timer)(void*p);
};



#endif //SOUNDINT_H

