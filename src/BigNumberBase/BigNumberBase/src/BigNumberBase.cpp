#include "BigNumberBase.h"
#include "gmp.h"
#pragma comment(lib, "libgmp.a")
#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")
#pragma comment(lib, "libmsvcrt.a")

BigNumberBase::BigNumberBase()
{

}

BigNumberBase::BigNumberBase(int32_t num)
{

}

BigNumberBase::BigNumberBase(const char* num)
{

}

int main()
{
	mpz_t a, b, c, d;
	mpz_init(a);
	mpz_init(b);
	mpz_init(c);
	mpz_init(d);
	//计算2的1000次方
	mpz_init_set_ui(a, 2);
	mpz_pow_ui(c, a, 10);
	gmp_printf("c = %Zd\n", c);

	//计算12345678900987654321*98765432100123456789
	mpz_init_set_str(b, "12345678900987654321.21", 10);//10进制 
	mpz_init_set_str(c, "98765432100123456789.12", 10);
	mpz_mul(d, b, c);
	gmp_printf("d = %Zd\n", d);
	mpz_clear(a);
	mpz_clear(b);
	mpz_clear(c);
	mpz_clear(d);
	return 0;
}