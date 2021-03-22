#pragma once
#include <string>
#include "CEncodeDecodeMacro.h"

class CEncodeDecodeAPI CEncodeDecode
{
public:
	//���ؾ���MD5ת������ַ���
	static std::string MD5Encode(const std::string& src);

	//���ܽ��ܵ�key����һ�������ؼ��ܽ��ܺ���ַ�������Ҫ���ܵ��ַ����ﲻ����\0�����ܳ��Ȳ����Գ���int32_t����
	static std::string AESEncode(const std::string& key, const std::string& src);
	static std::string AESDecode(const std::string& key, const std::string& src);

	//rsa����
	static bool RSAKey(std::string& publicKey, std::string& privateKey);
	static std::string RSAEncode(const std::string& publicKey, const char* src);
	static std::string RSADecode(const std::string& privateKey, const std::string& src);

#ifdef USE_OPENSSL
	//sm2���ܣ�ǰ׺Ϊ0x04��publicKeyX��c1������ǰ׺������Ϊ33��65�������°�����ΪC1C3C2���ɰ�ΪC1C2C3
	static bool SM2Key(std::string& publicKeyX,
		std::string& publicKeyY,
		std::string& privateKey,
		bool hasPrefix = true);
	static bool SM2Encode(const std::string& publicKeyX,
		const std::string& publicKeyY,
		const std::string& src,
		std::string& c1,
		std::string& c2,
		std::string& c3);
	static std::string SM2Decode(const std::string& privateKey,
		const std::string& c1,
		const std::string& c2,
		const std::string& c3);
#endif

	//sm4���ܣ�key�Զ��壬key��src���붼��16�ֽڣ��ڲ������Ժ���\0
	static std::string SM4Encode(const std::string& key, const std::string& src);
	static std::string SM4Decode(const std::string& key, const std::string& ciphertext);
};