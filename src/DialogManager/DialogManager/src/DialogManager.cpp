#include "DialogManager.h"
#include "DialogRunTimeManager.h"

DialogManager::DialogManager()
{
	QObject::connect(&DialogRunTimeManager::instance(),
					 &DialogRunTimeManager::dialogSignal,
					 this,
					 &DialogManager::dialogSignal,
					 Qt::UniqueConnection);
}

DialogManager::~DialogManager()
{
	uninit();
}

bool DialogManager::registerCustomDialogFactory(DialogType dialogType,
											   CustomDialogFactory* factory,
											   CustomDialogFactoryDestroy destroyFunction)
{
	return DialogRunTimeManager::instance().registerCustomDialogFactory(dialogType, factory, destroyFunction);
}

void DialogManager::unregisterCustomDialogFactory(DialogType dialogType)
{
	DialogRunTimeManager::instance().unregisterCustomDialogFactory(dialogType);
}

bool DialogManager::registerCustomViewFactory(DialogType dialogType,
										 CustomViewFactory* factory,
										 DialogShowMode showMode,
										 CustomViewFactoryDestroy destroyFunction)
{
	return DialogRunTimeManager::instance().registerCustomViewFactory(dialogType, factory, showMode, destroyFunction);
}

void DialogManager::unregisterCustomViewFactory(DialogType dialogType)
{
	DialogRunTimeManager::instance().unregisterCustomViewFactory(dialogType);
}

void DialogManager::makeDialog(DialogParam& param)
{
	DialogRunTimeManager::instance().makeDialog(param);
}

void DialogManager::operateDialog(OperateParam& param)
{
	DialogRunTimeManager::instance().operateDialog(param);
}

void DialogManager::uninit()
{
	if (DialogRunTimeManager::hasInstance())
	{
		DialogRunTimeManager::instance().uninit();
	}
}

#if defined(DIALOGMANAGER_ENABLE_TEST_MAIN)
// DialogManager手动测试入口示例：
// 需要手动验证DialogManager时，定义DIALOGMANAGER_ENABLE_TEST_MAIN，
// 将库工程临时改成EXE工程运行；测试结束后再恢复为库工程。
#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <new>
#include "CustomDialog.h"
#include "CustomViewBase.h"

static const DialogType kDialogManagerTestViewType = CUSTOM_DIALOG_TYPE_BEGIN + 1;
static const DialogType kDialogManagerTestDialogType = CUSTOM_DIALOG_TYPE_BEGIN + 2;
static const quint64 kDialogManagerTestOrderDetailBusiness = 10001;
static const quint64 kDialogManagerTestOrderEditBusiness = 10002;
static const quint64 kDialogManagerTestOrderLogBusiness = 10003;
static const quint64 kDialogManagerTestDownloadBusiness = 20001;

class DialogManagerTestView : public CustomViewBase
{
public:
	/** 构造函数，只创建内容区本身，具体展示文本由initView和updateView按参数刷新
	@param [in] parent 父控件指针
	*/
	explicit DialogManagerTestView(QWidget* parent = nullptr) :
	CustomViewBase(parent),
	m_label(nullptr)
	{

	}

	/** 初始化内容区，验证自定义View能被CustomDialog承载并接收业务复用键
	@param [in] param 创建窗口时传入的参数
	@return 返回true表示内容区可以正常显示
	*/
	bool initView(const DialogParam& param)
	{
		QVBoxLayout* layout = new (std::nothrow) QVBoxLayout(this);
		m_label = new (std::nothrow) QLabel(this);
		if (layout == nullptr || m_label == nullptr)
		{
			return false;
		}
		layout->addWidget(m_label);
		setLayout(layout);
		updateView(param);
		return true;
	}

	/** 返回测试窗口期望尺寸，让手动测试时不用依赖外层默认尺寸
	@return 返回测试内容区期望尺寸
	*/
	QSize preferredSize() const
	{
		return QSize(320, 160);
	}

	/** 复用同一业务窗口时刷新内容，用来验证makeDialog命中旧窗口后不会重新创建
	@param [in] param 再次makeDialog时传入的新参数
	*/
	void updateView(const DialogParam& param)
	{
		if (m_label == nullptr)
		{
			return;
		}
		m_label->setText(QStringLiteral("businessId=%1, userId=%2, title=%3")
			.arg(param.m_businessId).arg(param.m_userId).arg(param.m_title));
	}

private:
	// 测试内容文本，显示当前窗口绑定的业务键和标题
	QLabel* m_label;
};

