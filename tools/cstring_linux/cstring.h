/*
  class:	CString
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <iostream>
#include <string.h>

using namespace std;

#if defined(_WIN32)
    #include <windows.h>
    #define vsnprintf _vsnprintf
#elif defined(__GNUC__)
    #include "ptspinlock.h"
#endif

#ifndef __CSTRING_H__
#define __CSTRING_H__

//#define INT_MAX 2147483647 

#ifndef byte
typedef unsigned char byte;
#endif

#ifndef LPCSTR
typedef const char* LPCSTR;
#endif

#ifndef LPSTR
typedef char* LPSTR;
#endif

//#ifndef
//typedef
//#endif

/////////////////////////////////////////////////////////////////////////////
// CString - String class

struct CStringData
{
    volatile long nRefs;     // reference count
    int nDataLength;
    int nAllocLength;
    // char data[nAllocLength]

    char* data()
        { return (char*)(this + 1); }
};


class CString
{
public:
// Constructors
    CString();
    CString(const CString& stringSrc);
    CString(char ch, int nRepeat = 1);
    CString(LPCSTR lpsz);
    CString(LPCSTR lpch, int nLength);
    CString(const unsigned char* psz);

// Attributes & Operations
    // as an array of characters
    int GetLength() const;
    bool IsEmpty() const;
    void Empty();                       // free up the data

    char GetAt(int nIndex) const;      // 0 based
    char operator[](int nIndex) const; // same as GetAt
    void SetAt(int nIndex, char ch);
    operator LPCSTR() const;           // as a C string

    // overloaded assignment
    const CString& operator=(const CString& stringSrc);
    const CString& operator=(char ch);
    const CString& operator=(LPCSTR lpsz);
    const CString& operator=(const unsigned char* psz);

    // string concatenation
    const CString& operator+=(const CString& string);
    const CString& operator+=(char ch);
    const CString& operator+=(LPCSTR lpsz);

    friend CString operator+(const CString& string1, const CString& string2);
    friend CString operator+(const CString& string, char ch);
    friend CString operator+(char ch, const CString& string);
    friend CString operator+(const CString& string, LPCSTR lpsz);
    friend CString operator+(LPCSTR lpsz, const CString& string);
    friend ostream& operator<<(ostream& os, const CString& string);
    friend istream& operator>>(istream& is, CString& string);

    // string comparison
    int Compare(LPCSTR lpsz) const;         // straight character
    int CompareNoCase(LPCSTR lpsz) const;   // ignore case

    // simple sub-string extraction
    CString Mid(int nFirst, int nCount) const;
    CString Mid(int nFirst) const;
    CString Left(int nCount) const;
    CString Right(int nCount) const;

    CString SpanIncluding(LPCSTR lpszCharSet) const;
    CString SpanExcluding(LPCSTR lpszCharSet) const;

    // upper/lower/reverse conversion
    void MakeUpper();
    void MakeLower();
    void MakeReverse();

    // trimming whitespace (either side)
    void TrimRight();
    void TrimLeft();

    // remove continuous occurrences of chTarget starting from right
    void TrimRight(char chTarget);
    // remove continuous occcurrences of characters in passed string,
    // starting from right
    void TrimRight(LPCSTR lpszTargets);
    // remove continuous occurrences of chTarget starting from left
    void TrimLeft(char chTarget);
    // remove continuous occcurrences of characters in
    // passed string, starting from left
    void TrimLeft(LPCSTR lpszTargets);

    // advanced manipulation
    // replace occurrences of chOld with chNew
    int Replace(char chOld, char chNew);
    // replace occurrences of substring lpszOld with lpszNew;
    // empty lpszNew removes instances of lpszOld
    int Replace(LPCSTR lpszOld, LPCSTR lpszNew);
    // remove occurrences of chRemove
    int Remove(char chRemove);
    // insert character at zero-based index; concatenates
    // if index is past end of string
    int Insert(int nIndex, char ch);
    // insert substring at zero-based index; concatenates
    // if index is past end of string
    int Insert(int nIndex, LPCSTR pstr);
    // delete nCount characters starting at zero-based index
    int Delete(int nIndex, int nCount = 1);

    // searching (return starting index, or -1 if not found)
    // look for a single character match
    int Find(char ch) const;                     // like "C" strchr
    int ReverseFind(char ch) const;
    int Find(char ch, int nStart) const;         // starting at index
    int FindOneOf(LPCSTR lpszCharSet) const;

    // look for a specific sub-string
    int Find(LPCSTR lpszSub) const;        // like "C" strstr
    int Find(LPCSTR lpszSub, int nStart) const;  // starting at index

    // Concatentation for non strings
    const CString& Append(int n);

    // simple formatting
    int Format(LPCSTR lpszFormat, ...);

    // Access to string implementation buffer as "C" character array
    LPSTR GetBuffer(int nMinBufLength);
    void ReleaseBuffer(int nNewLength = -1);
    LPSTR GetBufferSetLength(int nNewLength);
    void FreeExtra();

    // Use LockBuffer/UnlockBuffer to turn refcounting off
    LPSTR LockBuffer();
    void UnlockBuffer();

// Implementation
public:
    ~CString();
    int GetAllocLength() const;

protected:
    LPSTR m_pchData;   // pointer to ref counted string data
#if defined(__GNUC__)
    static ptSpinLock lockrefs;
#endif

    // implementation helpers
    CStringData* GetData() const;
    void Init();
    void AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
    bool AllocBuffer(int nLen);
    void AssignCopy(int nSrcLen, LPCSTR lpszSrcData);
    bool ConcatCopy(int nSrc1Len, LPCSTR lpszSrc1Data, int nSrc2Len, LPCSTR lpszSrc2Data);
    void ConcatInPlace(int nSrcLen, LPCSTR lpszSrcData);
    void CopyBeforeWrite();
    bool AllocBeforeWrite(int nLen);
    void Release();
    static void Release(CStringData* pData);
    static int SafeStrlen(LPCSTR lpsz);
    static void FreeData(CStringData* pData);
    static const CString& _GetEmptyString();

    static inline int safe_inc(int* value)
    {
#if defined(_WIN32)
        return InterlockedIncrement((long*)value);
#elif defined(__GNUC__)
        ptSpinLockAdp lock(&lockrefs);
        return ++(*value);
#else
    #error not support
#endif
    }

    static inline int safe_dec(int* value)
    {
#if defined(_WIN32)
        return InterlockedDecrement((long*)value);
#elif defined(__GNUC__)
        ptSpinLockAdp lock(&lockrefs);
        return --(*value);
#else
    #error not support
#endif
    }
};


// Compare helpers
bool operator==(const CString& s1, const CString& s2);
bool operator==(const CString& s1, LPCSTR s2);
bool operator==(LPCSTR s1, const CString& s2);
bool operator!=(const CString& s1, const CString& s2);
bool operator!=(const CString& s1, LPCSTR s2);
bool operator!=(LPCSTR s1, const CString& s2);
bool operator<(const CString& s1, const CString& s2);
bool operator<(const CString& s1, LPCSTR s2);
bool operator<(LPCSTR s1, const CString& s2);
bool operator>(const CString& s1, const CString& s2);
bool operator>(const CString& s1, LPCSTR s2);
bool operator>(LPCSTR s1, const CString& s2);
bool operator<=(const CString& s1, const CString& s2);
bool operator<=(const CString& s1, LPCSTR s2);
bool operator<=(LPCSTR s1, const CString& s2);
bool operator>=(const CString& s1, const CString& s2);
bool operator>=(const CString& s1, LPCSTR s2);
bool operator>=(LPCSTR s1, const CString& s2);


/////////////////////////////////////////////////////////////////////////////
// CString Implementation

inline CStringData* CString::GetData() const
    { assert(m_pchData != NULL); return ((CStringData*)m_pchData) - 1; }
inline void CString::Init()
    { m_pchData = _GetEmptyString().m_pchData; }
inline CString::CString(const unsigned char* lpsz)
    { Init(); *this = (LPCSTR)lpsz; }
inline const CString& CString::operator=(const unsigned char* lpsz)
    { *this = (LPCSTR)lpsz; return *this; }
inline int CString::GetLength() const
    { return GetData()->nDataLength; }
inline int CString::GetAllocLength() const
    { return GetData()->nAllocLength; }
inline bool CString::IsEmpty() const
    { return GetData()->nDataLength == 0; }
inline CString::operator LPCSTR() const
    { return m_pchData; }
inline int CString::SafeStrlen(LPCSTR lpsz)
    { return (lpsz == NULL) ? 0 : strlen(lpsz); }
inline CString::CString() { Init(); }

// CString support (windows specific)
inline int CString::Compare(LPCSTR lpsz) const
    { return strcmp(m_pchData, lpsz); }
inline int CString::CompareNoCase(LPCSTR lpsz) const
    { 
#if defined(_WIN32)
        return _stricmp(m_pchData, lpsz);
#elif defined(__GNUC__)
        return strcasecmp(m_pchData, lpsz);
#endif
    }

inline char CString::GetAt(int nIndex) const
{
    assert(nIndex >= 0);
    assert(nIndex < GetData()->nDataLength);
    return m_pchData[nIndex];
}
inline char CString::operator[](int nIndex) const
{
    // same as GetAt
    assert(nIndex >= 0);
    assert(nIndex < GetData()->nDataLength);
    return m_pchData[nIndex];
}
inline bool operator==(const CString& s1, const CString& s2)
    { return s1.Compare(s2) == 0; }
inline bool operator==(const CString& s1, LPCSTR s2)
    { return s1.Compare(s2) == 0; }
inline bool operator==(LPCSTR s1, const CString& s2)
    { return s2.Compare(s1) == 0; }
inline bool operator!=(const CString& s1, const CString& s2)
    { return s1.Compare(s2) != 0; }
inline bool operator!=(const CString& s1, LPCSTR s2)
    { return s1.Compare(s2) != 0; }
inline bool operator!=(LPCSTR s1, const CString& s2)
    { return s2.Compare(s1) != 0; }
inline bool operator<(const CString& s1, const CString& s2)
    { return s1.Compare(s2) < 0; }
inline bool operator<(const CString& s1, LPCSTR s2)
    { return s1.Compare(s2) < 0; }
inline bool operator<(LPCSTR s1, const CString& s2)
    { return s2.Compare(s1) > 0; }
inline bool operator>(const CString& s1, const CString& s2)
    { return s1.Compare(s2) > 0; }
inline bool operator>(const CString& s1, LPCSTR s2)
    { return s1.Compare(s2) > 0; }
inline bool operator>(LPCSTR s1, const CString& s2)
    { return s2.Compare(s1) < 0; }
inline bool operator<=(const CString& s1, const CString& s2)
    { return s1.Compare(s2) <= 0; }
inline bool operator<=(const CString& s1, LPCSTR s2)
    { return s1.Compare(s2) <= 0; }
inline bool operator<=(LPCSTR s1, const CString& s2)
    { return s2.Compare(s1) >= 0; }
inline bool operator>=(const CString& s1, const CString& s2)
    { return s1.Compare(s2) >= 0; }
inline bool operator>=(const CString& s1, LPCSTR s2)
    { return s1.Compare(s2) >= 0; }
inline bool operator>=(LPCSTR s1, const CString& s2)
    { return s2.Compare(s1) <= 0; }

#endif // __CSTRING_H__

