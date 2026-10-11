#include "Semaphore.h"
#include <stdexcept>
#include <functional>
#if defined(__linux__) && !defined(__ANDROID__)
#include <sys/sem.h>
#include <thread>
#endif

#if defined(__ANDROID__) || defined(__APPLE__)
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <cstring>
#include <limits.h>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef __ANDROID__
#include <poll.h>
#include <sys/inotify.h>
#elif defined(__APPLE__)
#include <sys/event.h>
#endif

/*
Semaphore跨平台功能与边界
更新日期：2026-10-09
范围：本轮只补齐Semaphore的Android、macOS、iOS实现及独立测试，不处理NetClientManager，不更新公共include/lib，不发布其他产品。
入口：调用方继续包含Semaphore/SemaphoreAPI.h；原public方法、名称、参数和默认值全部不变。平台专用配置及实现为private；不增加配置setter、共享状态类型或头文件。
书写约定：先说明功能契约，再说明平台机制、使用前提及验证限制；不能用“有分支”代替编译或运行证明。

一、通用功能
1. 本进程计数通知：signal累积，wait消费一次，先通知后等待有效；有限wait的0只检查，正值等待，无参数无限等待。
2. 本进程事件：event/eventAll只唤醒正在等待的调用，不保存计数，允许虚假唤醒；调用者自行检查业务条件。
3. 跨进程通知：createProcessSemaphore创建，openProcessSemaphore按相同逻辑名称打开；processSignal增加一个通知，processWait消费一个；不同名称互不串扰。
4. 名称代表同步对象，不代表进程。库不限制多个同名进程并存，不自动追加PID。需要各自独立通道时，调用者在名称中加入自己的会话/实例标识；需要通信时双方必须约定相同名称。
5. 同一实例已打开时重复create/open不切换对象、不清计数；close及析构释放本实例资源，重复close安全；失败后同一实例可以重试。
6. 本进程接口与跨进程接口互不混用。等待只表示取得通知，不保证业务队列中一定存在数据，不提供公平性或硬实时保证。
详细说明：相同名称只有在相同系统命名空间/共享目录且具备访问权限时才代表同一对象；跨平台编译不意味着不同操作系统之间可用此库远程通信。

二、Windows/Linux保留项
规则：两端原有实现、类布局、构建脚本及公开声明保持本轮修改前原样，不顺手修订旧规则。
Windows：CreateSemaphore/OpenSemaphore/ReleaseSemaphore/WaitForSingleObject/CloseHandle；多个实例create同一活跃对象不重置计数，最后一个句柄关闭后对象销毁。signalCount仅在Windows有效。
Linux：既有/dev/shm名称文件及System V IPC；不同实例create同名对象仍会SETVAL清零；创建者close仍会IPC_RMID，已打开对端随之失效。open仍沿用现有semget规则，不在本轮改成Windows语义。
超时：Windows原有processWait(-1)等同INFINITE；Linux无限等待继续使用无参数processWait，不改变其负值及EINTR行为。
详细说明：Windows/Linux原本就不是逐项完全一致。新增三端补齐相同的创建、按名共享、计数、等待、关闭及重试能力，重复创建和最后关闭生命周期采用Windows语义；不能宣称保留两端实现的同时消除了上述历史差异。跨平台业务统一采用先停止所有等待任务、再关闭对象的协议，不依赖Linux创建者提前关闭后对端仍可用。

三、Android/macOS/iOS统一实现
机制：普通共享文件保存计数；目录级flock串行化create/open/close/计数操作；每个已打开实例持有共享文件LOCK_SH，作为由内核管理的存活租约，不用PID表或用户态引用计数。
创建：新对象计数为0；同名对象仍有使用者时加入该对象，不重置计数。没有活跃使用者的合法旧对象由create回收为新一代；open只打开活跃对象，不偷偷创建。
关闭：本实例释放存活租约；仍有其他使用者则名称保留，创建者先关闭也不妨碍新对端按名打开；最后一个使用者正常关闭时删除名称，删除前核对inode，避免误删后来创建的对象。
异常退出：内核自动释放该进程的文件描述符及锁，其他使用者继续有效；全部异常退出可能留下文件，下一次open拒绝并清理无使用者的合法旧代次，create将合法旧代次重新置0，不复活旧通知。
格式：共享文件v2为5个uint32_t，顺序为0x53454D32、版本2、字节数20、计数、计数反码；先写完整初始化记录，最后发布完成标记。计数范围0至INT32_MAX；signalCount仍是Windows专属参数。
恢复：仅无使用者的create可恢复零长度文件或完成标记为0且其他字段合法的v2记录；错误长度、旧v1、未知版本、损坏反码及越界计数拒绝且不覆盖。双方须使用匹配版本，不能与旧Android mmap/Apple sem_open实现混用。
等待：Android用inotify+poll；Apple用kqueue/EVFILT_VNODE。每次阻塞等待都有独立观察者，先注册、再检查计数，避免多个等待者消费同一事件而丢失唤醒；事件合并不改变真实通知计数。
期限：无参数及-1无限等待；0只检查；正值以steady_clock固定截止时间等待；EINTR或无效唤醒不重新开始预算；低于-1返回false且不消费通知。无固定间隔轮询、sleep等待或新增工作线程。
错误：新增分支的create/open/signal/wait内部异常在公开边界捕获，诊断输出stderr；失败create/open保持未打开，有限wait返回false，未打开时通知/无参数等待无操作。close即使清理失败也释放自己的描述符，不向外抛异常。
详细说明：文件锁和短文件读写有系统调用开销；超时及响应仍受调度、目录锁竞争、文件系统影响，不保证与原生Windows信号量相同性能。只支持具备flock及文件事件机制的本地文件系统，不把网络文件系统或对抗性篡改当作可靠IPC通道。持锁进程退出会解锁，存储损坏会明确失败，不伪造成功。

四、共享目录与系统权限
默认：未设置SEMAPHORE_PROCESS_SCOPE时，使用TMPDIR；TMPDIR不存在或为空则使用系统P_tmpdir。在该目录内创建仅当前UID可写的Semaphore.<UID>目录；同一目录下名称仍不追加PID。
覆盖：SEMAPHORE_PROCESS_SCOPE可选，值为通信双方均能访问的已存在绝对目录，不是Apple组标识。库校验并捕获目录；打开后通知/等待/关闭不重读环境，close后重新create/open再读取。
启动：只在宿主启动层、并发业务开始前提供目录配置。运行中不能动态setenv/unsetenv改变正在使用的实例；不用当前工作目录、猜测包名或写死应用路径。
Android：同一应用普通多进程可使用同一应用UID和共享私有目录。系统不保证原生进程已有可写TMPDIR；默认临时目录不可访问时，宿主须在启动层提供真实可访问目录，库不能从纯C++自动获得Android Context。不同应用UID、isolated进程及SELinux限制不能靠相同名称绕过。
Apple：无沙箱macOS可使用默认目录；App/Extension等沙箱宿主需配置合法共享权限，并提供系统返回的真实共享容器目录，不传App Group标识字符串。库不申请entitlement，也不允许iOS任意创建常驻子进程。
安全：名称拒绝空值、点路径、分隔符及NUL，按文件系统名称上限校验而不截断；拒绝不安全目录、文件符号链接、非普通文件、不同UID、硬链接及组/他人可写的资源。不再受Apple原生POSIX信号量31字节名称限制。
元数据：.Semaphore.lock是目录级锁，通信期间不得删除/替换；默认UID目录及锁文件允许长期复用，不随某一个信号量关闭删除。宿主确认全部参与者退出后才能清理目录。
详细说明：Windows同样有会话命名空间与ACL，Linux有IPC权限；“功能对齐”不表示库能绕过每个平台的系统安全边界，或让没有共享目录权限的两个App直接通信。

五、并发与资源协议
规则：允许多个线程/进程通知和等待；单个实例的create/open/close/析构不得与任何通知/等待并发。先停止业务并唤醒、等全部调用退出，再close或析构；无参数等待没有自动取消。
调用者：不复制Semaphore对象；不把继承自fork的同一对象当作独立拥有者。POSIX子进程建立自己的实例并重新open；显式解锁继承描述符可能影响父进程，子进程采用受控退出/exec生命周期。
锁序：先目录锁，再共享文件租约/计数；计数等待不持目录锁阻塞事件，不等待另一个实例释放存活租约。任何持锁读写错误都有释放路径；每次等待退出/异常都close观察者。
封装：文件校验、记录读取、字节写入和按inode删除均为Semaphore的private const成员函数，没有共享静态变量，也不改变实例成员。const/private只是封装及只读约束，不代替跨进程同步；同一共享文件的读写/删除仍由调用方持有目录锁保护，create/open/close不得与通知/等待并发。
依赖：C++11及系统C库，无Qt、JNI、Objective-C或新第三方库；Windows/Linux条件展开结果与编辑前快照相同，新增私有成员仅在Android/Apple出现。

六、验证与未验证项
测试入口：本文件末尾注释掉的main；下面集中保留使用方法及稳定ID预期，覆盖通用行为、跨进程握手、无限等待、按名共享、生命周期、进程退出、损坏文件及配置恢复。
历史验证：迁移前本机VS2013 x64静态库和DLL两种真实调用方式，各13 PASS、0 FAIL、11 SKIP、47条检查，跳过不计通过；private封装复查结果相同。源码条件检查确认Windows/Linux实现及五端public接口未改。当前同文件main的验证结果见下方测试说明，不混淆历史结果。
未验证：本机无可用Linux运行环境、Android NDK和Apple SDK；新增三端尚未目标编译/真机运行。尤其需验证Android实际应用UID双进程、Apple App/Extension共享容器、并发竞争/EINTR、异常终止及长期资源回落；不得把源码检查当成五端已验证。
清理：本轮隔离构建和检查临时文件在记录结果后删除，不清理公共include/lib或其他会话产物。
参考：Windows CreateSemaphore https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createsemaphorew
参考：Android沙箱 https://source.android.com/docs/security/app-sandbox
参考：Apple App Groups https://developer.apple.com/documentation/BundleResources/Entitlements/com.apple.security.application-groups
参考：Apple flock https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/flock.2.html
参考：Apple kqueue https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/kevent.2.html

Semaphore跨平台测试说明
更新日期：2026-10-09
范围：只测试Semaphore库，不编译/发布NetClientManager或其他产品，不修改公共include/lib；未启用功能开发工作流，不增加工作流确认点。
入口：本文件末尾注释掉的main，全部测试辅助逻辑为main中的局部lambda；直接调用本文件实现，无替身、独立测试源文件、Qt或新增线程库依赖。
书写约定：每个稳定ID写场景、操作、独立预期及适用平台；撤销ID保留SKIP，不复用；有平台差异时写清楚，不以修改预期掩盖未支持。

一、构建与运行
1. 启用：将本文件末尾从测试专用include到main最后一行整体取消一层//注释；以C++11编译为测试EXE。测试结束后整段重新注释，不能只注释main而留下辅助测试逻辑。生产构建不启用本段，不修改public接口或另建测试工程。
   Windows：在VS2013开发者命令行使用cl /nologo /EHsc /MD /W4 /DSTATIC_LIB /Fe:<临时输出路径>/SemaphoreTest.exe Semaphore.cpp；测试include只在取消注释后参与编译。也可临时将现有VS配置的类型改为应用程序，验证结束恢复原DLL/静态库配置，不提交测试构建配置。
   Linux：使用支持C++11的工具链编译本文件并链接pthread；Android指定真实NDK工具链，Apple指定目标SDK。缺SDK不得算编译通过。
2. 执行：SemaphoreTest --case <1至24> --scope <测试专用绝对目录>；省略--case运行所有适用瞬时用例。Windows/Linux库忽略scope；Android/Apple使用它作为共享目录，不能传Apple组标识。
3. 默认目录：Android/Apple不传scope时按TMPDIR/P_tmpdir运行常规用例；故障文件用例12/13/22及默认目录受控注入用例24必须有专用scope，无则SKIP，不向根目录写素材。
4. 配置：程序只在独立单线程测试进程内受控修改SEMAPHORE_PROCESS_SCOPE/TMPDIR，故障后恢复；不修改父进程环境，不代表产品运行中可以动态改环境。真实App须在业务启动前配置。
5. 对端：--peer、--peer-infinite、--abandon仅供测试内部启动自己的子进程；iOS CLI不能任意fork，10/20/21须SKIP并另做合法App/Extension集成。
6. 输出：每项PASS/FAIL/SKIP与实际checks；任一FAIL退出1，参数错误退出2，无FAIL退出0。退出0含SKIP，不表示全部已验证。
7. 看门狗：本测试真实子进程有父进程回执/退出期限，只结束自己创建的子进程，不按名称结束其他进程；运行者为整个测试进程设置合理外部期限，避免未知错误无限阻塞。未授权压力/长时间测试，不再有CTest独立入口。
8. 资源：测试名称含PID仅用于素材隔离，不是库自动给名称加PID。故障素材由测试产生并删除；不递归删除传入scope。目录级.Semaphore.lock及默认UID目录由宿主在确认全部参与者退出后清理。

二、稳定用例与独立预期
1. 本进程计数：先signal两次，wait(0)恰好成功两次，再次false；通知不丢失、不重复。
2. 本进程有限等待：未通知时wait(20)返回false，耗时容忍调度但不可明显提前或超过1秒；随后signal仍可消费。
3. 事件不积累：无等待者时event/eventAll不使后续eventWait(0)成功。
4. 未打开对象：processSignal/processWait无参数不崩溃，processWait(0)false；重复close无副作用。
5. 按名共享与计数：创建方通知三次，open对端恰好消费三次；对端关闭后新对端仍可按名打开，并消费创建方新通知。
6. 同一实例重复调用：已打开时重复create/open不清除已有通知；close两次后重新create从0开始。
7. 缺失对象（新增三端）：open不抛平台专属异常、不假创建；随后signal仍不能消费，证明处于关闭状态；同一实例create后恢复正常。
8. 参数与配置捕获（新增三端）：空名、点路径、分隔符、NUL及相对目录拒绝；活跃实例不随测试进程环境变化切换目录或丢计数；恢复启动配置。
9. 超时边界（新增三端）：0立即检查；20ms可超时；-2返回false且不消费已通知计数；-1取得保留通知。
10. 真实双进程有限握手：不同方向两个对象，子报告就绪；父用20ms反向检查证明子未收到通知时不提前回执，再发通知并取得回执；子正常退出0，不以同进程两个实例代替跨进程。
11. 创建方先close（Windows/新增三端）：已打开对端仍能通知并恰好消费一次；旧Linux创建者删除规则不改，此项SKIP。
12. 错误长度（新增三端、需专用scope）：自建错误文件，create/open失败且不改内容；删除自有素材后同一实例可重试。
13. 文件符号链接（新增三端、需专用scope）：自建链接及目标，create/open拒绝且不改目标；显式删素材后恢复。
14. 已撤销：旧Apple POSIX信号量31字节限制不再适用，保留ID并SKIP，新的文件名上限见23。
15. 已撤销：旧public配置setter已删除，保留ID并SKIP，不重新引入接口。
16. 私有配置和原public接口：新增三端拒绝非法目录且可重试；Windows/Linux忽略启动变量；全程无public setter，close/recreate为空。
17. 已撤销：旧Android原生sem_t映射v1布局已弃用，保留ID并SKIP，新的共享记录测试见22。
18. 同名多实例完整生命周期（Windows/新增三端）：不同实例create不清现有通知；创建者关闭后第三实例open仍与现存实例共享同一计数；最后close后open失败，新create为空。旧Linux清零/删除规则不改，此项SKIP。
19. 名称隔离：两个不同名字独立计数；同名字的第三实例共享正确对象，双向证明通知不串扰。允许多个实例同时存在，名称不等于进程身份。
20. 进程直接退出（Windows/Android/macOS CLI）：子创建并留下一次通知后ExitProcess/_exit，不执行Semaphore析构；父等子实际退出，open不能复活旧通知，create从0开始且可再通知。旧Linux及iOS CLI SKIP。
21. 真实双进程无限等待：子报告就绪并走无参数processWait；父用20ms反向检查证明没有提前回执，再通知并收到回执；父有独立看门狗，验证无参数等待确实阻塞而非空实现；iOS CLI SKIP。
22. v2故障与恢复（新增三端、需专用scope）：分别损坏完成标记/版本/长度字段/反码/计数上限，create/open拒绝并保持字节不变；无使用者的合法未发布记录及合法旧代次可由create恢复为0；活跃对端create不重置通知，创建者关闭后仍可使用；全部自有素材删除后同一实例可重试。
23. 文件名称上限（新增三端）：超长逻辑名称拒绝而不截断，signal/wait不能误作用于一个截断名称；短名称重试可用。
24. 可选目录配置（新增三端、需专用scope）：在独立测试进程内将TMPDIR设为专用目录、清空scope；create/open仅用旧public方法即可共享通知；结束后恢复TMPDIR及scope，不向库新增配置方法。

三、本轮实际验证
2026-10-09历史验证：迁移前独立测试的VS2013 x64静态库调用及DLL调用各13 PASS、0 FAIL、11 SKIP、47条检查；下面同文件main直接验证正式实现，不将旧结果当作当前DLL调用验证。
通过ID：1至6、10、11、16、18至21。Windows跳过7至9、12至15、17、22至24；其中14/15/17是撤销ID，其余平台不适用。两种链接方式不当作26个不同用例。
当前同文件验证：全部辅助测试逻辑迁入main局部lambda，实际取消注释后用VS2013 x64编译并执行，结果仍为13 PASS、0 FAIL、11 SKIP、47条检查；非法参数和撤销ID另行核对。执行后已恢复整段注释，生产静态库/DLL构建不包含main或辅助测试逻辑。此结果不是新的DLL导入调用测试。
源码检查：与本轮编辑前快照对比Windows/Linux条件展开后的头文件、成员及.cpp实现一致；五端public区域一致，没有新增配置函数或共享类型。条件检查不等于SDK编译。
封装复查：四个文件级辅助函数改为private const成员函数后，函数体仅更新成员调用名，没有新增共享静态变量或改变锁协议；再次核对两端实现及五端public接口不变。VS2013 x64静态/DLL回归均仍为13 PASS、0 FAIL、11 SKIP、47条检查，新增三端仍未SDK验证。
设施自检：非法ID、非数字、缺参数及未知参数应退出2；撤销ID输出SKIP且checks=0；Windows传非空scope仍用旧实现。
未执行：Android NDK/Apple SDK编译、目标真机、Linux运行回归；本机缺对应环境，不把缺SDK或SKIP计通过。未运行压力测试，隔离构建及检查文件记录结果后删除。

四、目标环境必须补验证
1. Android同一应用普通双进程、不同PID/同名逻辑通道；在真实应用UID下验证，不仅adb shell。隔离UID/目录无权限应明确失败，不绕过沙箱。
2. macOS默认目录，以及Apple真实App/Extension共享容器；核实宿主权限、所有者、目录配置与生命周期，不能把iOS CLI的SKIP当支持任意子进程。
3. 多等待者竞争同一对象、多个通知被文件事件合并、信号发生在注册/计数检查/阻塞之间；总成功次数必须等于总通知数，无永久等待。每次阻塞调用独立观察者的代码审阅不代替受控并发验证。
4. 定时等待受到EINTR/非消费唤醒/时钟调整、创建者异常退出但对端仍活跃、全部异常退出后新open/create、初始化中退出、损坏记录拒绝和重试。
5. 重复创建/关闭、FD耗尽、目录不可写、文件被撤销、资源回落和长时间竞争；按授权区分瞬时/压力，不擅自展开耗时全量。
6. 多线程验证如需测试自管工作线程，必须用CTaskThreadManager；本测试入口不为验证Semaphore反过来依赖或改动生产线程库。相关集成及并发压力结果单独记录，不能冒充本轮完成。
*/

