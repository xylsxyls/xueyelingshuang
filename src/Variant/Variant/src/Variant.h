#pragma once
#include "VariantMacro.h"
#include <stdint.h>
#include <string>

/** Variant的独占存储；长度不超过16的字节内联，长字节才使用堆指针
*/
union VariantStorage
{
    // 数字的原生位模式；对应INT32、UINT64和DOUBLE
    uint64_t m_scalar;
    // STRING/BLOB长度超过16时拥有的数组，仅所属Variant释放
    char* m_heap;
    // STRING/BLOB长度不超过16时的完整字节，不补NUL结束符
    char m_small[16];
};

// 拥有一个标量或字节值；数字和不超过16字节的字符串/二进制不分配。
// 同一实例须由调用者串行访问；保留既有类型号和宿主原生字节序序列化。
class VariantAPI Variant
{
public:
    // 沿用原公开Variant::VariantType名称及数值，避免调用方更改既有类型引用。
    enum VariantType : int32_t
    {
        // 未赋值或非法类型/宽度输入。
        INIT,
        // 四字节有符号整数。
        INT32,
        // 八字节无符号整数。
        UINT64,
        // 八字节浮点数，复制时保持原始位模式。
        DOUBLE,
        // 文本类型；常规文本入口只保留首个\0之前的字节，不保存结束符。
        // 原始字节反序列化入口可恢复含\0的历史STRING，详见带isBlob的构造函数。
        STRING,
        // 按显式长度保存的二进制字节，允许内部\0；std::string构造和赋值均生成此类型。
        BLOB
    };

public:
    /** 创建INIT空值，不分配
    */
    Variant();

    /** 擦除并释放独占字节，不抛异常
    */
    ~Variant();

    /** 深拷贝；分配失败时不遗留半构造资源
    @param [in] value 本次调用借用
    */
    Variant(const Variant& value);

    /** 先复制后替换，返回当前对象引用，不为被忽略的返回值再深拷贝
    @param [in] value 本次借用，允许自赋值；分配失败保持当前值
    @return 当前实例；需要独立快照时显式构造Variant副本
    */
    Variant& operator=(const Variant& value);

    /** 转移独占内容，无堆分配；源恢复INIT
    @param [in,out] value 本次独占借用，之后只可作为INIT重新使用
    */
    Variant(Variant&& value);

    /** 转移内容并释放当前旧值，无堆分配；自移动不变
    @param [in,out] value 本次独占借用，非自身时源恢复INIT
    @return 当前实例
    */
    Variant& operator=(Variant&& value);

    /** 交换独占存储，不分配；交换后旧借用指针失效
    @param [in,out] value 本次独占借用，可为自身
    */
    void swap(Variant& value);

    /** 直接复制带长度的原始值，避免先构造std::string；捕获分配异常
    @param [in] data 借用至返回，可指向当前值；size为0时可空
    @param [in] size 字节数；数字必须为4/8字节，INIT必须为0
    @param [in] type 结果类型；STRING同BLOB按长度保留内部NUL，不做文本转换
    @return 成功替换；参数错误或分配失败保持当前值
    */
    bool assignBytes(const void* data, size_t size, VariantType type = BLOB);

    /** 零复制借用原始字节，不包含结束符；不能跨对象修改、移动或释放保存
    @return 长度为0时nullptr；其余只读字节由本实例拥有，禁止释放或写入
    */
    const char* data() const;

    /** 查询原始字节长度，不分配
    @return data对应的字节数；数字是原生表示，非文本长度
    */
    size_t size() const;

    /** 仅按长度校验线格式，不读取输入内存
    @param [in] size 原始字节数
    @param [in] type 整数类型号，不得先把未经校验的数转为枚举
    @return 类型有效且数字宽度完整；STRING/BLOB允许任意长度
    */
    static bool isValidBlob(size_t size, int32_t type);

    /** 创建四字节整数，不分配
    @param [in] value 完整int32值
    */
    Variant(const int32_t& value);

    /** 创建八字节整数，不分配
    @param [in] value 完整uint64值
    */
    Variant(const uint64_t& value);

    /** 创建浮点值，不分配
    @param [in] value 保留原始位模式
    */
    Variant(const double& value);

    /** 创建STRING，只复制首个\0之前的文本，不保存结束符
    @param [in] value 本次借用的NUL终止文本；nullptr生成空STRING，\0后的字节不读取
    */
    Variant(const char* value);

    /** 创建BLOB，按value.size()复制全部字节，内容没有\0时仍为BLOB
    @param [in] value 本次借用，可为空；含\0的数据须先用std::string(data, length)保留完整长度
    */
    Variant(const std::string& value);

    /** 按指定类型解释文本或恢复原始序列化字节；非法类型/宽度为INIT
    @param [in] data 本次借用且携带完整长度；isBlob=false时数字沿用C文本转换
    @param [in] type 结果类型；不会因为isBlob=true而统一变为BLOB
    @param [in] isBlob false时STRING在首个\0处截断，BLOB仍复制全部字节；true用于反序列化，
        数字校验固定宽度，STRING/BLOB均按长度原样恢复，允许内部\0且不校验文本编码
    */
    Variant(const std::string& data, VariantType type, bool isBlob = false);

