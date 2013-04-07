/****************************************************************************\
*                                                                            *
*  ISE (Iris Server Engine) Project                                          *
*  http://github.com/haoxingeng/ise                                          *
*                                                                            *
*  Copyright 2013 HaoXinGeng (haoxingeng@gmail.com)                          *
*  All rights reserved.                                                      *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
\****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// ise_classes.h
// Classes:
//   * CBuffer
//   * CDateTime
//   * CAutoInvokable
//   * CAutoInvoker
//   * CAutoLocker
//   * CCriticalSection
//   * CSemaphore
//   * CSignalMasker
//   * CSeqNumberAlloc
//   * CStream
//   * CMemoryStream
//   * CFileStream
//   * CList
//   * CPropertyList
//   * CStrings
//   * CStrList
//   * CCustomObjectList
//   * CObjectList
//   * CUrl
//   * CPacket
//   * IEventHandler
//   * CEventInvoker
//   * CCallBackDef
//   * CCallBackList
//   * CCustomParams
//   * CLogger
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_CLASSES_H_
#define _ISE_CLASSES_H_

#include "ise_options.h"

#ifdef ISE_WIN32
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#endif

#ifdef ISE_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/timeb.h>
#include <sys/file.h>
#include <iostream>
#include <fstream>
#include <string>
#endif

#include "ise_global_defs.h"
#include "ise_errmsgs.h"
#include "ise_exceptions.h"

using namespace std;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class declares

class CBuffer;
class CDateTime;
class CAutoInvoker;
class CAutoInvokable;
class CCriticalSection;
class CSemaphore;
class CSignalMasker;
class CSeqNumberAlloc;
class CStream;
class CMemoryStream;
class CFileStream;
class CList;
class CPropertyList;
class CStrings;
class CStrList;
class CUrl;
class CPacket;
template<typename ObjectType> class CCustomObjectList;
template<typename ObjectType> class CObjectList;
template<typename CallBackType> class CCallBackDef;
template<typename CallBackType> class CCallBackList;
class CCustomParams;
class CLogger;

///////////////////////////////////////////////////////////////////////////////
// class CBuffer - ������

class CBuffer
{
protected:
	void *m_pBuffer;
	int m_nSize;
	int m_nPosition;
private:
	inline void Init() { m_pBuffer = NULL; m_nSize = 0; m_nPosition = 0; }
	void Assign(const CBuffer& src);
	void VerifyPosition();
public:
	CBuffer();
	CBuffer(const CBuffer& src);
	explicit CBuffer(int nSize);
	CBuffer(const void *pBuffer, int nSize);
	virtual ~CBuffer();

	CBuffer& operator = (const CBuffer& rhs);
	const char& operator[] (int nIndex) const { return ((char*)m_pBuffer)[nIndex]; }
	char& operator[] (int nIndex) { return const_cast<char&>(((const CBuffer&)(*this))[nIndex]); }
	operator char*() const { return (char*)m_pBuffer; }
	char* Data() const { return (char*)m_pBuffer; }
	char* c_str() const;
	void Assign(const void *pBuffer, int nSize);
	void Clear() { SetSize(0); }
	void SetSize(int nSize, bool bInitZero = false);
	int GetSize() const { return m_nSize; }
	void EnsureSize(int nSize) { if (GetSize() < nSize) SetSize(nSize); }
	void SetPosition(int nPosition);
	int GetPosition() const { return m_nPosition; }

	bool LoadFromStream(CStream& Stream);
	bool LoadFromFile(const string& strFileName);
	bool SaveToStream(CStream& Stream);
	bool SaveToFile(const string& strFileName);
};

///////////////////////////////////////////////////////////////////////////////
// class CDateTime - ����ʱ����

class CDateTime
{
private:
	time_t m_tTime;     // (��1970-01-01 00:00:00 �����������UTCʱ��)

public:
	CDateTime() { m_tTime = 0; }
	CDateTime(const CDateTime& src) { m_tTime = src.m_tTime; }
	explicit CDateTime(time_t src) { m_tTime = src; }
	explicit CDateTime(const string& src) { *this = src; }

	static CDateTime CurrentDateTime();

	CDateTime& operator = (const CDateTime& rhs)
		{ m_tTime = rhs.m_tTime; return *this; }
	CDateTime& operator = (const time_t rhs)
		{ m_tTime = rhs; return *this; }
	CDateTime& operator = (const string& strDateTime);

	CDateTime operator + (const CDateTime& rhs) const { return CDateTime(m_tTime + rhs.m_tTime); }
	CDateTime operator + (time_t rhs) const { return CDateTime(m_tTime + rhs); }
	CDateTime operator - (const CDateTime& rhs) const { return CDateTime(m_tTime - rhs.m_tTime); }
	CDateTime operator - (time_t rhs) const { return CDateTime(m_tTime - rhs); }

	bool operator == (const CDateTime& rhs) const { return m_tTime == rhs.m_tTime; }
	bool operator != (const CDateTime& rhs) const { return m_tTime != rhs.m_tTime; }
	bool operator > (const CDateTime& rhs) const  { return m_tTime > rhs.m_tTime; }
	bool operator < (const CDateTime& rhs) const  { return m_tTime < rhs.m_tTime; }
	bool operator >= (const CDateTime& rhs) const { return m_tTime >= rhs.m_tTime; }
	bool operator <= (const CDateTime& rhs) const { return m_tTime <= rhs.m_tTime; }

	operator time_t() const { return m_tTime; }

	void EncodeDateTime(int nYear, int nMonth, int nDay,
		int nHour = 0, int nMinute = 0, int nSecond = 0);
	void DecodeDateTime(int *pYear, int *pMonth, int *pDay,
		int *pHour, int *pMinute, int *pSecond,
		int *pWeekDay = NULL, int *pYearDay = NULL) const;

	string DateString(const string& strDateSep = "-") const;
	string DateTimeString(const string& strDateSep = "-",
		const string& strDateTimeSep = " ", const string& strTimeSep = ":") const;
};

///////////////////////////////////////////////////////////////////////////////
// class CAutoInvokable/CAutoInvoker - �Զ���������/�Զ�������
//
// ˵��:
// 1. ������������ʹ�ã������𵽺� "����ָ��" ���Ƶ����ã�������ջ�����Զ����ٵ����ԣ���ջ
//    ����������������Զ����� CAutoInvokable::InvokeInitialize() �� InvokeFinalize()��
//    �˶���һ��ʹ������Ҫ��Դ�ĶԳ��Բ�������(�������/����)��
// 2. ʹ������̳� CAutoInvokable �࣬��д InvokeInitialize() �� InvokeFinalize()
//    ������������Ҫ���õĵط����� CAutoInvoker ��ջ����

class CAutoInvokable
{
public:
	friend class CAutoInvoker;

protected:
	virtual void InvokeInitialize() {}
	virtual void InvokeFinalize() {}
public:
	virtual ~CAutoInvokable() {}
};

class CAutoInvoker
{
private:
	CAutoInvokable *m_pObject;
public:
	explicit CAutoInvoker(CAutoInvokable& Object)
		{ m_pObject = &Object; m_pObject->InvokeInitialize(); }

	explicit CAutoInvoker(CAutoInvokable *pObject)
		{ m_pObject = pObject; if (m_pObject) m_pObject->InvokeInitialize(); }

	virtual ~CAutoInvoker()
		{ if (m_pObject) m_pObject->InvokeFinalize(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CAutoLocker - �߳��Զ�������
//
// ˵��:
// 1. ��������C++��ջ�����Զ����ٵ����ԣ��ڶ��̻߳����½��оֲ���Χ�ٽ������⣻
// 2. ʹ�÷���: ����Ҫ����ķ�Χ���Ծֲ�������ʽ���������󼴿ɣ�
//
// ʹ�÷���:
//   �����Ѷ���: CCriticalSection m_Lock;
//   �Զ������ͽ���:
//   {
//       CAutoLocker Locker(m_Lock);
//       //...
//   }

typedef CAutoInvoker CAutoLocker;

///////////////////////////////////////////////////////////////////////////////
// class CCriticalSection - �߳��ٽ���������
//
// ˵��:
// 1. �������ڶ��̻߳������ٽ������⣬���������� Lock��Unlock �� TryLock��
// 2. �߳�������Ƕ�׵��� Lock��Ƕ�׵��ú���������ͬ������ Unlock �ſɽ�����

class CCriticalSection : public CAutoInvokable
{
private:
#ifdef ISE_WIN32
	CRITICAL_SECTION m_Lock;
#endif
#ifdef ISE_LINUX
	pthread_mutex_t m_Lock;
#endif

protected:
	virtual void InvokeInitialize() { Lock(); }
	virtual void InvokeFinalize() { Unlock(); }

public:
	CCriticalSection();
	virtual ~CCriticalSection();

	// ����
	void Lock();
	// ����
	void Unlock();
	// ���Լ��� (���Ѿ����ڼ���״̬���������� false)
	bool TryLock();
};

///////////////////////////////////////////////////////////////////////////////
// class CSemaphore - �߳������

class CSemaphore
{
private:
#ifdef ISE_WIN32
	HANDLE m_Sem;
#endif
#ifdef ISE_LINUX
	sem_t m_Sem;
#endif

	UINT m_nInitValue;
private:
	void DoCreateSem();
	void DoDestroySem();
public:
	explicit CSemaphore(UINT nInitValue = 0);
	virtual ~CSemaphore();

	void Increase();
	void Wait();
	void Reset();
};

///////////////////////////////////////////////////////////////////////////////
// class CSignalMasker - �ź�������

#ifdef ISE_LINUX
class CSignalMasker
{
private:
	sigset_t m_OldSet;
	sigset_t m_NewSet;
	bool m_bBlock;
	bool m_bAutoRestore;

	int SigProcMask(int nHow, const sigset_t *pNewSet, sigset_t *pOldSet);
public:
	explicit CSignalMasker(bool bAutoRestore = false);
	virtual ~CSignalMasker();

	// ���� Block/UnBlock ����������źż���
	void SetSignals(int nSigCount, ...);
	void SetSignals(int nSigCount, va_list argList);

	// �ڽ��̵�ǰ�����źż������ SetSignals ���õ��ź�
	void Block();
	// �ڽ��̵�ǰ�����źż��н�� SetSignals ���õ��ź�
	void UnBlock();

	// �����������źż��ָ�Ϊ Block/UnBlock ֮ǰ��״̬
	void Restore();
};
#endif

///////////////////////////////////////////////////////////////////////////////
// class CSeqNumberAlloc - �������кŷ�������
//
// ˵��:
// 1. �������̰߳�ȫ��ʽ����һ�����ϵ������������У��û�����ָ�����е���ʼֵ��
// 2. ����һ���������ݰ���˳��ſ��ƣ�

class CSeqNumberAlloc
{
private:
	CCriticalSection m_Lock;
	UINT m_nCurrentId;

public:
	explicit CSeqNumberAlloc(UINT nStartId = 0);

	// ����һ���·����ID
	UINT AllocId();
};

///////////////////////////////////////////////////////////////////////////////
// class CStream - �� ����

enum SEEK_ORIGIN
{
	SO_BEGINNING    = 0,
	SO_CURRENT      = 1,
	SO_END          = 2
};

class CStream
{
public:
	virtual ~CStream() {}

	virtual int Read(void *pBuffer, int nCount) = 0;
	virtual int Write(const void *pBuffer, int nCount) = 0;
	virtual INT64 Seek(INT64 nOffset, SEEK_ORIGIN nSeekOrigin) = 0;

	void ReadBuffer(void *pBuffer, int nCount);
	void WriteBuffer(const void *pBuffer, int nCount);

	INT64 GetPosition() { return Seek(0, SO_CURRENT); }
	void SetPosition(INT64 nPos) { Seek(nPos, SO_BEGINNING); }

	virtual INT64 GetSize();
	virtual void SetSize(INT64 nSize) {}
};

///////////////////////////////////////////////////////////////////////////////
// class CMemoryStream - �ڴ�����

class CMemoryStream : public CStream
{
public:
	enum { DEFAULT_MEMORY_DELTA = 1024 };    // ȱʡ�ڴ��������� (�ֽ����������� 2 �� N �η�)
	enum { MIN_MEMORY_DELTA = 256 };         // ��С�ڴ���������

private:
	char *m_pMemory;
	int m_nCapacity;
	int m_nSize;
	int m_nPosition;
	int m_nMemoryDelta;
private:
	void SetMemoryDelta(int nNewMemoryDelta);
	void SetPointer(char *pMemory, int nSize);
	void SetCapacity(int nNewCapacity);
	char* Realloc(int& nNewCapacity);
public:
	explicit CMemoryStream(int nMemoryDelta = DEFAULT_MEMORY_DELTA);
	virtual ~CMemoryStream();

	virtual int Read(void *pBuffer, int nCount);
	virtual int Write(const void *pBuffer, int nCount);
	virtual INT64 Seek(INT64 nOffset, SEEK_ORIGIN nSeekOrigin);
	virtual void SetSize(INT64 nSize);
	bool LoadFromStream(CStream& Stream);
	bool LoadFromFile(const string& strFileName);
	bool SaveToStream(CStream& Stream);
	bool SaveToFile(const string& strFileName);
	void Clear();
	char* GetMemory() { return m_pMemory; }
};

///////////////////////////////////////////////////////////////////////////////
// class CFileStream - �ļ�����

// �ļ����򿪷�ʽ (UINT nOpenMode)
#ifdef ISE_WIN32
enum
{
	FM_CREATE           = 0xFFFF,
	FM_OPEN_READ        = 0x0000,
	FM_OPEN_WRITE       = 0x0001,
	FM_OPEN_READ_WRITE  = 0x0002,

	FM_SHARE_EXCLUSIVE  = 0x0010,
	FM_SHARE_DENY_WRITE = 0x0020,
	FM_SHARE_DENY_NONE  = 0x0040
};
#endif
#ifdef ISE_LINUX
enum
{
	FM_CREATE           = 0xFFFF,
	FM_OPEN_READ        = O_RDONLY,  // 0
	FM_OPEN_WRITE       = O_WRONLY,  // 1
	FM_OPEN_READ_WRITE  = O_RDWR,    // 2

	FM_SHARE_EXCLUSIVE  = 0x0010,
	FM_SHARE_DENY_WRITE = 0x0020,
	FM_SHARE_DENY_NONE  = 0x0030
};
#endif

// ȱʡ�ļ���ȡȨ�� (nRights)
#ifdef ISE_WIN32
enum { DEFAULT_FILE_ACCESS_RIGHTS = 0 };
#endif
#ifdef ISE_LINUX
enum { DEFAULT_FILE_ACCESS_RIGHTS = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH };
#endif

class CFileStream : public CStream
{
private:
	string m_strFileName;
	HANDLE m_hHandle;
private:
	void Init();
	HANDLE FileCreate(const string& strFileName, UINT nRights);
	HANDLE FileOpen(const string& strFileName, UINT nOpenMode);
	void FileClose(HANDLE hHandle);
	int FileRead(HANDLE hHandle, void *pBuffer, int nCount);
	int FileWrite(HANDLE hHandle, const void *pBuffer, int nCount);
	INT64 FileSeek(HANDLE hHandle, INT64 nOffset, SEEK_ORIGIN nSeekOrigin);
public:
	CFileStream();
	CFileStream(const string& strFileName, UINT nOpenMode, UINT nRights = DEFAULT_FILE_ACCESS_RIGHTS);
	virtual ~CFileStream();

	bool Open(const string& strFileName, UINT nOpenMode,
		UINT nRights = DEFAULT_FILE_ACCESS_RIGHTS, CFileException* pException = NULL);
	void Close();

	virtual int Read(void *pBuffer, int nCount);
	virtual int Write(const void *pBuffer, int nCount);
	virtual INT64 Seek(INT64 nOffset, SEEK_ORIGIN nSeekOrigin);
	virtual void SetSize(INT64 nSize);

	string GetFileName() const { return m_strFileName; }
	HANDLE GetHandle() const { return m_hHandle; }
	bool IsOpen() const;
};

///////////////////////////////////////////////////////////////////////////////
// class CList - �б���
//
// ˵��:
// 1. �����ʵ��ԭ���� Delphi::TList ��ȫ��ͬ��
// 2. ���б����������ŵ�:
//    a. ���з�������ȷ (STL���ޱ�ǿ�����Ի�ɬ)��
//    b. ֧���±������ȡ����Ԫ�� (STL::list��֧��)��
//    c. ֧�ֿ��ٻ�ȡ�б��� (STL::list��֧��)��
//    d. ֧��β��������ɾԪ�أ�
// 3. ���б���������ȱ��:
//    a. ��֧��ͷ�����в��Ŀ�����ɾԪ�أ�
//    b. ֻ֧�ֵ�һ����Ԫ��(Pointer����)��

class CList
{
private:
	POINTER *m_pList;
	int m_nCount;
	int m_nCapacity;

protected:
	virtual void Grow();

	POINTER Get(int nIndex) const;
	void Put(int nIndex, POINTER Item);
	void SetCapacity(int nNewCapacity);
	void SetCount(int nNewCount);

public:
	CList();
	virtual ~CList();

	int Add(POINTER Item);
	void Insert(int nIndex, POINTER Item);
	void Delete(int nIndex);
	int Remove(POINTER Item);
	POINTER Extract(POINTER Item);
	void Move(int nCurIndex, int nNewIndex);
	void Resize(int nCount);
	void Clear();

	POINTER First() const;
	POINTER Last() const;
	int IndexOf(POINTER Item) const;
	int GetCount() const;
	bool IsEmpty() const { return (GetCount() <= 0); }

	CList& operator = (const CList& rhs);
	const POINTER& operator[] (int nIndex) const;
	POINTER& operator[] (int nIndex);
};

///////////////////////////////////////////////////////////////////////////////
// class CPropertyList - �����б���
//
// ˵��:
// 1. �����б��е�ÿ����Ŀ��������(Name)������ֵ(Value)��ɡ�
// 2. �����������ظ��������ִ�Сд�������в��ɺ��еȺ�"="������ֵ��Ϊ����ֵ��

class CPropertyList
{
public:
	enum { NAME_VALUE_SEP = '=' };        // Name �� Value ֮��ķָ���
	enum { PROP_ITEM_SEP  = ',' };        // ������Ŀ֮��ķָ���
	enum { PROP_ITEM_QUOT = '"' };

	struct CPropertyItem
	{
		string strName, strValue;
	public:
		CPropertyItem(const CPropertyItem& src) :
			strName(src.strName), strValue(src.strValue) {}
		CPropertyItem(const string& strNameA, const string& strValueA) :
			strName(strNameA), strValue(strValueA) {}
	};

private:
	CList m_Items;                        // (CPropertyItem* [])
private:
	CPropertyItem* Find(const string& strName);
	static bool IsReservedChar(char ch);
	static bool HasReservedChar(const string& str);
	static char* ScanStr(char *pStr, char ch);
	static string MakeQuotedStr(const string& str);
	static string ExtractQuotedStr(char*& pStr);
public:
	CPropertyList();
	virtual ~CPropertyList();

	void Add(const string& strName, const string& strValue);
	bool Remove(const string& strName);
	void Clear();
	int IndexOf(const string& strName) const;
	bool NameExists(const string& strName) const;
	bool GetValue(const string& strName, string& strValue) const;
	int GetCount() const { return m_Items.GetCount(); }
	bool IsEmpty() const { return (GetCount() <= 0); }
	const CPropertyItem& GetItems(int nIndex) const;
	string GetPropString() const;
	void SetPropString(const string& strPropString);

	CPropertyList& operator = (const CPropertyList& rhs);
	string& operator[] (const string& strName);
};

///////////////////////////////////////////////////////////////////////////////
// class CStrings - �ַ����б������

class CStrings
{
private:
	enum STRINGS_DEFINED
	{
		SD_DELIMITER         = 0x0001,
		SD_QUOTE_CHAR        = 0x0002,
		SD_NAME_VALUE_SEP    = 0x0004,
		SD_LINE_BREAK        = 0x0008
	};

public:
	// Calls BeginUpdate() and EndUpdate() automatically in a scope.
	class CAutoUpdater
	{
	private:
		CStrings& m_Strings;
	public:
		CAutoUpdater(CStrings& Strings) : m_Strings(Strings)
			{ m_Strings.BeginUpdate(); }
		~CAutoUpdater()
			{ m_Strings.EndUpdate(); }
	};

protected:
	UINT m_nDefined;
	char m_chDelimiter;
	string m_strLineBreak;
	char m_chQuoteChar;
	char m_chNameValueSeparator;
	int m_nUpdateCount;
private:
	void Assign(const CStrings& src);
protected:
	void Init();
	void Error(const char* lpszMsg, int nData) const;
	int GetUpdateCount() const { return m_nUpdateCount; }
	string ExtractName(const char* lpszStr) const;
protected:
	virtual void SetUpdateState(bool bUpdating) {}
	virtual int CompareStrings(const char* str1, const char* str2) const;
public:
	CStrings();
	virtual ~CStrings() {}

	virtual int Add(const char* lpszStr);
	virtual int Add(const char* lpszStr, POINTER pData);
	virtual void AddStrings(const CStrings& Strings);
	virtual void Insert(int nIndex, const char* lpszStr) = 0;
	virtual void Insert(int nIndex, const char* lpszStr, POINTER pData);
	virtual void Clear() = 0;
	virtual void Delete(int nIndex) = 0;
	virtual bool Equals(const CStrings& Strings);
	virtual void Exchange(int nIndex1, int nIndex2);
	virtual void Move(int nCurIndex, int nNewIndex);
	virtual bool Exists(const char* lpszStr) const;
	virtual int IndexOf(const char* lpszStr) const;
	virtual int IndexOfName(const char* lpszName) const;
	virtual int IndexOfData(POINTER pData) const;

	virtual bool LoadFromStream(CStream& Stream);
	virtual bool LoadFromFile(const char* lpszFileName);
	virtual bool SaveToStream(CStream& Stream) const;
	virtual bool SaveToFile(const char* lpszFileName) const;

	virtual int GetCapacity() const { return GetCount(); }
	virtual void SetCapacity(int nValue) {}
	virtual int GetCount() const = 0;
	bool IsEmpty() const { return (GetCount() <= 0); }
	char GetDelimiter() const;
	void SetDelimiter(char chValue);
	string GetLineBreak() const;
	void SetLineBreak(const char* lpszValue);
	char GetQuoteChar() const;
	void SetQuoteChar(char chValue);
	char GetNameValueSeparator() const;
	void SetNameValueSeparator(char chValue);
	string CombineNameValue(const char* lpszName, const char* lpszValue) const;
	string GetName(int nIndex) const;
	string GetValue(const char* lpszName) const;
	string GetValue(int nIndex) const;
	void SetValue(const char* lpszName, const char* lpszValue);
	void SetValue(int nIndex, const char* lpszValue);
	virtual POINTER GetData(int nIndex) const { return NULL; }
	virtual void SetData(int nIndex, POINTER pData) {}
	virtual string GetText() const;
	virtual void SetText(const char* lpszValue);
	string GetCommaText() const;
	void SetCommaText(const char* lpszValue);
	string GetDelimitedText() const;
	void SetDelimitedText(const char* lpszValue);
	virtual const string& GetString(int nIndex) const = 0;
	virtual void SetString(int nIndex, const char* lpszValue);

	void BeginUpdate();
	void EndUpdate();

	CStrings& operator = (const CStrings& rhs);
	const string& operator[] (int nIndex) const { return GetString(nIndex); }
};

///////////////////////////////////////////////////////////////////////////////
// class CStrList - �ַ����б���

class CStrList : public CStrings
{
public:
	friend int StringListCompareProc(const CStrList& StringList, int nIndex1, int nIndex2);

public:
	/// The comparison function prototype that used by Sort().
	typedef int (*STRINGLIST_COMPARE_PROC)(const CStrList& StringList, int nIndex1, int nIndex2);

	/// Indicates the response when an application attempts to add a duplicate entry to a list.
	enum DUPLICATE_MODE
	{
		DM_IGNORE,      // Ignore attempts to add duplicate entries (do not add the duplicate).
		DM_ACCEPT,      // Allow the list to contain duplicate entries (add the duplicate).
		DM_ERROR        // Throw an exception when the application tries to add a duplicate.
	};

private:
	struct CStringItem
	{
		string *pStr;
		POINTER pData;
	};

private:
	CStringItem *m_pList;
	int m_nCount;
	int m_nCapacity;
	DUPLICATE_MODE m_nDupMode;
	bool m_bSorted;
	bool m_bCaseSensitive;
private:
	void Init();
	void Assign(const CStrList& src);
	void InternalClear();
	string& StringObjectNeeded(int nIndex) const;
	void ExchangeItems(int nIndex1, int nIndex2);
	void Grow();
	void QuickSort(int l, int r, STRINGLIST_COMPARE_PROC pfnCompareProc);
protected: // override
	virtual void SetUpdateState(bool bUpdating);
	virtual int CompareStrings(const char* str1, const char* str2) const;
protected: // virtual
	/// Occurs immediately before the list of strings changes.
	virtual void OnChanging() {}
	/// Occurs immediately after the list of strings changes.
	virtual void OnChanged() {}
	/// Internal method used to insert a string to the list.
	virtual void InsertItem(int nIndex, const char* lpszStr, POINTER pData);
public:
	CStrList();
	CStrList(const CStrList& src);
	virtual ~CStrList();

	virtual int Add(const char* lpszStr);
	virtual int Add(const char* lpszStr, POINTER pData);
	virtual void Clear();
	virtual void Delete(int nIndex);
	virtual void Exchange(int nIndex1, int nIndex2);
	virtual int IndexOf(const char* lpszStr) const;
	virtual void Insert(int nIndex, const char* lpszStr);
	virtual void Insert(int nIndex, const char* lpszStr, POINTER pData);

	virtual int GetCapacity() const { return m_nCapacity; }
	virtual void SetCapacity(int nValue);
	virtual int GetCount() const { return m_nCount; }
	virtual POINTER GetData(int nIndex) const;
	virtual void SetData(int nIndex, POINTER pData);
	virtual const string& GetString(int nIndex) const;
	virtual void SetString(int nIndex, const char* lpszValue);

	virtual bool Find(const char* lpszStr, int& nIndex) const;
	virtual void Sort();
	virtual void Sort(STRINGLIST_COMPARE_PROC pfnCompareProc);

	DUPLICATE_MODE GetDupMode() const { return m_nDupMode; }
	void SetDupMode(DUPLICATE_MODE nValue) { m_nDupMode = nValue; }
	bool GetSorted() const { return m_bSorted; }
	virtual void SetSorted(bool bValue);
	bool GetCaseSensitive() const { return m_bCaseSensitive; }
	virtual void SetCaseSensitive(bool bValue);

	CStrList& operator = (const CStrList& rhs);
};

///////////////////////////////////////////////////////////////////////////////
// class CUrl - URL������

class CUrl
{
public:
	// The URL parts.
	enum URL_PART
	{
		URL_PROTOCOL  = 0x0001,
		URL_HOST      = 0x0002,
		URL_PORT      = 0x0004,
		URL_PATH      = 0x0008,
		URL_FILENAME  = 0x0010,
		URL_BOOKMARK  = 0x0020,
		URL_USERNAME  = 0x0040,
		URL_PASSWORD  = 0x0080,
		URL_PARAMS    = 0x0100,
		URL_ALL       = 0xFFFF,
	};

private:
	string m_strProtocol;
	string m_strHost;
	string m_strPort;
	string m_strPath;
	string m_strFileName;
	string m_strBookmark;
	string m_strUserName;
	string m_strPassword;
	string m_strParams;
public:
	CUrl(const string& strUrl = "");
	CUrl(const CUrl& src);
	virtual ~CUrl() {}

	void Clear();
	CUrl& operator = (const CUrl& rhs);

	string GetUrl() const;
	string GetUrl(UINT nParts);
	void SetUrl(const string& strValue);

	const string& GetProtocol() const { return m_strProtocol; }
	const string& GetHost() const { return m_strHost; }
	const string& GetPort() const { return m_strPort; }
	const string& GetPath() const { return m_strPath; }
	const string& GetFileName() const { return m_strFileName; }
	const string& GetBookmark() const { return m_strBookmark; }
	const string& GetUserName() const { return m_strUserName; }
	const string& GetPassword() const { return m_strPassword; }
	const string& GetParams() const { return m_strParams; }

	void SetProtocol(const string& strValue) { m_strProtocol = strValue; }
	void SetHost(const string& strValue) { m_strHost = strValue; }
	void SetPort(const string& strValue) { m_strPort = strValue; }
	void SetPath(const string& strValue) { m_strPath = strValue; }
	void SetFileName(const string& strValue) { m_strFileName = strValue; }
	void SetBookmark(const string& strValue) { m_strBookmark = strValue; }
	void SetUserName(const string& strValue) { m_strUserName = strValue; }
	void SetPassword(const string& strValue) { m_strPassword = strValue; }
	void SetParams(const string& strValue) { m_strParams = strValue; }
};

///////////////////////////////////////////////////////////////////////////////
// class CPacket - ���ݰ�����

class CPacket
{
public:
	// ȱʡ�ڴ��������� (�ֽ����������� 2 �� N �η�)
	enum { DEFAULT_MEMORY_DELTA = 1024 };

private:
	void Init();
protected:
	CMemoryStream *m_pStream;
	bool m_bAvailable;
	bool m_bIsPacked;
protected:
	void ThrowUnpackError();
	void ThrowPackError();
	void CheckUnsafeSize(int nValue);

	void ReadBuffer(void *pBuffer, int nBytes);
	void ReadINT8(INT8& nValue) { ReadBuffer(&nValue, sizeof(INT8)); }
	void ReadINT16(INT16& nValue) { ReadBuffer(&nValue, sizeof(INT16)); }
	void ReadINT32(INT32& nValue) { ReadBuffer(&nValue, sizeof(INT32)); }
	void ReadINT64(INT64& nValue) { ReadBuffer(&nValue, sizeof(INT64)); }
	void ReadString(std::string& str);
	void ReadBlob(std::string& str);
	void ReadBlob(CStream& Stream);
	void ReadBlob(CBuffer& Buffer);
	INT8 ReadINT8() { INT8 v; ReadINT8(v); return v; }
	INT16 ReadINT16() { INT16 v; ReadINT16(v); return v; }
	INT32 ReadINT32() { INT32 v; ReadINT32(v); return v; }
	INT64 ReadINT64() { INT64 v; ReadINT64(v); return v; }
	bool ReadBool() { INT8 v; ReadINT8(v); return (v? true : false); }
	std::string ReadString() { std::string v; ReadString(v); return v; }

	void WriteBuffer(const void *pBuffer, int nBytes);
	void WriteINT8(const INT8& nValue) { WriteBuffer(&nValue, sizeof(INT8)); }
	void WriteINT16(const INT16& nValue) { WriteBuffer(&nValue, sizeof(INT16)); }
	void WriteINT32(const INT32& nValue) { WriteBuffer(&nValue, sizeof(INT32)); }
	void WriteINT64(const INT64& nValue) { WriteBuffer(&nValue, sizeof(INT64)); }
	void WriteBool(bool bValue) { WriteINT8(bValue ? 1 : 0); }
	void WriteString(const std::string& str);
	void WriteBlob(void *pBuffer, int nBytes);
	void WriteBlob(const CBuffer& Buffer);

	void FixStrLength(std::string& str, int nLength);
	void TruncString(std::string& str, int nMaxLength);
protected:
	virtual void DoPack() {}
	virtual void DoUnpack() {}
	virtual void DoAfterPack() {}
	virtual void DoEncrypt() {}
	virtual void DoDecrypt() {}
	virtual void DoCompress() {}
	virtual void DoDecompress() {}
public:
	CPacket();
	virtual ~CPacket();

	bool Pack();
	bool Unpack(void *pBuffer, int nBytes);
	bool Unpack(const CBuffer& Buffer);
	void Clear();
	void EnsurePacked();

	char* GetBuffer() const { return (m_pStream? (char*)m_pStream->GetMemory() : NULL); }
	int GetSize() const { return (m_pStream? (int)m_pStream->GetSize() : 0); }
	bool Available() const { return m_bAvailable; }
	bool IsPacked() const { return m_bIsPacked; }
};

///////////////////////////////////////////////////////////////////////////////
// class CCustomObjectList - �����б����

template<typename ObjectType>
class CCustomObjectList
{
public:
	class CLock : public CAutoInvokable
	{
	private:
		CCriticalSection *m_pLock;
	protected:
		virtual void InvokeInitialize() { if (m_pLock) m_pLock->Lock(); }
		virtual void InvokeFinalize() { if (m_pLock) m_pLock->Unlock(); }
	public:
		CLock(bool bActive) : m_pLock(NULL) { if (bActive) m_pLock = new CCriticalSection(); }
		virtual ~CLock() { delete m_pLock; }
	};

	typedef ObjectType* ObjectPtr;

protected:
	CList m_Items;            // �����б�
	bool m_bOwnsObjects;      // Ԫ�ر�ɾ��ʱ���Ƿ��Զ��ͷ�Ԫ�ض���
	mutable CLock m_Lock;

protected:
	virtual void NotifyDelete(int nIndex)
	{
		if (m_bOwnsObjects)
		{
			ObjectPtr pItem = (ObjectPtr)m_Items[nIndex];
			m_Items[nIndex] = NULL;
			delete pItem;
		}
	}
protected:
	int Add(ObjectPtr pItem, bool bAllowDuplicate = true)
	{
		ISE_ASSERT(pItem);
		CAutoLocker Locker(m_Lock);

		if (bAllowDuplicate || m_Items.IndexOf(pItem) == -1)
			return m_Items.Add(pItem);
		else
			return -1;
	}

	int Remove(ObjectPtr pItem)
	{
		CAutoLocker Locker(m_Lock);

		int nResult = m_Items.IndexOf(pItem);
		if (nResult >= 0)
		{
			NotifyDelete(nResult);
			m_Items.Delete(nResult);
		}
		return nResult;
	}

	ObjectPtr Extract(ObjectPtr pItem)
	{
		CAutoLocker Locker(m_Lock);

		ObjectPtr pResult = NULL;
		int i = m_Items.Remove(pItem);
		if (i >= 0)
			pResult = pItem;
		return pResult;
	}

	ObjectPtr Extract(int nIndex)
	{
		CAutoLocker Locker(m_Lock);

		ObjectPtr pResult = NULL;
		if (nIndex >= 0 && nIndex < m_Items.GetCount())
		{
			pResult = (ObjectPtr)m_Items[nIndex];
			m_Items.Delete(nIndex);
		}
		return pResult;
	}

	void Delete (int nIndex)
	{
		CAutoLocker Locker(m_Lock);

		if (nIndex >= 0 && nIndex < m_Items.GetCount())
		{
			NotifyDelete(nIndex);
			m_Items.Delete(nIndex);
		}
	}

	void Insert(int nIndex, ObjectPtr pItem)
	{
		ISE_ASSERT(pItem);
		CAutoLocker Locker(m_Lock);
		m_Items.Insert(nIndex, pItem);
	}

	int IndexOf(ObjectPtr pItem) const
	{
		CAutoLocker Locker(m_Lock);
		return m_Items.IndexOf(pItem);
	}

	bool Exists(ObjectPtr pItem) const
	{
		CAutoLocker Locker(m_Lock);
		return m_Items.IndexOf(pItem) >= 0;
	}

	ObjectPtr First() const
	{
		CAutoLocker Locker(m_Lock);
		return (ObjectPtr)m_Items.First();
	}

	ObjectPtr Last() const
	{
		CAutoLocker Locker(m_Lock);
		return (ObjectPtr)m_Items.Last();
	}

	void Clear()
	{
		CAutoLocker Locker(m_Lock);

		for (int i = m_Items.GetCount() - 1; i >= 0; i--)
			NotifyDelete(i);
		m_Items.Clear();
	}

	void FreeObjects()
	{
		CAutoLocker Locker(m_Lock);

		for (int i = m_Items.GetCount() - 1; i >= 0; i--)
		{
			ObjectPtr pItem = (ObjectPtr)m_Items[i];
			m_Items[i] = NULL;
			delete pItem;
		}
	}

	int GetCount() const { return m_Items.GetCount(); }
	ObjectPtr& GetItem(int nIndex) const { return (ObjectPtr&)m_Items[nIndex]; }
	CCustomObjectList& operator = (const CCustomObjectList& rhs) { m_Items = rhs.m_Items; return *this; }
	ObjectPtr& operator[] (int nIndex) const { return GetItem(nIndex); }
	bool IsEmpty() const { return (GetCount() <= 0); }
	void SetOwnsObjects(bool bValue) { m_bOwnsObjects = bValue; }
public:
	CCustomObjectList() :
	  m_Lock(false), m_bOwnsObjects(true) {}

	CCustomObjectList(bool bThreadSafe, bool bOwnsObjects) :
	m_Lock(bThreadSafe), m_bOwnsObjects(bOwnsObjects) {}

	virtual ~CCustomObjectList() { Clear(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CObjectList - �����б���

template<typename ObjectType>
class CObjectList : public CCustomObjectList<ObjectType>
{
public:
	CObjectList() :
		CCustomObjectList<ObjectType>(false, true) {}
	CObjectList(bool bThreadSafe, bool bOwnsObjects) :
		CCustomObjectList<ObjectType>(bThreadSafe, bOwnsObjects) {}
	virtual ~CObjectList() {}

	using CCustomObjectList<ObjectType>::Add;
	using CCustomObjectList<ObjectType>::Remove;
	using CCustomObjectList<ObjectType>::Extract;
	using CCustomObjectList<ObjectType>::Delete;
	using CCustomObjectList<ObjectType>::Insert;
	using CCustomObjectList<ObjectType>::IndexOf;
	using CCustomObjectList<ObjectType>::Exists;
	using CCustomObjectList<ObjectType>::First;
	using CCustomObjectList<ObjectType>::Last;
	using CCustomObjectList<ObjectType>::Clear;
	using CCustomObjectList<ObjectType>::FreeObjects;
	using CCustomObjectList<ObjectType>::GetCount;
	using CCustomObjectList<ObjectType>::GetItem;
	using CCustomObjectList<ObjectType>::operator=;
	using CCustomObjectList<ObjectType>::operator[];
	using CCustomObjectList<ObjectType>::IsEmpty;
	using CCustomObjectList<ObjectType>::SetOwnsObjects;
};

///////////////////////////////////////////////////////////////////////////////
// class IEventHandler/CEventInvoker - C++�¼�֧����

// ���¼���������
class CNullSender {};
class CNullParam {};

// �¼��������ӿ�
template<class SenderType, class ParamType>
class IEventHandler
{
public:
	virtual ~IEventHandler() {}
	virtual void HandleEvent(const SenderType& Sender, const ParamType& Param) = 0;
};

// �¼�������
template<class SenderType, class ParamType>
class CEventInvoker
{
public:
	typedef IEventHandler<SenderType, ParamType> EventHanderType;
private:
	CList m_HandlerList;       // (EventHanderType*)[]
public:
	virtual ~CEventInvoker() {}

	virtual void RegisterHandler(EventHanderType *pHandler)
	{
		if (pHandler && m_HandlerList.IndexOf(pHandler) == -1)
			m_HandlerList.Add(pHandler);
	}

	virtual void UnregisterHandler(EventHanderType *pHandler)
	{
		m_HandlerList.Remove(pHandler);
	}

	virtual void Invoke(const SenderType& Sender, const ParamType& Param)
	{
		for (register int i = 0; i < m_HandlerList.GetCount(); i++)
			((EventHanderType*)m_HandlerList[i])->HandleEvent(Sender, Param);
	}
};

///////////////////////////////////////////////////////////////////////////////
// class CCallBackDef - �ص�����

template<typename CallBackType>
class CCallBackDef
{
public:
	CallBackType pProc;
	void *pParam;
public:
	CCallBackDef() { Clear(); }
	CCallBackDef(CallBackType pProc_, void *pParam_) : pProc(pProc_), pParam(pParam_) {}
	void Clear() { pProc = NULL; pParam = NULL; }
};

///////////////////////////////////////////////////////////////////////////////
// class CCallBackList - �ص��б�

template<typename CallBackType>
class CCallBackList
{
public:
	typedef CCallBackDef<CallBackType> CALLBACK_ITEM;
	typedef CObjectList<CALLBACK_ITEM> CALLBACK_LIST;
private:
	CALLBACK_LIST m_Items;
private:
	bool Exists(CallBackType pProc)
	{
		for (int i = 0; i < m_Items.GetCount(); i++)
			if (m_Items[i]->pProc == pProc) return true;
		return false;
	}
public:
	CCallBackList() : m_Items(true, true) {}

	void Register(CallBackType pProc, void *pParam = NULL)
	{
		if (pProc && !Exists(pProc))
			m_Items.Add(new CALLBACK_ITEM(pProc, pParam));
	}

	int GetCount() const { return m_Items.GetCount(); }
	const CALLBACK_ITEM& GetItem(int nIndex) const { return *m_Items[nIndex]; }
};

///////////////////////////////////////////////////////////////////////////////
// class CCustomParams - �û��Զ������

class CCustomParams
{
public:
	enum { MAX_PARAM_COUNT = 8 };
private:
	int m_nCount;
	PVOID m_pParams[MAX_PARAM_COUNT];
private:
	inline void Init();
public:
	CCustomParams();
	CCustomParams(const CCustomParams& src);
	CCustomParams(int nCount, ...);

	bool Add(PVOID pValue);
	void Clear();
	int GetCount() const { return m_nCount; }

	CCustomParams& operator = (const CCustomParams& rhs);
	PVOID& operator[] (int nIndex);
	const PVOID& operator[] (int nIndex) const;
};

///////////////////////////////////////////////////////////////////////////////
// class CLogger - ��־��

class CLogger
{
private:
	string m_strFileName;        // ��־�ļ���
	bool m_bNewFileDaily;        // �Ƿ�ÿ����һ���������ļ��洢��־
	CCriticalSection m_Lock;
private:
	string GetLogFileName();
	bool OpenFile(CFileStream& FileStream, const string& strFileName);
	void WriteToFile(const string& strString);
private:
	CLogger();
public:
	~CLogger() {}
	static CLogger& Instance();

	void SetFileName(const string& strFileName, bool bNewFileDaily = false);

	void WriteStr(const char *sString);
	void WriteStr(const string& str) { WriteStr(str.c_str()); }
	void WriteFmt(const char *sFormatString, ...);
	void WriteException(const CException& e);
};

///////////////////////////////////////////////////////////////////////////////
// ��������

extern const CCustomParams EMPTY_PARAMS;

///////////////////////////////////////////////////////////////////////////////
// ȫ�ֺ���

inline CLogger& Logger() { return CLogger::Instance(); }

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_CLASSES_H_
