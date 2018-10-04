#include "CKeyboard.h"
#include "DmSoft/DmSoftAPI.h"
#include "CRandom/CRandomAPI.h"

enum
{
	KeyPressTimeMin = 50,
	KeyPressTimeMax = 100
};

bool CKeyboard::KeyDown(int32_t code)
{
	return DmSoft::KeyDown(code) == 1;
}

bool CKeyboard::KeyUp(int32_t code)
{
	return DmSoft::KeyUp(code) == 1;
}

bool CKeyboard::KeyPress(int32_t code, int32_t sleepTime)
{
	bool result = true;
	result = result && (DmSoft::KeyDown(code) == 1);
    if (sleepTime == -1)
    {
        sleepTime = CRandom::Int(KeyPressTimeMin, KeyPressTimeMax);
    }
	Sleep(sleepTime);
	result = result && (DmSoft::KeyUp(code) == 1);
	return result;
}

bool CKeyboard::InputString(const string& str, int32_t sleepTime)
{
	int32_t times = str.length() * 2 - 1;
	int32_t sleepTimeTemp = sleepTime;
	bool result = true;
	int32_t i = -1;
	while (i++ != str.length() - 1)
    {
		vector<int32_t> vecCode = GetCode(str[i]);
		if (sleepTime == -1) sleepTimeTemp = CRandom::Int(KeyPressTimeMin, KeyPressTimeMax);
		result = result && KeyManyPress(vecCode, sleepTimeTemp);
	}
	return result;
}

bool CKeyboard::KeyManyPress(const vector<int32_t>& vecCode, int32_t sleepTime)
{
    if (vecCode.size() == 0)
    {
        return true;
    }
    if (sleepTime == -1)
    {
        sleepTime = CRandom::Int(KeyPressTimeMin, KeyPressTimeMax);
    }
	vector<int32_t> vecSleepTime = CRandom::Hash(sleepTime, vecCode.size() * 2 - 1);
	bool result = true;
	int32_t i = -1;
	while (i++ != vecCode.size() - 1)
    {
		result = result && KeyDown(vecCode.at(i));
		Sleep(vecSleepTime.at(i));
	}
	int32_t j = i;
	i = -1;
	while (i++ != vecCode.size() - 2)
    {
		result = result && KeyUp(vecCode.at(i));
		Sleep(vecSleepTime.at(j++));
	}
	result = result && KeyUp(vecCode.at(i));
	return result;
}