class DialogManagerTestViewFactory : public CustomViewFactory
{
public:
	/** 创建测试内容区，DialogManager会把它放进默认CustomDialog壳子中
	@param [in] param 创建窗口时传入的参数
	@return 返回新建内容区，失败时返回nullptr
	*/
	CustomViewBase* createView(const DialogParam& param)
	{
		Q_UNUSED(param);
		return new (std::nothrow) DialogManagerTestView;
	}
};

class DialogManagerTestDialogFactory : public CustomDialogFactory
{
public:
	/** 创建完整自定义窗口壳子，验证业务可以替换CustomDialog外层行为
	@param [in] param 创建窗口时传入的参数
	@return 返回自定义窗口，失败时返回nullptr
	*/
	CustomDialog* createDialog(const DialogParam& param)
	{
		CustomDialog* dialog = nullptr;
		CustomViewBase* view = nullptr;
		try
		{
			dialog = new (std::nothrow) CustomDialog;
			if (dialog == nullptr)
			{
				return nullptr;
			}
			view = createView(param);
			if (view == nullptr)
			{
				delete dialog;
				return nullptr;
			}
			if (!dialog->setView(view))
			{
				delete view;
				delete dialog;
				return nullptr;
			}
			dialog->setShowMode(MODELESS_DIALOG_SHOW_MODE);
		}
		catch (...)
		{
			delete view;
			delete dialog;
			return nullptr;
		}
		return dialog;
	}

	/** 创建完整自定义窗口内部的内容区
	@param [in] param 创建窗口时传入的参数
	@return 返回新建内容区，失败时返回nullptr
	*/
	CustomViewBase* createView(const DialogParam& param)
	{
		Q_UNUSED(param);
		return new (std::nothrow) DialogManagerTestView;
	}

	/** 测试工厂固定使用普通非模态窗口，避免自动化测试被exec阻塞
	@param [in] param 创建窗口时传入的参数
	@return 返回普通非模态展示模式
	*/
	DialogShowMode showMode(const DialogParam& param) const
	{
		Q_UNUSED(param);
		return MODELESS_DIALOG_SHOW_MODE;
	}
};

/** 销毁自定义View工厂，验证业务传入的销毁函数能接管跨模块释放方式
@param [in] factory 需要释放的自定义View工厂
*/
static void DestroyDialogManagerTestViewFactory(CustomViewFactory* factory)
{
	delete factory;
}

/** 销毁完整自定义Dialog工厂，验证注销工厂时不会在锁内释放业务对象
@param [in] factory 需要释放的自定义Dialog工厂
*/
static void DestroyDialogManagerTestDialogFactory(CustomDialogFactory* factory)
{
	delete factory;
}

/** 处理Qt延迟释放事件，辅助测试覆盖窗口关闭后的deleteLater路径
*/
static void ProcessDialogManagerTestEvents()
{
	QCoreApplication::processEvents();
	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	QCoreApplication::processEvents();
}

/** 检查测试条件并输出失败说明，避免测试失败时用断言主动中断进程
@param [in] condition 需要验证的条件
@param [in] message 条件失败时输出的说明
@return 返回true表示当前条件通过
*/
static bool CheckDialogManagerTestCase(bool condition, const char* message)
{
	if (condition)
	{
		return true;
	}
	qCritical() << "[DialogManagerTest failed]" << message;
	return false;
}

/** 输出DialogManager信号中的关键业务身份，辅助人工确认回调参数是否完整
@param [in] signalParam DialogManager发出的信号参数
*/
static void PrintDialogManagerTestSignal(const DialogSignalPtr& signalParam)
{
	if (signalParam.isNull())
	{
		qDebug() << "dialogSignal null";
		return;
	}
	switch (signalParam->signalType())
	{
	case ALREADY_SHOWN_SIGNAL:
	{
		const AlreadyShownSignalParam* param = dynamic_cast<const AlreadyShownSignalParam*>(signalParam.data());
		if (param != nullptr)
		{
			qDebug() << "alreadyShown" << param->m_dialog << param->m_businessId << param->m_userId;
		}
		break;
	}
	case POP_DIALOG_DONE_SIGNAL:
	case NOTIFY_DIALOG_DONE_SIGNAL:
	case STATIC_DIALOG_DONE_SIGNAL:
	case CUSTOM_DIALOG_DONE_SIGNAL:
	{
		const DialogDoneSignalParam* param = dynamic_cast<const DialogDoneSignalParam*>(signalParam.data());
		if (param != nullptr)
		{
			qDebug() << "dialogDone" << param->m_dialogId << param->m_businessId
				<< param->m_userId << param->m_dialogType << param->m_result;
		}
		break;
	}
	default:
		qDebug() << "dialogSignal" << signalParam->signalType();
		break;
	}
}

