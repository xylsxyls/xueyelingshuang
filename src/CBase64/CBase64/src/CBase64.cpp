#include "CBase64.h"
#include <string>

std::string CBase64::encode(const char* str, int32_t bytes, Base64Options options /*= Base64Encoding*/)
{
	const char alphabet_base64[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
		"ghijklmn" "opqrstuv" "wxyz0123" "456789+/";
	const char alphabet_base64url[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
		"ghijklmn" "opqrstuv" "wxyz0123" "456789-_";
	const char* const alphabet = options & Base64UrlEncoding ? alphabet_base64url : alphabet_base64;
	const char padchar = '=';
	int padlen = 0;

	std::string tmp;
	tmp.resize((bytes + 2) / 3 * 4);
	//QByteArray tmp((d->size + 2) / 3 * 4, Qt::Uninitialized);

	int i = 0;
	char* out = (char*)tmp.data();
	while (i < bytes)
	{
		// encode 3 bytes at a time
		int chunk = 0;
		chunk |= int(unsigned char(str[i++])) << 16;
		if (i == bytes)
		{
			padlen = 2;
		}
		else
		{
			chunk |= int(unsigned char(str[i++])) << 8;
			if (i == bytes)
			{
				padlen = 1;
			}
			else
			{
				chunk |= int(unsigned char(str[i++]));
			}
		}

		int j = (chunk & 0x00fc0000) >> 18;
		int k = (chunk & 0x0003f000) >> 12;
		int l = (chunk & 0x00000fc0) >> 6;
		int m = (chunk & 0x0000003f);
		*out++ = alphabet[j];
		*out++ = alphabet[k];

		if (padlen > 1)
		{
			if ((options & OmitTrailingEquals) == 0)
			{
				*out++ = padchar;
			}
		}
		else
		{
			*out++ = alphabet[l];
		}
		if (padlen > 0)
		{
			if ((options & OmitTrailingEquals) == 0)
			{
				*out++ = padchar;
			}
		}
		else
		{
			*out++ = alphabet[m];
		}
	}
	//Q_ASSERT((options & OmitTrailingEquals) || (out == tmp.size() + tmp.data()));
	if (options & OmitTrailingEquals)
	{
		tmp.substr(0, out - tmp.data());
	}
	return tmp;
}

std::string CBase64::decode(const char* str, int32_t bytes, Base64Options options /*= Base64Encoding*/)
{
	unsigned int buf = 0;
	int nbits = 0;
	std::string tmp;
	tmp.resize(bytes * 3 / 4);
	//QByteArray tmp((base64.size() * 3) / 4, Qt::Uninitialized);

	int offset = 0;
	for (int i = 0; i < bytes; ++i)
	{
		int ch = str[i];
		int d;

		if (ch >= 'A' && ch <= 'Z')
		{
			d = ch - 'A';
		}
		else if (ch >= 'a' && ch <= 'z')
		{
			d = ch - 'a' + 26;
		}
		else if (ch >= '0' && ch <= '9')
		{
			d = ch - '0' + 52;
		}
		else if (ch == '+' && (options & Base64UrlEncoding) == 0)
		{
			d = 62;
		}
		else if (ch == '-' && (options & Base64UrlEncoding) != 0)
		{
			d = 62;
		}
		else if (ch == '/' && (options & Base64UrlEncoding) == 0)
		{
			d = 63;
		}
		else if (ch == '_' && (options & Base64UrlEncoding) != 0)
		{
			d = 63;
		}
		else
		{
			d = -1;
		}
			
		if (d != -1)
		{
			buf = (buf << 6) | d;
			nbits += 6;
			if (nbits >= 8)
			{
				nbits -= 8;
				tmp[offset++] = buf >> nbits;
				buf &= (1 << nbits) - 1;
			}
		}
	}

	tmp.substr(0, offset);
	return tmp;
}