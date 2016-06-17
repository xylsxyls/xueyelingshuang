#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Connector.h"
#include "ace/INET_Addr.h"
#include "ace/Time_Value.h"   

#include <string>  
#include <iostream>  
using namespace std;  

int main(int argc, char *argv[])   
{  
	ACE_INET_Addr addr(3000,"127.0.0.1");  

	ACE_SOCK_Connector connector;      
	ACE_Time_Value timeout(10,0);  
	ACE_SOCK_Stream peer;  

	if(connector.connect(peer,addr,&timeout) != 0)  
	{  
		cout<<"connection failed !"<<endl;  
		return 1;  
	}  
	cout<<"conneced !"<<endl;  

	string s="hello world";  
	peer.send(s.c_str(),s.length());    //发送数据  
	peer.send(s.c_str(),s.length());    //发送数据  
	cout<<endl<<"send:\t"<<s<<endl;  

	ssize_t bc=0;            //接收的字节数  

	char buf[1024]; 
	bc=peer.recv(buf,1024,&timeout);    //接收数据  
	if(bc>=0)  
	{  
		buf[bc]='\0';  
		cout<<endl<<"recv:\t"<<buf<<endl;  
	} 

	peer.send(s.c_str(),s.length());    //发送数据 

	bc=peer.recv(buf,1024,&timeout);    //接收数据  
	if(bc>=0)  
	{  
		buf[bc]='\0';  
		cout<<endl<<"recv:\t"<<buf<<endl;  
	}  
	peer.close();  

	getchar();  

	return 0;   
}