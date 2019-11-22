#include <SDKDDKVer.h>
#include "Cdmsoft.h"
#include "DmSoft.h"
#include <stdint.h>
#include <io.h>

Cdmsoft dmSoft;
DmSoft dmsoft;
DmSoft::DmSoft(){
	char szFilePath[1024] = {};
	::GetModuleFileNameA(NULL, szFilePath, 1024);
	std::string dmPath = szFilePath;
	dmPath = dmPath.substr(0, (int32_t)dmPath.find_last_of("/\\") + 1) + "dm.dll";
	if (::_access(dmPath.c_str(), 0) == 0)
	{
		WinExec(("regsvr32 /s " + dmPath).c_str(), SW_HIDE);
		//?注册
		CoInitialize(NULL);
		CLSID clsid;
		//?利用“根名称.类名”获取CLSID，&就是把指针给函数，也就是传址
		HRESULT hr = CLSIDFromProgID(OLESTR("dm.dmsoft"), &clsid);
		dmSoft.CreateDispatch(clsid);
	}
}

string DmSoft::Ver(){
	return (LPSTR)(LPCTSTR)dmSoft.Ver();
}

long DmSoft::SetPath(LPCTSTR path){
	return dmSoft.SetPath(path);
}

string DmSoft::Ocr(long x1, long y1, long x2, long y2, LPCTSTR color, double sim){
	return (LPSTR)(LPCTSTR)dmSoft.Ocr(x1, y1, x2, y2, color, sim);
}

long DmSoft::FindStr(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, VARIANT* x, VARIANT* y){
	return dmSoft.FindStr(x1, y1, x2, y2, str, color, sim, x, y);
}

long DmSoft::GetResultCount(LPCTSTR str){
	return dmSoft.GetResultCount(str);
}

long DmSoft::GetResultPos(LPCTSTR str, long index, VARIANT* x, VARIANT* y){
	return dmSoft.GetResultPos(str, index, x, y);
}

long DmSoft::DmStrStr(LPCTSTR s, LPCTSTR str){
	return dmSoft.DmStrStr(s, str);
}

long DmSoft::SendCommand(LPCTSTR cmd){
	return dmSoft.SendCommand(cmd);
}

long DmSoft::UseDict(long index){
	return dmSoft.UseDict(index);
}

string DmSoft::GetBasePath(){
	return (LPSTR)(LPCTSTR)dmSoft.GetBasePath();
}

long DmSoft::SetDictPwd(LPCTSTR pwd){
	return dmSoft.SetDictPwd(pwd);
}

string DmSoft::OcrInFile(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR color, double sim){
	return (LPSTR)(LPCTSTR)dmSoft.OcrInFile(x1, y1, x2, y2, pic_name, color, sim);
}

long DmSoft::Capture(long x1, long y1, long x2, long y2, LPCTSTR file){
	return dmSoft.Capture(x1, y1, x2, y2, file);
}

long DmSoft::KeyPress(long vk){
	return dmSoft.KeyPress(vk);
}

long DmSoft::KeyDown(long vk){
	return dmSoft.KeyDown(vk);
}

long DmSoft::KeyUp(long vk){
	return dmSoft.KeyUp(vk);
}

long DmSoft::LeftClick(){
	return dmSoft.LeftClick();
}

long DmSoft::RightClick(){
	return dmSoft.RightClick();
}

long DmSoft::MiddleClick(){
	return dmSoft.MiddleClick();
}

long DmSoft::LeftDoubleClick(){
	return dmSoft.LeftDoubleClick();
}

long DmSoft::LeftDown(){
	return dmSoft.LeftDown();
}

long DmSoft::LeftUp(){
	return dmSoft.LeftUp();
}

long DmSoft::RightDown(){
	return dmSoft.RightDown();
}

long DmSoft::RightUp(){
	return dmSoft.RightUp();
}

long DmSoft::MoveTo(long x, long y){
	return dmSoft.MoveTo(x, y);
}

long DmSoft::MoveR(long rx, long ry){
	return dmSoft.MoveR(rx, ry);
}

string DmSoft::GetColor(long x, long y){
	return (LPSTR)(LPCTSTR)dmSoft.GetColor(x, y);
}

string DmSoft::GetColorBGR(long x, long y){
	return (LPSTR)(LPCTSTR)dmSoft.GetColorBGR(x, y);
}

string DmSoft::RGB2BGR(LPCTSTR rgb_color){
	return (LPSTR)(LPCTSTR)dmSoft.RGB2BGR(rgb_color);
}

string DmSoft::BGR2RGB(LPCTSTR bgr_color){
	return (LPSTR)(LPCTSTR)dmSoft.BGR2RGB(bgr_color);
}

long DmSoft::UnBindWindow(){
	return dmSoft.UnBindWindow();
}

long DmSoft::CmpColor(long x, long y, LPCTSTR color, double sim){
	return dmSoft.CmpColor(x, y, color, sim);
}

long DmSoft::ClientToScreen(long hwnd, VARIANT* x, VARIANT* y){
	return dmSoft.ClientToScreen(hwnd, x, y);
}

long DmSoft::ScreenToClient(long hwnd, VARIANT* x, VARIANT* y){
	return dmSoft.ScreenToClient(hwnd, x, y);
}

long DmSoft::ShowScrMsg(long x1, long y1, long x2, long y2, LPCTSTR msg, LPCTSTR color){
	return dmSoft.ShowScrMsg(x1, y1, x2, y2, msg, color);
}

long DmSoft::SetMinRowGap(long row_gap){
	return dmSoft.SetMinRowGap(row_gap);
}

long DmSoft::SetMinColGap(long col_gap){
	return dmSoft.SetMinColGap(col_gap);
}

long DmSoft::FindColor(long x1, long y1, long x2, long y2, LPCTSTR color, double sim, long dir, VARIANT* x, VARIANT* y){
	return dmSoft.FindColor(x1, y1, x2, y2, color, sim, dir, x, y);
}

string DmSoft::FindColorEx(long x1, long y1, long x2, long y2, LPCTSTR color, double sim, long dir){
	return (LPSTR)(LPCTSTR)dmSoft.FindColorEx(x1, y1, x2, y2, color, sim, dir);
}

long DmSoft::SetWordLineHeight(long line_height){
	return dmSoft.SetWordLineHeight(line_height);
}

