#include "CEncodeDecode.h"
#include "MD5.h"
#include "aes_encryptor.h"

string CEncodeDecode::MD5Decode(string src){
	return MD5(src).toString();
}

string CEncodeDecode::AESEncode(string key,string src){
	int nKey = key.size();
	unsigned char* pKey = NULL;
	if(nKey < 16){
		pKey = (unsigned char*)calloc(16,1);
		memcpy(pKey,key.c_str(),key.length());
	}
	AesEncryptor *aes = NULL;
	if(nKey < 16) aes = new AesEncryptor(pKey);
	else aes = new AesEncryptor((unsigned char*)key.c_str());
	string strResult = aes->EncryptString(src);
	delete[] pKey;
	delete aes;
	return strResult;
}

string CEncodeDecode::AESDecode(string key,string src){
	int nKey = key.size();
	unsigned char* pKey = NULL;
	if(nKey < 16){
		pKey = (unsigned char*)calloc(16,1);
		memcpy(pKey,key.c_str(),key.length());
	}
	AesEncryptor *aes = NULL;
	if(nKey < 16) aes = new AesEncryptor(pKey);
	else aes = new AesEncryptor((unsigned char*)key.c_str());
	string strResult = aes->DecryptString(src);
	delete[] pKey;
	delete aes;
	return strResult;
}

#include <vector>
#include <time.h>
using namespace std;

string Randomstring(int nMin,int nMax,vector<char> vecCharacter){
	if(nMin < 0 || nMax < 0 || nMax - nMin < 0) return "";
	srand((unsigned int)time(0));
	int nLength = rand() % (nMax - nMin + 1) + nMin;
	int nSize = vecCharacter.size();
	string strResult = "";
	while(strResult.length() < (unsigned int)nLength){
		strResult = strResult + vecCharacter.at(rand() % nSize);
	}
	return strResult;
}

int main(){
	vector<char> vecCharacter;
	char i = 64;
	while(i++ != 90){
		vecCharacter.push_back(i);
	}
	i = 96;
	while(i++ != 122){
		vecCharacter.push_back(i);
	}
	i = 47;
	while(i++ != 57){
		vecCharacter.push_back(i);
	}
	while(1){
		string strKey = Randomstring(1,17,vecCharacter);
		string strMessage = Randomstring(1,4096,vecCharacter);
		CEncodeDecode encode;
		string strEncode = encode.AESEncode(strKey,strMessage);
		CEncodeDecode decode;
		string strDecode = decode.AESDecode(strKey,strEncode);
		if(strDecode != strMessage){
			printf("1");
		}
	}
	return 0;
}