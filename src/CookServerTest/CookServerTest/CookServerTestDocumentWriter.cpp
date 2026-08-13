#include "CookServerTestDocumentWriter.h"
#include "CookServerTestConfig.h"
#include "CookServerTestHelper.h"
#include <fstream>
#include <sstream>

CookServerTestDocumentWriter::CookServerTestDocumentWriter()
{

}

CookServerTestDocumentWriter::~CookServerTestDocumentWriter()
{

}

bool CookServerTestDocumentWriter::writeDocuments() const
{
	bool featureOk = writeTextFile(g_cookServerTestConfig.cookServerFeatureDocumentPath(), buildFeatureDocument());
	bool deployOk = writeTextFile(g_cookServerTestConfig.cookServerDeployDocumentPath(), buildDeployDocument());
	return featureOk && deployOk;
}

bool CookServerTestDocumentWriter::writeTextFile(const std::string& path, const std::string& text) const
{
	std::ofstream output(path.c_str(), std::ios::binary | std::ios::trunc);
	if (!output.is_open())
	{
		return false;
	}
	std::string utf8Text = CookServerTestHelper::localTextToUtf8(text);
	while (!utf8Text.empty() &&
		(utf8Text[utf8Text.size() - 1] == '\r' ||
		utf8Text[utf8Text.size() - 1] == '\n' ||
		utf8Text[utf8Text.size() - 1] == ' ' ||
		utf8Text[utf8Text.size() - 1] == '\t'))
	{
		utf8Text.erase(utf8Text.size() - 1);
	}
	output.write(utf8Text.data(), static_cast<std::streamsize>(utf8Text.size()));
	output.flush();
	return output.good();
}

std::string CookServerTestDocumentWriter::buildFeatureDocument() const
{
	std::ostringstream oss;
	oss << "CookServer目前功能说明文档\r\n";
	oss << "\r\n";
	oss << "一、服务定位\r\n";
	oss << "CookServer 是做菜小程序/APP 的 HTTP 后端简化版本，当前核心目标是支撑 CookWx 这样的客户端完成刷菜谱、购买菜谱、选择菜单、生成做菜排程和开始做菜流程。\r\n";
	oss << "\r\n";
	oss << "二、当前已实现能力\r\n";
	oss << "1. HTTP JSON 接口服务。\r\n";
	oss << "默认监听端口 5207，返回 JSON 响应，支持 CORS，客户端可以通过 HTTP 调用接口。\r\n";
	oss << "\r\n";
	oss << "2. 健康检查。\r\n";
	oss << "GET /api/health 返回服务名、版本、公网 IP、监听地址和端口，用于部署后确认服务可访问。\r\n";
	oss << "\r\n";
	oss << "3. 菜谱目录。\r\n";
	oss << "GET /api/recipes 返回分类、菜谱列表和账号信息。服务启动时优先读取 CookServer.exe 同级目录下 res/recipes 目录内的单菜 JSON 文件，每个 JSON 文件表示一道菜，文件名必须等于菜谱 ID 加 .json。当前菜谱配置 schemaVersion 为 \"1.0\"，包括 cook_000001 糖醋排骨、cook_000002 电饭锅米饭、cook_000003 番茄炒蛋、cook_000004 蒜蓉青菜、cook_000005 豆腐紫菜汤、cook_000006 黑椒牛柳、cook_000007 照烧鸡腿。前五个为免费/系统菜谱，后两个为付费示例菜谱。\r\n";
	oss << "\r\n";
	oss << "4. 推荐流。\r\n";
	oss << "GET /api/feed 返回推荐、精选、关注、好友四个 tab，以及可展示在前端信息流中的菜谱卡片数据，包括标题、描述、作者、价格、点赞数、评论数和关联菜谱 ID。\r\n";
	oss << "\r\n";
	oss << "5. 账号金币。\r\n";
	oss << "GET /api/account 获取账号信息。POST /api/account/recharge 进行金币充值模拟。当前账号状态会保存到 CookServer.exe 同级目录下的 CookServerState.txt。账号内保存 ownedRecipeIds、purchasedRecipeIds 和 favoriteRecipeIds，真正下发哪些菜由服务端根据账号持有的菜谱 ID 和菜谱资源目录决定。\r\n";
	oss << "\r\n";
	oss << "6. 菜谱购买。\r\n";
	oss << "POST /api/recipes/purchase 根据 recipeId 购买付费菜谱。免费菜谱或已拥有菜谱会直接返回已拥有，金币不足会返回失败消息。\r\n";
	oss << "\r\n";
	oss << "7. 收藏切换。\r\n";
	oss << "POST /api/favorites/toggle 根据 recipeId 切换收藏状态，并返回最新账号和菜谱状态。\r\n";
	oss << "\r\n";
	oss << "8. 做菜排程预览。\r\n";
	oss << "POST /api/plan/preview 接收 recipeIds 数组，返回 summary、selectedRecipes、timeline 和 freeBlocks。summary 包含总耗时、操作耗时、自由时间、边角时间等字段。timeline 是按时间线安排好的做菜步骤。\r\n";
	oss << "\r\n";
	oss << "9. 开始做菜。\r\n";
	oss << "POST /api/plan/start 和预览接口类似，但会额外返回 planId，表示客户端可以进入本地按步骤执行和提醒的状态。\r\n";
	oss << "\r\n";
	oss << "10. 日志和崩溃信息。\r\n";
	oss << "CookServer 已接入 LogManager 日志库和 CDump。服务启动、请求开始/结束、充值、购买、收藏、排程等关键流程会输出日志，崩溃时可保留堆栈信息用于定位问题。\r\n";
	oss << "\r\n";
	oss << "三、当前尚未实现或仍是简化版的能力\r\n";
	oss << "1. 当前菜谱已经配置化为 res/recipes/*.json，但还没有接入后台管理、作者上传、内容审核和版本迁移工具。\r\n";
	oss << "2. 当前账号、购买和收藏状态使用本地文本文件保存，尚未接入 MySQL/Redis。\r\n";
	oss << "3. 当前支付、提现、评论、关注、好友、发布视频、上传图片、真实内容审核仍是前后端后续阶段功能。\r\n";
	oss << "4. 当前没有真实微信登录鉴权，接口通过 userId 参数模拟不同用户。\r\n";
	oss << "5. 当前 HTTP 服务还不是完整生产网关形态，正式给微信小程序使用时需要 HTTPS、域名备案/配置、反向代理和更完整的限流鉴权。\r\n";
	return oss.str();
}

