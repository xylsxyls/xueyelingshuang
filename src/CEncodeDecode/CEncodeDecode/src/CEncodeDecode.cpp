#include "CEncodeDecode.h"
#include "MD5.h"
#include "aes_encryptor.h"
#include <vector>
#include <time.h>
#include <string.h>

std::string CEncodeDecode::MD5Decode(const std::string& src)
{
	return MD5(src).toString();
}

std::string CEncodeDecode::AESEncode(const std::string& key, const std::string& src)
{
	int nKey = key.size();
	unsigned char* pKey = nullptr;
	if (nKey < 16)
	{
		pKey = (unsigned char*)calloc(16, 1);
		memcpy(pKey, key.c_str(), key.length());
	}
	AesEncryptor* aes = nullptr;
	if (nKey < 16)
	{
		aes = new AesEncryptor(pKey);
	}
	else
	{
		aes = new AesEncryptor((unsigned char*)key.c_str());
	}
	std::string strResult = aes->EncryptString(src);
	delete[] pKey;
	delete aes;
	return strResult;
}

std::string CEncodeDecode::AESDecode(const std::string& key, const std::string& src)
{
	int nKey = key.size();
	unsigned char* pKey = nullptr;
	if (nKey < 16)
	{
		pKey = (unsigned char*)calloc(16, 1);
		memcpy(pKey, key.c_str(), key.length());
	}
	AesEncryptor* aes = nullptr;
	if (nKey < 16)
	{
		aes = new AesEncryptor(pKey);
	}
	else
	{
		aes = new AesEncryptor((unsigned char*)key.c_str());
	}
	std::string strResult = aes->DecryptString(src);
	delete[] pKey;
	delete aes;
	return strResult;
}

/*
static std::string Randomstring(int nMin, int nMax, const std::vector<char>& vecCharacter)
{
	if (nMin < 0 || nMax < 0 || nMax - nMin < 0)
	{
		return "";
	}
	srand((unsigned int)time(0));
	int nLength = rand() % (nMax - nMin + 1) + nMin;
	int nSize = vecCharacter.size();
	std::string strResult;
	while (strResult.length() < (unsigned int)nLength)
	{
		strResult = strResult + vecCharacter.at(rand() % nSize);
	}
	return strResult;
}

int main()
{
	vector<char> vecCharacter;
	char i = 64;
	while (i++ != 90)
	{
		vecCharacter.push_back(i);
	}
	i = 96;
	while (i++ != 122)
	{
		vecCharacter.push_back(i);
	}
	i = 47;
	while (i++ != 57)
	{
		vecCharacter.push_back(i);
	}
	while (1)
	{
		string strKey = Randomstring(1, 17, vecCharacter);
		string strMessage = Randomstring(1, 4096, vecCharacter);
		CEncodeDecode encode;
		string strEncode = encode.AESEncode(strKey, strMessage);
		CEncodeDecode decode;
		string strDecode = decode.AESDecode(strKey, strEncode);
		if (strDecode != strMessage)
		{
			printf("1");
		}
	}
	return 0;
}
*/