long DmSoft::SetWordGap(long word_gap){
	return dmSoft.SetWordGap(word_gap);
}

long DmSoft::SetRowGapNoDict(long row_gap){
	return dmSoft.SetRowGapNoDict(row_gap);
}

long DmSoft::SetColGapNoDict(long col_gap){
	return dmSoft.SetColGapNoDict(col_gap);
}

long DmSoft::SetWordLineHeightNoDict(long line_height){
	return dmSoft.SetWordLineHeightNoDict(line_height);
}

long DmSoft::SetWordGapNoDict(long word_gap){
	return dmSoft.SetWordGapNoDict(word_gap);
}

long DmSoft::GetWordResultCount(LPCTSTR str){
	return dmSoft.GetWordResultCount(str);
}

long DmSoft::GetWordResultPos(LPCTSTR str, long index, VARIANT* x, VARIANT* y){
	return dmSoft.GetWordResultPos(str, index, x, y);
}

string DmSoft::GetWordResultStr(LPCTSTR str, long index){
	return (LPSTR)(LPCTSTR)dmSoft.GetWordResultStr(str, index);
}

string DmSoft::GetWords(long x1, long y1, long x2, long y2, LPCTSTR color, double sim){
	return (LPSTR)(LPCTSTR)dmSoft.GetWords(x1, y1, x2, y2, color, sim);
}

string DmSoft::GetWordsNoDict(long x1, long y1, long x2, long y2, LPCTSTR color){
	return (LPSTR)(LPCTSTR)dmSoft.GetWordsNoDict(x1, y1, x2, y2, color);
}

long DmSoft::SetShowErrorMsg(long show){
	return dmSoft.SetShowErrorMsg(show);
}

long DmSoft::GetClientSize(long hwnd, VARIANT* width, VARIANT* height){
	return dmSoft.GetClientSize(hwnd, width, height);
}

long DmSoft::MoveWindow(long hwnd, long x, long y){
	return dmSoft.MoveWindow(hwnd, x, y);
}

string DmSoft::GetColorHSV(long x, long y){
	return (LPSTR)(LPCTSTR)dmSoft.GetColorHSV(x, y);
}

string DmSoft::GetAveRGB(long x1, long y1, long x2, long y2){
	return (LPSTR)(LPCTSTR)dmSoft.GetAveRGB(x1, y1, x2, y2);
}

string DmSoft::GetAveHSV(long x1, long y1, long x2, long y2){
	return (LPSTR)(LPCTSTR)dmSoft.GetAveHSV(x1, y1, x2, y2);
}

long DmSoft::GetForegroundWindow(){
	return dmSoft.GetForegroundWindow();
}

long DmSoft::GetForegroundFocus(){
	return dmSoft.GetForegroundFocus();
}

long DmSoft::GetMousePointWindow(){
	return dmSoft.GetMousePointWindow();
}

long DmSoft::GetPointWindow(long x, long y){
	return dmSoft.GetPointWindow(x, y);
}

string DmSoft::EnumWindow(long parent, LPCTSTR title, LPCTSTR class_name, long filter){
	return (LPSTR)(LPCTSTR)dmSoft.EnumWindow(parent, title, class_name, filter);
}

long DmSoft::GetWindowState(long hwnd, long flag){
	return dmSoft.GetWindowState(hwnd, flag);
}

long DmSoft::GetWindow(long hwnd, long flag){
	return dmSoft.GetWindow(hwnd, flag);
}

long DmSoft::GetSpecialWindow(long flag){
	return dmSoft.GetSpecialWindow(flag);
}

long DmSoft::DmSetWindowText(long hwnd, LPCTSTR text){
	return dmSoft.DmSetWindowText(hwnd, text);
}

long DmSoft::SetWindowSize(long hwnd, long width, long height){
	return dmSoft.SetWindowSize(hwnd, width, height);
}

long DmSoft::GetWindowRect(long hwnd, VARIANT* x1, VARIANT* y1, VARIANT* x2, VARIANT* y2){
	return dmSoft.GetWindowRect(hwnd, x1, y1, x2, y2);
}

string DmSoft::GetWindowTitle(long hwnd){
	return (LPSTR)(LPCTSTR)dmSoft.GetWindowTitle(hwnd);
}

string DmSoft::GetWindowClass(long hwnd){
	return (LPSTR)(LPCTSTR)dmSoft.GetWindowClass(hwnd);
}

long DmSoft::SetWindowState(long hwnd, long flag){
	return dmSoft.SetWindowState(hwnd, flag);
}

long DmSoft::CreateFoobarRect(long hwnd, long x, long y, long w, long h){
	return dmSoft.CreateFoobarRect(hwnd, x, y, w, h);
}

long DmSoft::CreateFoobarRoundRect(long hwnd, long x, long y, long w, long h, long rw, long rh){
	return dmSoft.CreateFoobarRoundRect(hwnd, x, y, w, h, rw, rh);
}

long DmSoft::CreateFoobarEllipse(long hwnd, long x, long y, long w, long h){
	return dmSoft.CreateFoobarEllipse(hwnd, x, y, w, h);
}

long DmSoft::CreateFoobarCustom(long hwnd, long x, long y, LPCTSTR pic, LPCTSTR trans_color, double sim){
	return dmSoft.CreateFoobarCustom(hwnd, x, y, pic, trans_color, sim);
}

long DmSoft::FoobarFillRect(long hwnd, long x1, long y1, long x2, long y2, LPCTSTR color){
	return dmSoft.FoobarFillRect(hwnd, x1, y1, x2, y2, color);
}

long DmSoft::FoobarDrawText(long hwnd, long x, long y, long w, long h, LPCTSTR text, LPCTSTR color, long align){
	return dmSoft.FoobarDrawText(hwnd, x, y, w, h, text, color, align);
}

long DmSoft::FoobarDrawPic(long hwnd, long x, long y, LPCTSTR pic, LPCTSTR trans_color){
	return dmSoft.FoobarDrawPic(hwnd, x, y, pic, trans_color);
}

long DmSoft::FoobarUpdate(long hwnd){
	return dmSoft.FoobarUpdate(hwnd);
}

long DmSoft::FoobarLock(long hwnd){
	return dmSoft.FoobarLock(hwnd);
}

