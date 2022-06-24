#pragma once
#include <string>
#include <map>
#include <mutex>
#include <functional>

namespace leveldb
{
	class DB;
}

/** LevelDB������
*/
class LevelDBManager
{
protected:
	/** ���캯��
	*/
	LevelDBManager();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static LevelDBManager& instance();

	/** �������캯��
	@param [in] other ������
	*/
	LevelDBManager(const LevelDBManager& other) = delete;

	/** ��ֵ���캯��
	@param [in] other ��ֵ��
	*/
    LevelDBManager& operator=(const LevelDBManager& other) = delete;

public:
	/** ��DB�ļ����̰߳�ȫ
	@param [in] path �ļ�·�����ļ������ڻ��Զ��������ļ�����·��������ڷ��򴴽�ʧ��
	@param [in] fileId �ļ�ID��������ļ�ID�������ĸ��ļ������ļ�IDΪ0ʱ���������map
	*/
	void openDBFile(const std::string& path, int32_t fileId = 0);

	/** �������ã��̰߳�ȫ
	@param [in] key ���ùؼ���
	@param [in] value ����ֵ
	@param [in] fileId �ļ�ID
	*/
	void setConfig(const std::string& key, const std::string& value, int32_t fileId = 0);

	/** ��ȡ���ã��̰߳�ȫ
	@param [in] key ���ùؼ���
	@param [out] value ����ֵ
	@param [in] fileId �ļ�ID
	@return �����Ƿ��ҵ���������
	*/
	bool getConfig(const std::string& key, std::string& value, int32_t fileId = 0);

	/** ����ƥ�����ü����Ͷ�ȡ���ò�ͬ���ǣ������key����ƥ�䣬���ҵ�ƥ��ľͻ�ѭ������ص���������ѭ������ʱfindConfig�����˳����̰߳�ȫ
	@param [in] key ���ùؼ��֣�����ƥ��
	@param [in] findConfigFun ���Ϊkey���Ҳ�Ϊvalue
	@param [in] fileId �ļ�ID
	*/
	void findConfig(const std::string& key, const std::function<void(const std::string&, const std::string&)>& findConfigFun, int32_t fileId = 0);

	/** ɾ�����ã��̰߳�ȫ
	@param [in] key ���ùؼ���
	@param [in] fileId �ļ�ID
	*/
	void deleteConfig(const std::string& key, int32_t fileId = 0);

	/** �ر�DB�ļ����̰߳�ȫ
	@param [in] fileId �ļ�ID
	*/
	void closeDBFile(int32_t fileId = 0);

protected:
	/** �����ļ�ָ��
	@param [in] fileId �ļ�ID
	@return �����ļ�ָ��
	*/
	leveldb::DB* findDBFile(int32_t fileId);

private:
	//Ĭ���ļ�ָ�룬�ļ�IDΪ0
	leveldb::DB* m_defaultDBFile;
	//���ļ�IDΪ0֮����ļ�ָ��
	std::map<int32_t, leveldb::DB*> m_dbFile;
	//���ڴ򿪹ر��ļ��Ķ��߳�ͬ����
	std::mutex m_mutex;
};