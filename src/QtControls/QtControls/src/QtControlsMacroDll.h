#pragma once

#define QtControlsAPI _declspec(dllimport)

#ifdef _DEBUG
#pragma comment(lib, "QtControlsd.lib")
#else
#pragma comment(lib, "QtControls.lib")
#endif