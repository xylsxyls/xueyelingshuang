
#include <stdarg.h>

#include "cstring.h"
#include "fixalloc.h"

using namespace std;

#if defined(_WIN32)
    #define vsnprintf _vsnprintf
#endif

static inline LPSTR NextChar(LPCSTR p);
static char* _cstrchr(const char* p, char ch);
static char* _cstrrchr(const char* p, char ch);
static char* _cstrrev(char* pStr);
static char* _cstrstr(const char* pStr, const char* pCharSet);

// Globals
// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
static int rgInitData[] = { -1, 0, 0, 0 };
static CStringData* StrDataNil = (CStringData*)&rgInitData;
static LPCSTR StrPchNil = (LPCSTR)(((byte*)&rgInitData) + sizeof(CStringData));

#define ROUND(x,y) (((x)+(y-1))&~(y-1))
#define ROUND4(x) ROUND(x, 4)
static CFixedAlloc StrAlloc64(ROUND4(65*sizeof(char)+sizeof(CStringData)));
static CFixedAlloc StrAlloc128(ROUND4(129*sizeof(char)+sizeof(CStringData)));
static CFixedAlloc StrAlloc256(ROUND4(257*sizeof(char)+sizeof(CStringData)));
static CFixedAlloc StrAlloc512(ROUND4(513*sizeof(char)+sizeof(CStringData)));

#if defined(__GNUC__)
ptSpinLock CString::lockrefs;
#endif

const CString& CString::_GetEmptyString()
{
	return *(CString*)&StrPchNil;
}

CString::CString(const CString& stringSrc)
{
    assert(stringSrc.GetData()->nRefs != 0);
    if (stringSrc.GetData()->nRefs >= 0)
    {
        assert(stringSrc.GetData() != StrDataNil);
        m_pchData = stringSrc.m_pchData;
        safe_inc((int*)&GetData()->nRefs);
    }
    else
    {
        Init();
        *this = stringSrc.m_pchData;
    }
}

bool CString::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
    assert(nLen >= 0);
    assert(nLen <= INT_MAX - 1);    // max size (enough room for 1 extra)

    if (nLen == 0)
    {
        Init();
    }
    else
    {
        CStringData* pData = NULL;

        if (nLen <= 64)
        {
            pData = (CStringData*)StrAlloc64.Alloc();
            pData->nAllocLength = 64;
        }
        else if (nLen <= 128)
        {
            pData = (CStringData*)StrAlloc128.Alloc();
            pData->nAllocLength = 128;
        }
        else if (nLen <= 256)
        {
            pData = (CStringData*)StrAlloc256.Alloc();
            pData->nAllocLength = 256;
        }
        else if (nLen <= 512)
        {
            pData = (CStringData*)StrAlloc512.Alloc();
            pData->nAllocLength = 512;
        }
        else
        {
            pData = (CStringData*)new byte[sizeof(CStringData) + (nLen + 1) * sizeof(char)];
            pData->nAllocLength = nLen;
        }

        pData->nRefs = 1;
        pData->data()[nLen] = '\0';
        pData->nDataLength = nLen;
        m_pchData = pData->data();
    }

    return true;
}

void CString::Release()
{
    if (GetData() != StrDataNil)
    {
        assert(GetData()->nRefs != 0);
		if (safe_dec((int*)&GetData()->nRefs) <= 0)
            FreeData(GetData());
        Init();
    }
}

void CString::Release(CStringData* pData)
{
    if (pData != StrDataNil)
    {
        assert(pData->nRefs != 0);
        if (safe_dec((int*)&pData->nRefs) <= 0)
            FreeData(pData);
    }
}

