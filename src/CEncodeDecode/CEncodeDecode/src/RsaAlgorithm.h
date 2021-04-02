#ifndef RSA_H
#define RSA_H

#include <string>

//! MAX是数组的最大个数，LEN为结构体slink的占用内存空间大小 */
#define RSA_ALGORITHM_MAX 100
#define RSA_ALGORITHM_LEN sizeof(struct RsaAlgorithm::slink)
//! 能够进行动态链接库编译的RSA类
/*!
@see class _declspec(dllexport) RSA
将RSA算法写成动态链接库的形式方便调用，其中有公钥，私钥和明文
就可以进行明文的加密和解密
*/
class RsaAlgorithm
{
public:
	//! 新定义的数据结构slink
	/*!
	@see struct slink
	数据结构中，bignum存储的是随机生成的大数，next是指向后面的指针
	@see int bignum[MAX]
	*/
	typedef struct slink
	{
		int bignum[RSA_ALGORITHM_MAX];/*!< bignum[98]用来标记正负号，1正，0负bignum[99]来标记实际长度*/
		struct slink* next;
	}slink;

public:
	//! RSA 的构造函数
	/*!
	@see RSA()
	其中应该对RSA类中的一些变量进行相应的初始化
	*/
	RsaAlgorithm();
	//! RSA的析构函数
	/*!
	@see ~RSA()
	释放内存
	*/
	~RsaAlgorithm();

public:
	//! RSA的大数运算函数库
	/** @大数比较函数
	@see int cmp(int, int)
	*/
	int cmp(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX]);
	/** @大数类型转换函数
	@see void mov(int a[MAX],int *b);
	*/
	void mov(int a[RSA_ALGORITHM_MAX], int *b);
	/** @大数乘积函数
	@see void mul(int a1[MAX],int a2[MAX],int *c);
	*/
	void mul(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX], int *c);
	/** @大数相加函数
	@see void add(int a1[MAX],int a2[MAX],int *c);
	*/
	void add(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX], int *c);
	/** @大数大数相减函数
	@see  void sub(int a1[MAX],int a2[MAX],int *c);
	*/
	void sub(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX], int *c);
	/*! @大数取模函数
	@see void mod(int a[MAX],int b[MAX],int  *c);
	@attention /c=a mod b//注意：经检验知道此处A和C的数组都改变了。
	*/
	void mod(int a[RSA_ALGORITHM_MAX], int b[RSA_ALGORITHM_MAX], int  *c);
	/*! @大数相除函数
	@see void divt(int t[MAX],int b[MAX],int  *c ,int *w);
	@attention //试商法//调用以后w为a mod b, C为a  div b;
	*/
	void divt(int t[RSA_ALGORITHM_MAX], int b[RSA_ALGORITHM_MAX], int  *c, int *w);
	/*! @解决 了 m=a*b mod n;
	/*!
	@see void mulmod(int a[MAX] ,int b[MAX] ,int n[MAX],int *m);
	*/
	void mulmod(int a[RSA_ALGORITHM_MAX], int b[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], int *m);
	/*! @解决 m=a^p  mod n的函数问题
	/*!
	@see void expmod(int a[MAX] ,int p[MAX] ,int n[MAX],int *m);
	*/
	void expmod(int a[RSA_ALGORITHM_MAX], int p[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], int *m);
	/*!  @判断是否为素数
	@see int   is_prime_san(int p[MAX] );
	*/
	int is_prime_san(int p[RSA_ALGORITHM_MAX]);
	/*! @判断两个大数之间是否互质
	@see int coprime(int e[MAX],int s[MAX]);
	*/
	int coprime(int e[RSA_ALGORITHM_MAX], int s[RSA_ALGORITHM_MAX]);
	/*!  @随机产生素数
	@see void prime_random(int *p,int *q);
	*/
	void prime_random(int *p, int *q);
	/*! @产生素数e
	@see void erand(int e[MAX],int m[MAX]);
	*/
	void erand(int e[RSA_ALGORITHM_MAX], int m[RSA_ALGORITHM_MAX]);
	/*! @根据规则产生其他的数
	@see void rsad(int e[MAX],int g[MAX],int *d);
	*/
	void rsad(int e[RSA_ALGORITHM_MAX], int g[RSA_ALGORITHM_MAX], int *d);
	/*! @求解密密钥d的函数(根据Euclid算法)
	@see unsigned long  rsa(unsigned long p,unsigned long q,unsigned long e);
	*/
	unsigned long rsa(unsigned long p, unsigned long q, unsigned long e);

	//! RSA的产生大数的公钥和私钥的函数
	/*!
	@see bool RSAKey();
	@param 没有任何输入，
	@param 随机产生e，d，n的函数，其运行时间比较长，需要等待
	@return[bool] 成功返回true，失败返回false
	*/
	bool RSAKey();

	//! RSA的进行文件加密的函数
	/*!
	@see string tencrypto(int e[MAX], int n[MAX], char* text);
	@param[int[] e，n为随机产生的公钥，利用公钥进行加密
	@param[char* text为明文，明文以char*的格式存储
	@return[string] 返回值为加密成功之后的密文，采用string类型进行存储
	*/
	std::string tencrypto(int e[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], const char* text);

	//! RSA的进行文件解密的函数
	/*!
	@see string tdecrypto(int d[MAX], int n[MAX], string text);
	@param[int[] d,n为私钥，由函数RSAKey()产生
	@param[string text为密文，对应加密函数，密文的格式为string
	@return[string] 解密之后的明文采用string进行存储
	*/
	std::string tdecrypto(int d[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], const std::string& text);

public:
	/** @brief 定义的全局变量，存储密钥 */
	int p[RSA_ALGORITHM_MAX], q[RSA_ALGORITHM_MAX], n[RSA_ALGORITHM_MAX], d[RSA_ALGORITHM_MAX], e[RSA_ALGORITHM_MAX], m[RSA_ALGORITHM_MAX], p1[RSA_ALGORITHM_MAX], q1[RSA_ALGORITHM_MAX];

private:
	int i;
	char c;
	//struct slink *head,*h1,*h2;
};

#endif //RSA_H