long DmSoft::FoobarUnlock(long hwnd){
	return dmSoft.FoobarUnlock(hwnd);
}

long DmSoft::FoobarSetFont(long hwnd, LPCTSTR font_name, long size, long flag){
	return dmSoft.FoobarSetFont(hwnd, font_name, size, flag);
}

long DmSoft::FoobarTextRect(long hwnd, long x, long y, long w, long h){
	return dmSoft.FoobarTextRect(hwnd, x, y, w, h);
}

long DmSoft::FoobarPrintText(long hwnd, LPCTSTR text, LPCTSTR color){
	return dmSoft.FoobarPrintText(hwnd, text, color);
}

long DmSoft::FoobarClearText(long hwnd){
	return dmSoft.FoobarClearText(hwnd);
}

long DmSoft::FoobarTextLineGap(long hwnd, long gap){
	return dmSoft.FoobarTextLineGap(hwnd, gap);
}

long DmSoft::Play(LPCTSTR file){
	return dmSoft.Play(file);
}

long DmSoft::FaqCapture(long x1, long y1, long x2, long y2, long quality, long delay, long time){
	return dmSoft.FaqCapture(x1, y1, x2, y2, quality, delay, time);
}

long DmSoft::FaqRelease(long handle){
	return dmSoft.FaqRelease(handle);
}

string DmSoft::FaqSend(LPCTSTR server, long handle, long request_type, long time_out){
	return (LPSTR)(LPCTSTR)dmSoft.FaqSend(server, handle, request_type, time_out);
}

long DmSoft::Beep(long fre, long delay){
	return dmSoft.Beep(fre, delay);
}

long DmSoft::FoobarClose(long hwnd){
	return dmSoft.FoobarClose(hwnd);
}

long DmSoft::MoveDD(long dx, long dy){
	return dmSoft.MoveDD(dx, dy);
}

long DmSoft::FaqGetSize(long handle){
	return dmSoft.FaqGetSize(handle);
}

long DmSoft::LoadPic(LPCTSTR pic_name){
	return dmSoft.LoadPic(pic_name);
}

long DmSoft::FreePic(LPCTSTR pic_name){
	return dmSoft.FreePic(pic_name);
}

long DmSoft::GetScreenData(long x1, long y1, long x2, long y2){
	return dmSoft.GetScreenData(x1, y1, x2, y2);
}

long DmSoft::FreeScreenData(long handle){
	return dmSoft.FreeScreenData(handle);
}

long DmSoft::WheelUp(){
	return dmSoft.WheelUp();
}

long DmSoft::WheelDown(){
	return dmSoft.WheelDown();
}

long DmSoft::SetMouseDelay(LPCTSTR type, long delay){
	return dmSoft.SetMouseDelay(type, delay);
}

long DmSoft::SetKeypadDelay(LPCTSTR type, long delay){
	return dmSoft.SetKeypadDelay(type, delay);
}

string DmSoft::GetEnv(long index, LPCTSTR name){
	return (LPSTR)(LPCTSTR)dmSoft.GetEnv(index, name);
}

long DmSoft::SetEnv(long index, LPCTSTR name, LPCTSTR value){
	return dmSoft.SetEnv(index, name, value);
}

long DmSoft::SendString(long hwnd, LPCTSTR str){
	return dmSoft.SendString(hwnd, str);
}

long DmSoft::DelEnv(long index, LPCTSTR name){
	return dmSoft.DelEnv(index, name);
}

string DmSoft::GetPath(){
	return (LPSTR)(LPCTSTR)dmSoft.GetPath();
}

long DmSoft::SetDict(long index, LPCTSTR dict_name){
	return dmSoft.SetDict(index, dict_name);
}

long DmSoft::FindPic(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR delta_color, double sim, long dir, VARIANT* x, VARIANT* y){
	return dmSoft.FindPic(x1, y1, x2, y2, pic_name, delta_color, sim, dir, x, y);
}

string DmSoft::FindPicEx(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR delta_color, double sim, long dir){
	return (LPSTR)(LPCTSTR)dmSoft.FindPicEx(x1, y1, x2, y2, pic_name, delta_color, sim, dir);
}

long DmSoft::SetClientSize(long hwnd, long width, long height){
	return dmSoft.SetClientSize(hwnd, width, height);
}

long DmSoft::ReadInt(long hwnd, LPCTSTR addr, long type){
	return dmSoft.ReadInt(hwnd, addr, type);
}

float DmSoft::ReadFloat(long hwnd, LPCTSTR addr){
	return dmSoft.ReadFloat(hwnd, addr);
}

double DmSoft::ReadDouble(long hwnd, LPCTSTR addr){
	return dmSoft.ReadDouble(hwnd, addr);
}

string DmSoft::FindInt(long hwnd, LPCTSTR addr_range, long int_value_min, long int_value_max, long type){
	return (LPSTR)(LPCTSTR)dmSoft.FindInt(hwnd, addr_range, int_value_min, int_value_max, type);
}

string DmSoft::FindFloat(long hwnd, LPCTSTR addr_range, float float_value_min, float float_value_max){
	return (LPSTR)(LPCTSTR)dmSoft.FindFloat(hwnd, addr_range, float_value_min, float_value_max);
}

string DmSoft::FindDouble(long hwnd, LPCTSTR addr_range, double double_value_min, double double_value_max){
	return (LPSTR)(LPCTSTR)dmSoft.FindDouble(hwnd, addr_range, double_value_min, double_value_max);
}

string DmSoft::FindString(long hwnd, LPCTSTR addr_range, LPCTSTR string_value, long type){
	return (LPSTR)(LPCTSTR)dmSoft.FindString(hwnd, addr_range, string_value, type);
}

long DmSoft::GetModuleBaseAddr(long hwnd, LPCTSTR module_name){
	return dmSoft.GetModuleBaseAddr(hwnd, module_name);
}

string DmSoft::MoveToEx(long x, long y, long w, long h){
	return (LPSTR)(LPCTSTR)dmSoft.MoveToEx(x, y, w, h);
}

string DmSoft::MatchPicName(LPCTSTR pic_name){
	return (LPSTR)(LPCTSTR)dmSoft.MatchPicName(pic_name);
}

long DmSoft::AddDict(long index, LPCTSTR dict_info){
	return dmSoft.AddDict(index, dict_info);
}