std::string CookServerTestDocumentWriter::buildDeployDocument() const
{
	std::ostringstream oss;
	oss << "CookServer部署文档\r\n";
	oss << "\r\n";
	oss << "一、部署目标\r\n";
	oss << "本文档面向运维人员，说明如何把已经编译好的 CookServer.exe 部署到服务器并保持进程可运行。当前版本默认监听 TCP 5207，测试环境可直接通过 http://服务器IP:5207 访问。\r\n";
	oss << "\r\n";
	oss << "二、部署目录\r\n";
	oss << "1. 建议为每个服务单独创建目录，例如 D:\\CookServer 或 /opt/CookServer。\r\n";
	oss << "2. CookServer.exe 必须放在部署目录根部，启动时也建议把工作目录设置为该目录。\r\n";
	oss << "3. 当前版本会把状态文件、日志文件、dump 文件写到 CookServer.exe 同级目录，因此该目录必须允许运行账号读写和创建文件。\r\n";
	oss << "4. 不要把多个同名 CookServer 进程放在同一个目录同时运行，否则日志、状态文件和端口会互相影响。\r\n";
	oss << "\r\n";
	oss << "三、部署文件\r\n";
	oss << "1. 必须文件：CookServer.exe。\r\n";
	oss << "2. 如果部署的是 Debug/DLL 版本，还需要把同批次生成的依赖 DLL 放到 CookServer.exe 同级目录。Release 静态库版本通常只需要 CookServer.exe，但仍以交付包内实际文件为准。\r\n";
	oss << "3. res/recipes 是菜谱资源目录，必须放在 CookServer.exe 同级目录下。目录内每个 JSON 文件表示一道菜，文件名格式为 cook_000001.json，文件根对象必须包含 schemaVersion 字段，当前版本为 \"1.0\"。\r\n";
	oss << "4. 如果 res/recipes 目录缺失、没有有效 JSON 或 JSON 格式错误，当前版本会写日志并回退到程序内置示例菜谱；正式部署仍建议把 res/recipes 作为必备资源目录管理。\r\n";
	oss << "5. 当前监听端口、监听地址、公网 IP、默认金币、请求体大小等配置仍在程序内部的 Config 中。\r\n";
	oss << "6. CookServerState.txt 是运行时状态文件，不需要首次手动创建。全新部署可以不放该文件；需要保留测试账号金币、购买、收藏状态时，可以随 exe 一起备份和迁移。\r\n";
	oss << "7. 日志文件和 CookServerd_log/CookServer_log 等归档目录由程序运行时自动生成，不需要提前创建。\r\n";
	oss << "\r\n";
	oss << "四、服务器准备\r\n";
	oss << "1. 放通入站 TCP 5207，云服务器还需要在安全组中放通 5207。\r\n";
	oss << "2. 确认同一台机器上没有其他进程占用 5207。\r\n";
	oss << "3. Windows Server 2012 上建议使用专门的低权限运行账号，给部署目录读写权限即可。\r\n";
	oss << "4. 如果要给微信小程序正式环境访问，建议在前面增加 HTTPS 反向代理，由代理监听 443，转发到本机 5207。\r\n";
	oss << "\r\n";
	oss << "五、启动方式\r\n";
	oss << "1. 进入部署目录，直接运行 CookServer.exe。\r\n";
	oss << "2. 程序启动后会在控制台阻塞运行，不要关闭该窗口；需要后台常驻时，用 Windows 服务包装工具、任务计划程序或进程守护工具托管。\r\n";
	oss << "3. 启动后访问 http://127.0.0.1:5207/api/health，确认返回 ok=true。\r\n";
	oss << "4. 在外部机器访问 http://服务器IP:5207/api/health，确认防火墙和安全组配置正确。\r\n";
	oss << "\r\n";
	oss << "六、验收方式\r\n";
	oss << "1. 将 CookServerTest.exe 放到任意可以访问服务器的机器上运行。\r\n";
	oss << "2. 本机验收命令：CookServerTest.exe --host=127.0.0.1 --port=5207。\r\n";
	oss << "3. 远程验收命令：CookServerTest.exe --host=175.24.198.162 --port=5207。\r\n";
	oss << "4. 压力验收命令示例：CookServerTest.exe --host=175.24.198.162 --port=5207 --stressThreads=32 --stressLoops=100。\r\n";
	oss << "5. 测试报告会写到 CookServerTest.exe 同级目录下的 CookServerTestReport.txt。\r\n";
	oss << "\r\n";
	oss << "七、运行时文件说明\r\n";
	oss << "1. res/recipes/*.json：单菜菜谱配置文件。每个账号实际拥有哪些菜，只保存菜谱 ID，服务端根据这些 ID 从资源目录加载菜谱并下发。\r\n";
	oss << "2. CookServerState.txt：简化版账号状态文件，保存金币、购买、收藏、拥有菜谱 ID 等状态。\r\n";
	oss << "3. *.log：运行日志，线上排查优先查看最新日志文件和日志归档目录。\r\n";
	oss << "4. *.dmp：程序崩溃时由 CDump 生成的 dump 文件，用于定位崩溃堆栈。\r\n";
	oss << "5. 旧日志归档目录：程序重启后，LogManager 会按当前日志配置归档或清理旧日志。\r\n";
	oss << "\r\n";
	oss << "八、停止、升级和回滚\r\n";
	oss << "1. 停止服务前先确认没有正在执行的测试或客户端请求；当前版本可以直接关闭进程。\r\n";
	oss << "2. 升级前备份整个部署目录，至少保留 CookServer.exe、res/recipes、CookServerState.txt、日志和 dump。\r\n";
	oss << "3. 替换 CookServer.exe、res/recipes 和同批次依赖文件后重新启动服务。\r\n";
	oss << "4. 升级后必须执行 /api/health 验证和 CookServerTest 黑盒测试。\r\n";
	oss << "5. 回滚时停止当前进程，恢复备份文件，再重新启动旧版本。\r\n";
	return oss.str();
}