#if defined(__ANDROID__) || defined(__APPLE__)
struct stat Semaphore::processSemaphoreFileInfo(int32_t descriptor) const
{
    struct stat info;
    if (::fstat(descriptor, &info) != 0)
    {
        throw std::system_error(errno, std::generic_category(), "fstat(process semaphore)");
    }
    if (!S_ISREG(info.st_mode) || info.st_uid != ::geteuid() || info.st_nlink != 1 ||
        (info.st_mode & (S_IWGRP | S_IWOTH)) != 0)
    {
        throw std::runtime_error("Unsafe process semaphore file");
    }
    return info;
}

std::array<uint32_t, 5> Semaphore::readProcessSemaphoreState(int32_t descriptor, bool allowUnpublished) const
{
    std::array<uint32_t, 5> state;
    if (processSemaphoreFileInfo(descriptor).st_size != 5 * sizeof(uint32_t))
    {
        throw std::runtime_error("Process semaphore size changed");
    }
    size_t offset = 0;
    while (offset < sizeof(state))
    {
        const ssize_t count = ::pread(descriptor, reinterpret_cast<char*>(state.data()) + offset,
            sizeof(state) - offset, static_cast<off_t>(offset));
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        if (count <= 0)
        {
            throw std::runtime_error("Incomplete process semaphore state");
        }
        offset += static_cast<size_t>(count);
    }
    if ((state[0] != 0x53454D32 && !(allowUnpublished && state[0] == 0)) ||
        state[1] != 2 || state[2] != sizeof(state) ||
        state[3] > static_cast<uint32_t>((std::numeric_limits<int32_t>::max)()) ||
        state[4] != ~state[3])
    {
        throw std::runtime_error("Process semaphore state/version mismatch");
    }
    return state;
}

