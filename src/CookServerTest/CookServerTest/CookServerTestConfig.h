#pragma once
#include <stdint.h>
#include <string>

#define g_cookServerTestConfig CookServerTestConfig::instance()

/** CookServerTest运行配置，负责保存目标服务地址、超时和压力测试规模
*/
class CookServerTestConfig
{
public:
	/** 获取全局测试配置实例
	@return 返回配置实例引用
	*/
	static CookServerTestConfig& instance();

	/** 构造函数，只初始化成员默认值
	*/
	CookServerTestConfig();

	/** 析构函数，配置对象不持有系统资源
	*/
	~CookServerTestConfig();

	/** 初始化配置，并解析命令行覆盖项
	@param [in] argc 命令行参数数量
	@param [in] argv 命令行参数内容
	@return 返回是否初始化成功
	*/
	bool init(int32_t argc, char* argv[]);

	/** 反初始化配置，允许测试程序在同一进程内重复初始化
	*/
	void uninit();

	/** 获取测试报告输出完整路径
	@return 返回CookServerTestReport.txt完整路径
	*/
	std::string reportPath() const;

	/** 获取CookServer功能说明文档完整路径
	@return 返回CookServer功能说明文档完整路径
	*/
	std::string cookServerFeatureDocumentPath() const;

	/** 获取CookServer部署说明文档完整路径
	@return 返回CookServer部署说明文档完整路径
	*/
	std::string cookServerDeployDocumentPath() const;

private:
	/** 从命令行参数中解析--key=value形式的配置项
	@param [in] argument 单个命令行参数
	@return 返回该参数是否被识别并处理
	*/
	bool parseArgument(const std::string& argument);

	/** 将字符串解析为int32_t并限制最小最大值
	@param [in] text 数字字符串
	@param [in] defaultValue 解析失败时使用的默认值
	@param [in] minValue 最小允许值
	@param [in] maxValue 最大允许值
	@return 返回解析后的整数
	*/
	int32_t parseInt32(const std::string& text, int32_t defaultValue, int32_t minValue, int32_t maxValue) const;

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// 目标CookServer主机地址，默认使用本机，避免压力测试误打公网服务器
	std::string m_host;
	// 测试用户ID前缀，每次运行会默认附加当前进程ID和时间戳
	std::string m_userIdPrefix;
	// 测试报告输出文件名
	std::string m_reportFileName;
	// CookServer功能说明文档文件名
	std::string m_featureDocumentFileName;
	// CookServer部署文档文件名
	std::string m_deployDocumentFileName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	// 目标CookServer HTTP端口
	int32_t m_port;
	// 单次HTTP连接、发送和接收超时时间，单位毫秒
	int32_t m_timeoutMs;
	// 单次HTTP响应最大允许字节数
	int32_t m_maxResponseBytes;
	// 压力测试线程数量
	int32_t m_stressThreadCount;
	// 每个压力测试线程循环次数
	int32_t m_stressLoopCount;
	// 是否执行压力测试
	bool m_enableStress;
	// 是否已经完成初始化
	bool m_isInit;
};