#include "CEncodeDecode.h"
#include "MD5.h"
#include "aes_encryptor.h"
#include <vector>
#include <time.h>
#include <string.h>
#include "RSA.h"
#include <stdint.h>
#include <memory>

static void Split(std::vector<std::string>& result, const std::string& splitString, char separate_character)
{
	result.clear();
	size_t lastPosition = 0;
	int32_t index = -1;
	while (-1 != (index = (int32_t)splitString.find(separate_character, lastPosition)))
	{
		result.push_back(splitString.substr(lastPosition, index - lastPosition));
		lastPosition = index + 1;
	}
	result.push_back(splitString.substr(lastPosition));
}

std::string CEncodeDecode::MD5Encode(const std::string& src)
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

bool CEncodeDecode::RSAKey(std::string& publicKey, std::string& privateKey)
{
	RSA rsa;
	if (!rsa.RSAKey())
	{
		return false;
	}

	publicKey.clear();
	privateKey.clear();
	int32_t index = -1;
	while (index++ != RSA_MAX - 2)
	{
		publicKey.append(std::to_string(rsa.e[index]) + ",");
		privateKey.append(std::to_string(rsa.d[index]) + ",");
	}
	publicKey.append(std::to_string(rsa.e[index]) + ";");
	privateKey.append(std::to_string(rsa.d[index]) + ";");

	index = -1;
	while (index++ != RSA_MAX - 2)
	{
		publicKey.append(std::to_string(rsa.n[index]) + ",");
		privateKey.append(std::to_string(rsa.n[index]) + ",");
	}
	publicKey.append(std::to_string(rsa.n[index]));
	privateKey.append(std::to_string(rsa.n[index]));
	return true;
}

std::string CEncodeDecode::RSAEncode(const std::string& publicKey, const char* src)
{
	std::vector<std::string> vecen;
	Split(vecen, publicKey, ';');
	if (vecen.size() != 2)
	{
		return "";
	}
	std::vector<std::string> vece;
	Split(vece, vecen[0], ',');
	std::vector<std::string> vecn;
	Split(vecn, vecen[1], ',');
	std::unique_ptr<int[]> e(new int[RSA_MAX]);
	std::unique_ptr<int[]> n(new int[RSA_MAX]);
	int32_t index = -1;
	while (index++ != RSA_MAX - 1)
	{
		e[index] = atoi(vece[index].c_str());
		n[index] = atoi(vecn[index].c_str());
	}
	RSA rsa;
	return rsa.tencrypto(e.get(), n.get(), src);
}

std::string CEncodeDecode::RSADecode(const std::string& privateKey, const std::string& src)
{
	std::vector<std::string> vecdn;
	Split(vecdn, privateKey, ';');
	if (vecdn.size() != 2)
	{
		return "";
	}
	std::vector<std::string> vecd;
	Split(vecd, vecdn[0], ',');
	std::vector<std::string> vecn;
	Split(vecn, vecdn[1], ',');
	std::unique_ptr<int[]> d(new int[RSA_MAX]);
	std::unique_ptr<int[]> n(new int[RSA_MAX]);
	int32_t index = -1;
	while (index++ != RSA_MAX - 1)
	{
		d[index] = atoi(vecd[index].c_str());
		n[index] = atoi(vecn[index].c_str());
	}
	RSA rsa;
	return rsa.tdecrypto(d.get(), n.get(), src);
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