void Semaphore::writeProcessSemaphoreBytes(int32_t descriptor, const void* bytes, size_t size, off_t offset) const
{
    size_t written = 0;
    while (written < size)
    {
        const ssize_t count = ::pwrite(descriptor, static_cast<const char*>(bytes) + written,
            size - written, offset + static_cast<off_t>(written));
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        if (count <= 0)
        {
            throw std::system_error(count < 0 ? errno : EIO, std::generic_category(),
                "pwrite(process semaphore)");
        }
        written += static_cast<size_t>(count);
    }
}

void Semaphore::unlinkProcessSemaphore(int32_t descriptor, const std::string& name) const
{
    struct stat held;
    struct stat named;
    if (::fstat(descriptor, &held) != 0)
    {
        throw std::system_error(errno, std::generic_category(), "fstat(semaphore cleanup)");
    }
    if (::lstat(name.c_str(), &named) != 0)
    {
        if (errno == ENOENT)
        {
            return;
        }
        throw std::system_error(errno, std::generic_category(), "lstat(semaphore cleanup)");
    }
    if (held.st_dev == named.st_dev && held.st_ino == named.st_ino && ::unlink(name.c_str()) != 0)
    {
        throw std::system_error(errno, std::generic_category(), "unlink(process semaphore)");
    }
}
#endif

#if defined(__linux__) && !defined(__ANDROID__)
union semun
{
	int val;
	struct semid_ds* buf;
	unsigned short* arry;
};
#endif

Semaphore::Semaphore():
#ifdef _MSC_VER
m_processSemaphore(nullptr),
#elif defined(__linux__) && !defined(__ANDROID__)
m_processSemaphoreId(-1),
#endif
m_count(0)
#if defined(__ANDROID__) || defined(__APPLE__)
, m_processSemaphoreScope()
, m_mobileProcessName()
, m_mobileProcessFile(-1)
#endif
{

}

Semaphore::~Semaphore()
{
	closeProcessSemaphore();
}

void Semaphore::signal()
{
	std::unique_lock<std::mutex> lock(m_mtx);
	++m_count;
	m_cv.notify_one();
}

void Semaphore::wait()
{
	std::unique_lock<std::mutex> lock(m_mtx);
	while (m_count == 0)
	{
		m_cv.wait(lock);
	}
	--m_count;
}

bool Semaphore::wait(uint32_t timeout)
{
	std::unique_lock<std::mutex> lock(m_mtx);
	// 固定截止时间；虚假唤醒不能让总等待时间不断延长。
	const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
	while (m_count == 0)
	{
		if (m_cv.wait_until(lock, deadline) == std::cv_status::timeout && m_count == 0)
		{
			return false;
		}
	}
	--m_count;
	return true;
}

void Semaphore::createProcessSemaphore(const std::string& name, int32_t signalCount)
{
#ifdef _MSC_VER
	if (m_processSemaphore != nullptr)
	{
		return;
	}
	m_processSemaphore = ::CreateSemaphore(nullptr, 0, signalCount, name.c_str());
#elif defined(__linux__) && !defined(__ANDROID__)
	if (m_processSemaphoreId != -1)
	{
		return;
	}
	std::string fileName = "/dev/shm/Semaphore." + name;
	system(("touch \"" + fileName + "\"").c_str());
	m_processSemaphoreId = semget(ftok(fileName.c_str(), 0), 1, 0644 | IPC_CREAT);
	union semun sem_union;
	sem_union.val = 0;
	if(semctl(m_processSemaphoreId, 0, SETVAL, sem_union) == -1)
	{
		printf("create semphore failed, name = %s, Id = %d\n", name.c_str(), m_processSemaphoreId);
	}
	m_name = name;
#elif defined(__ANDROID__) || defined(__APPLE__)
    (void)signalCount;
    try
    {
        openPortableProcessSemaphore(name, true);
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Process semaphore create failed: %s\n", error.what());
    }
#else
    throw std::runtime_error("Process semaphores require Windows or Linux desktop; in-process Semaphore is supported");
#endif
}

void Semaphore::openProcessSemaphore(const std::string& name)
{
#ifdef _MSC_VER
	if (m_processSemaphore != nullptr)
	{
		return;
	}
	m_processSemaphore = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, name.c_str());
#elif defined(__linux__) && !defined(__ANDROID__)
	if (m_processSemaphoreId != -1)
	{
		return;
	}
	key_t key = ftok(("/dev/shm/Semaphore." + name).c_str(), 0);
	if (key == -1)
	{
		return;
	}
	m_processSemaphoreId = semget(key, 1, 0644 | IPC_CREAT);
#elif defined(__ANDROID__) || defined(__APPLE__)
    try
    {
        openPortableProcessSemaphore(name, false);
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Process semaphore open failed: %s\n", error.what());
    }
#else
    throw std::runtime_error("Process semaphores are unavailable on this target");
#endif
}

void Semaphore::closeProcessSemaphore()
{
#ifdef _MSC_VER
	if (m_processSemaphore == nullptr)
	{
		return;
	}
	::CloseHandle(m_processSemaphore);
	m_processSemaphore = nullptr;
#elif defined(__linux__) && !defined(__ANDROID__)
	if (m_name.empty())
	{
		m_processSemaphoreId = -1;
		return;
	}
	if (m_processSemaphoreId == -1)
	{
		return;
	}
	//删除信号量
	union semun sem_union;
	if(semctl(m_processSemaphoreId, 0, IPC_RMID, sem_union) == -1)
	{
		printf("Failed to delete semaphore, name = %s, Id = %d\n", m_name.c_str(), m_processSemaphoreId);
	}
	m_processSemaphoreId = -1;
	system(("rm \"/dev/shm/Semaphore." + m_name + "\"").c_str());
#elif defined(__ANDROID__) || defined(__APPLE__)
    if (m_mobileProcessFile < 0)
    {
        return;
    }
    int32_t directoryLock = -1;
    try
    {
        directoryLock = lockProcessSemaphoreDirectory();
        if (::flock(m_mobileProcessFile, LOCK_UN) != 0)
        {
            throw std::system_error(errno, std::generic_category(), "flock(semaphore release)");
        }
        int32_t result;
        do
        {
            result = ::flock(m_mobileProcessFile, LOCK_EX | LOCK_NB);
        }
        while (result != 0 && errno == EINTR);
        if (result == 0)
        {
            unlinkProcessSemaphore(m_mobileProcessFile, m_mobileProcessName);
        }
        else if (errno != EWOULDBLOCK && errno != EAGAIN)
        {
            throw std::system_error(errno, std::generic_category(), "flock(semaphore last user)");
        }
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Process semaphore close failed: %s\n", error.what());
    }
    ::close(m_mobileProcessFile);
    m_mobileProcessFile = -1;
    m_mobileProcessName.clear();
    unlockProcessSemaphoreDirectory(directoryLock);
#endif
}

