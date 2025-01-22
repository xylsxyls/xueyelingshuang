#include "RsaAlgorithm.h"
#include <time.h>
#include <string.h>

RsaAlgorithm::RsaAlgorithm()
{

}

RsaAlgorithm::~RsaAlgorithm()
{

}

/*----------------------------创建自己的大数运算库---------------------------------*/
int RsaAlgorithm::cmp(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX])
{
	int l1, l2;
	int i;
	l1 = a1[99];
	l2 = a2[99];
	if (l1>l2)
		return 1;
	if (l1<l2)
		return -1;
	for (i = (l1 - 1); i >= 0; i--)
	{
		if (a1[i]>a2[i])
			return 1;
		if (a1[i]<a2[i])
			return -1;
	}
	return 0;
}

void RsaAlgorithm::mov(int a[RSA_ALGORITHM_MAX], int *b)
{
	int j;
	for (j = 0; j<RSA_ALGORITHM_MAX; j++)
		b[j] = a[j];
	return;
}

void RsaAlgorithm::mul(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX], int *c)
{
	int i, j;
	int y;
	int x;
	int z;
	int w;
	int l1, l2;
	l1 = a1[RSA_ALGORITHM_MAX - 1];
	l2 = a2[RSA_ALGORITHM_MAX - 1];
	if (a1[RSA_ALGORITHM_MAX - 2] == '-'&& a2[RSA_ALGORITHM_MAX - 2] == '-')
		c[RSA_ALGORITHM_MAX - 2] = 0;
	else if (a1[RSA_ALGORITHM_MAX - 2] == '-')
		c[RSA_ALGORITHM_MAX - 2] = '-';
	else if (a2[RSA_ALGORITHM_MAX - 2] == '-')
		c[RSA_ALGORITHM_MAX - 2] = '-';
	for (i = 0; i<l1; i++)
	{
		for (j = 0; j<l2; j++)
		{
			x = a1[i] * a2[j];
			y = x / 10;
			z = x % 10;
			w = i + j;
			c[w] = c[w] + z;
			c[w + 1] = c[w + 1] + y + c[w] / 10;
			c[w] = c[w] % 10;
		}
	}
	w = l1 + l2;
	if (c[w - 1] == 0)w = w - 1;
	c[RSA_ALGORITHM_MAX - 1] = w;
	return;
}

void RsaAlgorithm::add(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX], int *c)
{
	int i, l1, l2;
	int len, temp[RSA_ALGORITHM_MAX];
	int k = 0;
	l1 = a1[RSA_ALGORITHM_MAX - 1];
	l2 = a2[RSA_ALGORITHM_MAX - 1];
	if ((a1[RSA_ALGORITHM_MAX - 2] == '-') && (a2[RSA_ALGORITHM_MAX - 2] == '-'))
	{
		c[RSA_ALGORITHM_MAX - 2] = '-';
	}
	else if (a1[RSA_ALGORITHM_MAX - 2] == '-')
	{
		mov(a1, temp);
		temp[RSA_ALGORITHM_MAX - 2] = 0;
		sub(a2, temp, c);
		return;
	}
	else if (a2[RSA_ALGORITHM_MAX - 2] == '-')
	{
		mov(a2, temp);
		temp[98] = 0;
		sub(a1, temp, c);
		return;
	}

	if (l1<l2)len = l1;
	else len = l2;
	for (i = 0; i<len; i++)
	{
		c[i] = (a1[i] + a2[i] + k) % 10;
		k = (a1[i] + a2[i] + k) / 10;
	}
	if (l1>len)
	{
		for (i = len; i<l1; i++)
		{
			c[i] = (a1[i] + k) % 10;
			k = (a1[i] + k) / 10;
		}
		if (k != 0)
		{
			c[l1] = k;
			len = l1 + 1;
		}
		else len = l1;
	}
	else
	{
		for (i = len; i<l2; i++)
		{
			c[i] = (a2[i] + k) % 10;
			k = (a2[i] + k) / 10;
		}
		if (k != 0)
		{
			c[l2] = k;
			len = l2 + 1;
		}
		else len = l2;
	}
	c[99] = len;
	return;
}

