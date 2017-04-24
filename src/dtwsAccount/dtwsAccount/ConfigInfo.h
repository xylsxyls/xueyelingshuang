#pragma once
#include <SDKDDKVer.h>
#include "CStorage/CStorageAPI.h"

class ConfigInfo : public CStorage{
public:
	enum{
		accountNum,
		passwordNum,
		bigServerNameNum,
		smallServerNameNum,
		jobNum,
		NameNum,
		ifCreatedNum
	};
	enum{
		rect1,
		rect2,
		rect3,
		rect4,
		rect5,
		rect6,
		rect7,
		rect8,
		rect9,
		rect12,
		rect123,
		rect14,
		rect147,
		rect23,
		rect25,
		rect258,
		rect36,
		rect369,
		rect45,
		rect456,
		rect47,
		rect56,
		rect58,
		rect69,
		rect78,
		rect789,
		rect89,
		rect1245,
		rect2356,
		rect4578,
		rect5689,
		rect123456,
		rect456789,
		rect124578,
		rect235689,
		rect123456789,

		accountPath,
		loginPoint,
		hasCreated,
		serverNameRect,
		serverNamePicPath,
		bigServerAreaRect,
		smallServerAreaRect,
		picPath,
		bmp,
		selected,
		notSelected,
		loginConfirmRect,
		loginConfirmPicPath,
		createCharacterRect,
		createCharacterPicPath,
		createBeginMove,
		jobRect,
		createNextRect,
		createNextPicPath,
		createNextSleep,
		createConfirmPicPath,
		moveCreateConfirm,
		namePoint,
		intoGamePicPath
	};
public:
	ConfigInfo();
};

extern ConfigInfo configInfo;