#include "CBigNumber.h"
#include <cstring>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

CBigNumber::CBigNumber(){
	memset(number, 0, BigNumberLength);
}

CBigNumber::CBigNumber(int InitNumber){
	memset(number, 0, BigNumberLength);
	*this = InitNumber;
}

CBigNumber::CBigNumber(char* strInitNumber){
	memset(number, 0, BigNumberLength);
	*this = strInitNumber;
}

CBigNumber CBigNumber::operator = (int InitNumber){
	char temp[15] = {};
	sprintf(temp, "%d", InitNumber);
	UnnecessaryToX(temp, 0);
	*this = temp;
	return *this;
}

CBigNumber CBigNumber::operator = (char *strInitNumber){
	strcpy_x(number, strInitNumber, 0);
	_strrev(number);
	return *this;
}

CBigNumber operator + (CBigNumber x, CBigNumber y){
	CBigNumber z;
	if (x == -1 || y == -1) return -1;
	int temp = 0;
	int i = -1;
	while (1){
		i++;
		if (x.number[i] == 0 && y.number[i] == 0){
			if (z.number[i] == 1) z.number[i] = 49;
			break;
		}
		z.number[i] = z.number[i] + (x.number[i] == 0 ? x.number[i] : x.number[i] - 48) + (y.number[i] == 0 ? y.number[i] : y.number[i] - 48) + 48;
		if (z.number[i] > 57){
			z.number[i] = z.number[i] - 10;
			z.number[i + 1]++;
		}
	}
	return z;
}

CBigNumber operator - (CBigNumber x, CBigNumber y){
	CBigNumber z;
	if (x == -1 || y == -1) return z = -1;
	if (strlen(x.number) < strlen(y.number)) return z = "-1";
	int temp = 0;
	int i = -1;
	while (1){
		i++;
		if (x.number[i] == 0 && y.number[i] == 0){
			int n = strlen(z.number);
			while (n-- != 0){
				if (n == 0) return z;
				if (z.number[n] == 48) z.number[n] = 0;
				else return z;
			}
		}
		if (x.number[i] >= y.number[i]){
			z.number[i] = z.number[i] + (x.number[i] == 0 ? x.number[i] : x.number[i] - 48) - (y.number[i] == 0 ? y.number[i] : y.number[i] - 48) + 48;
		}
		else{
			z.number[i] = z.number[i] + 10 + (x.number[i] == 0 ? x.number[i] : x.number[i] - 48) - (y.number[i] == 0 ? y.number[i] : y.number[i] - 48) + 48;
			x.number[i + 1]--;
			int n = i;
			while (x.number[n + 1] == 47){
				x.number[n + 1] = 57;
				x.number[n + 2]--;
				n++;
			}
			if (x.number[i + 1] == -1) return z = "-1";
		}
	}
}

CBigNumber operator * (CBigNumber x, CBigNumber y){
	CBigNumber z;
	if (x == -1 || y == -1) return -1;
	int i = 0, j = 0;
	int xLength = strlen(x.number);
	int yLength = strlen(y.number);
	for (i = 0; i < xLength; i++){
		for (j = 0; j < yLength; j++){
			z.number[i + j] = (z.number[i + j] == 0 ? z.number[i + j] : z.number[i + j] - 48) + (x.number[i] - 48) * (y.number[j] - 48);
			if (z.number[i + j] / 10 != 0){
				z.number[i + j + 1] = (z.number[i + j + 1] == 0 ? z.number[i + j + 1] : z.number[i + j + 1] - 48) + z.number[i + j] / 10 + 48;
			}
			z.number[i + j] = z.number[i + j] % 10 + 48;
		}
	}
	i = strlen(z.number);
	while (i-- != 1){
		if (z.number[i] == 48) z.number[i] = 0;
		else break;
	}
	return z;
}

CBigNumber operator ^ (CBigNumber x, CBigNumber y){
	CBigNumber z;
	if (x == -1 || y == -1) return -1;
	CBigNumber temp;
	temp = "1";
	z = "1";
	while (1){
		if (y == "0") break;
		y = y - temp;
		z = z * x;
	}
	return z;
}