vector<int32_t> CKeyboard::GetCode(char c)
{
	vector<int32_t> vecResult;
	if (c == '`')
    {
		vecResult.push_back(VK_OEM_3);
	}
	else if (c == '~')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back(VK_OEM_3);
	}
	else if (c == '1')
    {
		vecResult.push_back('1');
	}
	else if (c == '!')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('1');
	}
	else if (c == '2')
    {
		vecResult.push_back('2');
	}
	else if (c == '@')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('2');
	}
	else if (c == '3')
    {
		vecResult.push_back('3');
	}
	else if (c == '#')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('3');
	}
	else if (c == '4')
    {
		vecResult.push_back('4');
	}
	else if (c == '$')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('4');
	}
	else if (c == '5')
    {
		vecResult.push_back('5');
	}
	else if (c == '%')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('5');
	}
	else if (c == '6')
    {
		vecResult.push_back('6');
	}
	else if (c == '^')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('6');
	}
	else if (c == '7')
    {
		vecResult.push_back('7');
	}
	else if (c == '&')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('7');
	}
	else if (c == '8')
    {
		vecResult.push_back('8');
	}
	else if (c == '*')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('8');
	}
	else if (c == '9')
    {
		vecResult.push_back('9');
	}
	else if (c == '(')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('9');
	}
	else if (c == '0')
    {
		vecResult.push_back('0');
	}
	else if (c == ')')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('0');
	}
	else if (c == '-')
    {
		vecResult.push_back(VK_OEM_MINUS);
	}
	else if (c == '_')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back(VK_OEM_MINUS);
	}
	else if (c == '=')
    {
		vecResult.push_back(VK_OEM_PLUS);
	}
	else if (c == '+')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back(VK_OEM_PLUS);
	}
	else if (c == '\\')
    {
		vecResult.push_back(VK_OEM_5);
	}
	else if (c == '|')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back(VK_OEM_5);
	}
	else if (c == '\t')
    {
		vecResult.push_back('\t');
	}
	else if (c == 'Q')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('Q');
	}
	else if (c == 'q')
    {
		vecResult.push_back('Q');
	}
	else if (c == 'W')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('W');
	}
	else if (c == 'w')
    {
		vecResult.push_back('W');
	}
	else if (c == 'E')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('E');
	}
	else if (c == 'e')
    {
		vecResult.push_back('E');
	}
	else if (c == 'R')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('R');
	}
	else if (c == 'r')
    {
		vecResult.push_back('R');
	}
	else if (c == 'T')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('T');
	}
	else if (c == 't')
    {
		vecResult.push_back('T');
	}
	else if (c == 'Y')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('Y');
	}
	else if (c == 'y')
    {
		vecResult.push_back('Y');
	}
	else if (c == 'U')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('U');
	}
	else if (c == 'u')
    {
		vecResult.push_back('U');
	}
	else if (c == 'I')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('I');
	}
	else if (c == 'i')
    {
		vecResult.push_back('I');
	}
	else if (c == 'O')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('O');
	}
	else if (c == 'o')
    {
		vecResult.push_back('O');
	}
	else if (c == 'P')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('P');
	}
	else if (c == 'p')
    {
		vecResult.push_back('P');
	}
	else if (c == 'A')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('A');
	}
	else if (c == 'a')
    {
		vecResult.push_back('A');
	}
	else if (c == 'S')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('S');
	}
	else if (c == 's')
    {
		vecResult.push_back('S');
	}
	else if (c == 'D')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('D');
	}
	else if (c == 'd')
    {
		vecResult.push_back('D');
	}
	else if (c == 'F')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('F');
	}
	else if (c == 'f')
    {
		vecResult.push_back('F');
	}
	else if (c == 'G')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('G');
	}
	else if (c == 'g')
    {
		vecResult.push_back('G');
	}
	else if (c == 'H')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('H');
	}
	else if (c == 'h')
    {
		vecResult.push_back('H');
	}
	else if (c == 'J')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('J');
	}
	else if (c == 'j')
    {
		vecResult.push_back('J');
	}
	else if (c == 'K')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('K');
	}
	else if (c == 'k')
    {
		vecResult.push_back('K');
	}
	else if (c == 'L')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('L');
	}
	else if (c == 'l')
    {
		vecResult.push_back('L');
	}
	else if (c == 'Z')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('Z');
	}
	else if (c == 'z')
    {
		vecResult.push_back('Z');
	}
	else if (c == 'X')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('X');
	}
	else if (c == 'x')
    {
		vecResult.push_back('X');
	}
	else if (c == 'C')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('C');
	}
	else if (c == 'c')
    {
		vecResult.push_back('C');
	}
	else if (c == 'V')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('V');
	}
	else if (c == 'v')
    {
		vecResult.push_back('V');
	}
	else if (c == 'B')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('B');
	}
	else if (c == 'b')
    {
		vecResult.push_back('B');
	}
	else if (c == 'N')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('N');
	}
	else if (c == 'n')
    {
		vecResult.push_back('N');
	}
	else if (c == 'M')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back('M');
	}
	else if (c == 'm')
    {
		vecResult.push_back('M');
	}
	else if (c == '\t')
    {
		vecResult.push_back('\t');
	}
	else if (c == '\n')
    {
		vecResult.push_back(VK_RETURN);
	}
	else if (c == ' ')
    {
		vecResult.push_back(' ');
	}
	else if (c == '{')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back(VK_OEM_4);
	}
	else if (c == '[')
    {
		vecResult.push_back(VK_OEM_4);
	}
	else if (c == '}')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back(VK_OEM_6);
	}
	else if (c == ']')
    {
		vecResult.push_back(VK_OEM_6);
	}
	else if (c == ':')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back(VK_OEM_1);
	}
	else if (c == ';')
    {
		vecResult.push_back(VK_OEM_1);
	}
	else if (c == '\"')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back(VK_OEM_7);
	}
	else if (c == '\'')
    {
		vecResult.push_back(VK_OEM_7);
	}
	else if (c == '<')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back(VK_OEM_COMMA);
	}
	else if (c == ',')
    {
		vecResult.push_back(VK_OEM_COMMA);
	}
	else if (c == '>')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back(VK_OEM_PERIOD);
	}
	else if (c == '.')
    {
		vecResult.push_back(VK_OEM_PERIOD);
	}
	else if (c == '?')
    {
		vecResult.push_back(VK_SHIFT);
		vecResult.push_back(VK_OEM_2);
	}
	else if (c == '/')
    {
		vecResult.push_back(VK_OEM_2);
	}
	return vecResult;
}

/*
int32_t main(){
	Sleep(2000);
	std::vector<int32_t> vec;
	//vec.push_back(CKeyboard::Ctrl);
	//vec.push_back(CKeyboard::Alt);
	//vec.push_back(CKeyboard::Delete);
	//vec.push_back(CKeyboard::Delete);
	//vec.push_back('C');
	CKeyboard::KeyDown(CKeyboard::Ctrl);
	Sleep(100);
	CKeyboard::KeyDown(CKeyboard::Alt);
	Sleep(100);
	CKeyboard::KeyDown(CKeyboard::Delete);
	Sleep(100);
	printf("2");
	getchar();
	CKeyboard::KeyUp(CKeyboard::Ctrl);
	Sleep(100);
	CKeyboard::KeyUp(CKeyboard::Alt);
	Sleep(100);
	CKeyboard::KeyUp(CKeyboard::Delete);

	//bool x = CKeyboard::KeyManyPress(vec);
	printf("1");
	getchar();
	//CKeyboard::KeyPress(CKeyboard::CapsLock);
	return 0;
}
*/