void Semaphore::processSignal()
{
#ifdef _MSC_VER
	if (m_processSemaphore == nullptr)
	{
		return;
	}
	::ReleaseSemaphore(m_processSemaphore, 1, nullptr);
#elif defined(__linux__) && !defined(__ANDROID__)
	if (m_processSemaphoreId == -1)
	{
		return;
	}
	struct sembuf sem;
	sem.sem_num = 0;
	sem.sem_op = 1;
	sem.sem_flg = 0;
	if(semop(m_processSemaphoreId, &sem, 1) == -1)
	{
		std::thread::id threadId = std::this_thread::get_id();
		int32_t currentThreadId = static_cast<int32_t>(std::hash<std::thread::id>()(threadId));
		printf("processSignal failed, errno = %d, name = %s, Id = %d, threadId = %d\n", errno, m_name.c_str(), m_processSemaphoreId, currentThreadId);
	}
#elif defined(__ANDROID__) || defined(__APPLE__)
    if (m_mobileProcessFile < 0)
    {
        return;
    }
    int32_t directoryLock = -1;
    try
    {
        directoryLock = lockProcessSemaphoreDirectory();
        std::array<uint32_t, 5> state = readProcessSemaphoreState(m_mobileProcessFile);
        if (state[3] == static_cast<uint32_t>((std::numeric_limits<int32_t>::max)()))
        {
            throw std::overflow_error("Process semaphore count limit reached");
        }
        ++state[3];
        state[4] = ~state[3];
        writeProcessSemaphoreBytes(m_mobileProcessFile, state.data() + 3,
            2 * sizeof(uint32_t), 3 * sizeof(uint32_t));
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Process semaphore signal failed: %s\n", error.what());
    }
    unlockProcessSemaphoreDirectory(directoryLock);
#else
    throw std::runtime_error("Process semaphores are unavailable on this target");
#endif
}

void Semaphore::processWait()
{
#ifdef _MSC_VER
	if (m_processSemaphore == nullptr)
	{
		return;
	}
	::WaitForSingleObject(m_processSemaphore, INFINITE);
#elif defined(__linux__) && !defined(__ANDROID__)
	if (m_processSemaphoreId == -1)
	{
		return;
	}
	struct sembuf sem;
	sem.sem_num = 0;
	sem.sem_op = -1;
	sem.sem_flg = 0;
	int32_t result = 0;
	while((result = semop(m_processSemaphoreId, &sem, 1)) == -1 && errno == EINTR);
	if (result == 0)
	{
		return;
	}
	std::thread::id threadId = std::this_thread::get_id();
	int32_t currentThreadId = static_cast<int32_t>(std::hash<std::thread::id>()(threadId));
	printf("processWait failed, result = %d, errno = %d, name = %s, Id = %d, threadId = %d\n", result, errno, m_name.c_str(), m_processSemaphoreId, currentThreadId);
#elif defined(__ANDROID__) || defined(__APPLE__)
    (void)processWait(-1);
#else
    throw std::runtime_error("Process semaphores are unavailable on this target");
#endif
}

bool Semaphore::processWait(int32_t timeout)
{
#ifdef _MSC_VER
	if (m_processSemaphore == nullptr)
	{
		return false;
	}
	DWORD dwResult = ::WaitForSingleObject(m_processSemaphore, timeout);
	if (dwResult == WAIT_OBJECT_0)
	{
		return true;
	}
	else if (dwResult == WAIT_TIMEOUT)
	{
		return false;
	}
	return false;
#elif defined(__linux__) && !defined(__ANDROID__)
	if (m_processSemaphoreId == -1)
	{
		return false;
	}
	struct sembuf sem;
	sem.sem_num = 0;
	sem.sem_op = -1;
	sem.sem_flg = 0;
	struct timespec ts;
	ts.tv_sec = timeout / 1000;
	ts.tv_nsec = (timeout % 1000) * 1000 * 1000;
	int32_t result = 0;
	while ((result = semtimedop(m_processSemaphoreId, &sem, 1, &ts)) == -1 && errno == EINTR);
	//errno只有在错误时才会更新
	if (result == 0)
	{
		return true;
	}
	//超时
	else if (result == -1 && errno == EAGAIN)
	{
		return false;
	}
	std::thread::id threadId = std::this_thread::get_id();
	int32_t currentThreadId = static_cast<int32_t>(std::hash<std::thread::id>()(threadId));
	printf("semtimedop failed, result = %d, errno = %d, name = %s, Id = %d, threadId = %d\n", result, errno, m_name.c_str(), m_processSemaphoreId, currentThreadId);
	return false;
#elif defined(__ANDROID__) || defined(__APPLE__)
    if (m_mobileProcessFile < 0)
    {
        return false;
    }
    try
    {
        return waitPortableProcessSemaphore(timeout);
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Process semaphore wait failed: %s\n", error.what());
        return false;
    }
#else
    throw std::runtime_error("Process semaphores are unavailable on this target");
#endif
}

void Semaphore::event()
{
	std::unique_lock<std::mutex> lock(m_mtx);
	m_cv.notify_one();
}

void Semaphore::eventAll()
{
	std::unique_lock<std::mutex> lock(m_mtx);
	m_cv.notify_all();
}

void Semaphore::eventWait()
{
	std::unique_lock<std::mutex> lock(m_mtx);
	m_cv.wait(lock);
}

bool Semaphore::eventWait(int32_t timeout)
{
	std::unique_lock<std::mutex> lock(m_mtx);
	if (m_cv.wait_for(lock, std::chrono::milliseconds(timeout)) == std::cv_status::timeout)
	{
		return false;
	}
	return true;
}

#if defined(__ANDROID__) || defined(__APPLE__)
void Semaphore::initProcessSemaphoreDirectory()
{
    const char* configured = std::getenv("SEMAPHORE_PROCESS_SCOPE");
    const bool explicitDirectory = configured != nullptr && configured[0] != '\0';
    const char* temporary = std::getenv("TMPDIR");
    std::string directory = explicitDirectory ? configured :
        (temporary != nullptr && temporary[0] != '\0' ? temporary : P_tmpdir);
    if (directory.empty() || directory[0] != '/')
    {
        throw std::invalid_argument("Process semaphore directory must be absolute");
    }
    char canonical[PATH_MAX];
    if (::realpath(directory.c_str(), canonical) == nullptr)
    {
        throw std::system_error(errno, std::generic_category(), "realpath(semaphore directory)");
    }
    directory = canonical;
    if (!explicitDirectory)
    {
        directory += "/Semaphore." + std::to_string(static_cast<unsigned long>(::geteuid()));
        if (::mkdir(directory.c_str(), 0700) != 0 && errno != EEXIST)
        {
            throw std::system_error(errno, std::generic_category(), "mkdir(semaphore directory)");
        }
    }
    struct stat info;
    if (::lstat(directory.c_str(), &info) != 0)
    {
        throw std::system_error(errno, std::generic_category(), "lstat(semaphore directory)");
    }
    if (!S_ISDIR(info.st_mode) || info.st_uid != ::geteuid() ||
        (info.st_mode & (S_IWGRP | S_IWOTH)) != 0)
    {
        throw std::runtime_error("Unsafe or inaccessible process semaphore directory");
    }
    m_processSemaphoreScope = directory;
}

std::string Semaphore::mobileProcessSemaphoreName(const std::string& name) const
{
    if (name.empty() || name == "." || name == ".." ||
        name.find('\0') != std::string::npos || name.find('/') != std::string::npos ||
        name.find('\\') != std::string::npos)
    {
        throw std::invalid_argument("Invalid process semaphore name");
    }
    const std::string fileName = "Semaphore." + name;
    const long limit = ::pathconf(m_processSemaphoreScope.c_str(), _PC_NAME_MAX);
    if (limit < 0 || fileName.size() > static_cast<size_t>(limit))
    {
        throw std::invalid_argument("Process semaphore name exceeds filesystem limit");
    }
    return m_processSemaphoreScope + "/" + fileName;
}

