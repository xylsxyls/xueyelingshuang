// FIXALLOC.H - declarations for fixed block allocator

#include "plex.h"
#include "ptmutexlock.h"

#ifndef __FIXALLOC_H__
#define __FIXALLOC_H__

/////////////////////////////////////////////////////////////////////////////
// CFixedAlloc

class CFixedAlloc
{
// Constructors
public:
    CFixedAlloc(unsigned int nAllocSize, unsigned int nBlockSize = 64);

// Attributes
    unsigned int GetAllocSize() { return m_nAllocSize; }

// Operations
public:
    void* Alloc();  // return a chunk of memory of nAllocSize
    void Free(void* p); // free chunk of memory returned from Alloc
    void FreeAll(); // free everything allocated from this allocator

// Implementation
public:
    ~CFixedAlloc();

protected:
    struct CNode
    {
        CNode* pNext;   // only valid when in free list
    };

    unsigned int m_nAllocSize;  // size of each block from Alloc
    unsigned int m_nBlockSize;  // number of blocks to get at a time
    CPlex* m_pBlocks;   // linked list of blocks (is nBlocks*nAllocSize)
    CNode* m_pNodeFree; // first free node (NULL if no free nodes)
    ptMutexLock m_protect;
};

#endif

