#include "EcologicalBalance.h"
#include <stdint.h>
#include <stdio.h>
#include <random>
#include <time.h>
#include <vector>
#include <iostream>

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close EcologicalBalance");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#endif

const int32_t g_beginScore = 0;
int32_t g_uuid = 0;

enum
{
	CANDID = 1,
	HAGGLE,
	LIAR
};

struct Person
{
	int32_t m_type;
	int32_t m_score;
	int32_t m_uuid;
	Person()
	{
		m_score = g_beginScore;
		m_type = 0;
		m_uuid = (++g_uuid);
	}
};

struct Candid : public Person
{
	Candid()
	{
		m_type = CANDID;
	}
};

struct Haggle : public Person
{
	Haggle()
	{
		m_type = HAGGLE;
	}
	std::vector<int32_t> m_vecLiar;
};

struct Liar : public Person
{
	Liar()
	{
		m_type = LIAR;
	}
};

void Cooperation(Person* person1, Person* person2)
{
	if (person1 == nullptr || person2 == nullptr)
	{
		printf("person nullptr\n");
		return;
	}
	if (person1->m_type == CANDID && person2->m_type == CANDID)
	{
		person1->m_score += 3;
		person2->m_score += 3;
	}
	else if (person1->m_type == CANDID && person2->m_type == HAGGLE)
	{
		person1->m_score += 3;
		person2->m_score += 3;
	}
	else if (person1->m_type == CANDID && person2->m_type == LIAR)
	{
		person1->m_score += (-2);
		person2->m_score += 5;
	}
	else if (person1->m_type == HAGGLE && person2->m_type == CANDID)
	{
		person1->m_score += 3;
		person2->m_score += 3;
	}
	else if (person1->m_type == HAGGLE && person2->m_type == HAGGLE)
	{
		person1->m_score += 2;
		person2->m_score += 2;
	}
	else if (person1->m_type == HAGGLE && person2->m_type == LIAR)
	{
		auto& vecLiar = ((Haggle*)person1)->m_vecLiar;
		auto it = std::find(vecLiar.begin(), vecLiar.end(), person2->m_uuid);
		if (it != vecLiar.end())
		{
			return;
		}
		person1->m_score += 0;
		person2->m_score += 3;
		vecLiar.push_back(person2->m_uuid);
	}
	else if (person1->m_type == LIAR && person2->m_type == CANDID)
	{
		person1->m_score += 5;
		person2->m_score += (-2);
	}
	else if (person1->m_type == LIAR && person2->m_type == HAGGLE)
	{
		auto& vecLiar = ((Haggle*)person2)->m_vecLiar;
		auto it = std::find(vecLiar.begin(), vecLiar.end(), person1->m_uuid);
		if (it != vecLiar.end())
		{
			return;
		}
		person1->m_score += 3;
		person2->m_score += 0;
		vecLiar.push_back(person1->m_uuid);
	}
	else if (person1->m_type == LIAR && person2->m_type == LIAR)
	{
		person1->m_score += 0;
		person2->m_score += 0;
	}
	else
	{
		printf("type error\n");
		return;
	}
}

std::string InputString(const std::string& tip)
{
	printf("%s", tip.c_str());
	std::string result;
	char ch = 0;
	while ((ch = std::cin.get()) != '\n')
	{
		result += ch;
	}
	return result;
}

int32_t main()
{
	while (true)
	{
		int32_t candidCount = atoi(InputString("candid = ").c_str());
		int32_t haggleCount = atoi(InputString("haggle = ").c_str());
		int32_t liarCount = atoi(InputString("liar = ").c_str());
		double opportunity = atof(InputString("opportunity = ").c_str());
		int32_t liarMoreTimes = atoi(InputString("liarMoreTimes = ").c_str());

		std::default_random_engine random((unsigned int)time(nullptr));
		std::vector<Person*> vecPerson;
		std::uniform_int_distribution<int> rand(1, candidCount + haggleCount + liarCount - 1);

		int32_t opportunityCount = (int32_t)((candidCount + haggleCount + liarCount) * opportunity);

		int32_t index = -1;
		while (index++ != candidCount - 1)
		{
			vecPerson.push_back(new Candid);
		}

		index = -1;
		while (index++ != haggleCount - 1)
		{
			vecPerson.push_back(new Haggle);
		}

		index = -1;
		while (index++ != liarCount - 1)
		{
			vecPerson.push_back(new Liar);
		}

		int32_t generationTimes = 1000;
		int32_t generationIndex = -1;
		while (generationIndex++ != generationTimes - 1)
		{
			int32_t opportunityIndex = -1;
			while (opportunityIndex++ != opportunityCount - 1)
			{
				index = -1;
				while (index++ != candidCount + haggleCount + liarCount - 1)
				{
					int randNum = rand(random);
					if (randNum >= vecPerson[index]->m_uuid)
					{
						++randNum;
					}
					Cooperation(vecPerson[index], vecPerson[randNum - 1]);
					int32_t liarMoreTimesIndex = -1;
					while (liarMoreTimesIndex++ != liarMoreTimes - 1)
					{
						if (vecPerson[index]->m_type == LIAR)
						{
							randNum = rand(random);
							if (randNum >= vecPerson[index]->m_uuid)
							{
								++randNum;
							}
							Cooperation(vecPerson[index], vecPerson[randNum - 1]);
						}
					}
				}
			}
			index = -1;
			while (index++ != candidCount + haggleCount + liarCount - 1)
			{
				if (vecPerson[index]->m_type == HAGGLE)
				{
					((Haggle*)vecPerson[index])->m_vecLiar.clear();
				}
			}
		}

		double candidScore = 0;
		double haggleScore = 0;
		double liarScore = 0;
		int32_t candidNum = 0;
		int32_t haggleNum = 0;
		int32_t liarNum = 0;

		index = -1;
		while (index++ != vecPerson.size() - 1)
		{
			std::string name;
			if (vecPerson[index]->m_type == CANDID)
			{
				++candidNum;
				candidScore = candidScore * ((candidNum - 1) / (double)candidNum) + vecPerson[index]->m_score / (double)candidNum;
			}
			else if (vecPerson[index]->m_type == HAGGLE)
			{
				++haggleNum;
				haggleScore = haggleScore * ((haggleNum - 1) / (double)haggleNum) + vecPerson[index]->m_score / (double)haggleNum;
			}
			else if (vecPerson[index]->m_type == LIAR)
			{
				++liarNum;
				liarScore = liarScore * ((liarNum - 1) / (double)liarNum) + vecPerson[index]->m_score / (double)liarNum;
			}
		}
		printf("candid = %d\n", (int32_t)candidScore);
		printf("haggle = %d\n", (int32_t)haggleScore);
		printf("liar = %d\n", (int32_t)liarScore);
		getchar();
	}
	return 0;
}