#pragma once
#ifdef _WIN32
#define _HttpServiceAPI
#endif
#include "HttpStatus.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpHandler.h"
#include "HttpServiceOptions.h"
#include "HttpService.h"
#include "HttpInterface.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"HttpServiced.lib")
#else
#pragma comment(lib,"HttpService.lib")
#endif
#endif