void RsaAlgorithm::sub(int a1[RSA_ALGORITHM_MAX], int a2[RSA_ALGORITHM_MAX], int *c)
{
	int i, l1, l2;
	int len, t1[RSA_ALGORITHM_MAX], t2[RSA_ALGORITHM_MAX];
	int k = 0;
	l1 = a1[RSA_ALGORITHM_MAX - 1];
	l2 = a2[RSA_ALGORITHM_MAX - 1];
	if ((a1[RSA_ALGORITHM_MAX - 2] == '-') && (a2[RSA_ALGORITHM_MAX - 2] == '-'))
	{
		mov(a1, t1);
		mov(a2, t2);
		t1[RSA_ALGORITHM_MAX - 2] = 0;
		t2[RSA_ALGORITHM_MAX - 2] = 0;
		sub(t2, t1, c);
		return;
	}
	else if (a2[RSA_ALGORITHM_MAX - 2] == '-')
	{
		mov(a2, t2);
		t2[RSA_ALGORITHM_MAX - 2] = 0;
		add(a1, t2, c);
		return;
	}
	else if (a1[RSA_ALGORITHM_MAX - 2] == '-')
	{
		mov(a2, t2);
		t2[RSA_ALGORITHM_MAX - 2] = '-';
		add(a1, t2, c);
		return;
	}
	if (cmp(a1, a2) == 1)
	{
		len = l2;
		for (i = 0; i<len; i++)
		{
			if ((a1[i] - k - a2[i])<0)
			{
				c[i] = (a1[i] - a2[i] - k + 10) % 10;
				k = 1;
			}
			else
			{
				c[i] = (a1[i] - a2[i] - k) % 10;
				k = 0;
			}
		}
		for (i = len; i<l1; i++)
		{
			if ((a1[i] - k)<0)
			{
				c[i] = (a1[i] - k + 10) % 10;
				k = 1;
			}
			else
			{
				c[i] = (a1[i] - k) % 10;
				k = 0;
			}
		}
		if (c[l1 - 1] == 0)/*使得数组C中的前面所以0字符不显示了，如1000-20=0980--->显示为980了*/
		{
			len = l1 - 1;
			i = 2;
			while (c[l1 - i] == 0)/*111456-111450=00006，消除0后变成了6；*/
			{
				len = l1 - i;
				i++;
			}
		}
		else
		{
			len = l1;
		}
	}
	else
		if (cmp(a1, a2) == (-1))
		{
			c[RSA_ALGORITHM_MAX - 2] = '-';
			len = l1;
			for (i = 0; i<len; i++)
			{
				if ((a2[i] - k - a1[i])<0)
				{
					c[i] = (a2[i] - a1[i] - k + 10) % 10;
					k = 1;
				}
				else
				{
					c[i] = (a2[i] - a1[i] - k) % 10;
					k = 0;
				}
			}
			for (i = len; i<l2; i++)
			{
				if ((a2[i] - k)<0)
				{
					c[i] = (a2[i] - k + 10) % 10;
					k = 1;
				}
				else
				{
					c[i] = (a2[i] - k) % 10;
					k = 0;
				}
			}
			if (c[l2 - 1] == 0)
			{
				len = l2 - 1;
				i = 2;
				while (c[l1 - i] == 0)
				{
					len = l1 - i;
					i++;
				}
			}
			else len = l2;
		}
		else if (cmp(a1, a2) == 0)
		{
			len = 1;
			c[len - 1] = 0;
		}
	c[RSA_ALGORITHM_MAX - 1] = len;
	return;
}

