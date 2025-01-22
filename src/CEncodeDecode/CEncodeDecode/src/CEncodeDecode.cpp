#include "CEncodeDecode.h"
#include "MD5.h"
#include "aes_encryptor.h"
#include <vector>
#include <time.h>
#include <string.h>
#include "RsaAlgorithm.h"
#include <stdint.h>
#include <memory>
#include "sm2_cipher_error_codes.h"
#include "sm2_create_key_pair.h"
#include "sm2_encrypt_and_decrypt.h"
#include "sm4.h"
#ifdef USE_OPENSSL
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <algorithm>
#if (_MSC_VER == 1900)
#include <iterator>
#endif
#ifdef _WIN64
#pragma comment(lib, "libcrypto64.lib")
#pragma comment(lib, "libssl64.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#elif _WIN32
#pragma comment(lib, "libcrypto32.lib")
#pragma comment(lib, "libssl32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#endif
#endif

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
	int nKey = (int)key.size();
	unsigned char* pKey = nullptr;
	if (nKey < 16)
	{
		pKey = new unsigned char[16];
		memset(pKey, 0, 16);
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
	int nKey = (int)key.size();
	unsigned char* pKey = nullptr;
	if (nKey < 16)
	{
		pKey = new unsigned char[16];
		memset(pKey, 0, 16);
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
	RsaAlgorithm rsa;
	if (!rsa.RSAKey())
	{
		return false;
	}

	publicKey.clear();
	privateKey.clear();
	int32_t index = -1;
	while (index++ != RSA_ALGORITHM_MAX - 2)
	{
		publicKey.append(std::to_string(rsa.e[index]) + ",");
		privateKey.append(std::to_string(rsa.d[index]) + ",");
	}
	publicKey.append(std::to_string(rsa.e[index]) + ";");
	privateKey.append(std::to_string(rsa.d[index]) + ";");

	index = -1;
	while (index++ != RSA_ALGORITHM_MAX - 2)
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
	std::unique_ptr<int[]> e(new int[RSA_ALGORITHM_MAX]);
	std::unique_ptr<int[]> n(new int[RSA_ALGORITHM_MAX]);
	int32_t index = -1;
	while (index++ != RSA_ALGORITHM_MAX - 1)
	{
		e[index] = atoi(vece[index].c_str());
		n[index] = atoi(vecn[index].c_str());
	}
	RsaAlgorithm rsa;
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
	std::unique_ptr<int[]> d(new int[RSA_ALGORITHM_MAX]);
	std::unique_ptr<int[]> n(new int[RSA_ALGORITHM_MAX]);
	int32_t index = -1;
	while (index++ != RSA_ALGORITHM_MAX - 1)
	{
		d[index] = atoi(vecd[index].c_str());
		n[index] = atoi(vecn[index].c_str());
	}
	RsaAlgorithm rsa;
	return rsa.tdecrypto(d.get(), n.get(), src);
}

bool CEncodeDecode::SM2Key(std::string& publicKeyX, std::string& publicKeyY, std::string& privateKey, bool hasPrefix)
{
#ifdef USE_OPENSSL
	privateKey.clear();
	publicKeyX.clear();
	publicKeyY.clear();

	SM2_KEY_PAIR key_pair;
	memset((void*)&key_pair, 0, sizeof(SM2_KEY_PAIR));

	int32_t error_code = sm2_create_key_pair(&key_pair);
	if (error_code != 0)
	{
		printf("Create SM2 key pair failed, error_code = %d\n", error_code);
		return false;
	}
	
	privateKey.append((char*)key_pair.pri_key, 32);

	publicKeyX.append((char*)key_pair.pub_key + (hasPrefix ? 0 : 1), (hasPrefix ? 33 : 32));
	publicKeyY.append((char*)key_pair.pub_key + 33, 32);
	return true;
#else
	return false;
#endif
}

bool CEncodeDecode::SM2Encode(const std::string& publicKeyX,
	const std::string& publicKeyY,
	const std::string& src,
	std::string& c1,
	std::string& c2,
	std::string& c3)
{
#ifdef USE_OPENSSL
	c1.clear();
	c2.clear();
	c3.clear();
	if ((publicKeyX.size() != 32 && publicKeyX.size() != 33) || publicKeyY.size() != 32 || src.empty())
	{
		return false;
	}
	std::string xKey = publicKeyX;
	if (xKey.size() == 32)
	{
		char prefix = 4;
		xKey.insert(0, 1, prefix);
	}

	c1.resize(65);
	c2.resize(src.size());
	c3.resize(32);
	
	int32_t error_code = sm2_encrypt_code((const unsigned char*)(&src[0]),
		(int)src.size(),
		(const unsigned char*)(&(xKey + publicKeyY)[0]),
		(unsigned char*)(&c1[0]),
		(unsigned char*)(&c3[0]),
		(unsigned char*)(&c2[0]));
	if (error_code != 0)
	{
		c1.clear();
		c2.clear();
		c3.clear();
		printf("Create SM2 ciphertext failed, error_code = %d\n", error_code);
		return false;
	}
	if (publicKeyX.size() == 32)
	{
		c1 = c1.substr(1, 32);
	}
	return true;
#else
	return false;
#endif
}

std::string CEncodeDecode::SM2Decode(const std::string& privateKey,
	const std::string& c1,
	const std::string& c2,
	const std::string& c3)
{
#ifdef USE_OPENSSL
	if ((c1.size() != 65 && c1.size() != 64) || c3.size() != 32 || c2.empty() || privateKey.size() != 32)
	{
		return "";
	}
	std::string result;
	result.resize(c2.size());
	std::string prefixC1 = c1;
	if (prefixC1.size() == 64)
	{
		char prefix = 4;
		prefixC1.insert(0, 1, prefix);
	}

	int32_t error_code = sm2_decrypt_code((const unsigned char*)(&c1[0]),
		(const unsigned char*)(&c3[0]),
		(const unsigned char*)(&c2[0]),
		(int)c2.size(),
		(const unsigned char*)(&privateKey[0]),
		(unsigned char*)(&result[0]));
	if (error_code != 0)
	{
		result.clear();
		printf("Decrypt SM2 ciphertext failed, error_code = %d\n", error_code);
		return "";
	}
	return result;
#else
	return "";
#endif
}

std::string CEncodeDecode::SM4Encode(const std::string& key, const std::string& src)
{
	if (key.size() != 16 || (src.size() % 16 != 0) || src.empty())
	{
		return "";
	}
	sm4_context ctx;
	//encrypt standard testing vector
	sm4_setkey_enc(&ctx, (unsigned char*)(&key[0]));
	std::string result;
	result.resize(src.size());
	size_t index = -1;
	size_t count = src.size() / 16;
	while (index++ != count - 1)
	{
		sm4_crypt_ecb(&ctx, 1, 16, (unsigned char*)(&src[index * 16]), (unsigned char*)(&result[index * 16]));
	}
	return result;
}

std::string CEncodeDecode::SM4Decode(const std::string& key, const std::string& ciphertext)
{
	if (key.size() != 16 || (ciphertext.size() % 16 != 0) || ciphertext.empty())
	{
		return "";
	}
	sm4_context ctx;
	sm4_setkey_dec(&ctx, (unsigned char*)(&key[0]));
	std::string result;
	result.resize(ciphertext.size());
	size_t index = -1;
	size_t count = ciphertext.size() / 16;
	while (index++ != count - 1)
	{
		sm4_crypt_ecb(&ctx, 0, 16, (unsigned char*)(&ciphertext[index * 16]), (unsigned char*)(&result[index * 16]));
	}
	return result;
}

X509* CEncodeDecode::getCert(const std::string& path)
{
#ifdef USE_OPENSSL
	unsigned char certData[4099] = {};
	unsigned char* pTmp = nullptr;
	FILE* fp = fopen(path.c_str(), "rb");
	if (fp == nullptr)
	{
		return nullptr;
	}
	//X509证书结构体，保存用户证书
	X509* cert = nullptr;
	size_t certlen = fread(certData, 1, 4096, fp);
	fclose(fp);
	//判断是否为DER编码的用户证书，并转化为X509结构体
	pTmp = certData;
	cert = d2i_X509(nullptr, (const unsigned char **)&pTmp, (long)certlen);
	if (cert != nullptr)
	{
		return cert;
	}
	//判断是否为PEM格式的数字证书
	BIO* b = BIO_new_file(path.c_str(), "r");
	PEM_read_bio_X509(b, &cert, nullptr, nullptr);
	BIO_free(b);
	return cert;
#else
	return nullptr;
#endif
}

void CEncodeDecode::releaseCert(X509* cert)
{
#ifdef USE_OPENSSL
	if (cert == nullptr)
	{
		return;
	}
	X509_free(cert);
#endif
}

std::string CEncodeDecode::getPublicKey(X509* cert)
{
#ifdef USE_OPENSSL
	if (cert == nullptr)
	{
		return "";
	}
	//保存证书公钥
	EVP_PKEY* pubKeytemp = nullptr;
	unsigned char* pTmp = nullptr;
	nullptr;
	//获取证书公钥
	pubKeytemp = X509_get_pubkey(cert);
	if (pubKeytemp == nullptr)
	{
		return "";
	}

	X509_PUBKEY* pubKeytempss = X509_get_X509_PUBKEY(cert);
	int derpubkeyLen = i2d_PublicKey(pubKeytemp, nullptr);
	if (derpubkeyLen < 1)
	{
		return "";
	}

	unsigned char* buf = (unsigned char*)malloc(derpubkeyLen);
	i2d_PublicKey(pubKeytemp, &buf);

	EC_KEY* ec_key = EVP_PKEY_get0_EC_KEY(pubKeytemp);
	if (ec_key == nullptr)
	{
		return "";
	}
	size_t buflen = EC_KEY_key2buf(ec_key, EC_KEY_get_conv_form(ec_key), (unsigned char**)&pTmp, nullptr);

	BIGNUM* pub_key_BIGNUM = BN_new();
	BN_bin2bn(pTmp, (int)buflen, pub_key_BIGNUM);
	char* hexKey = BN_bn2hex(pub_key_BIGNUM);
	std::string publicKeyData;
	publicKeyData.resize(1024);
	strcpy(&publicKeyData[0], hexKey);
	publicKeyData.resize(strlen((char*)(&publicKeyData[0])));

	EVP_PKEY_free(pubKeytemp);

	if (publicKeyData.size() % 2 != 0)
	{
		return "";
	}
	if (publicKeyData.empty())
	{
		return "";
	}

	std::string upperKey;
	std::transform(publicKeyData.begin(), publicKeyData.end(), std::back_inserter(upperKey), ::toupper);

	std::string result;
	int32_t upperKeyIndex = -1;
	while (upperKeyIndex++ != upperKey.size() - 1)
	{
		char first = upperKey[upperKeyIndex];
		char second = upperKey[++upperKeyIndex];
		if (first >= 65)
		{
			first = first - 7;
		}
		if (second >= 65)
		{
			second = second - 7;
		}
		result.push_back((first - 48) * 16 + (second - 48));
	}
	return result;
#else
	return "";
#endif
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