void CString::Empty()
{
    if (GetData()->nDataLength == 0)
        return;

    if (GetData()->nRefs >= 0)
        Release();
    else
        *this = "";

    assert(GetData()->nDataLength == 0);
    assert(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

void CString::CopyBeforeWrite()
{
    if (GetData()->nRefs > 1)
    {
        CStringData* pData = GetData();
        Release();
        if(AllocBuffer(pData->nDataLength))
            memcpy(m_pchData, pData->data(), (pData->nDataLength + 1) * sizeof(char));
    }
    assert(GetData()->nRefs <= 1);
}

bool CString::AllocBeforeWrite(int nLen)
{
    bool bRet = true;
    if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
    {
        Release();
        bRet = AllocBuffer(nLen);
    }
    assert(GetData()->nRefs <= 1);
    return bRet;
}

const CString& CString::Append(int n)
{
	char szBuffer[10];
	sprintf(szBuffer,"%d",n);
	ConcatInPlace(SafeStrlen(szBuffer), szBuffer);
	return *this;
}


CString::~CString()
//  free any attached data
{
    if (GetData() != StrDataNil)
    {
        if (safe_dec((int*)&GetData()->nRefs) <= 0)
            FreeData(GetData());
    }
}

void CString::AllocCopy(CString& dest, int nCopyLen, int nCopyIndex,
     int nExtraLen) const
{
    // will clone the data attached to this string
    // allocating 'nExtraLen' characters
    // Places results in uninitialized string 'dest'
    // Will copy the part or all of original data to start of new string

    int nNewLen = nCopyLen + nExtraLen;
    if (nNewLen == 0)
    {
        dest.Init();
    }
    else
    {
        if(dest.AllocBuffer(nNewLen))
            memcpy(dest.m_pchData, m_pchData + nCopyIndex, nCopyLen * sizeof(char));
    }
}

CString::CString(LPCSTR lpsz)
{
    Init();

    int nLen = SafeStrlen(lpsz);
    if (nLen != 0)
    {
        if(AllocBuffer(nLen))
            memcpy(m_pchData, lpsz, nLen * sizeof(char));
    }
}


// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CString&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CString::AssignCopy(int nSrcLen, LPCSTR lpszSrcData)
{
    if(AllocBeforeWrite(nSrcLen))
    {
        memcpy(m_pchData, lpszSrcData, nSrcLen * sizeof(char));
        GetData()->nDataLength = nSrcLen;
        m_pchData[nSrcLen] = '\0';
    }
}

const CString& CString::operator=(const CString& stringSrc)
{
    if (m_pchData != stringSrc.m_pchData)
	{
        if ((GetData()->nRefs < 0 && GetData() != StrDataNil) || stringSrc.GetData()->nRefs < 0)
        {
            // actual copy necessary since one of the strings is locked
            AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
        }
        else
        {
        	// can just copy references around
            Release();
            assert(stringSrc.GetData() != StrDataNil);
            m_pchData = stringSrc.m_pchData;
            safe_inc((int*)&GetData()->nRefs);
        }
    }
    return *this;
}

const CString& CString::operator=(LPCSTR lpsz)
{
    assert(lpsz != NULL);
    AssignCopy(SafeStrlen(lpsz), lpsz);
    return *this;
}

// Concatenation
// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CString + CString
// and for ? = char, LPCSTR
//          CString + ?
//          ? + CString

bool CString::ConcatCopy(int nSrc1Len, LPCSTR lpszSrc1Data,
    int nSrc2Len, LPCSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CString object

    bool bRet = true;
    int nNewLen = nSrc1Len + nSrc2Len;
    if (nNewLen != 0)
    {
        bRet = AllocBuffer(nNewLen);
        if (bRet)
        {
            memcpy(m_pchData, lpszSrc1Data, nSrc1Len * sizeof(char));
            memcpy(m_pchData + nSrc1Len, lpszSrc2Data, nSrc2Len * sizeof(char));
        }
    }
    return bRet;
}

CString operator+(const CString& string1, const CString& string2)
{
    CString s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, string2.GetData()->nDataLength, string2.m_pchData);
    return s;
}

CString operator+(const CString& string, LPCSTR lpsz)
{
    assert(lpsz != NULL);
    CString s;
    s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData, CString::SafeStrlen(lpsz), lpsz);
    return s;
}

CString operator+(LPCSTR lpsz, const CString& string)
{
    assert(lpsz != NULL);
    CString s;
    s.ConcatCopy(CString::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength, string.m_pchData);
    return s;
}