void RsaAlgorithm::mod(int a[RSA_ALGORITHM_MAX], int b[RSA_ALGORITHM_MAX], int  *c)/*/c=a mod b//注意：经检验知道此处A和C的数组都改变了。*/
{
	int d[RSA_ALGORITHM_MAX] = {};
	mov(a, d);
	while (cmp(d, b) != (-1))/*/c=a-b-b-b-b-b.......until(c<b)*/
	{
		sub(d, b, c);
		mov(c, d);/*/c复制给a*/
	}
	return;
}

void RsaAlgorithm::divt(int t[RSA_ALGORITHM_MAX], int b[RSA_ALGORITHM_MAX], int  *c, int *w)/*//试商法//调用以后w为a mod b, C为a  div b;*/
{
	int a1, b1, i, j, m;/*w用于暂时保存数据*/
	int d[RSA_ALGORITHM_MAX], e[RSA_ALGORITHM_MAX], f[RSA_ALGORITHM_MAX], g[RSA_ALGORITHM_MAX], a[RSA_ALGORITHM_MAX];
	mov(t, a);
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)
		e[i] = 0;
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)
		d[i] = 0;
	for (i = 0; i<RSA_ALGORITHM_MAX; i++) g[i] = 0;
	a1 = a[RSA_ALGORITHM_MAX - 1];
	b1 = b[RSA_ALGORITHM_MAX - 1];
	if (cmp(a, b) == (-1))
	{
		c[0] = 0;
		c[RSA_ALGORITHM_MAX - 1] = 1;
		mov(t, w);
		return;
	}
	else if (cmp(a, b) == 0)
	{
		c[0] = 1;
		c[RSA_ALGORITHM_MAX - 1] = 1;
		w[0] = 0;
		w[RSA_ALGORITHM_MAX - 1] = 1;
		return;
	}
	m = (a1 - b1);
	for (i = m; i >= 0; i--)/*341245/3=341245-300000*1--->41245-30000*1--->11245-3000*3--->2245-300*7--->145-30*4=25--->25-3*8=1*/
	{
		for (j = 0; j<RSA_ALGORITHM_MAX; j++)
			d[j] = 0;
		d[i] = 1;
		d[RSA_ALGORITHM_MAX - 1] = i + 1;
		mov(b, g);
		mul(g, d, e);
		while (cmp(a, e) != (-1))
		{
			c[i]++;
			sub(a, e, f);
			mov(f, a);/*f复制给g*/
		}
		for (j = i; j<RSA_ALGORITHM_MAX; j++)/*高位清零*/
			e[j] = 0;
	}
	mov(a, w);
	if (c[m] == 0) c[RSA_ALGORITHM_MAX - 1] = m;
	else c[RSA_ALGORITHM_MAX - 1] = m + 1;
	return;
}