long DmSoft::EnterCri(){
	return dmSoft.EnterCri();
}

long DmSoft::LeaveCri(){
	return dmSoft.LeaveCri();
}

long DmSoft::WriteInt(long hwnd, LPCTSTR addr, long type, long v){
	return dmSoft.WriteInt(hwnd, addr, type, v);
}

long DmSoft::WriteFloat(long hwnd, LPCTSTR addr, float v){
	return dmSoft.WriteFloat(hwnd, addr, v);
}

long DmSoft::WriteDouble(long hwnd, LPCTSTR addr, double v){
	return dmSoft.WriteDouble(hwnd, addr, v);
}

long DmSoft::WriteString(long hwnd, LPCTSTR addr, long type, LPCTSTR v){
	return dmSoft.WriteString(hwnd, addr, type, v);
}

long DmSoft::AsmAdd(LPCTSTR asm_ins){
	return dmSoft.AsmAdd(asm_ins);
}

long DmSoft::AsmClear(){
	return dmSoft.AsmClear();
}

long DmSoft::AsmCall(long hwnd, long mode){
	return dmSoft.AsmCall(hwnd, mode);
}

long DmSoft::FindMultiColor(long x1, long y1, long x2, long y2, LPCTSTR first_color, LPCTSTR offset_color, double sim, long dir, VARIANT* x, VARIANT* y){
	return dmSoft.FindMultiColor(x1, y1, x2, y2, first_color, offset_color, sim, dir, x, y);
}

string DmSoft::FindMultiColorEx(long x1, long y1, long x2, long y2, LPCTSTR first_color, LPCTSTR offset_color, double sim, long dir){
	return (LPSTR)(LPCTSTR)dmSoft.FindMultiColorEx(x1, y1, x2, y2, first_color, offset_color, sim, dir);
}

string DmSoft::AsmCode(long base_addr){
	return (LPSTR)(LPCTSTR)dmSoft.AsmCode(base_addr);
}

string DmSoft::Assemble(LPCTSTR asm_code, long base_addr, long is_upper){
	return (LPSTR)(LPCTSTR)dmSoft.Assemble(asm_code, base_addr, is_upper);
}

long DmSoft::SetWindowTransparent(long hwnd, long v){
	return dmSoft.SetWindowTransparent(hwnd, v);
}

string DmSoft::ReadData(long hwnd, LPCTSTR addr, long len){
	return (LPSTR)(LPCTSTR)dmSoft.ReadData(hwnd, addr, len);
}

long DmSoft::WriteData(long hwnd, LPCTSTR addr, LPCTSTR data){
	return dmSoft.WriteData(hwnd, addr, data);
}

string DmSoft::FindData(long hwnd, LPCTSTR addr_range, LPCTSTR data){
	return (LPSTR)(LPCTSTR)dmSoft.FindData(hwnd, addr_range, data);
}

long DmSoft::SetPicPwd(LPCTSTR pwd){
	return dmSoft.SetPicPwd(pwd);
}

long DmSoft::Log(LPCTSTR info){
	return dmSoft.Log(info);
}

string DmSoft::FindStrE(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim){
	return (LPSTR)(LPCTSTR)dmSoft.FindStrE(x1, y1, x2, y2, str, color, sim);
}

string DmSoft::FindColorE(long x1, long y1, long x2, long y2, LPCTSTR color, double sim, long dir){
	return (LPSTR)(LPCTSTR)dmSoft.FindColorE(x1, y1, x2, y2, color, sim, dir);
}

string DmSoft::FindPicE(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR delta_color, double sim, long dir){
	return (LPSTR)(LPCTSTR)dmSoft.FindPicE(x1, y1, x2, y2, pic_name, delta_color, sim, dir);
}

string DmSoft::FindMultiColorE(long x1, long y1, long x2, long y2, LPCTSTR first_color, LPCTSTR offset_color, double sim, long dir){
	return (LPSTR)(LPCTSTR)dmSoft.FindMultiColorE(x1, y1, x2, y2, first_color, offset_color, sim, dir);
}

long DmSoft::SetExactOcr(long exact_ocr){
	return dmSoft.SetExactOcr(exact_ocr);
}

string DmSoft::ReadString(long hwnd, LPCTSTR addr, long type, long len){
	return (LPSTR)(LPCTSTR)dmSoft.ReadString(hwnd, addr, type, len);
}

long DmSoft::FoobarTextPrintDir(long hwnd, long dir){
	return dmSoft.FoobarTextPrintDir(hwnd, dir);
}

string DmSoft::OcrEx(long x1, long y1, long x2, long y2, LPCTSTR color, double sim){
	return (LPSTR)(LPCTSTR)dmSoft.OcrEx(x1, y1, x2, y2, color, sim);
}

long DmSoft::SetDisplayInput(LPCTSTR mode){
	return dmSoft.SetDisplayInput(mode);
}

long DmSoft::GetTime(){
	return dmSoft.GetTime();
}

long DmSoft::GetScreenWidth(){
	return dmSoft.GetScreenWidth();
}

long DmSoft::GetScreenHeight(){
	return dmSoft.GetScreenHeight();
}

long DmSoft::BindWindowEx(long hwnd, LPCTSTR display, LPCTSTR mouse, LPCTSTR keypad, LPCTSTR public_desc, long mode){
	return dmSoft.BindWindowEx(hwnd, display, mouse, keypad, public_desc, mode);
}

string DmSoft::GetDiskSerial(){
	return (LPSTR)(LPCTSTR)dmSoft.GetDiskSerial();
}

string DmSoft::Md5(LPCTSTR str){
	return (LPSTR)(LPCTSTR)dmSoft.Md5(str);
}

string DmSoft::GetMac(){
	return (LPSTR)(LPCTSTR)dmSoft.GetMac();
}

long DmSoft::ActiveInputMethod(long hwnd, LPCTSTR id){
	return dmSoft.ActiveInputMethod(hwnd, id);
}

long DmSoft::CheckInputMethod(long hwnd, LPCTSTR id){
	return dmSoft.CheckInputMethod(hwnd, id);
}

long DmSoft::FindInputMethod(LPCTSTR id){
	return dmSoft.FindInputMethod(id);
}

