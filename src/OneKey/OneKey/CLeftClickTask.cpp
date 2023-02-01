#include "stdafx.h"
#include "CLeftClickTask.h"
#include "CMouse/CMouseAPI.h"
#include <vector>

void CLeftClickTask::DoTask()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	std::vector<xyls::Rect> vecRect = 
	{
		{ 991, 0, 1919, 108 },
		{ 805, 753, 1207, 891 },
		{ 891, 892, 1322, 1037 },
		{ 0, 0, 458, 377 },
		{ 1350, 871, 1920, 1080 },
		{ 1668, 550, 1920, 1080 },
		{ 1456, 676, 1920, 1080 },
		{ 1806, 0, 1920, 461},
		{ 1153, 737, 1263, 847 },
		{ 1194, 616, 1304, 726 },
		{ 1263, 511, 1373, 621 },
		{ 1370, 432, 1480, 542 },
		{ 1482, 383, 1592, 493 },
		{ 0, 381, 132, 515 }
	};
	int32_t index = -1;
	while (index++ != vecRect.size() - 1)
	{
		if (vecRect[index].contain(currentPos))
		{
			return;
		}
	}
	CMouse::LeftClick(10);
}