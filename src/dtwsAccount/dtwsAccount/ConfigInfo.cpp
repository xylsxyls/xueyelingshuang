#include "ConfigInfo.h"
#include "Point/PointAPI.h"
#include "Rect/RectAPI.h"
#include "CScreen/CScreenAPI.h"
#include "DmSoft/DmSoftAPI.h"

ConfigInfo configInfo;

ConfigInfo::ConfigInfo(){
	int width = DmSoft::GetScreenWidth();
	int height = DmSoft::GetScreenHeight();
	(*this)[rect1] = Rect((int)(0 / 3.0 * width), (int)(0 / 3.0 * height), (int)(1 / 3.0 * width), (int)(1 / 3.0 * height));
	(*this)[rect2] = Rect((int)(1 / 3.0 * width), (int)(0 / 3.0 * height), (int)(2 / 3.0 * width), (int)(1 / 3.0 * height));
	(*this)[rect3] = Rect((int)(2 / 3.0 * width), (int)(0 / 3.0 * height), (int)(3 / 3.0 * width), (int)(1 / 3.0 * height));
	(*this)[rect4] = Rect((int)(0 / 3.0 * width), (int)(1 / 3.0 * height), (int)(1 / 3.0 * width), (int)(2 / 3.0 * height));
	(*this)[rect5] = Rect((int)(1 / 3.0 * width), (int)(1 / 3.0 * height), (int)(2 / 3.0 * width), (int)(2 / 3.0 * height));
	(*this)[rect6] = Rect((int)(2 / 3.0 * width), (int)(1 / 3.0 * height), (int)(3 / 3.0 * width), (int)(2 / 3.0 * height));
	(*this)[rect7] = Rect((int)(0 / 3.0 * width), (int)(2 / 3.0 * height), (int)(1 / 3.0 * width), (int)(3 / 3.0 * height));
	(*this)[rect8] = Rect((int)(1 / 3.0 * width), (int)(2 / 3.0 * height), (int)(2 / 3.0 * width), (int)(3 / 3.0 * height));
	(*this)[rect9] = Rect((int)(2 / 3.0 * width), (int)(2 / 3.0 * height), (int)(3 / 3.0 * width), (int)(3 / 3.0 * height));
	(*this)[rect12] = string("dtwsAccount.txt");
	(*this)[rect123] = string("dtwsAccount.txt");
	(*this)[rect14] = string("dtwsAccount.txt");
	(*this)[rect147] = string("dtwsAccount.txt");
	(*this)[rect23] = string("dtwsAccount.txt");
	(*this)[rect25] = string("dtwsAccount.txt");
	(*this)[rect258] = string("dtwsAccount.txt");
	(*this)[rect36] = string("dtwsAccount.txt");
	(*this)[rect369] = string("dtwsAccount.txt");
	(*this)[rect45] = string("dtwsAccount.txt");
	(*this)[rect456] = string("dtwsAccount.txt");
	(*this)[rect47] = string("dtwsAccount.txt");
	(*this)[rect56] = string("dtwsAccount.txt");
	(*this)[rect58] = string("dtwsAccount.txt");
	(*this)[rect69] = string("dtwsAccount.txt");
	(*this)[rect78] = string("dtwsAccount.txt");
	(*this)[rect789] = string("dtwsAccount.txt");
	(*this)[rect89] = string("dtwsAccount.txt");
	(*this)[rect1245] = string("dtwsAccount.txt");
	(*this)[rect2356] = string("dtwsAccount.txt");
	(*this)[rect4578] = string("dtwsAccount.txt");
	(*this)[rect5689] = string("dtwsAccount.txt");
	(*this)[rect123456] = string("dtwsAccount.txt");
	(*this)[rect456789] = string("dtwsAccount.txt");
	(*this)[rect124578] = string("dtwsAccount.txt");
	(*this)[rect235689] = string("dtwsAccount.txt");
	(*this)[rect123456789] = string("dtwsAccount.txt");

	(*this)[accountPath] = string("dtwsAccount.txt");
	(*this)[loginPoint] = Point(995, 416);
	(*this)[hasCreated] = string("hasCreated");
	(*this)[serverNameRect] = Rect(771, 313, 1209, 381);
	(*this)[serverNamePicPath] = string("pic/服务器名称.bmp");
	(*this)[bigServerAreaRect] = Rect(541, 386, 658, 671);
	(*this)[smallServerAreaRect] = Rect(810, 377, 1384, 739);
	(*this)[picPath] = string("pic/");
	(*this)[bmp] = string(".bmp");
	(*this)[selected] = string("选中");
	(*this)[notSelected] = string("未选中");
	(*this)[loginConfirmRect] = Rect(1281, 741, 1418, 819);
	(*this)[loginConfirmPicPath] = string("pic/登录确定.bmp");
	(*this)[createCharacterRect] = Rect(54, 669, 231, 732);
	(*this)[createCharacterPicPath] = string("pic/创建角色.bmp");
	(*this)[createBeginMove] = Point(463, 707);
	(*this)[jobRect] = Rect(5, 148, 268, 837);
	(*this)[createNextRect] = Rect(958, 971, 1137, 1036);
	(*this)[createNextPicPath] = string("pic/创建角色下一步.bmp");
	(*this)[createNextSleep] = 3000;
	(*this)[createConfirmPicPath] = string("pic/创建确定.bmp");
	(*this)[moveCreateConfirm] = Point(703, 833);
	(*this)[namePoint] = Point(951, 946);
	(*this)[intoGamePicPath] = string("pic/进入游戏.bmp");
}