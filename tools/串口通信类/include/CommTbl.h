// CommTbl.h: interface for the CCommTbl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMTBL_H__92203A87_4E0D_47CB_A75E_F56A90C08088__INCLUDED_)
#define AFX_COMMTBL_H__92203A87_4E0D_47CB_A75E_F56A90C08088__INCLUDED_

#include <vector>
#include <list>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

template<class T> class CCommTbl  
{
public:

private:
    std::list<T> m_vctTbl;

public:
    CCommTbl<T> operator=(std::vector<T> vct)
    {
        m_vctTbl.clear();
        for(size_t i=0; i<vct.size(); i++)
            m_vctTbl.push_back(vct[i]);
        return *this;
    }
	CCommTbl<T> operator=(CCommTbl<T> vct)
	{
		m_vctTbl.clear();
		for(int i=0; i<vct.size(); i++)
			m_vctTbl.push_back(vct[i]);
		return *this;
	}

    T& operator[](int idx)
    {
        return *(get(idx));
    }

    void add(T stTbl)
    {
        m_vctTbl.push_back(stTbl);
    };

	void add(CCommTbl<T> vctTbl)
	{
		for(int i=0;i<vctTbl.size();i++)
			m_vctTbl.push_back(vctTbl[i]);
	};

    void push_back(T stTbl)
    {
        m_vctTbl.push_back(stTbl);
    };

	void push_back(CCommTbl<T> vctTbl)
	{
		for(int i=0;i<vctTbl.size();i++)
			m_vctTbl.push_back(vctTbl[i]);
	};

    void clear()
    {
        m_vctTbl.clear();
    };

    int count()
    {
        return m_vctTbl.size();
    }
    int size()
    {
        return m_vctTbl.size();
    }
    bool remove(UINT iPos)
    {
        if(m_vctTbl.size() <= iPos)
            return false;
        std::list<T>::iterator st_Iter;
        st_Iter = m_vctTbl.begin();
        for(size_t i=0; i<iPos; i++)
            st_Iter++;
        m_vctTbl.erase(st_Iter);
        return true;
    };

    bool remove(T* pData)
    {
        std::list<T>::iterator st_Iter;

        for(st_Iter = m_vctTbl.begin(); st_Iter != m_vctTbl.end(); st_Iter++)
        {
            if(pData == &(*st_Iter))
            {
                m_vctTbl.erase(st_Iter);
                return true;
            }
        }
        return false;
    }

    int remove(T* pMask, bool (*Compare)(T*,T*))
    {
        int iCount = 0;
        std::list<T>::iterator st_Iter;
        for(st_Iter = m_vctTbl.begin(); st_Iter != m_vctTbl.end(); )
        {
            if(!Compare || (Compare && Compare(pMask, &(*st_Iter))) )
            {
                m_vctTbl.erase(st_Iter);
                st_Iter = m_vctTbl.begin();
                iCount++;
            }
            else
            {
                st_Iter++;
            }
        }
        return iCount;
    };

    T* get(unsigned int iPos)
    {
        if(m_vctTbl.size() <= iPos)
            return NULL;
        std::list<T>::iterator st_Iter;
        st_Iter = m_vctTbl.begin();
        for(unsigned int i=0; i<iPos; i++)
            st_Iter++;
        return &(*st_Iter);
    };

    T* get(T* pMask, bool (*Compare)(T*,T*))
    {
        std::list<T>::iterator st_Iter;
        for(st_Iter=m_vctTbl.begin(); st_Iter!=m_vctTbl.end(); st_Iter++)
        {
            if(!Compare || (Compare && Compare(pMask, &(*st_Iter))) )
            {
                return &(*st_Iter);
            }
        }
        return NULL;
    };

    std::vector<T> gets(T* pMask=NULL, bool (*Compare)(T*,T*)=NULL)
    {
        std::vector<T> vct;
        std::list<T>::iterator st_Iter;
        for(st_Iter=m_vctTbl.begin(); st_Iter!=m_vctTbl.end(); st_Iter++)
        {
            if(!Compare || (Compare && Compare(pMask, &(*st_Iter))) )
            {
                vct.push_back(*st_Iter);
            }
        }
        return vct;
    };

	std::vector<T*> getsp(T* pMask=NULL, bool (*Compare)(T*,T*)=NULL)
	{
		std::vector<T*> vct;
		std::list<T>::iterator st_Iter;
		for(st_Iter=m_vctTbl.begin(); st_Iter!=m_vctTbl.end(); st_Iter++)
		{
			if(!Compare || (Compare && Compare(pMask, &(*st_Iter))) )
			{
				vct.push_back(&(*st_Iter));
			}
		}
		return vct;
	};
    
    bool swap(unsigned int iPos, unsigned int iPos2)
	{
		if(iPos>=m_vctTbl.size()) return false;
		if(iPos2>=m_vctTbl.size()) return false;
		std::list<T>::iterator it = m_vctTbl.begin();
		std::list<T>::iterator it2 = m_vctTbl.begin();
		advance(it, iPos);
		advance(it2, iPos2);
		std::swap(*it, *it2);
		return true;
	}
};

#endif // !defined(AFX_COMMTBL_H__92203A87_4E0D_47CB_A75E_F56A90C08088__INCLUDED_)