void CBigNumber::HighNumberInto(CBigNumber *x, CBigNumber *y){
	int yLengthTemp = strlen((*y).number);
	while (yLengthTemp-- != 0) (*y).number[yLengthTemp + 1] = (*y).number[yLengthTemp];
	(*y).number[0] = (*x).number[strlen((*x).number) - 1];
	(*x).number[strlen((*x).number) - 1] = 0;
}

CBigNumber operator / (CBigNumber x, CBigNumber y){
	CBigNumber z;
	if (x == -1 || y == -1) return -1;
	int xLength = strlen(x.number);
	int yLength = strlen(y.number);
	if (y == "0") return z = "-1";
	if ((x - y) == "-1") return z = "0";
	CBigNumber TempDividend;
	while (1){
		x.HighNumberInto(&x, &TempDividend);
		if (strlen(TempDividend.number) == yLength) break;
	}
	CBigNumber ResultNumber;
	int ResultTemp = 0;
	while (1){
		ResultNumber = "1";
		while (1){
			if ((TempDividend - y * ResultNumber) == "-1") break;
			ResultNumber.number[0]++;
		}
		ResultNumber.number[0]--;
		z.number[ResultTemp++] = ResultNumber.number[0];
		if (z.number[0] == 48){
			z.number[0] = 0;
			ResultTemp--;
		}
		TempDividend = TempDividend - y * ResultNumber;
		if (strlen(x.number) == 0) break;
		x.HighNumberInto(&x, &TempDividend);
		if (TempDividend.number[1] == 48 && strlen(TempDividend.number) == 2) TempDividend.number[1] = 0;
	}
	_strrev(z.number);
	return z;
}

CBigNumber operator % (CBigNumber x, CBigNumber y){
	CBigNumber z;
	if (x == -1 || y == -1) return -1;
	if (y == 0) return -1;
	z = x - x / y * y;
	return z;
}

CBigNumber operator + (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp + y;
}

CBigNumber operator - (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp - y;
}

CBigNumber operator * (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp * y;
}

CBigNumber operator ^ (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp ^ y;
}

CBigNumber operator / (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp / y;
}

CBigNumber operator % (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp % y;
}

CBigNumber operator + (CBigNumber x, int y){
	CBigNumber temp = y;
	return x + temp;
}

CBigNumber operator - (CBigNumber x, int y){
	CBigNumber temp = y;
	return x - temp;
}

CBigNumber operator * (CBigNumber x, int y){
	CBigNumber temp = y;
	return x * temp;
}

CBigNumber operator ^ (CBigNumber x, int y){
	CBigNumber temp = y;
	return x ^ temp;
}

CBigNumber operator / (CBigNumber x, int y){
	CBigNumber temp = y;
	return x / temp;
}

CBigNumber operator % (CBigNumber x, int y){
	CBigNumber temp = y;
	return x % temp;
}

CBigNumber operator + (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp + y;
}

CBigNumber operator - (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp - y;
}

CBigNumber operator * (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp * y;
}

CBigNumber operator ^ (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp ^ y;
}

CBigNumber operator / (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp / y;
}

CBigNumber operator % (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp % y;
}

CBigNumber operator + (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x + temp;
}

CBigNumber operator - (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x - temp;
}

CBigNumber operator * (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x * temp;
}

CBigNumber operator ^ (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x ^ temp;
}

CBigNumber operator / (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x / temp;
}

CBigNumber operator % (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x % temp;
}

MultiBool operator == (CBigNumber x, CBigNumber y){
	int i = 0, result = 1;
	while (1){
		if (x.number[i] == 0 && y.number[i] == 0) break;
		result = result && (x.number[i] == y.number[i]);
		if (result == 0) break;
		i++;
	}
	return result;
}

MultiBool operator != (CBigNumber x, CBigNumber y){
	if (x == -1 || y == -1) return -1;
	return !(x == y);
}

MultiBool operator > (CBigNumber x, CBigNumber y){
	if (x == -1 || y == -1) return -1;
	return ((y - x) == -1);
}