    /** 校验序列化数据的类型和固定宽度，不读取越界数字
    @param [in] data 原始字节
    @param [in] type 先校验整数类型号，再转换枚举
    @return INIT仅接受空值，数字宽度必须完整；STRING/BLOB允许任意长度和内部\0，不校验文本编码
    */
    static bool isValidBlob(const std::string& data, int32_t type);

    /** 替换为整数，不分配
    @param [in] value int32值
    @return 当前实例引用，不产生额外副本
    */
    Variant& operator=(const int32_t& value);

    /** 替换为整数，不分配
    @param [in] value uint64值
    @return 当前实例引用，不产生额外副本
    */
    Variant& operator=(const uint64_t& value);

    /** 替换为浮点数，不分配
    @param [in] value double位模式
    @return 当前实例引用，不产生额外副本
    */
    Variant& operator=(const double& value);

    /** 先复制再替换为STRING，只保留首个\0之前的文本，不保存结束符
    @param [in] value 本次借用的NUL终止文本；nullptr生成空STRING
    @return 当前实例引用；分配失败保持原值
    */
    Variant& operator=(const char* value);

    /** 先复制再替换为BLOB，内容没有\0时也不改为STRING
    @param [in] value 本次借用，按size()保留全部字节，包括内部\0
    @return 当前实例引用；分配失败保持原值
    */
    Variant& operator=(const std::string& value);

    /** 沿用历史仅比较长度和原始字节的规则，不额外比较类型号
    @param [in] value 比较对象，仅本次借用
    @return 字节一致
    */
    bool operator==(const Variant& value) const;

    /** 比较原始字节差异
    @param [in] value 比较对象，仅本次借用
    @return 字节不同
    */
    bool operator!=(const Variant& value) const;

    /** 读取对应类型的整数，不做隐式字符串转换
    @return INT32完整值，其他类型或空值为0
    */
    operator int32_t() const;

    /** 读取对应类型的整数
    @return UINT64完整值，其他类型或空值为0
    */
    operator uint64_t() const;

    /** 读取对应类型的浮点数
    @return DOUBLE值，其他类型或空值为0
    */
    operator double() const;

    /** 拷贝文本或二进制字节
    @return STRING/BLOB的完整字节，内部\0保留；其他类型为空，长度须使用std::string::size()
    */
    operator std::string() const;

    /** 查询原始类型号
    @return 当前VariantType
    */
    VariantType type() const;

    /** 擦除并释放内容，恢复INIT；不保留大缓冲
    */
    void clear();

    /** 按固定classic locale和17位浮点精度输出历史文本表示
    @return 数字的独立文本；STRING/BLOB保持原字节及内部\0，不是转义或十六进制编码
    */
    std::string toString() const;

    /** 取得历史原生字节序表示
    @return 独立字节，保留内部\0，不写入类型号；须另存类型并按size()取长度
    */
    std::string toBlob() const;

    /** 输出文本表示；格式化仅在显式请求时进行
    @param [out] result 被数字文本或STRING/BLOB完整字节替换，内部\0保留，空值清空
    */
    void toString(std::string& result) const;

    /** 输出历史原生字节表示
    @param [out] result 被当前完整字节替换，内部\0保留，空值清空；长度使用size()
    */
    void toBlob(std::string& result) const;

protected:
    /** 替换为无分配int32
    @param [in] value 本次借用，可来自当前标量
    */
    void setInt32(const int32_t& value);

    /** 替换为无分配uint64
    @param [in] value 本次借用，可来自当前标量
    */
    void setUint64(const uint64_t& value);

    /** 替换为无分配double
    @param [in] value 本次借用，复制原始位
    */
    void setDouble(const double& value);

    /** 先复制再释放旧字节并设为STRING；以首个\0为终点，不保存结束符
    @param [in] value 本次借用的NUL终止文本，可空；\0后的内容不属于输入文本
    */
    void setString(const char* value);

    /** 先复制再释放旧字节并设为BLOB；按长度保存，不解释\0或文本编码
    @param [in] value 本次借用的完整字节，size()包含内部\0
    */
    void setBlob(const std::string& value);

    /** 深拷贝并提交，允许自赋值
    @param [in] value 本次借用的源
    */
    void setVariant(const Variant& value);

private:
    /** 复制原始字节并提交；调用者已验证类型/宽度，分配失败保持旧值
    @param [in] data 本次借用，可来自当前值；size非0时必须有效
    @param [in] size 完整字节数
    @param [in] type 目标类型
    */
    void setBytes(const char* data, size_t size, VariantType type);

    /** 擦除并释放字节，清空标量和长度；调用者负责设置类型
    */
    void clearMemory();

    /** 取得当前原始字节，不转移所有权
    @return m_length为0时为空，其余借用至本对象修改或析构
    */
    const char* bytes() const;

private:
    // 原公开类型号，保持ProtoMessage线格式。
    VariantType m_type;
    // 实际原始字节数；历史空文本数字的长度为0。
    size_t m_length;
    // 类型和长度决定有效成员；只有长STRING/BLOB持有独占堆指针
    VariantStorage m_storage;
};