int32_t Semaphore::lockProcessSemaphoreDirectory() const
{
    const std::string lockPath = m_processSemaphoreScope + "/.Semaphore.lock";
    const int32_t descriptor = ::open(lockPath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (descriptor < 0)
    {
        throw std::system_error(errno, std::generic_category(), "open(semaphore directory lock)");
    }
    try
    {
        processSemaphoreFileInfo(descriptor);
        int32_t result;
        do
        {
            result = ::flock(descriptor, LOCK_EX);
        }
        while (result != 0 && errno == EINTR);
        if (result != 0)
        {
            throw std::system_error(errno, std::generic_category(), "flock(semaphore directory lock)");
        }
    }
    catch (...)
    {
        ::close(descriptor);
        throw;
    }
    return descriptor;
}

void Semaphore::unlockProcessSemaphoreDirectory(int32_t descriptor) const
{
    if (descriptor >= 0)
    {
        ::close(descriptor);
    }
}

void Semaphore::openPortableProcessSemaphore(const std::string& name, bool create)
{
    if (m_mobileProcessFile >= 0)
    {
        return;
    }
    initProcessSemaphoreDirectory();
    std::string fullName = mobileProcessSemaphoreName(name);
    const int32_t directoryLock = lockProcessSemaphoreDirectory();
    int32_t descriptor = -1;
    bool created = false;
    try
    {
        if (create)
        {
            descriptor = ::open(fullName.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
            created = descriptor >= 0;
            if (descriptor < 0 && errno == EEXIST)
            {
                descriptor = ::open(fullName.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
            }
        }
        else
        {
            descriptor = ::open(fullName.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
        }
        if (descriptor < 0)
        {
            throw std::system_error(errno, std::generic_category(), "open(process semaphore)");
        }
        const struct stat info = processSemaphoreFileInfo(descriptor);
        int32_t result;
        do
        {
            result = ::flock(descriptor, LOCK_EX | LOCK_NB);
        }
        while (result != 0 && errno == EINTR);
        const bool inactive = result == 0;
        if (!inactive && errno != EWOULDBLOCK && errno != EAGAIN)
        {
            throw std::system_error(errno, std::generic_category(), "flock(semaphore live users)");
        }
        // Only a creator may recover an unpublished or valid abandoned generation.
        if (info.st_size == 0 && create && inactive)
        {
            if (::ftruncate(descriptor, 5 * sizeof(uint32_t)) != 0)
            {
                throw std::system_error(errno, std::generic_category(), "ftruncate(process semaphore)");
            }
            const uint32_t state[5] = { 0, 2, 5 * sizeof(uint32_t), 0, ~uint32_t(0) };
            writeProcessSemaphoreBytes(descriptor, state, sizeof(state), 0);
            const uint32_t magic = 0x53454D32;
            writeProcessSemaphoreBytes(descriptor, &magic, sizeof(magic), 0);
        }
        else
        {
            if (info.st_size != 5 * sizeof(uint32_t))
            {
                throw std::runtime_error("Process semaphore has an incompatible size");
            }
            std::array<uint32_t, 5> state = readProcessSemaphoreState(descriptor, inactive && create);
            if (inactive)
            {
                if (!create)
                {
                    unlinkProcessSemaphore(descriptor, fullName);
                    throw std::runtime_error("Process semaphore has no live users");
                }
                state[3] = 0;
                state[4] = ~state[3];
                writeProcessSemaphoreBytes(descriptor, state.data() + 3,
                    2 * sizeof(uint32_t), 3 * sizeof(uint32_t));
                if (state[0] == 0)
                {
                    const uint32_t magic = 0x53454D32;
                    writeProcessSemaphoreBytes(descriptor, &magic, sizeof(magic), 0);
                }
            }
        }
        do
        {
            result = ::flock(descriptor, LOCK_SH | LOCK_NB);
        }
        while (result != 0 && errno == EINTR);
        if (result != 0)
        {
            throw std::system_error(errno, std::generic_category(), "flock(semaphore live lease)");
        }
        m_mobileProcessName.swap(fullName);
        m_mobileProcessFile = descriptor;
    }
    catch (...)
    {
        if (created && descriptor >= 0)
        {
            try
            {
                unlinkProcessSemaphore(descriptor, fullName);
            }
            catch (...)
            {
                // Keep the original failure; the next creator can inspect an abandoned file.
            }
        }
        if (descriptor >= 0)
        {
            ::close(descriptor);
        }
        unlockProcessSemaphoreDirectory(directoryLock);
        throw;
    }
    unlockProcessSemaphoreDirectory(directoryLock);
}

int32_t Semaphore::createProcessSemaphoreObserver(const std::string& name, int32_t descriptor) const
{
#ifdef __ANDROID__
    (void)descriptor;
    const int32_t observer = ::inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (observer < 0)
    {
        throw std::system_error(errno, std::generic_category(), "inotify_init1(semaphore)");
    }
    if (::inotify_add_watch(observer, name.c_str(), IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF) < 0)
    {
        const int32_t error = errno;
        ::close(observer);
        throw std::system_error(error, std::generic_category(), "inotify_add_watch(semaphore)");
    }
#elif defined(__APPLE__)
    (void)name;
    const int32_t observer = ::kqueue();
    if (observer < 0)
    {
        throw std::system_error(errno, std::generic_category(), "kqueue(semaphore)");
    }
    struct kevent change;
    EV_SET(&change, descriptor, EVFILT_VNODE, EV_ADD | EV_CLEAR,
        NOTE_WRITE | NOTE_DELETE | NOTE_RENAME | NOTE_REVOKE, 0, nullptr);
    if (::fcntl(observer, F_SETFD, FD_CLOEXEC) < 0 ||
        ::kevent(observer, &change, 1, nullptr, 0, nullptr) < 0)
    {
        const int32_t error = errno;
        ::close(observer);
        throw std::system_error(error, std::generic_category(), "kevent(register semaphore)");
    }
#endif
    return observer;
}

bool Semaphore::tryConsumeProcessSemaphore()
{
    const int32_t directoryLock = lockProcessSemaphoreDirectory();
    bool consumed = false;
    try
    {
        std::array<uint32_t, 5> state = readProcessSemaphoreState(m_mobileProcessFile);
        if (state[3] != 0)
        {
            --state[3];
            state[4] = ~state[3];
            writeProcessSemaphoreBytes(m_mobileProcessFile, state.data() + 3,
                2 * sizeof(uint32_t), 3 * sizeof(uint32_t));
            consumed = true;
        }
    }
    catch (...)
    {
        unlockProcessSemaphoreDirectory(directoryLock);
        throw;
    }
    unlockProcessSemaphoreDirectory(directoryLock);
    return consumed;
}

bool Semaphore::waitPortableProcessSemaphore(int32_t timeout)
{
    if (timeout < -1)
    {
        return false;
    }
    if (timeout == 0)
    {
        return tryConsumeProcessSemaphore();
    }
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout < 0 ? 0 : timeout);
    // Register before checking the counter, so a signal between check and blocking is not lost.
    const int32_t observer = createProcessSemaphoreObserver(m_mobileProcessName, m_mobileProcessFile);
    bool consumed = false;
    try
    {
        for (;;)
        {
            if (tryConsumeProcessSemaphore())
            {
                consumed = true;
                break;
            }
            const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            if (timeout >= 0 && now >= deadline)
            {
                break;
            }
#ifdef __ANDROID__
            int32_t remaining = -1;
            if (timeout >= 0)
            {
                const int64_t nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - now).count();
                remaining = static_cast<int32_t>((nanos + 999999) / 1000000);
            }
            struct pollfd event = { observer, POLLIN, 0 };
            const int32_t result = ::poll(&event, 1, remaining);
            if (result < 0 && errno == EINTR)
            {
                continue;
            }
            if (result < 0 || (event.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
            {
                throw std::system_error(result < 0 ? errno : EIO,
                    std::generic_category(), "poll(process semaphore)");
            }
            if (result > 0)
            {
                char events[4096];
                const ssize_t read = ::read(observer, events, sizeof(events));
                if (read < 0 && errno != EINTR && errno != EAGAIN)
                {
                    throw std::system_error(errno, std::generic_category(), "read(semaphore events)");
                }
                size_t offset = 0;
                while (read > 0 && offset + sizeof(struct inotify_event) <= static_cast<size_t>(read))
                {
                    struct inotify_event notification;
                    std::memcpy(&notification, events + offset, sizeof(notification));
                    if ((notification.mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_IGNORED)) != 0)
                    {
                        throw std::runtime_error("Process semaphore name was removed");
                    }
                    offset += sizeof(notification) + notification.len;
                }
            }
#elif defined(__APPLE__)
            struct timespec remaining;
            if (timeout >= 0)
            {
                const int64_t nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - now).count();
                remaining.tv_sec = static_cast<time_t>(nanos / 1000000000);
                remaining.tv_nsec = static_cast<long>(nanos % 1000000000);
            }
            struct kevent event;
            const int32_t result = ::kevent(observer, nullptr, 0, &event, 1, timeout < 0 ? nullptr : &remaining);
            if (result < 0 && errno == EINTR)
            {
                continue;
            }
            if (result < 0 || (result > 0 &&
                ((event.flags & EV_ERROR) != 0 || (event.fflags & (NOTE_DELETE | NOTE_RENAME | NOTE_REVOKE)) != 0)))
            {
                throw std::runtime_error("Process semaphore observer failed or name was removed");
            }
#endif
        }
    }
    catch (...)
    {
        ::close(observer);
        throw;
    }
    ::close(observer);
    return consumed;
}
#endif

// 以下为本文件内置测试；测试时整段取消一层注释，测试完成后重新注释整段
//#include <chrono>
//#include <cstdio>
//#include <cstdlib>
//#include <fstream>
//#include <iterator>
//#include <limits.h>
//#include <stdexcept>
//#include <string>
//#ifdef _MSC_VER
//#include <windows.h>
//#else
//#include <cerrno>
//#include <csignal>
//#include <sys/wait.h>
//#include <unistd.h>
//#if defined(__ANDROID__) || defined(__APPLE__)
//#include <sys/stat.h>
//#endif
//#ifdef __APPLE__
//#include <TargetConditionals.h>
//#endif
//#endif
//
//int main(int argc, char* argv[])
//{
//    /** 判定一条独立预期并累计实际检查数
//    @param [in] condition 实际条件
//    @param [in] message 检查说明
//    @param [in,out] checks 本项已执行的检查数
//    */
//    auto check = [](bool condition, const char* message, int32_t& checks) -> void
//    {
//        ++checks;
//        if (!condition)
//        {
//            throw std::runtime_error(message);
//        }
//    };
//
//    /** 在独立单线程测试进程中设置启动配置，产品运行时不得这样动态变更环境
//    @param [in] scope Android/Apple共享目录，Windows/Linux库应忽略此环境变量
//    */
//    auto configureScope = [](const std::string& scope) -> void
//    {
//    #ifdef _MSC_VER
//        const int32_t result = ::_putenv_s("SEMAPHORE_PROCESS_SCOPE", scope.c_str());
//    #else
//        const int32_t result = ::setenv("SEMAPHORE_PROCESS_SCOPE", scope.c_str(), 1);
//    #endif
//        if (result != 0)
//        {
//            throw std::runtime_error("Failed to set owned test process environment");
//        }
//    };
//
//    /** 生成测试专属的短名称，避免不同进程和用例共享状态
//    @param [in] id 稳定用例ID
//    @return 含当前PID的ASCII逻辑名称
//    */
//    auto testName = [](int32_t id) -> std::string
//    {
//        char name[64];
//    #ifdef _MSC_VER
//        const uint32_t pid = ::GetCurrentProcessId();
//        ::sprintf_s(name, sizeof(name), "SP%u_%d", pid, id);
//    #else
//        const uint32_t pid = static_cast<uint32_t>(::getpid());
//        std::snprintf(name, sizeof(name), "SP%u_%d", pid, id);
//    #endif
//        return name;
//    };
//
//    /** 在独立对端进程打开两个方向的对象并完成一次握手
//    @param [in] name 父进程创建的对象前缀
//    @param [in] scope 通信范围
//    @param [in] infinite true检查无参数无限等待，false检查有限等待
//    @return 成功0，异常或超时1
//    */
//    auto runPeer = [&configureScope](const std::string& name, const std::string& scope, bool infinite) -> int32_t
//    {
//        try
//        {
//            Semaphore inbound;
//            Semaphore outbound;
//            configureScope(scope);
//            inbound.openProcessSemaphore(name + "a");
//            outbound.openProcessSemaphore(name + "b");
//            outbound.processSignal();
//            if (infinite)
//            {
//                inbound.processWait();
//            }
//            else if (!inbound.processWait(2000))
//            {
//                return 1;
//            }
//            outbound.processSignal();
//            return 0;
//        }
//        catch (const std::exception& error)
//        {
//            std::fprintf(stderr, "Peer failed: %s\n", error.what());
//            return 1;
//        }
//    };
//
//    /** 启动真实对端，检查双向通知及有限时间内的进程退出
//    @param [in] name 本项唯一名称前缀
//    @param [in] scope 通信范围
//    @param [in] infinite 是否检查无限等待；父进程看门狗仍有上限
//    @param [in,out] checks 实际检查数
//    @return 可在当前宿主执行时true，iOS CLI不能启动任意对端时false
//    */
//    auto testTwoProcesses = [&check, &configureScope, &runPeer](const std::string& name, const std::string& scope, int32_t& checks, bool infinite) -> bool
//    {
//    #if defined(__APPLE__) && TARGET_OS_IPHONE
//        (void)name;
//        (void)scope;
//        (void)checks;
//        (void)infinite;
//        return false;
//    #else
//        Semaphore outbound;
//        Semaphore inbound;
//        configureScope(scope);
//        outbound.createProcessSemaphore(name + "a");
//        inbound.createProcessSemaphore(name + "b");
//    #ifdef _MSC_VER
//        wchar_t executable[32768];
//        const DWORD length = ::GetModuleFileNameW(nullptr, executable, 32768);
//        check(length != 0 && length < 32768, "Resolve test executable", checks);
//        std::wstring command = L"\"" + std::wstring(executable, length) +
//            (infinite ? L"\" --peer-infinite " : L"\" --peer ");
//        command.append(name.begin(), name.end());
//        STARTUPINFOW startup = {};
//        startup.cb = sizeof(startup);
//        PROCESS_INFORMATION process = {};
//        check(::CreateProcessW(nullptr, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
//            nullptr, nullptr, &startup, &process) != FALSE, "Create peer process", checks);
//        ::CloseHandle(process.hThread);
//        bool response = false;
//        try
//        {
//            check(inbound.processWait(2500), "Peer reports ready before notification", checks);
//            check(!inbound.processWait(20), "Peer cannot acknowledge before receiving notification", checks);
//            outbound.processSignal();
//            response = inbound.processWait(2500);
//        }
//        catch (...)
//        {
//            ::TerminateProcess(process.hProcess, 1);
//            ::WaitForSingleObject(process.hProcess, 3000);
//            ::CloseHandle(process.hProcess);
//            throw;
//        }
//        const DWORD exited = ::WaitForSingleObject(process.hProcess, 3000);
//        DWORD exitCode = 1;
//        if (exited == WAIT_OBJECT_0)
//        {
//            ::GetExitCodeProcess(process.hProcess, &exitCode);
//        }
//        else
//        {
//            ::TerminateProcess(process.hProcess, 1);
//            ::WaitForSingleObject(process.hProcess, 3000);
//        }
//        ::CloseHandle(process.hProcess);
//        check(response, "Peer must acknowledge a different-direction semaphore", checks);
//        check(exited == WAIT_OBJECT_0 && exitCode == 0, "Peer must exit successfully", checks);
//    #else
//        const pid_t child = ::fork();
//        check(child >= 0, "Fork peer process", checks);
//        if (child == 0)
//        {
//            ::_exit(runPeer(name, scope, infinite));
//        }
//        bool response = false;
//        try
//        {
//            check(inbound.processWait(2500), "Peer reports ready before notification", checks);
//            check(!inbound.processWait(20), "Peer cannot acknowledge before receiving notification", checks);
//            outbound.processSignal();
//            response = inbound.processWait(2500);
//        }
//        catch (...)
//        {
//            ::kill(child, SIGKILL);
//            while (::waitpid(child, nullptr, 0) < 0 && errno == EINTR)
//            {
//
//            }
//            throw;
//        }
//        int32_t status = 0;
//        pid_t result = 0;
//        const std::chrono::steady_clock::time_point deadline =
//            std::chrono::steady_clock::now() + std::chrono::seconds(3);
//        do
//        {
//            result = ::waitpid(child, &status, WNOHANG);
//            if (result == 0)
//            {
//                ::usleep(1000);
//            }
//        }
//        while ((result == 0 || (result < 0 && errno == EINTR)) &&
//            std::chrono::steady_clock::now() < deadline);
//        if (result != child && !(result < 0 && errno == ECHILD))
//        {
//            ::kill(child, SIGKILL);
//            while (::waitpid(child, nullptr, 0) < 0 && errno == EINTR)
//            {
//
//            }
//        }
//        check(response, "Peer must acknowledge a different-direction semaphore", checks);
//        check(result == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
//            "Peer must exit successfully", checks);
//    #endif
//        return true;
//    #endif
//    };
//
//    /** 创建一个通知后直接退出，不执行Semaphore析构
//    @param [in] name 本测试拥有的逻辑名称
//    @param [in] scope 启动配置
//    @return 不返回；成功退出0，创建未生效退出1
//    */
//    auto runAbandonedPeer = [&configureScope](const std::string& name, const std::string& scope) -> void
//    {
//        configureScope(scope);
//        Semaphore owner;
//        owner.createProcessSemaphore(name);
//        owner.processSignal();
//        const bool ready = owner.processWait(0);
//        owner.processSignal();
//    #ifdef _MSC_VER
//        ::ExitProcess(ready ? 0 : 1);
//    #else
//        ::_exit(ready ? 0 : 1);
//    #endif
//    };
//
//    /** 验证进程退出释放存活租约，残留通知不能污染重新创建的对象
//    @param [in] name 本测试专属名称
//    @param [in] scope 同一共享目录
//    @param [in,out] checks 实际检查数
//    @return true执行，false为旧Linux语义或iOS CLI不适用
//    */
//    auto testAbandonedProcess = [&check, &runAbandonedPeer](const std::string& name, const std::string& scope, int32_t& checks) -> bool
//    {
//    #if (defined(__APPLE__) && TARGET_OS_IPHONE) || (defined(__linux__) && !defined(__ANDROID__))
//        (void)name;
//        (void)scope;
//        (void)checks;
//        return false;
//    #else
//    #ifdef _MSC_VER
//        (void)scope;
//        wchar_t executable[32768];
//        const DWORD length = ::GetModuleFileNameW(nullptr, executable, 32768);
//        check(length != 0 && length < 32768, "Resolve abandonment test executable", checks);
//        std::wstring command = L"\"" + std::wstring(executable, length) + L"\" --abandon ";
//        command.append(name.begin(), name.end());
//        STARTUPINFOW startup = {};
//        startup.cb = sizeof(startup);
//        PROCESS_INFORMATION process = {};
//        check(::CreateProcessW(nullptr, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
//            nullptr, nullptr, &startup, &process) != FALSE, "Start owned abandoning process", checks);
//        ::CloseHandle(process.hThread);
//        const DWORD result = ::WaitForSingleObject(process.hProcess, 3000);
//        DWORD exitCode = 1;
//        if (result == WAIT_OBJECT_0)
//        {
//            ::GetExitCodeProcess(process.hProcess, &exitCode);
//        }
//        else
//        {
//            ::TerminateProcess(process.hProcess, 1);
//            ::WaitForSingleObject(process.hProcess, 3000);
//        }
//        ::CloseHandle(process.hProcess);
//        check(result == WAIT_OBJECT_0 && exitCode == 0, "Peer exited without library destruction", checks);
//    #else
//        const pid_t child = ::fork();
//        check(child >= 0, "Fork abandoning process", checks);
//        if (child == 0)
//        {
//            runAbandonedPeer(name, scope);
//        }
//        int32_t status = 0;
//        pid_t result = 0;
//        const std::chrono::steady_clock::time_point deadline =
//            std::chrono::steady_clock::now() + std::chrono::seconds(3);
//        do
//        {
//            result = ::waitpid(child, &status, WNOHANG);
//            if (result == 0)
//            {
//                ::usleep(1000);
//            }
//        }
//        while ((result == 0 || (result < 0 && errno == EINTR)) &&
//            std::chrono::steady_clock::now() < deadline);
//        if (result != child && !(result < 0 && errno == ECHILD))
//        {
//            ::kill(child, SIGKILL);
//            while (::waitpid(child, nullptr, 0) < 0 && errno == EINTR)
//            {
//
//            }
//        }
//        check(result == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
//            "Peer exited without library destruction", checks);
//    #endif
//        Semaphore owner;
//        owner.openProcessSemaphore(name);
//        owner.processSignal();
//        check(!owner.processWait(0), "Opening abandoned object must not resurrect old notification", checks);
//        owner.createProcessSemaphore(name);
//        check(!owner.processWait(0), "Recreated object must start empty", checks);
//        owner.processSignal();
//        check(owner.processWait(0), "Recreated object accepts new notifications", checks);
//        return true;
//    #endif
//    };
//
//    /** 执行一个稳定ID的瞬时用例，不支持的目标明确跳过
//    @param [in] id 稳定ID
//    @param [in] scope 显式测试范围
//    @param [in,out] checks 实际检查数
//    @return 执行时true，不适用时false
//    */
//    auto runCase = [&check, &configureScope, &testName, &testTwoProcesses, &testAbandonedProcess](int32_t id, const std::string& scope, int32_t& checks) -> bool
//    {
//        Semaphore owner;
//        if (id == 14 || id == 15 || id == 17)
//        {
//            return false;
//        }
//        if (id <= 3)
//        {
//            if (id == 1)
//            {
//                owner.signal();
//                owner.signal();
//                check(owner.wait(0), "Consume first notification", checks);
//                check(owner.wait(0), "Consume second notification", checks);
//                check(!owner.wait(0), "No extra notification", checks);
//            }
//            else if (id == 2)
//            {
//                const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
//                check(!owner.wait(20), "Unsignaled thread wait times out", checks);
//                const int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
//                    std::chrono::steady_clock::now() - start).count();
//                check(elapsed >= 10 && elapsed < 1000, "Thread timeout duration", checks);
//                owner.signal();
//                check(owner.wait(0), "Thread wait remains reusable", checks);
//            }
//            else
//            {
//                owner.event();
//                owner.eventAll();
//                check(!owner.eventWait(0), "Events do not accumulate without a waiter", checks);
//            }
//            return true;
//        }
//        if (id == 4)
//        {
//            owner.processSignal();
//            owner.processWait();
//            check(!owner.processWait(0), "Uninitialized process wait fails", checks);
//            owner.closeProcessSemaphore();
//            owner.closeProcessSemaphore();
//            check(!owner.processWait(0), "Repeated close is harmless", checks);
//            return true;
//        }
//    #if !defined(__ANDROID__) && !defined(__APPLE__)
//        if ((id >= 7 && id <= 9) || id == 12 || id == 13 || id >= 22)
//        {
//            return false;
//        }
//    #endif
//    #if defined(__linux__) && !defined(__ANDROID__)
//        if (id == 11 || id == 18)
//        {
//            return false;
//        }
//    #endif
//    #if defined(__ANDROID__) || defined(__APPLE__)
//        if ((id == 12 || id == 13 || id == 22) && scope.empty())
//        {
//            return false;
//        }
//    #endif
//        configureScope(scope);
//        const std::string name = testName(id);
//        if (id == 10 || id == 21)
//        {
//            return testTwoProcesses(name, scope, checks, id == 21);
//        }
//        if (id == 20)
//        {
//            return testAbandonedProcess(name, scope, checks);
//        }
//        if (id == 5)
//        {
//            owner.createProcessSemaphore(name);
//            Semaphore peer;
//            peer.openProcessSemaphore(name);
//            owner.processSignal();
//            owner.processSignal();
//            owner.processSignal();
//            check(peer.processWait(0) && peer.processWait(0) && peer.processWait(0),
//                "Peer consumes three notifications", checks);
//            check(!peer.processWait(0), "Peer cannot consume a fourth", checks);
//            peer.closeProcessSemaphore();
//            Semaphore next;
//            next.openProcessSemaphore(name);
//            owner.processSignal();
//            check(next.processWait(0), "Closing peer does not delete owner resource", checks);
//        }
//        else if (id == 6)
//        {
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            owner.createProcessSemaphore(name);
//            owner.openProcessSemaphore(name);
//            check(owner.processWait(0), "Repeated create/open preserves count", checks);
//            check(!owner.processWait(0), "Repeated calls do not add notifications", checks);
//            owner.closeProcessSemaphore();
//            owner.closeProcessSemaphore();
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "Recreated object starts empty", checks);
//        }
//    #if defined(__ANDROID__) || defined(__APPLE__)
//        else if (id == 7)
//        {
//            owner.openProcessSemaphore(name);
//            owner.processSignal();
//            check(!owner.processWait(0), "Opening missing object stays closed, without exception", checks);
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Retry after failed open", checks);
//        }
//        else if (id == 8)
//        {
//            const std::string invalid[] = { "", ".", "..", "../outside", "a/b", "a\\b", std::string("a\0b", 3) };
//            for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index)
//            {
//                owner.createProcessSemaphore(invalid[index]);
//                owner.processSignal();
//                check(!owner.processWait(0), "Reject invalid logical name without exception", checks);
//            }
//            configureScope("relative");
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "Reject relative directory", checks);
//            configureScope(scope);
//            owner.createProcessSemaphore(name);
//            configureScope("invalid/group");
//            owner.createProcessSemaphore(name);
//            owner.openProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Live resource uses captured scope instead of environment", checks);
//            configureScope(scope);
//        }
//        else if (id == 9)
//        {
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "Zero timeout never blocks", checks);
//            const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
//            check(!owner.processWait(20), "Process timeout reports false", checks);
//            const int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
//                std::chrono::steady_clock::now() - start).count();
//            check(elapsed >= 10 && elapsed < 1000, "Process timeout duration", checks);
//            owner.processSignal();
//            check(!owner.processWait(-2), "Unsupported negative timeout fails without consuming", checks);
//            check(owner.processWait(-1), "Negative-one wait consumes preserved notification", checks);
//        }
//    #endif
//        else if (id == 11)
//        {
//            owner.createProcessSemaphore(name);
//            Semaphore peer;
//            peer.openProcessSemaphore(name);
//            owner.closeProcessSemaphore();
//            peer.processSignal();
//            check(peer.processWait(0), "Opened peer survives creator close", checks);
//            check(!peer.processWait(0), "Peer count remains correct", checks);
//        }
//    #if defined(__ANDROID__) || defined(__APPLE__)
//        else if (id == 12 || id == 13)
//        {
//            const std::string file = scope + "/Semaphore." + name;
//            const std::string target = scope + "/" + name + "_target";
//            const std::string fixture = id == 12 ? file : target;
//            {
//                std::ofstream output(fixture.c_str(), std::ios::binary | std::ios::trunc);
//                output << "broken";
//                output.close();
//                check(!output.fail(), "Write owned fault fixture", checks);
//                check(::chmod(fixture.c_str(), 0600) == 0, "Restrict owned fault fixture permissions", checks);
//            }
//            if (id == 13)
//            {
//                check(::symlink(target.c_str(), file.c_str()) == 0, "Create owned symlink", checks);
//            }
//            try
//            {
//                owner.createProcessSemaphore(name);
//                owner.processSignal();
//                check(!owner.processWait(0), "Create rejects incompatible file or symlink", checks);
//                owner.openProcessSemaphore(name);
//                check(!owner.processWait(0), "Open rejects incompatible file or symlink", checks);
//                std::ifstream input(fixture.c_str(), std::ios::binary);
//                std::string actual;
//                std::getline(input, actual);
//                input.close();
//                check(actual == "broken", "Failures must not overwrite fixture or link target", checks);
//            }
//            catch (...)
//            {
//                owner.closeProcessSemaphore();
//                ::unlink(file.c_str());
//                if (id == 13)
//                {
//                    ::unlink(target.c_str());
//                }
//                throw;
//            }
//            ::unlink(file.c_str());
//            if (id == 13)
//            {
//                ::unlink(target.c_str());
//            }
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Retry after removing owned fault fixture", checks);
//        }
//    #endif
//        else if (id == 16)
//        {
//    #if defined(__ANDROID__) || defined(__APPLE__)
//            configureScope("invalid/group");
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "Private initialization rejects invalid scope without exception", checks);
//            configureScope(scope);
//    #else
//            configureScope("ignored-on-desktop");
//    #endif
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Original public API works without any setter", checks);
//            owner.closeProcessSemaphore();
//            configureScope(scope);
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "Private initialization remains reusable after close", checks);
//        }
//        else if (id == 18)
//        {
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            Semaphore peer;
//            peer.createProcessSemaphore(name);
//            check(peer.processWait(0), "Another creator joins without resetting existing count", checks);
//            owner.closeProcessSemaphore();
//            Semaphore later;
//            later.openProcessSemaphore(name);
//            peer.processSignal();
//            check(later.processWait(0), "New opener still sees name after initial creator closes", checks);
//            later.processSignal();
//            check(peer.processWait(0), "Both remaining peers share exactly the same object", checks);
//            peer.closeProcessSemaphore();
//            later.processSignal();
//            check(later.processWait(0), "Last peer remains usable", checks);
//            later.closeProcessSemaphore();
//            owner.openProcessSemaphore(name);
//            owner.processSignal();
//            check(!owner.processWait(0), "Last close removes the live name", checks);
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "New generation has no old notifications", checks);
//        }
//        else if (id == 19)
//        {
//            owner.createProcessSemaphore(name + "a");
//            Semaphore other;
//            other.createProcessSemaphore(name + "b");
//            Semaphore peer;
//            peer.openProcessSemaphore(name + "a");
//            owner.processSignal();
//            check(!other.processWait(0), "Different names do not share a notification", checks);
//            check(peer.processWait(0), "Exact same name shares a notification", checks);
//            other.processSignal();
//            check(!owner.processWait(0), "Isolation holds in the other direction", checks);
//            check(other.processWait(0), "Other object retains its own notification", checks);
//        }
//    #if defined(__ANDROID__) || defined(__APPLE__)
//        else if (id == 22)
//        {
//            const uint32_t headers[][5] =
//            {
//                { 0x53454D31, 2, 20, 0, ~uint32_t(0) },
//                { 0x53454D32, 1, 20, 0, ~uint32_t(0) },
//                { 0x53454D32, 2, 24, 0, ~uint32_t(0) },
//                { 0x53454D32, 2, 20, 1, 0 },
//                { 0x53454D32, 2, 20, 0x80000000, 0x7FFFFFFF },
//                { 0, 2, 20, 0, ~uint32_t(0) },
//                { 0x53454D32, 2, 20, 3, ~uint32_t(3) }
//            };
//            const std::string file = scope + "/Semaphore." + name;
//            for (size_t index = 0; index < sizeof(headers) / sizeof(headers[0]); ++index)
//            {
//                const std::string fixture(reinterpret_cast<const char*>(headers[index]), sizeof(headers[index]));
//                std::ofstream output(file.c_str(), std::ios::binary | std::ios::trunc);
//                output.write(fixture.data(), static_cast<std::streamsize>(fixture.size()));
//                output.close();
//                check(!output.fail(), "Write owned v2 record fixture", checks);
//                check(::chmod(file.c_str(), 0600) == 0, "Restrict owned record fixture permissions", checks);
//                try
//                {
//                    if (index < 5)
//                    {
//                        owner.openProcessSemaphore(name);
//                        owner.createProcessSemaphore(name);
//                        owner.processSignal();
//                        check(!owner.processWait(0), "Incompatible or torn state must not become usable", checks);
//                        std::ifstream input(file.c_str(), std::ios::binary);
//                        const std::string actual((std::istreambuf_iterator<char>(input)),
//                            std::istreambuf_iterator<char>());
//                        check(!input.bad() && actual == fixture, "Rejected record is byte-for-byte unchanged", checks);
//                    }
//                    else
//                    {
//                        owner.createProcessSemaphore(name);
//                        check(!owner.processWait(0), "Inactive valid or unpublished generation is reset to empty", checks);
//                        owner.processSignal();
//                        Semaphore peer;
//                        peer.createProcessSemaphore(name);
//                        check(peer.processWait(0), "Live v2 generation is not reset", checks);
//                        check(!peer.processWait(0), "No duplicate notification in v2 generation", checks);
//                        owner.closeProcessSemaphore();
//                        peer.processSignal();
//                        check(peer.processWait(0), "Remaining peer keeps the generation alive", checks);
//                    }
//                    owner.closeProcessSemaphore();
//                    ::unlink(file.c_str());
//                }
//                catch (...)
//                {
//                    owner.closeProcessSemaphore();
//                    ::unlink(file.c_str());
//                    throw;
//                }
//            }
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Same instance retries after invalid fixture removal", checks);
//        }
//        else if (id == 23)
//        {
//            owner.createProcessSemaphore(std::string(4096, 'x'));
//            owner.processSignal();
//            check(!owner.processWait(0), "Oversized name is rejected, never truncated", checks);
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Short-name retry works", checks);
//        }
//        else if (id == 24)
//        {
//            if (scope.empty())
//            {
//                return false;
//            }
//            const char* previous = std::getenv("TMPDIR");
//            const bool hadPrevious = previous != nullptr;
//            const std::string saved = hadPrevious ? previous : "";
//            check(::setenv("TMPDIR", scope.c_str(), 1) == 0, "Set owned temporary-directory configuration", checks);
//            configureScope("");
//            try
//            {
//                owner.createProcessSemaphore(name);
//                Semaphore peer;
//                peer.openProcessSemaphore(name);
//                owner.processSignal();
//                check(peer.processWait(0), "Optional scope uses the same default directory for original API", checks);
//                owner.closeProcessSemaphore();
//                peer.closeProcessSemaphore();
//            }
//            catch (...)
//            {
//                owner.closeProcessSemaphore();
//                if (hadPrevious)
//                {
//                    ::setenv("TMPDIR", saved.c_str(), 1);
//                }
//                else
//                {
//                    ::unsetenv("TMPDIR");
//                }
//                configureScope(scope);
//                throw;
//            }
//            check((hadPrevious ? ::setenv("TMPDIR", saved.c_str(), 1) : ::unsetenv("TMPDIR")) == 0,
//                "Restore only the owned test process environment", checks);
//            configureScope(scope);
//        }
//    #endif
//        return true;
//    };
//
//    int32_t selected = 0;
//    std::string scope;
//    std::string peer;
//    bool infinitePeer = false;
//    std::string abandoningPeer;
//    for (int32_t index = 1; index < argc; ++index)
//    {
//        const std::string argument = argv[index];
//        if (index + 1 >= argc)
//        {
//            return 2;
//        }
//        if (argument == "--case")
//        {
//            char* end = nullptr;
//            const long value = std::strtol(argv[++index], &end, 10);
//            if (end == argv[index] || *end != '\0' || value < 1 || value > 24)
//            {
//                return 2;
//            }
//            selected = static_cast<int32_t>(value);
//        }
//        else if (argument == "--scope")
//        {
//            scope = argv[++index];
//        }
//        else if (argument == "--peer" || argument == "--peer-infinite")
//        {
//            infinitePeer = argument == "--peer-infinite";
//            peer = argv[++index];
//        }
//        else if (argument == "--abandon")
//        {
//            abandoningPeer = argv[++index];
//        }
//        else
//        {
//            return 2;
//        }
//    }
//    if (!peer.empty())
//    {
//        return runPeer(peer, scope, infinitePeer);
//    }
//    if (!abandoningPeer.empty())
//    {
//        runAbandonedPeer(abandoningPeer, scope);
//    }
//    int32_t failed = 0;
//    int32_t passed = 0;
//    int32_t skipped = 0;
//    int32_t totalChecks = 0;
//    for (int32_t id = 1; id <= 24; ++id)
//    {
//        if (selected != 0 && selected != id)
//        {
//            continue;
//        }
//        int32_t checks = 0;
//        try
//        {
//            if (runCase(id, scope, checks))
//            {
//                ++passed;
//                std::printf("PASS id=%d checks=%d\n", id, checks);
//            }
//            else
//            {
//                ++skipped;
//                std::printf("SKIP id=%d %s\n", id,
//                    (id == 14 || id == 15 || id == 17) ? "retired test" : "platform/host not applicable");
//            }
//        }
//        catch (const std::exception& error)
//        {
//            ++failed;
//            std::fprintf(stderr, "FAIL id=%d checks=%d: %s\n", id, checks, error.what());
//        }
//        totalChecks += checks;
//    }
//    std::printf("RESULT pass=%d fail=%d skip=%d checks=%d\n", passed, failed, skipped, totalChecks);
//    return failed == 0 ? 0 : 1;
//}