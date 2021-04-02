#ifndef RSA_H
#define RSA_H

#include <string>

//! MAX���������������LENΪ�ṹ��slink��ռ���ڴ�ռ��С */
#define RSA_ALGORITHM_MAX 100
#define RSA_ALGORITHM_LEN sizeof(struct RsaAlgorithm::slink)
//! �ܹ����ж�̬���ӿ�����RSA��
/*!
@see class _declspec(dllexport) RSA
��RSA�㷨д�ɶ�̬���ӿ����ʽ������ã������й�Կ��˽Կ������
�Ϳ��Խ������ĵļ��ܺͽ���
*/
class RsaAlgorithm
{
public:
	//! �¶�������ݽṹslink
	/*!
	@see struct slink
	���ݽṹ�У�bignum�洢����������ɵĴ�����next��ָ������ָ��
	@see int bignum[MAX]
	*/
	typedef struct slink
	{
		int bignum[RSA_ALGORITHM_MAX];/*!< bignum[98]������������ţ�1����0��bignum[99]�����ʵ�ʳ���*/
		struct slink* next;
	}slink;

public:
	//! RSA �Ĺ��캯��
	/*!
	@see RSA()
	����Ӧ�ö�RSA���е�һЩ����������Ӧ�ĳ�ʼ��
	*/
	RsaAlgorithm();
	//! RSA����������
	/*!
	@see ~RSA()
	�ͷ��ڴ�
	*/
	~RsaAlgorithm();

public:
	//! RSA�Ĵ������㺯����
	/** @�����ȽϺ���
	@see int cmp(int, int)
	*/
	int cmp(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX]);
	/** @��������ת������
	@see void mov(int a[MAX],int *b);
	*/
	void mov(int a[RSA_ALGORITHM_MAX], int *b);
	/** @�����˻�����
	@see void mul(int a1[MAX],int a2[MAX],int *c);
	*/
	void mul(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX], int *c);
	/** @������Ӻ���
	@see void add(int a1[MAX],int a2[MAX],int *c);
	*/
	void add(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX], int *c);
	/** @���������������
	@see  void sub(int a1[MAX],int a2[MAX],int *c);
	*/
	void sub(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX], int *c);
	/*! @����ȡģ����
	@see void mod(int a[MAX],int b[MAX],int  *c);
	@attention /c=a mod b//ע�⣺������֪���˴�A��C�����鶼�ı��ˡ�
	*/
	void mod(int a[RSA_ALGORITHM_MAX], int b[RSA_ALGORITHM_MAX], int  *c);
	/*! @�����������
	@see void divt(int t[MAX],int b[MAX],int  *c ,int *w);
	@attention //���̷�//�����Ժ�wΪa mod b, CΪa  div b;
	*/
	void divt(int t[RSA_ALGORITHM_MAX], int b[RSA_ALGORITHM_MAX], int  *c, int *w);
	/*! @��� �� m=a*b mod n;
	/*!
	@see void mulmod(int a[MAX] ,int b[MAX] ,int n[MAX],int *m);
	*/
	void mulmod(int a[RSA_ALGORITHM_MAX], int b[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], int *m);
	/*! @��� m=a^p  mod n�ĺ�������
	/*!
	@see void expmod(int a[MAX] ,int p[MAX] ,int n[MAX],int *m);
	*/
	void expmod(int a[RSA_ALGORITHM_MAX], int p[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], int *m);
	/*!  @�ж��Ƿ�Ϊ����
	@see int   is_prime_san(int p[MAX] );
	*/
	int is_prime_san(int p[RSA_ALGORITHM_MAX]);
	/*! @�ж���������֮���Ƿ���
	@see int coprime(int e[MAX],int s[MAX]);
	*/
	int coprime(int e[RSA_ALGORITHM_MAX], int s[RSA_ALGORITHM_MAX]);
	/*!  @�����������
	@see void prime_random(int *p,int *q);
	*/
	void prime_random(int *p, int *q);
	/*! @��������e
	@see void erand(int e[MAX],int m[MAX]);
	*/
	void erand(int e[RSA_ALGORITHM_MAX], int m[RSA_ALGORITHM_MAX]);
	/*! @���ݹ��������������
	@see void rsad(int e[MAX],int g[MAX],int *d);
	*/
	void rsad(int e[RSA_ALGORITHM_MAX], int g[RSA_ALGORITHM_MAX], int *d);
	/*! @�������Կd�ĺ���(����Euclid�㷨)
	@see unsigned long  rsa(unsigned long p,unsigned long q,unsigned long e);
	*/
	unsigned long rsa(unsigned long p, unsigned long q, unsigned long e);

	//! RSA�Ĳ��������Ĺ�Կ��˽Կ�ĺ���
	/*!
	@see bool RSAKey();
	@param û���κ����룬
	@param �������e��d��n�ĺ�����������ʱ��Ƚϳ�����Ҫ�ȴ�
	@return[bool] �ɹ�����true��ʧ�ܷ���false
	*/
	bool RSAKey();

	//! RSA�Ľ����ļ����ܵĺ���
	/*!
	@see string tencrypto(int e[MAX], int n[MAX], char* text);
	@param[int[] e��nΪ��������Ĺ�Կ�����ù�Կ���м���
	@param[char* textΪ���ģ�������char*�ĸ�ʽ�洢
	@return[string] ����ֵΪ���ܳɹ�֮������ģ�����string���ͽ��д洢
	*/
	std::string tencrypto(int e[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], const char* text);

	//! RSA�Ľ����ļ����ܵĺ���
	/*!
	@see string tdecrypto(int d[MAX], int n[MAX], string text);
	@param[int[] d,nΪ˽Կ���ɺ���RSAKey()����
	@param[string textΪ���ģ���Ӧ���ܺ��������ĵĸ�ʽΪstring
	@return[string] ����֮������Ĳ���string���д洢
	*/
	std::string tdecrypto(int d[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], const std::string& text);

public:
	/** @brief �����ȫ�ֱ������洢��Կ */
	int p[RSA_ALGORITHM_MAX], q[RSA_ALGORITHM_MAX], n[RSA_ALGORITHM_MAX], d[RSA_ALGORITHM_MAX], e[RSA_ALGORITHM_MAX], m[RSA_ALGORITHM_MAX], p1[RSA_ALGORITHM_MAX], q1[RSA_ALGORITHM_MAX];

private:
	int i;
	char c;
	//struct slink *head,*h1,*h2;
};

#endif //RSA_H