#pragma once
#include "ConfigManagerMacro.h"
#include <string>
#include <stdint.h>
#include <memory>

/********************************************************************************
@�궨��ʹ��˵��
1.keyֵ��sectionֵ����ʹ��ö��ֵ��sectionֵΨһ��keyֵ��sectionֵ��Ψһ��ö��ֵ�����ļ���common�ļ�����
2.��ʼ������
INIT_CONFIG(dbPath);
���û�е��øú�����ִ�д洢����exeͬ��Ŀ¼���Զ�����һ����exeͬ����db�ļ�
3.�洢ȫ�֣�ʹ�ú�GLOBAL_CONFIG
GLOBAL_CONFIG[keyֵ] = value;
GLOBAL_CONFIG[sectionֵ][keyֵ] = value;
����value����int32_t,uint64_t,double,std::string
�������ַ���Ҳ���ԣ�����
std::string str = "1234567";
str[3] = 0;
GLOBAL_CONFIG[sectionֵ][keyֵ] = str;
�ظ��洢�Ḳ��
4.�洢�û���أ�ʹ�ú�USER_CONFIG
����Ҫ�����û���userId
SET_CONFIG_USERID(userIdֵ);
ִ������仰��֮����û���ش洢���ڸ�userIdֵ��
USER_CONFIG[keyֵ] = value;
USER_CONFIG[sectionֵ][keyֵ] = value;
�÷�ͬ��
userIdֵ������;�ı䣬���ı�֮��Ĵ洢���͸ı�֮���userIdֵ���
����
SET_CONFIG_USERID(1000001);
USER_CONFIG[keyֵ] = value1;
SET_CONFIG_USERID(1000002);
USER_CONFIG[keyֵ] = value2;
������õ�value1��ֵ�����������1000001����
5.��ȡ�洢����
����ת��Ϊ4�����ͣ�int32_t,uint64_t,double,std::string
double result = GLOBAL_CONFIG[sectionֵ][keyֵ].toDouble();
double result = USER_CONFIG[sectionֵ][keyֵ].toDouble();
resultΪ�����ļ��е�ֵ
������ǵô�������ͣ������Ƚ��в鿴
std::string type = USER_CONFIG[sectionֵ][keyֵ].type();
int32_t��ӦTYPE_INT32
uint64_t��ӦTYPE_UINT64
double��ӦTYPE_DOUBLE
std::string��ӦTYPE_BLOB
6.ɾ������
DELETE_GLOBAL_CONFIG(keyֵ);
DELETE_GLOBAL_CONFIG(sectionֵ, keyֵ);
DELETE_USER_CONFIG(keyֵ);
DELETE_USER_CONFIG(sectionֵ, keyֵ);
ע����ɾ���û��������ʱuserId����ȷ��
7.ע��
���в�����������������������д
GLOBAL_CONFIG[keyֵ];
GLOBAL_CONFIG[keyֵ] = value;
����д�������
Fun(GLOBAL_CONFIG[keyֵ].toInt32());
8.����
Ŀǰһ�����430���������ݣ���ѯ��������ʱ��
��������������������ٶȿ�����ִ�д洢ǰ����CONFIG_BEGIN��ִ�ж����洢����ִ��CONFIG_END
����
CONFIG_BEGIN;
for(int32_t index = 0; index < 10000; ++index)
{
	GLOBAL_CONFIG[index] = index;
}
CONFIG_END;
9.�洢����
Ŀǰ�洢����ΪSAVE_LENGTH��(4G - 1)��С��uint32_t����
10.���̶߳����
֧�ֶ��̶߳����ͬʱʹ�ã�֧�ִ���ͬһ�����ݿ��ļ�
11.�÷�ʵ��
�洢�û���¼�˺��б�
GLOBAL_CONFIG[��¼�˺��б�����ö��ֵ][��ѡ�����˺�λ�õ����к�] = �������ַ���(��¼�˺�\0��¼����\0�û�ͷ��·��);
�洢��ɾ����ǰ�˺����������Ϸ��
SET_CONFIG_USERID(����ʱ�û���IDֵ);
USER_CONFIG[���������Ϸ������ö��ֵ] = ����������Ϸ��;
DELETE_USER_CONFIG(���������Ϸ������ö��ֵ);
********************************************************************************/

#define TYPE_BLOB "blob"
#define TYPE_INT32 "int32_t"
#define TYPE_UINT64 "uint64_t"
#define TYPE_DOUBLE "double"

#define SAVE_LENGTH 4294967295

#define GET_MACRO(_1, _2, NAME,...) NAME

#define DELETE_GLOBAL_CONFIG_FUN_1(key) ConfigManager::instance().deleteConfig(key)
#define DELETE_GLOBAL_CONFIG_FUN_2(section, key) ConfigManager::instance().deleteConfig(section, key)
#define DELETE_GLOBAL_CONFIG(...) GET_MACRO(__VA_ARGS__, DELETE_GLOBAL_CONFIG_FUN_2, DELETE_GLOBAL_CONFIG_FUN_1)(__VA_ARGS__)

#define DELETE_USER_CONFIG_FUN_1(key) (*(ConfigManager::instance().getUserConfigManager())).deleteConfig(key)
#define DELETE_USER_CONFIG_FUN_2(section, key) (*(ConfigManager::instance().getUserConfigManager())).deleteConfig(section, key)
#define DELETE_USER_CONFIG(...) GET_MACRO(__VA_ARGS__, DELETE_USER_CONFIG_FUN_2, DELETE_USER_CONFIG_FUN_1)(__VA_ARGS__)

