
#include <map>
#include "DynBase.h"

static std::map< string,ClassInfo*> *classInfoMap = NULL;
using namespace std;
IMPLEMENT_CLASS(Object)
	bool Object::Register(ClassInfo* ci)
{
	if(!classInfoMap)   {
		classInfoMap = new std::map< string,ClassInfo*>();
	}
	if(ci)  {
		if(classInfoMap->find(ci->m_className) == classInfoMap->end()){
			classInfoMap->insert(std::map< string,ClassInfo*>::value_type(ci->m_className,ci));
		}
	}
	return true;
}
Object* Object::CreateObject(std::string name)
{
	std::map< string,ClassInfo*>::const_iterator iter = classInfoMap->find(name);
	if(classInfoMap->end() != iter)  {
		return iter->second->CreateObject();
	}   
	return NULL;
}

bool Register(ClassInfo* ci)
{
	return Object::Register(ci);
}