MultiBool operator >= (CBigNumber x, CBigNumber y){
	if (x == -1 || y == -1) return -1;
	return (!((x - y) == -1));
}

MultiBool operator < (CBigNumber x, CBigNumber y){
	if (x == -1 || y == -1) return -1;
	return ((x - y) == -1);
}

MultiBool operator <= (CBigNumber x, CBigNumber y){
	if (x == -1 || y == -1) return -1;
	return (!((y - x) == -1));
}

MultiBool operator == (CBigNumber x, int y){
	CBigNumber temp = y;
	return x == temp;
}

MultiBool operator != (CBigNumber x, int y){
	CBigNumber temp = y;
	return x != temp;
}

MultiBool operator > (CBigNumber x, int y){
	CBigNumber temp = y;
	return x > temp;
}

MultiBool operator >= (CBigNumber x, int y){
	CBigNumber temp = y;
	return x >= temp;
}

MultiBool operator < (CBigNumber x, int y){
	CBigNumber temp = y;
	return x < temp;
}

MultiBool operator <= (CBigNumber x, int y){
	CBigNumber temp = y;
	return x <= temp;
}

MultiBool operator == (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp == y;
}

MultiBool operator != (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp != y;
}

MultiBool operator > (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp > y;
}

MultiBool operator >= (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp >= y;
}

MultiBool operator < (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp < y;
}

MultiBool operator <= (int x, CBigNumber y){
	CBigNumber temp = x;
	return temp <= y;
}

MultiBool operator == (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x == temp;
}

MultiBool operator != (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x != temp;
}

MultiBool operator > (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x > temp;
}

MultiBool operator >= (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x >= temp;
}

MultiBool operator < (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x < temp;
}

MultiBool operator <= (CBigNumber x, char* y){
	CBigNumber temp = y;
	return x <= temp;
}

MultiBool operator == (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp == y;
}

MultiBool operator != (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp != y;
}

MultiBool operator > (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp > y;
}

MultiBool operator >= (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp >= y;
}

MultiBool operator < (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp < y;
}

MultiBool operator <= (char* x, CBigNumber y){
	CBigNumber temp = x;
	return temp <= y;
}

CBigNumber CBigNumber::operator ++ (){
	*this = *this + 1;
	return *this;
}

CBigNumber CBigNumber::operator -- (){
	*this = *this - 1;
	return *this;
}

CBigNumber CBigNumber::operator ++ (int){
	CBigNumber temp = *this;
	*this = *this + 1;
	return temp;
}

CBigNumber CBigNumber::operator -- (int){
	CBigNumber temp = *this;
	*this = *this - 1;
	if (temp == 0) return -1;
	return temp;
}

int CBigNumber::InvertOut(char *OutString){
	if (OutString != NULL){
		memset(OutString, 0, strlen(OutString));
		strcpy_x(OutString, number, 0);
	}
	if (*this <= 2147483647) return atoi(number);
	else return -1;
}

int CBigNumber::Sequence(char *OutString){
	if (OutString != NULL){
		memset(OutString, 0, strlen(OutString));
		strcpy_x(OutString, number, 0);
		_strrev(OutString);
	}
	if (*this == -1) return -1;
	if (*this <= 2147483647){
		int length = strlen(number);
		int Result = 0;
		while (length-- != 0) Result = Result + (number[length] - 48) * Power(10, length);
		return Result;
	}
	else return -1;
}

int Power(int BaseNumber, int power){
	int BackupBaseNumber = BaseNumber;
	if (power == 0) return 1;
	else if (BaseNumber == 0 && power != 0) return 0;
	int i = 0;
	for (i = 1; i < power; i++){
		BaseNumber = BaseNumber * BackupBaseNumber;
	}
	return BaseNumber;
}

void UnnecessaryToXA(char *arr, int size, int number){
	int length = strlen(arr);
	while (size-- != length + 1) *(arr + size) = number;
}

int main()
{
	CBigNumber s = "1234567896513846";
	char* ss = new char[5000000];
	ss[5000000 - 1] = 0;
	(s ^ 30).Sequence(ss);
	printf("%s", ss);
	getchar();
	return 0;
}