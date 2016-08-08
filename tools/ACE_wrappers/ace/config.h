#ifndef ACE_CONFIG_H_3478342782347892344762347942367435
#define ACE_CONFIG_H_3478342782347892344762347942367435

#ifdef WIN32
	#include "ace/config-win32.h" 
#elif ANDROID
	#include "ace/config-android.h"
	#define ACE_HAS_UCONTEXT_T
#elif IOS_S
	#include "ace/config-macosx-iOS-simulator.h"
#elif IOS_H
	#include "ace/config-macosx-iOS-hardware.h"
#else
	#include "ace/config-linux.h" 
#endif

#endif