#define INIT_CONFIG(dbPath) ConfigManager::instance().init(dbPath)
#define GLOBAL_CONFIG ConfigManager::instance()
#define USER_CONFIG (*(ConfigManager::instance().getUserConfigManager()))
#define SET_CONFIG_USERID(userId) ConfigManager::instance().setUserId(userId)

//��ʱ������
//#define CONFIG_BEGIN ConfigManager::instance().transaction()
//#define CONFIG_END ConfigManager::instance().commit()

class SQLite;
class UserConfigManager;
class SQLiteResultSet;
class ProcessReadWriteMutex;

/** ���ù�����
*/
class ConfigManagerAPI ConfigManager
{
protected:
	/** ���캯��
	*/
	ConfigManager();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static ConfigManager& instance();

public:
	/** ����section��key
	@param [in] sectionOrKey section��keyֵ
	@return ���ع�����
	*/
	ConfigManager& operator[] (int32_t sectionOrKey);

	/** ����int32_tֵ
	@param [in] value int32_tֵ
	@return ���ع�����
	*/
	ConfigManager operator = (int32_t value);

	/** ����uint64_tֵ
	@param [in] value uint64_tֵ
	@return ���ع�����
	*/
	ConfigManager operator = (uint64_t value);

	/** ����doubleֵ
	@param [in] value doubleֵ
	@return ���ع�����
	*/
	ConfigManager operator = (double value);

	/** �����ַ�����������ַ���
	@param [in] value �ַ�����������ַ���
	@return ���ع�����
	*/
	ConfigManager operator = (const std::string& value);

	/** ��ȡ�洢����
	@return ���ش洢����
	*/
	std::string type();

	/** ת��Ϊint32_t����
	@return ����int32_t����
	*/
	int32_t toInt32();

	/** ת��Ϊuint64_t����
	@return ����uint64_t����
	*/
	uint64_t toUint64();

	/** ת��Ϊdouble����
	@return ����double����
	*/
	double toDouble();

	/** ת��Ϊstd::string���ͣ�������ͨ�ַ����Ͷ������ַ���
	@return ����std::string����
	*/
	std::string toString();

public:
	/** ��ʼ�������ļ�·������������øú������Զ�������Ŀ¼�º�ִ���ļ�ͬ����db�ļ�
	@param [in] configPath �����ļ�·��
	*/
	void init(const std::string& configPath = "");

	/** �������ݿ�
	*/
	void initSQLite();

	/** �����û�ID
	@param [in] userId �û�ID
	*/
	void setUserId(uint64_t userId);

	/** ��ȡ�û�������ù�����
	@return �����û�������ù�����
	*/
	UserConfigManager* getUserConfigManager();

	/** ɾ������
	@param [in] key keyֵ
	*/
	void deleteConfig(int32_t key);

	/** ɾ������
	@param [in] key keyֵ
	@param [in] section �ڵ�ֵ
	*/
	void deleteConfig(int32_t section, int32_t key);

	/** ��������
	*/
	void transaction();

	/** �ύ
	*/
	void commit();

protected:
	/** ������ã��������ַ�����
	@param [in] key keyֵ
	@param [in] value �������ַ���
	@param [in] section �ڵ�ֵ
	*/
	void addConfig(int32_t key, const std::string& value, int32_t section = 0);

	/** ��ȡ���ã��������ַ�����
	@param [in] key keyֵ
	@param [out] value �������ַ���
	@param [in] section �ڵ�ֵ
	*/
	void getConfig(int32_t key, std::string& value, int32_t section = 0);

	/** ������ã����Σ�
	@param [in] key keyֵ
	@param [in] value ����
	@param [in] section �ڵ�ֵ
	*/
	void addConfig(int32_t key, int32_t value, int32_t section = 0);

	/** ������ã����Σ�
	@param [in] key keyֵ
	@param [out] value ����
	@param [in] section �ڵ�ֵ
	*/
	void getConfig(int32_t key, int32_t& value, int32_t section = 0);

	/** ������ã��޷��ų����ͣ�
	@param [in] key keyֵ
	@param [in] value �޷��ų�����
	@param [in] section �ڵ�ֵ
	*/
	void addConfig(int32_t key, uint64_t value, int32_t section = 0);

	/** ������ã��޷��ų����ͣ�
	@param [in] key keyֵ
	@param [out] value �޷��ų�����
	@param [in] section �ڵ�ֵ
	*/
	void getConfig(int32_t key, uint64_t& value, int32_t section = 0);

	/** ������ã�˫���ȸ����ͣ�
	@param [in] key keyֵ
	@param [in] value ˫���ȸ�����
	@param [in] section �ڵ�ֵ
	*/
	void addConfig(int32_t key, double value, int32_t section = 0);

	/** ������ã�˫���ȸ����ͣ�
	@param [in] key keyֵ
	@param [out] value ˫���ȸ�����
	@param [in] section �ڵ�ֵ
	*/
	void getConfig(int32_t key, double& value, int32_t section = 0);

	/** ��ȡ��������
	@param [in] key keyֵ
	@param [in] section �ڵ�ֵ
	@return ������������
	*/
	std::string getConfigType(int32_t key, int32_t section = 0);

protected:
	void addConfigBase(int32_t key, const std::string& value, const std::string& type, int32_t section);
	bool hasConfigBase(int32_t key, int32_t section);
	bool updateConfigBase(int32_t key, const std::string& value, const std::string& type, int32_t section);
	bool insertConfigBase(int32_t key, const std::string& value, const std::string& type, int32_t section);
	std::string getConfigBase(int32_t key, int32_t section);
	void createTableIfNotExist(const std::string& tableName);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<SQLite> m_spConfig;
	std::shared_ptr<ProcessReadWriteMutex> m_spProcessMutex;
	std::string m_tableName;
	std::string m_databasePath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_key;
	int32_t m_section;
};