long DmSoft::GetCursorPos(VARIANT* x, VARIANT* y){
	return dmSoft.GetCursorPos(x, y);
}

long DmSoft::BindWindow(long hwnd, LPCTSTR display, LPCTSTR mouse, LPCTSTR keypad, long mode){
	return dmSoft.BindWindow(hwnd, display, mouse, keypad, mode);
}

long DmSoft::DmFindWindow(LPCTSTR class_name, LPCTSTR title_name){
	return dmSoft.DmFindWindow(class_name, title_name);
}

long DmSoft::GetScreenDepth(){
	return dmSoft.GetScreenDepth();
}

long DmSoft::SetScreen(long width, long height, long depth){
	return dmSoft.SetScreen(width, height, depth);
}

long DmSoft::ExitOs(long type){
	return dmSoft.ExitOs(type);
}

string DmSoft::GetDir(long type){
	return (LPSTR)(LPCTSTR)dmSoft.GetDir(type);
}

long DmSoft::GetOsType(){
	return dmSoft.GetOsType();
}

long DmSoft::DmFindWindowEx(long parent, LPCTSTR class_name, LPCTSTR title_name){
	return dmSoft.DmFindWindowEx(parent, class_name, title_name);
}

long DmSoft::SetExportDict(long index, LPCTSTR dict_name){
	return dmSoft.SetExportDict(index, dict_name);
}

string DmSoft::GetCursorShape(){
	return (LPSTR)(LPCTSTR)dmSoft.GetCursorShape();
}

long DmSoft::DownCpu(long rate){
	return dmSoft.DownCpu(rate);
}

string DmSoft::GetCursorSpot(){
	return (LPSTR)(LPCTSTR)dmSoft.GetCursorSpot();
}

long DmSoft::SendString2(long hwnd, LPCTSTR str){
	return dmSoft.SendString2(hwnd, str);
}

long DmSoft::FaqPost(LPCTSTR server, long handle, long request_type, long time_out){
	return dmSoft.FaqPost(server, handle, request_type, time_out);
}

string DmSoft::FaqFetch(){
	return (LPSTR)(LPCTSTR)dmSoft.FaqFetch();
}

string DmSoft::FetchWord(long x1, long y1, long x2, long y2, LPCTSTR color, LPCTSTR word){
	return (LPSTR)(LPCTSTR)dmSoft.FetchWord(x1, y1, x2, y2, color, word);
}

long DmSoft::CaptureJpg(long x1, long y1, long x2, long y2, LPCTSTR file, long quality){
	return dmSoft.CaptureJpg(x1, y1, x2, y2, file, quality);
}

long DmSoft::FindStrWithFont(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, LPCTSTR font_name, long font_size, long flag, VARIANT* x, VARIANT* y){
	return dmSoft.FindStrWithFont(x1, y1, x2, y2, str, color, sim, font_name, font_size, flag, x, y);
}

string DmSoft::FindStrWithFontE(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, LPCTSTR font_name, long font_size, long flag){
	return (LPSTR)(LPCTSTR)dmSoft.FindStrWithFontE(x1, y1, x2, y2, str, color, sim, font_name, font_size, flag);
}

string DmSoft::FindStrWithFontEx(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, LPCTSTR font_name, long font_size, long flag){
	return (LPSTR)(LPCTSTR)dmSoft.FindStrWithFontEx(x1, y1, x2, y2, str, color, sim, font_name, font_size, flag);
}

string DmSoft::GetDictInfo(LPCTSTR str, LPCTSTR font_name, long font_size, long flag){
	return (LPSTR)(LPCTSTR)dmSoft.GetDictInfo(str, font_name, font_size, flag);
}

long DmSoft::SaveDict(long index, LPCTSTR file){
	return dmSoft.SaveDict(index, file);
}

long DmSoft::GetWindowProcessId(long hwnd){
	return dmSoft.GetWindowProcessId(hwnd);
}

string DmSoft::GetWindowProcessPath(long hwnd){
	return (LPSTR)(LPCTSTR)dmSoft.GetWindowProcessPath(hwnd);
}

long DmSoft::LockInput(long lock){
	return dmSoft.LockInput(lock);
}

string DmSoft::GetPicSize(LPCTSTR pic_name){
	return (LPSTR)(LPCTSTR)dmSoft.GetPicSize(pic_name);
}

long DmSoft::GetID(){
	return dmSoft.GetID();
}

long DmSoft::CapturePng(long x1, long y1, long x2, long y2, LPCTSTR file){
	return dmSoft.CapturePng(x1, y1, x2, y2, file);
}

long DmSoft::CaptureGif(long x1, long y1, long x2, long y2, LPCTSTR file, long delay, long time){
	return dmSoft.CaptureGif(x1, y1, x2, y2, file, delay, time);
}

long DmSoft::ImageToBmp(LPCTSTR pic_name, LPCTSTR bmp_name){
	return dmSoft.ImageToBmp(pic_name, bmp_name);
}

long DmSoft::FindStrFast(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, VARIANT* x, VARIANT* y){
	return dmSoft.FindStrFast(x1, y1, x2, y2, str, color, sim, x, y);
}

string DmSoft::FindStrFastEx(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim){
	return (LPSTR)(LPCTSTR)dmSoft.FindStrFastEx(x1, y1, x2, y2, str, color, sim);
}

string DmSoft::FindStrFastE(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim){
	return (LPSTR)(LPCTSTR)dmSoft.FindStrFastE(x1, y1, x2, y2, str, color, sim);
}

long DmSoft::EnableDisplayDebug(long enable_debug){
	return dmSoft.EnableDisplayDebug(enable_debug);
}

long DmSoft::CapturePre(LPCTSTR file){
	return dmSoft.CapturePre(file);
}

long DmSoft::RegEx(LPCTSTR code, LPCTSTR Ver, LPCTSTR ip){
	return dmSoft.RegEx(code, Ver, ip);
}

string DmSoft::GetMachineCode(){
	return (LPSTR)(LPCTSTR)dmSoft.GetMachineCode();
}

long DmSoft::SetClipboard(LPCTSTR data){
	return dmSoft.SetClipboard(data);
}

string DmSoft::GetClipboard(){
	return (LPSTR)(LPCTSTR)dmSoft.GetClipboard();
}

long DmSoft::GetNowDict(){
	return dmSoft.GetNowDict();
}