/** 验证未传入businessId/userId时，DialogManager不会把两个普通窗口误认为同一个业务窗口
@return 返回true表示测试通过
*/
static bool RunDialogManagerUnkeyedDialogTest()
{
	TipShowDialogParam firstParam;
	firstParam.m_tip = QStringLiteral("未设置业务键的第一个窗口");
	DialogManager::instance().makeDialog(firstParam);

	TipShowDialogParam secondParam;
	secondParam.m_tip = QStringLiteral("未设置业务键的第二个窗口");
	DialogManager::instance().makeDialog(secondParam);
	ProcessDialogManagerTestEvents();

	bool result = true;
	result = CheckDialogManagerTestCase(firstParam.m_dialogId != 0, "unkeyed first dialog should be created") && result;
	result = CheckDialogManagerTestCase(secondParam.m_dialogId != 0, "unkeyed second dialog should be created") && result;
	result = CheckDialogManagerTestCase(firstParam.m_dialogId != secondParam.m_dialogId,
		"unkeyed dialogs should not reuse each other") && result;

	DestroyDialogByDialogIdOperateParam destroyFirstParam;
	destroyFirstParam.m_dialogId = firstParam.m_dialogId;
	DialogManager::instance().operateDialog(destroyFirstParam);

	DestroyDialogByDialogIdOperateParam destroySecondParam;
	destroySecondParam.m_dialogId = secondParam.m_dialogId;
	DialogManager::instance().operateDialog(destroySecondParam);
	ProcessDialogManagerTestEvents();
	return result;
}

/** 执行按业务键复用、查询、销毁的核心测试
@return 返回true表示业务复用键行为符合预期
*/
static bool RunDialogManagerUserKeyTest()
{
	AskShowDialogParam detailParam;
	detailParam.m_businessId = kDialogManagerTestOrderDetailBusiness;
	detailParam.m_userId = 8888;
	detailParam.m_isActivateWhenReuse = false;
	detailParam.m_title = QStringLiteral("订单详情通知");
	detailParam.m_tip = QStringLiteral("第一次创建订单详情窗口");
	DialogManager::instance().makeDialog(detailParam);
	bool result = true;
	result = CheckDialogManagerTestCase(detailParam.m_dialogId != 0, "business keyed dialog should be created") && result;

	AskShowDialogParam detailParamAgain = detailParam;
	detailParamAgain.m_dialogId = 0;
	detailParamAgain.m_tip = QStringLiteral("第二次应复用订单详情窗口");
	DialogManager::instance().makeDialog(detailParamAgain);
	result = CheckDialogManagerTestCase(detailParamAgain.m_dialogId == detailParam.m_dialogId,
		"same businessId and userId should reuse existing dialog without forced activation") && result;

	AskShowDialogParam editParam = detailParam;
	editParam.m_dialogId = 0;
	editParam.m_businessId = kDialogManagerTestOrderEditBusiness;
	editParam.m_tip = QStringLiteral("同一个订单号但不同业务窗口，不能误复用");
	DialogManager::instance().makeDialog(editParam);
	result = CheckDialogManagerTestCase(editParam.m_dialogId != 0, "same userId in another business should be created") && result;
	result = CheckDialogManagerTestCase(editParam.m_dialogId != detailParam.m_dialogId,
		"same userId but different businessId should not reuse") && result;

	AskShowDialogParam anotherUserParam = detailParam;
	anotherUserParam.m_dialogId = 0;
	anotherUserParam.m_userId = 9999;
	anotherUserParam.m_tip = QStringLiteral("同一个业务窗口类型但订单号不同，也不能误复用");
	DialogManager::instance().makeDialog(anotherUserParam);
	result = CheckDialogManagerTestCase(anotherUserParam.m_dialogId != 0,
		"same businessId with another userId should be created") && result;
	result = CheckDialogManagerTestCase(anotherUserParam.m_dialogId != detailParam.m_dialogId,
		"same businessId but different userId should not reuse") && result;

	DialogIdByUserIdOperateParam dialogIdParam;
	dialogIdParam.m_businessId = kDialogManagerTestOrderDetailBusiness;
	dialogIdParam.m_userId = 8888;
	DialogManager::instance().operateDialog(dialogIdParam);
	result = CheckDialogManagerTestCase(dialogIdParam.m_dialogId == detailParam.m_dialogId,
		"DialogIdByUserIdOperateParam should find dialog by composite key") && result;

	DialogExistByUserIdOperateParam existParam;
	existParam.m_businessId = kDialogManagerTestOrderEditBusiness;
	existParam.m_userId = 8888;
	DialogManager::instance().operateDialog(existParam);
	result = CheckDialogManagerTestCase(existParam.m_isExist,
		"DialogExistByUserIdOperateParam should find edit dialog") && result;

	DestroyDialogByUserIdOperateParam destroyEditParam;
	destroyEditParam.m_businessId = kDialogManagerTestOrderEditBusiness;
	destroyEditParam.m_userId = 8888;
	DialogManager::instance().operateDialog(destroyEditParam);

	existParam.m_isExist = true;
	DialogManager::instance().operateDialog(existParam);
	result = CheckDialogManagerTestCase(!existParam.m_isExist,
		"destroy by composite key should remove only matching dialog") && result;

	DestroyDialogByDialogIdOperateParam destroyAnotherUserParam;
	destroyAnotherUserParam.m_dialogId = anotherUserParam.m_dialogId;
	DialogManager::instance().operateDialog(destroyAnotherUserParam);
	ProcessDialogManagerTestEvents();
	return result;
}

