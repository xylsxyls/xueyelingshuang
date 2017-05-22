#pragma once
#include "CStorage/CStorageAPI.h"
#define GetError(error, T) errorInfo.storage[error].toValue<T>()

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