long DmSoft::Is64Bit(){
	return dmSoft.Is64Bit();
}

long DmSoft::GetColorNum(long x1, long y1, long x2, long y2, LPCTSTR color, double sim){
	return dmSoft.GetColorNum(x1, y1, x2, y2, color, sim);
}

string DmSoft::EnumWindowByProcess(LPCTSTR process_name, LPCTSTR title, LPCTSTR class_name, long filter){
	return (LPSTR)(LPCTSTR)dmSoft.EnumWindowByProcess(process_name, title, class_name, filter);
}

long DmSoft::GetDictCount(long index){
	return dmSoft.GetDictCount(index);
}

long DmSoft::GetLastError(){
	return dmSoft.GetLastError();
}

string DmSoft::GetNetTime(){
	return (LPSTR)(LPCTSTR)dmSoft.GetNetTime();
}

long DmSoft::EnableGetColorByCapture(long en){
	return dmSoft.EnableGetColorByCapture(en);
}

long DmSoft::CheckUAC(){
	return dmSoft.CheckUAC();
}

long DmSoft::SetUAC(long uac){
	return dmSoft.SetUAC(uac);
}

long DmSoft::DisableFontSmooth(){
	return dmSoft.DisableFontSmooth();
}

long DmSoft::CheckFontSmooth(){
	return dmSoft.CheckFontSmooth();
}

long DmSoft::SetDisplayAcceler(long level){
	return dmSoft.SetDisplayAcceler(level);
}

long DmSoft::DmFindWindowByProcess(LPCTSTR process_name, LPCTSTR class_name, LPCTSTR title_name){
	return dmSoft.DmFindWindowByProcess(process_name, class_name, title_name);
}

long DmSoft::DmFindWindowByProcessId(long process_id, LPCTSTR class_name, LPCTSTR title_name){
	return dmSoft.DmFindWindowByProcessId(process_id, class_name, title_name);
}

string DmSoft::ReadIni(LPCTSTR section, LPCTSTR key, LPCTSTR file){
	return (LPSTR)(LPCTSTR)dmSoft.ReadIni(section, key, file);
}

long DmSoft::WriteIni(LPCTSTR section, LPCTSTR key, LPCTSTR v, LPCTSTR file){
	return dmSoft.WriteIni(section, key, v, file);
}

long DmSoft::RunApp(LPCTSTR path, long mode){
	return dmSoft.RunApp(path, mode);
}

long DmSoft::delay(long mis){
	return dmSoft.delay(mis);
}

long DmSoft::DmFindWindowSuper(LPCTSTR spec1, long flag1, long type1, LPCTSTR spec2, long flag2, long type2){
	return dmSoft.DmFindWindowSuper(spec1, flag1, type1, spec2, flag2, type2);
}

string DmSoft::ExcludePos(LPCTSTR all_pos, long type, long x1, long y1, long x2, long y2){
	return (LPSTR)(LPCTSTR)dmSoft.ExcludePos(all_pos, type, x1, y1, x2, y2);
}

string DmSoft::FindNearestPos(LPCTSTR all_pos, long type, long x, long y){
	return (LPSTR)(LPCTSTR)dmSoft.FindNearestPos(all_pos, type, x, y);
}

string DmSoft::SortPosDistance(LPCTSTR all_pos, long type, long x, long y){
	return (LPSTR)(LPCTSTR)dmSoft.SortPosDistance(all_pos, type, x, y);
}

long DmSoft::FindPicMem(long x1, long y1, long x2, long y2, LPCTSTR pic_info, LPCTSTR delta_color, double sim, long dir, VARIANT* x, VARIANT* y){
	return dmSoft.FindPicMem(x1, y1, x2, y2, pic_info, delta_color, sim, dir, x, y);
}

string DmSoft::FindPicMemEx(long x1, long y1, long x2, long y2, LPCTSTR pic_info, LPCTSTR delta_color, double sim, long dir){
	return (LPSTR)(LPCTSTR)dmSoft.FindPicMemEx(x1, y1, x2, y2, pic_info, delta_color, sim, dir);
}

string DmSoft::FindPicMemE(long x1, long y1, long x2, long y2, LPCTSTR pic_info, LPCTSTR delta_color, double sim, long dir){
	return (LPSTR)(LPCTSTR)dmSoft.FindPicMemE(x1, y1, x2, y2, pic_info, delta_color, sim, dir);
}

string DmSoft::AppendPicAddr(LPCTSTR pic_info, long addr, long size){
	return (LPSTR)(LPCTSTR)dmSoft.AppendPicAddr(pic_info, addr, size);
}

long DmSoft::WriteFile(LPCTSTR file, LPCTSTR content){
	return dmSoft.WriteFile(file, content);
}

long DmSoft::Stop(long id){
	return dmSoft.Stop(id);
}

long DmSoft::SetDictMem(long index, long addr, long size){
	return dmSoft.SetDictMem(index, addr, size);
}

string DmSoft::GetNetTimeSafe(){
	return (LPSTR)(LPCTSTR)dmSoft.GetNetTimeSafe();
}

long DmSoft::ForceUnBindWindow(long hwnd){
	return dmSoft.ForceUnBindWindow(hwnd);
}

string DmSoft::ReadIniPwd(LPCTSTR section, LPCTSTR key, LPCTSTR file, LPCTSTR pwd){
	return (LPSTR)(LPCTSTR)dmSoft.ReadIniPwd(section, key, file, pwd);
}

long DmSoft::WriteIniPwd(LPCTSTR section, LPCTSTR key, LPCTSTR v, LPCTSTR file, LPCTSTR pwd){
	return dmSoft.WriteIniPwd(section, key, v, file, pwd);
}

long DmSoft::DecodeFile(LPCTSTR file, LPCTSTR pwd){
	return dmSoft.DecodeFile(file, pwd);
}

long DmSoft::KeyDownChar(LPCTSTR key_str){
	return dmSoft.KeyDownChar(key_str);
}

long DmSoft::KeyUpChar(LPCTSTR key_str){
	return dmSoft.KeyUpChar(key_str);
}

long DmSoft::KeyPressChar(LPCTSTR key_str){
	return dmSoft.KeyPressChar(key_str);
}