/** 执行自定义View和完整自定义Dialog测试，覆盖10000以上类型ID的注册和注销流程
@return 返回true表示自定义工厂创建、复用和注销流程正常
*/
static bool RunDialogManagerCustomDialogTest()
{
	bool result = true;
	bool viewRegistered = DialogManager::instance().registerCustomViewFactory(
		kDialogManagerTestViewType,
		new (std::nothrow) DialogManagerTestViewFactory,
		MODELESS_DIALOG_SHOW_MODE,
		DestroyDialogManagerTestViewFactory);
	result = CheckDialogManagerTestCase(viewRegistered, "custom view factory should register") && result;

	CustomDialogParam viewParam(kDialogManagerTestViewType);
	viewParam.m_businessId = kDialogManagerTestOrderLogBusiness;
	viewParam.m_userId = 8888;
	viewParam.m_title = QStringLiteral("订单日志");
	DialogManager::instance().makeDialog(viewParam);
	result = CheckDialogManagerTestCase(viewParam.m_dialogId != 0, "custom view dialog should be created") && result;

	CustomDialogParam viewParamAgain = viewParam;
	viewParamAgain.m_dialogId = 0;
	viewParamAgain.m_title = QStringLiteral("订单日志复用并刷新内容");
	DialogManager::instance().makeDialog(viewParamAgain);
	result = CheckDialogManagerTestCase(viewParamAgain.m_dialogId == viewParam.m_dialogId,
		"custom view dialog should reuse with same composite key") && result;

	bool dialogRegistered = DialogManager::instance().registerCustomDialogFactory(
		kDialogManagerTestDialogType,
		new (std::nothrow) DialogManagerTestDialogFactory,
		DestroyDialogManagerTestDialogFactory);
	result = CheckDialogManagerTestCase(dialogRegistered, "custom dialog factory should register") && result;

	CustomDialogParam dialogParam(kDialogManagerTestDialogType);
	dialogParam.m_businessId = kDialogManagerTestDownloadBusiness;
	dialogParam.m_userId = 10001;
	dialogParam.m_title = QStringLiteral("完整自定义下载窗口");
	DialogManager::instance().makeDialog(dialogParam);
	result = CheckDialogManagerTestCase(dialogParam.m_dialogId != 0, "custom dialog factory should create dialog") && result;

	DestroyDialogByUserIdOperateParam destroyViewParam;
	destroyViewParam.m_businessId = kDialogManagerTestOrderLogBusiness;
	destroyViewParam.m_userId = 8888;
	DialogManager::instance().operateDialog(destroyViewParam);

	DestroyDialogByUserIdOperateParam destroyDialogParam;
	destroyDialogParam.m_businessId = kDialogManagerTestDownloadBusiness;
	destroyDialogParam.m_userId = 10001;
	DialogManager::instance().operateDialog(destroyDialogParam);

	DialogManager::instance().unregisterCustomViewFactory(kDialogManagerTestViewType);
	DialogManager::instance().unregisterCustomDialogFactory(kDialogManagerTestDialogType);
	return result;
}