void RsaAlgorithm::mulmod(int a[RSA_ALGORITHM_MAX], int b[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], int *m)/*解决 了 m=a*b mod n;*/
{
	int c[RSA_ALGORITHM_MAX], d[RSA_ALGORITHM_MAX];
	int i;
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)
		d[i] = c[i] = 0;
	mul(a, b, c);
	divt(c, n, d, m);
	//for(i=0;i<m[MAX-1];i++)
	//	printf("%d",m[m[MAX-1]-i-1]);
	//printf("\nm  length is :  %d \n",m[MAX-1]);
}
/*-------------接下来的重点任务是要着手解决 m=a^p  mod n的函数问题------------*/
void RsaAlgorithm::expmod(int a[RSA_ALGORITHM_MAX], int p[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], int *m)
{
	int t[RSA_ALGORITHM_MAX], l[RSA_ALGORITHM_MAX], temp[RSA_ALGORITHM_MAX]; /*/t放入2，l放入1；*/
	int w[RSA_ALGORITHM_MAX], s[RSA_ALGORITHM_MAX], c[RSA_ALGORITHM_MAX], b[RSA_ALGORITHM_MAX], i;
	for (i = 0; i<RSA_ALGORITHM_MAX - 1; i++)
		b[i] = l[i] = t[i] = w[i] = 0;
	t[0] = 2; t[RSA_ALGORITHM_MAX - 1] = 1;
	l[0] = 1; l[RSA_ALGORITHM_MAX - 1] = 1;
	mov(l, temp);
	mov(a, m);
	mov(p, b);
	while (cmp(b, l) != 0)
	{
		for (i = 0; i<RSA_ALGORITHM_MAX; i++)
			w[i] = c[i] = 0;
		divt(b, t, w, c);/*// c=p mod 2  w= p /2*/
		mov(w, b);/*//p=p/2*/
		if (cmp(c, l) == 0) /*/余数c==1*/
		{
			for (i = 0; i<RSA_ALGORITHM_MAX; i++)
				w[i] = 0;
			mul(temp, m, w);
			mov(w, temp);
			for (i = 0; i<RSA_ALGORITHM_MAX; i++)
				w[i] = c[i] = 0;
			divt(temp, n, w, c);/* /c为余c=temp % n，w为商w=temp/n */
			mov(c, temp);
		}
		for (i = 0; i<RSA_ALGORITHM_MAX; i++)
			s[i] = 0;
		mul(m, m, s);//s=a*a
		for (i = 0; i<RSA_ALGORITHM_MAX; i++)
			c[i] = 0;
		divt(s, n, w, c);/*/w=s/n;c=s mod n*/
		mov(c, m);
	}
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)
		s[i] = 0;
	mul(m, temp, s);
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)
		c[i] = 0;
	divt(s, n, w, c);
	mov(c, m);/*余数s给m*/
	m[RSA_ALGORITHM_MAX - 2] = a[RSA_ALGORITHM_MAX - 2];/*为后面的汉字显示需要，用第99位做为标记*/
	return;/*/k=temp*k%n;*/
}

int RsaAlgorithm::is_prime_san(int p[RSA_ALGORITHM_MAX])
{
	int i, a[RSA_ALGORITHM_MAX], t[RSA_ALGORITHM_MAX], s[RSA_ALGORITHM_MAX], o[RSA_ALGORITHM_MAX];
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)
		s[i] = o[i] = a[i] = t[i] = 0;
	t[0] = 1;
	t[RSA_ALGORITHM_MAX - 1] = 1;
	a[0] = 2;// { 2,3,5,7 }
	a[RSA_ALGORITHM_MAX - 1] = 1;
	sub(p, t, s);
	expmod(a, s, p, o);
	if (cmp(o, t) != 0)
	{
		return 0;
	}
	a[0] = 3;
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)  o[i] = 0;
	expmod(a, s, p, o);
	if (cmp(o, t) != 0)
	{
		return 0;
	}
	a[0] = 5;
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)  o[i] = 0;
	expmod(a, s, p, o);
	if (cmp(o, t) != 0)
	{
		return 0;
	}
	a[0] = 7;
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)  o[i] = 0;
	expmod(a, s, p, o);
	if (cmp(o, t) != 0)
	{
		return 0;
	}
	return 1;
}

int RsaAlgorithm::coprime(int e[RSA_ALGORITHM_MAX], int s[RSA_ALGORITHM_MAX]) /* 判断两个大数之间是否互质*/
{
	int a[RSA_ALGORITHM_MAX], b[RSA_ALGORITHM_MAX], c[RSA_ALGORITHM_MAX], d[RSA_ALGORITHM_MAX], o[RSA_ALGORITHM_MAX], l[RSA_ALGORITHM_MAX];
	int i;
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)
		l[i] = o[i] = c[i] = d[i] = 0;
	o[0] = 0; o[RSA_ALGORITHM_MAX - 1] = 1;
	l[0] = 1; l[RSA_ALGORITHM_MAX - 1] = 1;
	mov(e, b);
	mov(s, a);
	do
	{
		if (cmp(b, l) == 0)
		{
			return 1;
		}
		for (i = 0; i<RSA_ALGORITHM_MAX; i++)
			c[i] = 0;
		divt(a, b, d, c);
		mov(b, a);/*b--->a*/
		mov(c, b);/*c--->b*/
	} while (cmp(c, o) != 0);
	/*	printf("Ihey are  not coprime!\n");*/
	return 0;
}

