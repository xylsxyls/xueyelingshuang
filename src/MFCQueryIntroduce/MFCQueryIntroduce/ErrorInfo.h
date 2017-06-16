#pragma once
#include "CStorage/CStorageAPI.h"

class ErrorInfo{
public:
	ErrorInfo();
	CStorage storage;
};

enum{
    Success = 0,
    NameError = 1,
    BirthError,
    SexError,
    MarriageError,
    EducationError
};

extern ErrorInfo errorInfo;