// Str.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <iostream>
#include "cstring.h"

using namespace std;

int main(int argc, char* argv[])
{

	CString s,s1,s2;
	s="s1";
	s1=s;
	s1="1234";
	s2=s+s1+CString("abc");
	cout << s2 <<endl;

}