void RsaAlgorithm::prime_random(int *p, int *q)
{
	int i, k;
	time_t t;
	p[0] = 1;
	q[0] = 3;
	p[RSA_ALGORITHM_MAX - 1] = 10;
	q[RSA_ALGORITHM_MAX - 1] = 11;
	do
	{
		t = time(NULL);
		srand((unsigned long)t);
		for (i = 1; i<p[RSA_ALGORITHM_MAX - 1] - 1; i++)
		{
			k = rand() % 10;
			p[i] = k;
		}
		k = rand() % 10;
		while (k == 0)
		{
			k = rand() % 10;
		}
		p[p[RSA_ALGORITHM_MAX - 1] - 1] = k;
	} while ((is_prime_san(p)) != 1);
	do
	{
		t = time(NULL);
		srand((unsigned long)t);
		for (i = 1; i<q[RSA_ALGORITHM_MAX - 1]; i++)
		{
			k = rand() % 10;
			q[i] = k;
		}
	} while ((is_prime_san(q)) != 1);
	return;
}

void RsaAlgorithm::erand(int e[RSA_ALGORITHM_MAX], int m[RSA_ALGORITHM_MAX])
{
	int i, k;
	time_t t;
	e[RSA_ALGORITHM_MAX - 1] = 5;
	do
	{
		t = time(NULL);
		srand((unsigned long)t);
		for (i = 0; i<e[RSA_ALGORITHM_MAX - 1] - 1; i++)
		{
			k = rand() % 10;
			e[i] = k;
		}
		while ((k = rand() % 10) == 0)
			k = rand() % 10;
		e[e[RSA_ALGORITHM_MAX - 1] - 1] = k;
	} while (coprime(e, m) != 1);
	return;
}

void RsaAlgorithm::rsad(int e[RSA_ALGORITHM_MAX], int g[RSA_ALGORITHM_MAX], int *d)
{
	int r[RSA_ALGORITHM_MAX], n1[RSA_ALGORITHM_MAX], n2[RSA_ALGORITHM_MAX], k[RSA_ALGORITHM_MAX], w[RSA_ALGORITHM_MAX];
	int i, t[RSA_ALGORITHM_MAX], b1[RSA_ALGORITHM_MAX], b2[RSA_ALGORITHM_MAX], temp[RSA_ALGORITHM_MAX];
	mov(g, n1);
	mov(e, n2);
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)
		k[i] = w[i] = r[i] = temp[i] = b1[i] = b2[i] = t[i] = 0;
	b1[RSA_ALGORITHM_MAX - 1] = 0; b1[0] = 0;/*/b1=0;*/
	b2[RSA_ALGORITHM_MAX - 1] = 1; b2[0] = 1;/*/b2=1;*/
	while (1)
	{
		for (i = 0; i<RSA_ALGORITHM_MAX; i++)
			k[i] = w[i] = 0;
		divt(n1, n2, k, w);/*/k=n1/n2;*/
		for (i = 0; i<RSA_ALGORITHM_MAX; i++)
			temp[i] = 0;
		mul(k, n2, temp);/*/temp=k*n2;*/
		for (i = 0; i<RSA_ALGORITHM_MAX; i++)
			r[i] = 0;
		sub(n1, temp, r);
		if ((r[RSA_ALGORITHM_MAX - 1] == 1) && (r[0] == 0))/*/r=0*/
		{
			break;
		}
		else
		{
			mov(n2, n1);/*/n1=n2;*/
			mov(r, n2);/*/n2=r;*/
			mov(b2, t);/*/t=b2;*/
			for (i = 0; i<RSA_ALGORITHM_MAX; i++)
				temp[i] = 0;
			mul(k, b2, temp);/*/b2=b1-k*b2;*/
			for (i = 0; i<RSA_ALGORITHM_MAX; i++)
				b2[i] = 0;
			sub(b1, temp, b2);
			mov(t, b1);
		}
	}
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)
		t[i] = 0;
	add(b2, g, t);
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)
		temp[i] = d[i] = 0;
	divt(t, g, temp, d);
}

