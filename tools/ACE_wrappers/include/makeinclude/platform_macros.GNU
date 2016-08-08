ifeq ($(platform),android)
	include $(ACE_ROOT)/include/makeinclude/platform_android.GNU 
else ifeq ($(platform),ios)
	include $(ACE_ROOT)/include/makeinclude/platform_macosx_iOS.GNU 
else
	include $(ACE_ROOT)/include/makeinclude/platform_linux.GNU
endif