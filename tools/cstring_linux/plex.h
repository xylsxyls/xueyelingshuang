/*
  class:	CPlex
  Note:		linux/windos下一个固定长度内存分配的实现
  		文件主要参考了MFC中的CPlex的实现，
  		移植到linux下
*/

#include <assert.h>
#include <stdio.h>

#ifndef __CPLEX_H__
#define __CPLEX_H__

struct CPlex     // warning variable length structure
{
    CPlex* pNext;
    // byte data[maxNum*elementSize];

    void* data() { return this+1; }

    // like 'calloc' but no zero fill
    // may throw memory exceptions
    static CPlex* Create(CPlex*& pHead, unsigned int nMax, unsigned int cbElement)
    {
        assert(nMax > 0 && cbElement > 0);
        CPlex* p = (CPlex*) new unsigned char[sizeof(CPlex) + nMax * cbElement];
                // may throw exception
        p->pNext = pHead;
        pHead = p;   // change head (adds in reverse order for simplicity)
        return p;
    }    

    void FreeDataChain()        // free this one and links
    {
        CPlex* p = this;
        while (p != NULL)
        {
            unsigned char* bytes = (unsigned char*) p;
            CPlex* pNext = p->pNext;
            delete[] bytes;
            p = pNext;
        }
    }
};


#endif //__CPLEX_H__

