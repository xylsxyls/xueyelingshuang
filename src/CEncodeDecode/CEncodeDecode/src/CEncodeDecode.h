﻿#pragma once
#include <string>
#include "CEncodeDecodeMacro.h"

typedef struct x509_st X509;

class CEncodeDecodeAPI CEncodeDecode
{
public:
	//返回经过MD5转换后的字符串
	static std::string MD5Encode(const std::string& src);

	//加密解密的key必须一样，返回加密解密后的字符串，需要加密的字符串里不能有\0，加密长度不可以超过int32_t上限
	static std::string AESEncode(const std::string& key, const std::string& src);
	static std::string AESDecode(const std::string& key, const std::string& src);

	//rsa加密
	static bool RSAKey(std::string& publicKey, std::string& privateKey);
	static std::string RSAEncode(const std::string& publicKey, const char* src);
	static std::string RSADecode(const std::string& privateKey, const std::string& src);

	//此接口需要用到openssl，编译的时候需要加预定义宏USE_OPENSSL
	//sm2加密，前缀为0x04，publicKeyX和c1可能有前缀，长度为33和65，密文新版排序为C1C3C2，旧版为C1C2C3
	//publicKeyX带前缀0x04，则加密出的c1会带，publicKeyX不带，则c1不带，解密时c1带不带都可以
	//加密长度不可以超过int32_t上限
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

	//sm4加密，key自定义，key必须是16字节，src必须是16的整数倍，内部都可以含有\0
	static std::string SM4Encode(const std::string& key, const std::string& src);
	static std::string SM4Decode(const std::string& key, const std::string& ciphertext);

	//此接口需要用到openssl，编译的时候需要加预定义宏USE_OPENSSL
	//cer证书相关
	static X509* getCert(const std::string& path);
	static void releaseCert(X509* cert);
	static std::string getPublicKey(X509* cert);
};