/*/求解密密钥d的函数(根据Euclid算法)96403770511368768000*/
unsigned long RsaAlgorithm::rsa(unsigned long p, unsigned long q, unsigned long e)  /*/求解密密钥d的函数(根据Euclid算法)*/
{
	unsigned long g, k, r, n1, n2, t;
	unsigned long b1 = 0, b2 = 1;
	g = (p - 1)*(q - 1);
	n1 = g;
	n2 = e;
	while (1)
	{
		k = n1 / n2;
		r = n1 - k*n2;
		if (r != 0)
		{
			n1 = n2;
			n2 = r;
			t = b2;
			b2 = b1 - k*b2;
			b1 = t;
		}
		else
		{
			break;
		}
	}
	return (g + b2) % g;
}

//! RSA的进行文件加密的函数
/*!
* @param[in] e，n为随机产生的公钥，利用公钥进行加密
* @param[in] text为明文，明文以char*的格式存储
* @return[string] 返回值为加密成功之后的密文，采用string类型进行存储
* @pre \e 进行加密的过程中进行了数据类型的转换
* @see e，d，n & text
*/
std::string RsaAlgorithm::tencrypto(int e[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], const char* text)/*//对有需要的文件进行加密*/
{
	int i, k, count, temp, c;
	char ch;
	struct RsaAlgorithm::slink *p, *p1, *p2;
	struct RsaAlgorithm::slink *h;
	h = p = p1 = p2 = (struct RsaAlgorithm::slink *)malloc(RSA_ALGORITHM_LEN);
	h = NULL;
	if (text == NULL)
	{
		return NULL;
	}
	count = 0;
	int j;
	for (j = 0; j < (int)strlen(text); j++)
	{
		ch = text[j];
		c = ch;
		k = 0;
		if (c<0)
		{
			c = abs(c);/*/把负数取正并且做一个标记*/
			p1->bignum[RSA_ALGORITHM_MAX - 2] = '0';
		}
		else
		{
			p1->bignum[RSA_ALGORITHM_MAX - 2] = '1';
		}

		while (c / 10 != 0)
		{
			temp = c % 10;
			c = c / 10;
			p1->bignum[k] = temp;
			k++;
		}
		p1->bignum[k] = c;
		p1->bignum[RSA_ALGORITHM_MAX - 1] = k + 1;
		count = count + 1;
		if (count == 1)
			h = p1;
		else p2->next = p1;
		p2 = p1;
		p1 = (struct RsaAlgorithm::slink *)malloc(RSA_ALGORITHM_LEN);
	}
	p2->next = NULL;

	std::string res;
	p = p1 = (struct RsaAlgorithm::slink *)malloc(RSA_ALGORITHM_LEN);
	p = h;
	if (h != NULL)
		do
		{
			expmod(p->bignum, e, n, p1->bignum);
			ch = p1->bignum[RSA_ALGORITHM_MAX - 2];
			res += ch;
			if ((p1->bignum[RSA_ALGORITHM_MAX - 1] / 10) == 0)/*/判断p1->bignum[99]的是否大于十；*/
			{
				ch = 0 + 48;
				res += ch;
				ch = p1->bignum[RSA_ALGORITHM_MAX - 1] + 48;
				res += ch;
			}
			else
			{
				ch = p1->bignum[RSA_ALGORITHM_MAX - 1] / 10 + 48;
				res += ch;
				ch = p1->bignum[RSA_ALGORITHM_MAX - 1] % 10 + 48;
				res += ch;
			}

			for (i = 0; i<p1->bignum[RSA_ALGORITHM_MAX - 1]; i++)
			{
				ch = p1->bignum[i] + 48;
				res += ch;
			}
			p = p->next;
			p1 = (struct RsaAlgorithm::slink *)malloc(RSA_ALGORITHM_LEN);
		} while (p != NULL);
		return res;
}