void CString::ConcatInPlace(int nSrcLen, LPCSTR lpszSrcData)
{
    //  -- the main routine for += operators

    // concatenating an empty string is a no-op!
    if (nSrcLen == 0)
        return;

    // if the buffer is too small, or we have a width mis-match, just
    //   allocate a new buffer (slow but sure)
    if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
    {
        // we have to grow the buffer, use the ConcatCopy routine
        CStringData* pOldData = GetData();
        if (ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData))
        {
            assert(pOldData != NULL);
            CString::Release(pOldData);
        }
    }
    else
    {
        // fast concatenation when buffer big enough
        memcpy(m_pchData + GetData()->nDataLength, lpszSrcData, nSrcLen * sizeof(char));
        GetData()->nDataLength += nSrcLen;
        assert(GetData()->nDataLength <= GetData()->nAllocLength);
        m_pchData[GetData()->nDataLength] = '\0';
    }
}

const CString& CString::operator+=(LPCSTR lpsz)
{
    assert(lpsz != NULL);
    ConcatInPlace(SafeStrlen(lpsz), lpsz);
    return *this;
}

const CString& CString::operator+=(char ch)
{
    ConcatInPlace(1, &ch);
    return *this;
}

const CString& CString::operator+=(const CString& string)
{
    ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
    return *this;
}

LPSTR CString::GetBuffer(int nMinBufLength)
{
    assert(nMinBufLength >= 0);

    if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
    {
        // we have to grow the buffer
        CStringData* pOldData = GetData();
        int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
        if (nMinBufLength < nOldLen)
            nMinBufLength = nOldLen;

        if(!AllocBuffer(nMinBufLength))
            return NULL;

        memcpy(m_pchData, pOldData->data(), (nOldLen + 1) * sizeof(char));
        GetData()->nDataLength = nOldLen;
        CString::Release(pOldData);
    }
    assert(GetData()->nRefs <= 1);

    // return a pointer to the character storage for this string
    assert(m_pchData != NULL);
    return m_pchData;
}

void CString::ReleaseBuffer(int nNewLength)
{
    CopyBeforeWrite();  // just in case GetBuffer was not called

    if (nNewLength == -1)
        nNewLength = strlen(m_pchData); // zero terminated

    assert(nNewLength <= GetData()->nAllocLength);
    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = '\0';
}

LPSTR CString::GetBufferSetLength(int nNewLength)
{
    assert(nNewLength >= 0);

    if(GetBuffer(nNewLength) == NULL)
        return NULL;

    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = '\0';
    return m_pchData;
}

void CString::FreeExtra()
{
    assert(GetData()->nDataLength <= GetData()->nAllocLength);
    if (GetData()->nDataLength != GetData()->nAllocLength)
    {
        CStringData* pOldData = GetData();
        if(AllocBuffer(GetData()->nDataLength))
        {
            memcpy(m_pchData, pOldData->data(), pOldData->nDataLength * sizeof(char));
            assert(m_pchData[GetData()->nDataLength] == '\0');
            CString::Release(pOldData);
        }
    }
    assert(GetData() != NULL);
}

LPSTR CString::LockBuffer()
{
    LPSTR lpsz = GetBuffer(0);
    if(lpsz != NULL)
        GetData()->nRefs = -1;
    return lpsz;
}

void CString::UnlockBuffer()
{
    assert(GetData()->nRefs == -1);
    if (GetData() != StrDataNil)
        GetData()->nRefs = 1;
}

int CString::Find(char ch) const
{
    return Find(ch, 0);
}

