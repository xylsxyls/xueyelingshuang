#include "1.h"
#include <thread>
using namespace std;

B::B(){
	
	std::thread thread2([](){
		int i = 10000;
		while (i-- != 0){
			LOGINFO("1!!!1");
		}
	});
	thread2.detach();
}

//B b;