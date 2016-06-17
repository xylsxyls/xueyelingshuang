#include "ace/SOCK_Acceptor.h"  
#include "ace/SOCK_Stream.h"  
#include "ace/INET_Addr.h"  
#include "ace/Time_Value.h"  

#include <string>  
#include <iostream>  
using namespace std;  

int main(int argc, char *argv[])   
{  
	ACE_INET_Addr port_to_listen(3000);        //绑定的端口  
	ACE_SOCK_Acceptor acceptor;  
	if (acceptor.open (port_to_listen, 1) == -1)     //绑定端口  
	{  
		cout<<endl<<"bind port fail"<<endl;  
		return -1;  
	}  

	while(true)  
	{  
		ACE_SOCK_Stream peer;        //和客户端的数据通路  
		ACE_Time_Value timeout (10, 0);  

		if (acceptor.accept (peer) != -1)    //建立和客户端的连接  
		{  
			cout<<endl<<endl<<"client connect. "<<endl;  
			char buffer[1024];  
			ssize_t bytes_received;  

			ACE_INET_Addr raddr;  
			peer.get_local_addr(raddr);  
			cout<<endl<<"local port\t"<<raddr.get_host_name()<<"\t"<<raddr.get_port_number()<<endl;  

			while ((bytes_received = peer.recv (buffer, sizeof(buffer))) != -1)    //读取客户端发送的数据  
			{  
				//if(bytes_received == 0) break;
				peer.send(buffer, bytes_received);    //对客户端发数据  
			}  
			peer.close ();  
		}  
	}  

	return 0;   
}