int CString::Find(char ch, int nStart) const
{
    int nLength = GetData()->nDataLength;
    if (nStart >= nLength)
        return -1;

    // find first single character
    LPSTR lpsz = _cstrchr(m_pchData + nStart, (unsigned char)ch);

    // return -1 if not found and index otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CString::FindOneOf(LPCSTR lpszCharSet) const
{
    assert(lpszCharSet != NULL);
    LPSTR lpsz = strpbrk(m_pchData, lpszCharSet);
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

void CString::MakeUpper()
{
    CopyBeforeWrite();
#if defined(_WIN32)
    _strlwr(m_pchData);
#else
    LPSTR p=m_pchData;
    while(*p != '\0')
    {
        *p = toupper(*p);
         p = NextChar(p);
    }
#endif
}

void CString::MakeLower()
{
    CopyBeforeWrite();
#if defined(_WIN32)
    _strupr(m_pchData);
#else
    LPSTR p=m_pchData;
    while(*p != '\0')
    {
        *p = tolower(*p);
        p = NextChar(p);
    }
#endif
}

void CString::MakeReverse()
{
    CopyBeforeWrite();
    _cstrrev(m_pchData);
}

void CString::SetAt(int nIndex, char ch)
{
    assert(nIndex >= 0);
    assert(nIndex < GetData()->nDataLength);

    CopyBeforeWrite();
    m_pchData[nIndex] = ch;
}

CString::CString(char ch, int nLength)
{
    Init();
    if (nLength >= 1)
    {
        if(AllocBuffer(nLength))
            memset(m_pchData, ch, nLength);
    }
}

CString::CString(LPCSTR lpch, int nLength)
{
    Init();
    if (nLength != 0)
    {
        if(AllocBuffer(nLength))
            memcpy(m_pchData, lpch, nLength * sizeof(char));
    }
}


const CString& CString::operator=(char ch)
{
    AssignCopy(1, &ch);
    return *this;
}

CString operator+(const CString& string1, char ch)
{
    CString s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
    return s;
}

CString operator+(char ch, const CString& string)
{
    CString s;
    s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
    return s;
}

ostream& operator<<(ostream& os, const CString& string)
{
    os << string.m_pchData;
    return os;
}

istream& operator>>(istream& is, CString& string)
{
    //is >> string;
    byte buff[1024];
    is.getline((LPSTR)buff, sizeof(buff));
    buff[sizeof(buff)-1] = '\0';
    string += buff;
    return is;
}

CString CString::Mid(int nFirst) const
{
    return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CString CString::Mid(int nFirst, int nCount) const
{
    // out-of-bounds requests return sensible things
    if (nFirst < 0)
        nFirst = 0;
    if (nCount < 0)
        nCount = 0;

    if (nFirst + nCount > GetData()->nDataLength)
        nCount = GetData()->nDataLength - nFirst;
    if (nFirst > GetData()->nDataLength)
        nCount = 0;

    CString dest;
    AllocCopy(dest, nCount, nFirst, 0);
    return dest;
}

CString CString::Right(int nCount) const
{
    if (nCount < 0)
        nCount = 0;
    else if (nCount > GetData()->nDataLength)
        nCount = GetData()->nDataLength;

    CString dest;
    AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
    return dest;
}

CString CString::Left(int nCount) const
{
    if (nCount < 0)
        nCount = 0;
    else if (nCount > GetData()->nDataLength)
        nCount = GetData()->nDataLength;

    CString dest;
    AllocCopy(dest, nCount, 0, 0);
    return dest;
}

// strspn equivalent
CString CString::SpanIncluding(LPCSTR lpszCharSet) const
{
    assert(lpszCharSet != NULL);
    return Left(strspn(m_pchData, lpszCharSet));
}

// strcspn equivalent
CString CString::SpanExcluding(LPCSTR lpszCharSet) const
{
    assert(lpszCharSet != NULL);
    return Left(strcspn(m_pchData, lpszCharSet));
}

int CString::ReverseFind(char ch) const
{
    // find last single character
    LPSTR lpsz = _cstrrchr(m_pchData, (unsigned char)ch);

    // return -1 if not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
int CString::Find(LPCSTR lpszSub) const
{
    return Find(lpszSub, 0);
}

int CString::Find(LPCSTR lpszSub, int nStart) const
{
    assert(lpszSub != NULL);

    int nLength = GetData()->nDataLength;
    if (nStart > nLength)
        return -1;

    // find first matching substring
    LPSTR lpsz = _cstrstr(m_pchData + nStart, lpszSub);

    // return -1 for not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}


// formatting (using wsprintf style formatting)
int CString::Format(LPCSTR lpszFormat, ...)
{
    assert(lpszFormat != NULL);
    byte buff[4096];

    va_list argList;
    va_start(argList, lpszFormat);
    int bRet = vsnprintf((LPSTR)buff, sizeof(buff) -1, lpszFormat, argList);
    assert(bRet>=0);
    if( bRet >= 0 ) {
        buff[bRet] = '\0';
    }
    va_end(argList);
    *this = buff; 
    return bRet;
}

void CString::TrimRight()
{
    CopyBeforeWrite();

    // find beginning of trailing spaces by starting at beginning (DBCS aware)
    LPSTR lpsz = m_pchData;
    LPSTR lpszLast = NULL;
    while (*lpsz != '\0')
    {
        if (isspace(*lpsz))
        {
            if (lpszLast == NULL)
                lpszLast = lpsz;
        }
        else
        {
            lpszLast = NULL;
        }
        lpsz = (LPSTR)NextChar(lpsz);
    }

    if (lpszLast != NULL)
    {
        // truncate at trailing space start
        *lpszLast = '\0';
        GetData()->nDataLength = (int)(lpszLast - m_pchData);
    }
}

void CString::TrimLeft()
{
    CopyBeforeWrite();

    // find first non-space character
    LPCSTR lpsz = m_pchData;
    while (isspace(*lpsz))
        lpsz = NextChar(lpsz);

    // fix up data and length
    int nDataLength = GetData()->nDataLength - (int)(lpsz - m_pchData);
    memmove(m_pchData, lpsz, (nDataLength + 1) * sizeof(char));
    GetData()->nDataLength = nDataLength;
}

void CString::TrimRight(LPCSTR lpszTargetList)
{
    // find beginning of trailing matches
    // by starting at beginning (DBCS aware)

    CopyBeforeWrite();
    LPSTR lpsz = m_pchData;
    LPSTR lpszLast = NULL;

    while (*lpsz != '\0')
    {
        if (_cstrchr(lpszTargetList, *lpsz) != NULL)
        {
            if (lpszLast == NULL)
                lpszLast = lpsz;
        }
        else
            lpszLast = NULL;
        lpsz = NextChar(lpsz);
    }

    if (lpszLast != NULL)
    {
        // truncate at left-most matching character
        *lpszLast = '\0';
        GetData()->nDataLength = (int)(lpszLast - m_pchData);
    }
}

void CString::TrimRight(char chTarget)
{
    // find beginning of trailing matches
    // by starting at beginning (DBCS aware)

    CopyBeforeWrite();
    LPSTR lpsz = m_pchData;
    LPSTR lpszLast = NULL;

    while (*lpsz != '\0')
    {
        if (*lpsz == chTarget)
        {
            if (lpszLast == NULL)
                lpszLast = lpsz;
        }
        else
            lpszLast = NULL;
        lpsz = NextChar(lpsz);
    }

    if (lpszLast != NULL)
    {
        // truncate at left-most matching character
        *lpszLast = '\0';
        GetData()->nDataLength = (int)(lpszLast - m_pchData);
    }
}

void CString::TrimLeft(LPCSTR lpszTargets)
{
    // if we're not trimming anything, we're not doing any work
    if (SafeStrlen(lpszTargets) == 0)
        return;

    CopyBeforeWrite();
    LPCSTR lpsz = m_pchData;

    while (*lpsz != '\0')
    {
        if (_cstrchr(lpszTargets, *lpsz) == NULL)
            break;
        lpsz = NextChar(lpsz);
    }

    if (lpsz != m_pchData)
    {
        // fix up data and length
        int nDataLength = GetData()->nDataLength - (int)(lpsz - m_pchData);
        memmove(m_pchData, lpsz, (nDataLength + 1) * sizeof(char));
        GetData()->nDataLength = nDataLength;
    }
}

void CString::TrimLeft(char chTarget)
{
    // find first non-matching character

    CopyBeforeWrite();
    LPCSTR lpsz = m_pchData;

    while (chTarget == *lpsz)
        lpsz = NextChar(lpsz);

    if (lpsz != m_pchData)
    {
        // fix up data and length
        int nDataLength = GetData()->nDataLength - (int)(lpsz - m_pchData);
        memmove(m_pchData, lpsz, (nDataLength + 1) * sizeof(char));
        GetData()->nDataLength = nDataLength;
    }
}

int CString::Delete(int nIndex, int nCount /* = 1 */)
{
    if (nIndex < 0)
        nIndex = 0;
    int nNewLength = GetData()->nDataLength;
    if (nCount > 0 && nIndex < nNewLength)
    {
        CopyBeforeWrite();
        int nbytesToCopy = nNewLength - (nIndex + nCount) + 1;

        memmove(m_pchData + nIndex, m_pchData + nIndex + nCount, nbytesToCopy * sizeof(char));
        GetData()->nDataLength = nNewLength - nCount;
    }

    return nNewLength;
}

int CString::Insert(int nIndex, char ch)
{
    CopyBeforeWrite();

    if (nIndex < 0)
        nIndex = 0;

    int nNewLength = GetData()->nDataLength;
    if (nIndex > nNewLength)
        nIndex = nNewLength;
    nNewLength++;

    if (GetData()->nAllocLength < nNewLength)
    {
        CStringData* pOldData = GetData();
        LPSTR pstr = m_pchData;
        if(!AllocBuffer(nNewLength))
            return -1;
        memcpy(m_pchData, pstr, (pOldData->nDataLength + 1) * sizeof(char));
        CString::Release(pOldData);
    }

    // move existing bytes down
    memmove(m_pchData + nIndex + 1, m_pchData + nIndex, (nNewLength - nIndex) * sizeof(char));
    m_pchData[nIndex] = ch;
    GetData()->nDataLength = nNewLength;

    return nNewLength;
}

int CString::Insert(int nIndex, LPCSTR pstr)
{
    if (nIndex < 0)
        nIndex = 0;

    int nInsertLength = SafeStrlen(pstr);
    int nNewLength = GetData()->nDataLength;
    if (nInsertLength > 0)
    {
        CopyBeforeWrite();
        if (nIndex > nNewLength)
            nIndex = nNewLength;
        nNewLength += nInsertLength;

        if (GetData()->nAllocLength < nNewLength)
        {
            CStringData* pOldData = GetData();
            LPSTR pstr = m_pchData;
            if(!AllocBuffer(nNewLength))
                return -1;
            memcpy(m_pchData, pstr, (pOldData->nDataLength + 1) * sizeof(char));
            CString::Release(pOldData);
        }

        // move existing bytes down
        memmove(m_pchData + nIndex + nInsertLength, m_pchData + nIndex, (nNewLength - nIndex - nInsertLength + 1) * sizeof(char));
        memcpy(m_pchData + nIndex, pstr, nInsertLength * sizeof(char));
        GetData()->nDataLength = nNewLength;
    }

    return nNewLength;
}

int CString::Replace(char chOld, char chNew)
{
    int nCount = 0;

    // short-circuit the nop case
    if (chOld != chNew)
    {
        // otherwise modify each character that matches in the string
        CopyBeforeWrite();
        LPSTR psz = m_pchData;
        LPSTR pszEnd = psz + GetData()->nDataLength;
        while (psz < pszEnd)
        {
            // replace instances of the specified character only
            if (*psz == chOld)
            {
                *psz = chNew;
                nCount++;
            }
            psz = NextChar(psz);
        }
    }
    return nCount;
}

int CString::Replace(LPCSTR lpszOld, LPCSTR lpszNew)
{
    // can't have empty or NULL lpszOld

    int nSourceLen = SafeStrlen(lpszOld);
    if (nSourceLen == 0)
        return 0;
    int nReplacementLen = SafeStrlen(lpszNew);

    // loop once to figure out the size of the result string
    int nCount = 0;
    LPSTR lpszStart = m_pchData;
    LPSTR lpszEnd = m_pchData + GetData()->nDataLength;
    LPSTR lpszTarget;
    while (lpszStart < lpszEnd)
    {
        while ((lpszTarget = _cstrstr(lpszStart, lpszOld)) != NULL)
        {
            nCount++;
            lpszStart = lpszTarget + nSourceLen;
        }
        lpszStart += strlen(lpszStart) + 1;
    }

    // if any changes were made, make them
    if (nCount > 0)
    {
        CopyBeforeWrite();

        // if the buffer is too small, just
        //   allocate a new buffer (slow but sure)
        int nOldLength = GetData()->nDataLength;
        int nNewLength =  nOldLength + (nReplacementLen - nSourceLen) * nCount;
        if (GetData()->nAllocLength < nNewLength || GetData()->nRefs > 1)
        {
            CStringData* pOldData = GetData();
            LPSTR pstr = m_pchData;
            if(!AllocBuffer(nNewLength))
                return -1;
            memcpy(m_pchData, pstr, pOldData->nDataLength * sizeof(char));
            CString::Release(pOldData);
        }
        // else, we just do it in-place
        lpszStart = m_pchData;
        lpszEnd = m_pchData + GetData()->nDataLength;

        // loop again to actually do the work
        while (lpszStart < lpszEnd)
        {
            while ( (lpszTarget = _cstrstr(lpszStart, lpszOld)) != NULL)
            {
                int nBalance = nOldLength - ((int)(lpszTarget - m_pchData) + nSourceLen);
                memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen, nBalance * sizeof(char));
                memcpy(lpszTarget, lpszNew, nReplacementLen * sizeof(char));
                lpszStart = lpszTarget + nReplacementLen;
                lpszStart[nBalance] = '\0';
                nOldLength += (nReplacementLen - nSourceLen);
            }
            lpszStart += strlen(lpszStart) + 1;
        }
        assert(m_pchData[nNewLength] == '\0');
        GetData()->nDataLength = nNewLength;
    }

    return nCount;
}

int CString::Remove(char chRemove)
{
    CopyBeforeWrite();

    LPSTR pstrSource = m_pchData;
    LPSTR pstrDest = m_pchData;
    LPSTR pstrEnd = m_pchData + GetData()->nDataLength;

    while (pstrSource < pstrEnd)
    {
        if (*pstrSource != chRemove)
        {
            *pstrDest = *pstrSource;
            pstrDest = NextChar(pstrDest);
        }
        pstrSource = NextChar(pstrSource);
    }
    *pstrDest = '\0';
    int nCount = (int)(pstrSource - pstrDest);
    GetData()->nDataLength -= nCount;

    return nCount;
}


void CString::FreeData(CStringData* pData)
{
    int nLen = pData->nAllocLength;
    if (nLen == 64)
        StrAlloc64.Free(pData);
    else if (nLen == 128)
        StrAlloc128.Free(pData);
    else if (nLen == 256)
        StrAlloc256.Free(pData);
    else  if (nLen == 512)
        StrAlloc512.Free(pData);
    else
    {
        assert(nLen > 512);
        delete[] (byte*)pData;
    }
}

char* _cstrchr(const char* p, char ch)
{
    //strchr for '\0' should succeed
    while (*p != 0)
    {
        if (*p == ch)
            break;
        p = NextChar(p);
    }
    return (char*)((*p == ch) ? p : NULL);
}
char* _cstrrchr(const char* p, char ch)
{
    const char* lpsz = NULL;
    while (*p != 0)
    {
        if (*p == ch)
            lpsz = p;
        p = NextChar(p);
    }
    return (char*)lpsz;
}
char* _cstrrev(char* pStr)
{
    // Optimize NULL, zero-length, and single-char case.
    if ((pStr == NULL) || (pStr[0] == '\0') || (pStr[1] == '\0'))
        return pStr;

    char* p = pStr;

    while (p[1] != 0) 
    {
        char* pNext = (char*)NextChar(p);
        if(pNext > p + 1)
        {
            char p1 = *(char*)p;
            *(char*)p = *(char*)(p + 1);
            *(char*)(p + 1) = p1;
        }
        p = pNext;
    }

    char* q = pStr;

    while (q < p)
    {
        char t = *q;
        *q = *p;
        *p = t;
        q++;
        p--;
    }
    return (char*)pStr;
}
char* _cstrstr(const char* pStr, const char* pCharSet)
{
    int nLen = strlen(pCharSet);
    if (nLen == 0)
        return (char*)pStr;

    const char* pRet = NULL;
    const char* pCur = pStr;
    while((pStr = _cstrchr(pCur, *pCharSet)) != NULL)
    {
        if(memcmp(pCur, pCharSet, nLen * sizeof(char)) == 0)
        {
            pRet = pCur;
            break;
        }
        pCur = NextChar(pCur);
    }
    return (char*) pRet;
}
//指向下一个字符，对于多字节来说，指针可能要加两次
inline LPSTR NextChar(LPCSTR p)
{
    if((byte)*p == 0)
        return (LPSTR)p;
    else if((byte)*(p+1) <= 128)    // 考虑到ascii=128为欧元符号
        return (LPSTR)(p+1);
    else                            // 多字节字符
        return (LPSTR)(p+2);
}
