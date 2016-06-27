#ifndef _COMMON_UI_H_  
#define _COMMON_UI_H_ 

#ifndef LWA_COLORKEY
#define LWA_COLORKEY 0x01
#endif
#ifndef LWA_ALPHA
#define LWA_ALPHA 0x02
#endif

// dwFlags = LWA_ALPHA 透明度 bAlpha: ＝0：整个窗口透明, =255 完全不透明 
// dwFlags = LWA_COLORKEY 指定颜色 crKey 透明
BOOL SetWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
void DealMSG();

#endif