/** 执行按窗口实例ID操作的测试，覆盖句柄、数量统计和整体释放流程
@return 返回true表示dialogId维度的操作符合预期
*/
static bool RunDialogManagerDialogIdTest()
{
	TipShowDialogParam tipParam;
	tipParam.m_businessId = kDialogManagerTestOrderDetailBusiness;
	tipParam.m_userId = 9999;
	tipParam.m_tip = QStringLiteral("用于测试dialogId操作的通知窗口");
	DialogManager::instance().makeDialog(tipParam);
	bool result = true;
	result = CheckDialogManagerTestCase(tipParam.m_dialogId != 0, "dialogId test dialog should be created") && result;

	DialogExistByDialogIdOperateParam existParam;
	existParam.m_dialogId = tipParam.m_dialogId;
	DialogManager::instance().operateDialog(existParam);
	result = CheckDialogManagerTestCase(existParam.m_isExist, "dialog should exist by dialogId") && result;

	DialogHandleOperateParam handleParam;
	handleParam.m_dialogId = tipParam.m_dialogId;
	DialogManager::instance().operateDialog(handleParam);
	result = CheckDialogManagerTestCase(handleParam.m_windowHandle != nullptr, "dialog handle should be available") && result;

	ChangeUserResultByDialogIdOperateParam changeParam;
	changeParam.m_dialogId = tipParam.m_dialogId;
	changeParam.m_userResult = 123;
	DialogManager::instance().operateDialog(changeParam);

	ChangeUserResultByUserIdOperateParam changeByUserParam;
	changeByUserParam.m_businessId = kDialogManagerTestOrderDetailBusiness;
	changeByUserParam.m_userId = 9999;
	changeByUserParam.m_userResult = 456;
	DialogManager::instance().operateDialog(changeByUserParam);

	DialogCountOperateParam countParam;
	DialogManager::instance().operateDialog(countParam);
	result = CheckDialogManagerTestCase(countParam.m_count > 0, "dialog count should include created dialog") && result;

	DestroyDialogByDialogIdOperateParam destroyParam;
	destroyParam.m_dialogId = tipParam.m_dialogId;
	DialogManager::instance().operateDialog(destroyParam);
	ProcessDialogManagerTestEvents();
	return result;
}

/** 执行需要人工观察或可能进入exec的内置窗口测试，验证模态窗口和静态窗口用法
@return 返回true表示人工测试代码执行到结尾
*/
static bool RunDialogManagerManualBuiltInTest()
{
	TipDialogParam tipParam;
	tipParam.m_title = QStringLiteral("模态提示框测试");
	tipParam.m_tip = QStringLiteral("这个窗口用于验证exec流程，关闭后测试继续");
	tipParam.m_businessId = kDialogManagerTestOrderDetailBusiness;
	tipParam.m_userId = 7777;
	DialogManager::instance().makeDialog(tipParam);

	AccountManagerDialogParam accountParam;
	accountParam.m_title = QStringLiteral("静态账号窗口测试");
	DialogManager::instance().makeDialog(accountParam);
	ProcessDialogManagerTestEvents();

	StaticDialogDialogIdOperateParam staticIdParam;
	staticIdParam.m_dialogType = ACCOUNT_MANAGER_DIALOG;
	DialogManager::instance().operateDialog(staticIdParam);
	bool result = true;
	result = CheckDialogManagerTestCase(staticIdParam.m_dialogId == accountParam.m_dialogId,
		"static dialog id should match created account manager dialog") && result;

	CloseStaticDialogOperateParam closeStaticParam;
	closeStaticParam.m_dialogType = ACCOUNT_MANAGER_DIALOG;
	DialogManager::instance().operateDialog(closeStaticParam);
	ProcessDialogManagerTestEvents();
	return result;
}

int main(int argc, char* argv[])
{
	QApplication application(argc, argv);
	QObject::connect(&DialogManager::instance(),
		&DialogManager::dialogSignal,
		PrintDialogManagerTestSignal);

	bool result = true;
	result = RunDialogManagerUnkeyedDialogTest() && result;
	result = RunDialogManagerUserKeyTest() && result;
	result = RunDialogManagerCustomDialogTest() && result;
	result = RunDialogManagerDialogIdTest() && result;
	// result = RunDialogManagerManualBuiltInTest() && result;

	DestroyAllOperateParam destroyAllParam;
	DialogManager::instance().operateDialog(destroyAllParam);
	ProcessDialogManagerTestEvents();
	DialogManager::instance().uninit();
	QTimer::singleShot(0, &application, SLOT(quit()));
	int qtResult = application.exec();
	return result ? qtResult : 1;
}
#endif