long DmSoft::KeyPressStr(LPCTSTR key_str, long delay){
	return dmSoft.KeyPressStr(key_str, delay);
}

long DmSoft::EnableKeypadPatch(long en){
	return dmSoft.EnableKeypadPatch(en);
}

long DmSoft::EnableKeypadSync(long en, long time_out){
	return dmSoft.EnableKeypadSync(en, time_out);
}

long DmSoft::EnableMouseSync(long en, long time_out){
	return dmSoft.EnableMouseSync(en, time_out);
}

long DmSoft::DmGuard(long en, LPCTSTR type){
	return dmSoft.DmGuard(en, type);
}

long DmSoft::FaqCaptureFromFile(long x1, long y1, long x2, long y2, LPCTSTR file, long quality){
	return dmSoft.FaqCaptureFromFile(x1, y1, x2, y2, file, quality);
}

string DmSoft::FindIntEx(long hwnd, LPCTSTR addr_range, long int_value_min, long int_value_max, long type, long step, long multi_thread, long mode){
	return (LPSTR)(LPCTSTR)dmSoft.FindIntEx(hwnd, addr_range, int_value_min, int_value_max, type, step, multi_thread, mode);
}

string DmSoft::FindFloatEx(long hwnd, LPCTSTR addr_range, float float_value_min, float float_value_max, long step, long multi_thread, long mode){
	return (LPSTR)(LPCTSTR)dmSoft.FindFloatEx(hwnd, addr_range, float_value_min, float_value_max, step, multi_thread, mode);
}

string DmSoft::FindDoubleEx(long hwnd, LPCTSTR addr_range, double double_value_min, double double_value_max, long step, long multi_thread, long mode){
	return (LPSTR)(LPCTSTR)dmSoft.FindDoubleEx(hwnd, addr_range, double_value_min, double_value_max, step, multi_thread, mode);
}

string DmSoft::FindStringEx(long hwnd, LPCTSTR addr_range, LPCTSTR string_value, long type, long step, long multi_thread, long mode){
	return (LPSTR)(LPCTSTR)dmSoft.FindStringEx(hwnd, addr_range, string_value, type, step, multi_thread, mode);
}

string DmSoft::FindDataEx(long hwnd, LPCTSTR addr_range, LPCTSTR data, long step, long multi_thread, long mode){
	return (LPSTR)(LPCTSTR)dmSoft.FindDataEx(hwnd, addr_range, data, step, multi_thread, mode);
}

long DmSoft::EnableRealMouse(long en, long mousedelay, long mousestep){
	return dmSoft.EnableRealMouse(en, mousedelay, mousestep);
}

long DmSoft::EnableRealKeypad(long en){
	return dmSoft.EnableRealKeypad(en);
}

long DmSoft::SendStringIme(LPCTSTR str){
	return dmSoft.SendStringIme(str);
}

long DmSoft::FoobarDrawLine(long hwnd, long x1, long y1, long x2, long y2, LPCTSTR color, long style, long width){
	return dmSoft.FoobarDrawLine(hwnd, x1, y1, x2, y2, color, style, width);
}

string DmSoft::FindStrEx(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim){
	return (LPSTR)(LPCTSTR)dmSoft.FindStrEx(x1, y1, x2, y2, str, color, sim);
}

long DmSoft::IsBind(long hwnd){
	return dmSoft.IsBind(hwnd);
}

long DmSoft::SetDisplayDelay(long t){
	return dmSoft.SetDisplayDelay(t);
}

long DmSoft::GetDmCount(){
	return dmSoft.GetDmCount();
}

long DmSoft::DisableScreenSave(){
	return dmSoft.DisableScreenSave();
}

long DmSoft::DisablePowerSave(){
	return dmSoft.DisablePowerSave();
}

long DmSoft::SetMemoryHwndAsProcessId(long en){
	return dmSoft.SetMemoryHwndAsProcessId(en);
}

long DmSoft::FindShape(long x1, long y1, long x2, long y2, LPCTSTR offset_color, double sim, long dir, VARIANT* x, VARIANT* y){
	return dmSoft.FindShape(x1, y1, x2, y2, offset_color, sim, dir, x, y);
}

string DmSoft::FindShapeE(long x1, long y1, long x2, long y2, LPCTSTR offset_color, double sim, long dir){
	return (LPSTR)(LPCTSTR)dmSoft.FindShapeE(x1, y1, x2, y2, offset_color, sim, dir);
}

string DmSoft::FindShapeEx(long x1, long y1, long x2, long y2, LPCTSTR offset_color, double sim, long dir){
	return (LPSTR)(LPCTSTR)dmSoft.FindShapeEx(x1, y1, x2, y2, offset_color, sim, dir);
}

string DmSoft::FindStrS(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, VARIANT* x, VARIANT* y){
	return (LPSTR)(LPCTSTR)dmSoft.FindStrS(x1, y1, x2, y2, str, color, sim, x, y);
}

string DmSoft::FindStrExS(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim){
	return (LPSTR)(LPCTSTR)dmSoft.FindStrExS(x1, y1, x2, y2, str, color, sim);
}

string DmSoft::FindStrFastS(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, VARIANT* x, VARIANT* y){
	return (LPSTR)(LPCTSTR)dmSoft.FindStrFastS(x1, y1, x2, y2, str, color, sim, x, y);
}

string DmSoft::FindStrFastExS(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim){
	return (LPSTR)(LPCTSTR)dmSoft.FindStrFastExS(x1, y1, x2, y2, str, color, sim);
}

string DmSoft::FindPicS(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR delta_color, double sim, long dir, VARIANT* x, VARIANT* y){
	return (LPSTR)(LPCTSTR)dmSoft.FindPicS(x1, y1, x2, y2, pic_name, delta_color, sim, dir, x, y);
}

string DmSoft::FindPicExS(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR delta_color, double sim, long dir){
	return (LPSTR)(LPCTSTR)dmSoft.FindPicExS(x1, y1, x2, y2, pic_name, delta_color, sim, dir);
}

long DmSoft::ClearDict(long index){
	return dmSoft.ClearDict(index);
}

string DmSoft::GetMachineCodeNoMac(){
	return (LPSTR)(LPCTSTR)dmSoft.GetMachineCodeNoMac();
}