//! RSA的进行文件解密的函数
/*!
* @param[in] d,n为私钥，由函数RSAKey()产生
* @param[in] text为密文，对应加密函数，密文的格式为string
* @return[string] 解密之后的明文采用string进行存储
* @pre \e 进行解密的过程中进行了数据类型的转换
* @see e，d，n & text
*/
std::string RsaAlgorithm::tdecrypto(int d[RSA_ALGORITHM_MAX], int n[RSA_ALGORITHM_MAX], const std::string& text)
{
	struct RsaAlgorithm::slink *h, *p1, *p2;
	char ch;
	int i, j, k, c, count, temp;
	i = 0;
	j = 3;
	count = 0;
	h = p1 = p2 = (struct RsaAlgorithm::slink *)malloc(RSA_ALGORITHM_LEN);

	int kk;
	for (kk = 0; kk < (int)text.length(); kk++)
	{
		ch = text.at(kk);
		c = ch;
		if (j == 3)
		{
			p1->bignum[RSA_ALGORITHM_MAX - 2] = c;
			j--;
		}
		else if (j == 2)
		{
			temp = c - 48;
			j--;
		}
		else if (j == 1)
		{
			p1->bignum[RSA_ALGORITHM_MAX - 1] = temp * 10 + c - 48;
			j--;
		}
		else if (j == 0)
		{
			p1->bignum[i] = c - 48;
			i++;
			if (i == p1->bignum[RSA_ALGORITHM_MAX - 1])
			{
				i = 0;
				j = 3;
				count++;
				if (count == 1)
					h = p1;
				else p2->next = p1;
				p2 = p1;
				p1 = (struct RsaAlgorithm::slink *)malloc(RSA_ALGORITHM_LEN);
			}
		}
	}
	p2->next = NULL;

	p2 = (struct RsaAlgorithm::slink *)malloc(RSA_ALGORITHM_LEN);
	p1 = h;
	k = 0;
	std::string res;
	if (h != NULL)/*/temp为暂存ASIIC码的int值*/
		do
		{
			for (i = 0; i<RSA_ALGORITHM_MAX; i++)
				p2->bignum[i] = 0;
			expmod(p1->bignum, d, n, p2->bignum);
			temp = p2->bignum[0] + p2->bignum[1] * 10 + p2->bignum[2] * 100;
			if ((p2->bignum[RSA_ALGORITHM_MAX - 2]) == '0')
			{
				temp = 0 - temp;
			}/*/转化为正确的ASIIC码，如-78-96形成汉字	*/
			ch = temp;/*  str[k]--->ch */
			res += ch;
			k++;
			p1 = p1->next;
			p2 = (struct RsaAlgorithm::slink *)malloc(RSA_ALGORITHM_LEN);
		} while (p1 != NULL);
		return res;
}
//
//
/*
* 产生密钥，公钥和私钥
* @param[in] 随机产生大数密钥
* @param[in] 利用自定义的大数运算规则进行计算
* @return 返回产生成功与否
* – false 表示产生密钥失败
* @pre \e 产生的密钥存储在定义的类中变量中
* @see e，d，n
*/
bool RsaAlgorithm::RSAKey()
{
	for (i = 0; i<RSA_ALGORITHM_MAX; i++)
		m[i] = p[i] = q[i] = n[i] = d[i] = e[i] = 0;/*/简单初始化一下*/
	prime_random(p, q);/*/随机产生两个大素数*/
	mul(p, q, n);
	mov(p, p1);
	p1[0]--;
	mov(q, q1);
	q1[0]--;      /*/q-1;*/
	mul(p1, q1, m);//m=(p-1)*(q-1)
	erand(e, m);
	rsad(e, m, d);
	return true;
}