#pragma once
#include <string>
#include <vector>
#include <stdint.h>
#include "CiniMacro.h"

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
class CSimpleIniTempl;
template<class SI_CHAR>
struct SI_NoCase;
template<class SI_CHAR>
class SI_ConvertA;
typedef CSimpleIniTempl < char, SI_NoCase<char>, SI_ConvertA<char> > CSimpleIniA;

/** ini������
*/
class CiniAPI Cini
{
public:
	/** ���캯��
	@param [in] iniPath �ļ�·��
	@param [in] useSimpleIni �Ƿ�ʹ��SimpleIni��Դ��
	@param [in] iniPath �ļ��Ƿ���utf-8���룬���ǵĻ�����ANSI��ʽ
	*/
	Cini(const std::string& iniPath, bool useSimpleIni = false, bool isUtf8 = true);

public:
	/** ����WindowsϵͳAPI��ȡʱ���������̶ȣ�Ĭ����1024
	@param [in] length ��󳤶�
	*/
	void setMaxLength(int32_t length);

	/** ����WindowsϵͳAPI��ȡʱ�ڵ���������Ĭ����1024
	@param [in] sectionNum ���ڵ����
	*/
	void setMaxSection(int32_t sectionNum);

	/** ��ȡ����
	@param [in] key ��ֵ
	@param [in] section �ڵ�ֵ
	*/
	std::string readIni(const std::string& key, const std::string& section = "Section") const;

	/** д������
	@param [in] key ��ֵ���������򴴽����������޸�
	@param [in] value ����
	@param [in] section �ڵ�ֵ
	*/
	void writeIni(const std::string& key, const std::string& value, const std::string& section = "Section") const;

	/** ��ȡ���нڵ㣨�ýӿ�ֻ��SimpleIni����Ч��
	@return �������нڵ�
	*/
	std::vector<std::string> getAllSection() const;

	/** ��ȡ���нڵ��µļ�ֵ�����ýӿ�ֻ��SimpleIni����Ч��
	@return �������нڵ��µļ�ֵ��
	*/
	std::vector<std::string> getAllKey(const std::string& section = "Section") const;

	/** ��ȡ�ýڵ��µ�һ����ֵ��
	@param [in] section �ڵ�ֵ
	@return ���ظýڵ��µ�һ����ֵ��
	*/
	std::string getFirstKey(const std::string& section = "Section") const;

	/** ɾ������
	@param [in] key ��ֵ
	@param [in] section �ڵ�ֵ
	*/
	void deleteKey(const std::string& key, const std::string& section = "Section") const;

	/** ɾ���ڵ�
	@param [in] section �ڵ�ֵ
	*/
	void deleteSection(const std::string& section = "Section") const;

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_iniPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_length;
	int32_t m_sectionNum;
	bool m_useSimpleIni;
	bool m_isUtf8;
	CSimpleIniA* m_simpleIni;
	bool m_loadSuccess;
};