long DmSoft::GetClientRect(long hwnd, VARIANT* x1, VARIANT* y1, VARIANT* x2, VARIANT* y2){
	return dmSoft.GetClientRect(hwnd, x1, y1, x2, y2);
}

long DmSoft::EnableFakeActive(long en){
	return dmSoft.EnableFakeActive(en);
}

long DmSoft::GetScreenDataBmp(long x1, long y1, long x2, long y2, VARIANT* data, VARIANT* size){
	return dmSoft.GetScreenDataBmp(x1, y1, x2, y2, data, size);
}

long DmSoft::EncodeFile(LPCTSTR file, LPCTSTR pwd){
	return dmSoft.EncodeFile(file, pwd);
}

string DmSoft::GetCursorShapeEx(long type){
	return (LPSTR)(LPCTSTR)dmSoft.GetCursorShapeEx(type);
}

long DmSoft::FaqCancel(){
	return dmSoft.FaqCancel();
}

string DmSoft::IntToData(long int_value, long type){
	return (LPSTR)(LPCTSTR)dmSoft.IntToData(int_value, type);
}

string DmSoft::FloatToData(float float_value){
	return (LPSTR)(LPCTSTR)dmSoft.FloatToData(float_value);
}

string DmSoft::DoubleToData(double double_value){
	return (LPSTR)(LPCTSTR)dmSoft.DoubleToData(double_value);
}

string DmSoft::StringToData(LPCTSTR string_value, long type){
	return (LPSTR)(LPCTSTR)dmSoft.StringToData(string_value, type);
}

long DmSoft::SetMemoryFindResultToFile(LPCTSTR file){
	return dmSoft.SetMemoryFindResultToFile(file);
}

long DmSoft::EnableBind(long en){
	return dmSoft.EnableBind(en);
}

long DmSoft::SetSimMode(long mode){
	return dmSoft.SetSimMode(mode);
}

long DmSoft::LockMouseRect(long x1, long y1, long x2, long y2){
	return dmSoft.LockMouseRect(x1, y1, x2, y2);
}

long DmSoft::SendPaste(long hwnd){
	return dmSoft.SendPaste(hwnd);
}

long DmSoft::IsDisplayDead(long x1, long y1, long x2, long y2, long t){
	return dmSoft.IsDisplayDead(x1, y1, x2, y2, t);
}

long DmSoft::GetKeyState(long vk){
	return dmSoft.GetKeyState(vk);
}

long DmSoft::DmCopyFile(LPCTSTR src_file, LPCTSTR dst_file, long over){
	return dmSoft.DmCopyFile(src_file, dst_file, over);
}

long DmSoft::IsFileExist(LPCTSTR file){
	return dmSoft.IsFileExist(file);
}

long DmSoft::DmDeleteFile(LPCTSTR file){
	return dmSoft.DmDeleteFile(file);
}

long DmSoft::DmMoveFile(LPCTSTR src_file, LPCTSTR dst_file){
	return dmSoft.DmMoveFile(src_file, dst_file);
}

long DmSoft::CreateFolder(LPCTSTR folder_name){
	return dmSoft.CreateFolder(folder_name);
}

long DmSoft::DeleteFolder(LPCTSTR folder_name){
	return dmSoft.DeleteFolder(folder_name);
}

long DmSoft::GetFileLength(LPCTSTR file){
	return dmSoft.GetFileLength(file);
}

string DmSoft::ReadFile(LPCTSTR file){
	return (LPSTR)(LPCTSTR)dmSoft.ReadFile(file);
}

long DmSoft::WaitKey(long key_code, long time_out){
	return dmSoft.WaitKey(key_code, time_out);
}

long DmSoft::DeleteIni(LPCTSTR section, LPCTSTR key, LPCTSTR file){
	return dmSoft.DeleteIni(section, key, file);
}

long DmSoft::DeleteIniPwd(LPCTSTR section, LPCTSTR key, LPCTSTR file, LPCTSTR pwd){
	return dmSoft.DeleteIniPwd(section, key, file, pwd);
}

long DmSoft::EnableSpeedDx(long en){
	return dmSoft.EnableSpeedDx(en);
}

long DmSoft::EnableIme(long en){
	return dmSoft.EnableIme(en);
}

long DmSoft::Reg(LPCTSTR code, LPCTSTR Ver){
	return dmSoft.Reg(code, Ver);
}

string DmSoft::SelectFile(){
	return (LPSTR)(LPCTSTR)dmSoft.SelectFile();
}

string DmSoft::SelectDirectory(){
	return (LPSTR)(LPCTSTR)dmSoft.SelectDirectory();
}

long DmSoft::LockDisplay(long lock){
	return dmSoft.LockDisplay(lock);
}

long DmSoft::FoobarSetSave(long hwnd, LPCTSTR file, long en, LPCTSTR header){
	return dmSoft.FoobarSetSave(hwnd, file, en, header);
}

string DmSoft::EnumWindowSuper(LPCTSTR spec1, long flag1, long type1, LPCTSTR spec2, long flag2, long type2, long sort){
	return (LPSTR)(LPCTSTR)dmSoft.EnumWindowSuper(spec1, flag1, type1, spec2, flag2, type2, sort);
}

long DmSoft::DownloadFile(LPCTSTR url, LPCTSTR save_file, long timeout){
	return dmSoft.DownloadFile(url, save_file, timeout);
}

long DmSoft::EnableKeypadMsg(long en){
	return dmSoft.EnableKeypadMsg(en);
}

long DmSoft::EnableMouseMsg(long en){
	return dmSoft.EnableMouseMsg(en);
}

long DmSoft::RegNoMac(LPCTSTR code, LPCTSTR Ver){
	return dmSoft.RegNoMac(code, Ver);
}

long DmSoft::RegExNoMac(LPCTSTR code, LPCTSTR Ver, LPCTSTR ip){
	return dmSoft.RegExNoMac(code, Ver, ip);
}

long DmSoft::SetEnumWindowDelay(long delay){
	return dmSoft.SetEnumWindowDelay(delay);
}

long DmSoft::FindMulColor(long x1, long y1, long x2, long y2, LPCTSTR color, double sim){
	return dmSoft.FindMulColor(x1, y1, x2, y2, color, sim);
}

string DmSoft::GetDict(long index, long font_index){
	return (LPSTR)(LPCTSTR)dmSoft.GetDict(index, font_index);
}