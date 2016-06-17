// fixalloc.cpp - implementation of fixed block allocator

#include "fixalloc.h"\

/////////////////////////////////////////////////////////////////////////////
// CFixedAlloc

CFixedAlloc::CFixedAlloc(unsigned int nAllocSize, unsigned int nBlockSize)
{
    assert(nAllocSize >= sizeof(CNode));
    assert(nBlockSize > 1);

    m_nAllocSize = nAllocSize;
    m_nBlockSize = nBlockSize;
    m_pNodeFree = NULL;
    m_pBlocks = NULL;
}

CFixedAlloc::~CFixedAlloc()
{
    FreeAll();
}

void CFixedAlloc::FreeAll()
{
    ptMutexLockAdp ptlock(&m_protect, true);
    m_pBlocks->FreeDataChain();
    m_pBlocks = NULL;
    m_pNodeFree = NULL;
}

void* CFixedAlloc::Alloc()
{
    ptMutexLockAdp ptlock(&m_protect, true);
    if (m_pNodeFree == NULL)
    {
        CPlex* pNewBlock = NULL;
        // add another block
        pNewBlock = CPlex::Create(m_pBlocks, m_nBlockSize, m_nAllocSize);

        // chain them into free list
        CNode* pNode = (CNode*)pNewBlock->data();
        // free in reverse order to make it easier to debug
        (unsigned char*&)pNode += (m_nAllocSize * m_nBlockSize) - m_nAllocSize;
        for (int i = m_nBlockSize-1; i >= 0; i--, (unsigned char*&)pNode -= m_nAllocSize)
        {
            pNode->pNext = m_pNodeFree;
            m_pNodeFree = pNode;
        }
    }
    assert(m_pNodeFree != NULL);  // we must have something

    // remove the first available node from the free list
    void* pNode = m_pNodeFree;
    m_pNodeFree = m_pNodeFree->pNext;

    return pNode;
}

void CFixedAlloc::Free(void* p)
{
    if (p != NULL)
    {
        ptMutexLockAdp ptlock(&m_protect, true);

        // simply return the node to the free list
        CNode* pNode = (CNode*)p;
        pNode->pNext = m_pNodeFree;
        m_pNodeFree = pNode;
    }
}
