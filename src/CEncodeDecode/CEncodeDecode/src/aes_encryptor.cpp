#include "aes_encryptor.h"
#include <climits>
#include <cstring>
#include <fstream>

AesEncryptor::AesEncryptor(unsigned char* key) :
m_encryptor(key)
{

}

AesEncryptor::~AesEncryptor()
{

}

int AesEncryptor::Char2Int(char value)
{
    if (value >= '0' && value <= '9')
    {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f')
    {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F')
    {
        return value - 'A' + 10;
    }
    return -1;
}

std::string AesEncryptor::EncryptString(const std::string& strInfor)
{
    // 保持历史格式：恰好整块时仍追加一个全零块；先检查再做长度加乘
    if (strInfor.size() > static_cast<size_t>(INT_MAX - 16))
    {
        return "";
    }
    try
    {
        size_t padded = strInfor.size() + 16 - strInfor.size() % 16;
        std::string buffer(padded, '\0');
        memcpy(&buffer[0], strInfor.data(), strInfor.size());
        if (!m_encryptor.Cipher(&buffer[0], static_cast<int>(padded)))
        {
            return "";
        }
        if (padded > buffer.max_size() / 2)
        {
            return "";
        }
        std::string output(padded * 2, '\0');
        const char* digits = "0123456789ABCDEF";
        for (size_t i = 0; i < padded; ++i)
        {
            unsigned char value = static_cast<unsigned char>(buffer[i]);
            output[i * 2] = digits[value >> 4];
            output[i * 2 + 1] = digits[value & 15];
        }
        return output;
    }
    catch (...)
    {
        return "";
    }
}

std::string AesEncryptor::DecryptString(const std::string& strMessage)
{
    std::string output;
    decryptText(strMessage, output);
    return output;
}

bool AesEncryptor::decryptText(const std::string& strMessage, std::string& output)
{
    output.clear();
    if (strMessage.empty() || strMessage.size() % 32 != 0 ||
        strMessage.size() / 2 > static_cast<size_t>(INT_MAX))
    {
        return false;
    }
    try
    {
        std::string buffer(strMessage.size() / 2, '\0');
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            int high = Char2Int(strMessage[i * 2]);
            int low = Char2Int(strMessage[i * 2 + 1]);
            if (high < 0 || low < 0)
            {
                return false;
            }
            buffer[i] = static_cast<char>((high << 4) | low);
        }
        if (!m_encryptor.InvCipher(&buffer[0], static_cast<int>(buffer.size())))
        {
            return false;
        }
        size_t end = buffer.find('\0');
        if (end != std::string::npos)
        {
            buffer.resize(end);
        }
        output.swap(buffer);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool AesEncryptor::readText(const char* path, std::string& data)
{
    data.clear();
    if (!path)
    {
        return false;
    }
    try
    {
        std::ifstream input(path);
        if (!input)
        {
            return false;
        }
        char block[4096];
        while (input.read(block, sizeof(block)) || input.gcount() != 0)
        {
            size_t count = static_cast<size_t>(input.gcount());
            if (data.size() > static_cast<size_t>(INT_MAX - 16) - count)
            {
                data.clear();
                return false;
            }
            data.append(block, count);
        }
        if (input.bad() || !input.eof())
        {
            data.clear();
            return false;
        }
        return true;
    }
    catch (...)
    {
        data.clear();
        return false;
    }
}

void AesEncryptor::EncryptTxtFile(const char* inputFileName, const char* outputFileName)
{
    std::string text;
    if (!outputFileName || !readText(inputFileName, text))
    {
        return;
    }
    std::string encoded = EncryptString(text);
    if (encoded.empty())
    {
        return;
    }
    try
    {
        std::ofstream output(outputFileName);
        output.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
    }
    catch (...)
    {
        return;
    }
}

void AesEncryptor::DecryptTxtFile(const char* inputFileName, const char* outputFileName)
{
    std::string encoded;
    if (!outputFileName || !readText(inputFileName, encoded))
    {
        return;
    }
    std::string text;
    if (!decryptText(encoded, text))
    {
        return;
    }
    try
    {
        std::ofstream output(outputFileName);
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
    }
    catch (...)
    {
        return;
    }
}