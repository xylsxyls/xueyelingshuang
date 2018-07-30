#include <stdio.h>
#include "CSystem/CSystemAPI.h"
#include "CGetPath/CGetPathAPI.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"

int main(int argc, char** argv)
{
	std::vector<std::string> params = CSystem::exeParam();
	if (params.size() != 1)
	{
		return -1;
	}
	params.clear();
	params.push_back("decode");
	std::string exePath = CGetPath::GetCurrentExePath();
	std::string decodePath = exePath + "..\\..\\..\\attacklogy\\AttackData\\";
	std::string encodePath = exePath + "..\\..\\..\\attacklogy\\AttackSrc\\";
	std::vector<std::string> encodeFilePaths = CGetPath::FindFilePath("", encodePath, 3);
	std::vector<std::string> decodeFilePaths = CGetPath::FindFilePath("", decodePath, 3);
	std::string param = params[0];
	if (param == "encode")
	{
		int32_t index = -1;
		while (index++ != encodeFilePaths.size() - 1)
		{
			if (encodeFilePaths[index] == "." || encodeFilePaths[index] == "..")
			{
				continue;
			}
			std::string name = CGetPath::GetName(encodeFilePaths[index], 1);
			std::string filePath = CGetPath::GetName(encodeFilePaths[index], 4);
			std::string decodeName = CEncodeDecode::AESEncode("Yangnan8", name);
			Ctxt txt(encodeFilePaths[index]);
			txt.LoadTxt(3, "");
			int32_t lineIndex = -1;
			while (lineIndex++ != txt.m_vectxt.size() - 1)
			{
				txt.m_vectxt[lineIndex][0] = CEncodeDecode::AESEncode("Yangnan8", txt.m_vectxt[lineIndex][0]);
			}
			std::string decodePath = encodePath;
			CStringManager::Replace(decodePath, "AttackSrc", "AttackData");
			txt.SaveAs(decodePath + decodeName);
		}
	}
	else if (param == "decode")
	{
		printf("key = ");
		char key[1024] = {};
		scanf("%s", key);
		bool isRevise = false;
		CSystem::CreateDir(encodePath);
		int32_t index = -1;
		while (index++ != decodeFilePaths.size() - 1)
		{
			if (decodeFilePaths[index] == "." || decodeFilePaths[index] == "..")
			{
				continue;
			}
			std::string name = CGetPath::GetName(decodeFilePaths[index], 1);
			std::string filePath = CGetPath::GetName(decodeFilePaths[index], 4);
			std::string decodeName = CEncodeDecode::AESDecode(key, name);
			Ctxt txt(decodeFilePaths[index]);
			txt.LoadTxt(3, "");
			int32_t lineIndex = -1;
			while (lineIndex++ != txt.m_vectxt.size() - 1)
			{
				txt.m_vectxt[lineIndex][0] = CEncodeDecode::AESDecode(key, txt.m_vectxt[lineIndex][0]);
			}
			std::string encodePath = decodePath;
			CStringManager::Replace(encodePath, "AttackData", "AttackSrc");
			Ctxt txtExist(encodePath + decodeName);
			txtExist.LoadTxt(3, "");
			if (txtExist.m_vectxt.size() == 0)
			{
				txt.SaveAs(encodePath + decodeName);
			}
			else if (txt.m_vectxt != txtExist.m_vectxt)
			{
				isRevise = true;
				printf("ÄÚÈÝÓÐÐÞ¸Ä£¬%s", decodeName.c_str());
				continue;
			}
		}
		if (isRevise)
		{
			CSystem::ClearScanf();
			getchar();
		}
	}
	return 0;
}