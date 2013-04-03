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
// �ļ�����: ise_classes.cpp
// ��������: ͨ�û������
///////////////////////////////////////////////////////////////////////////////

#include "ise_classes.h"
#include "ise_sysutils.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

union Int64Rec
{
	INT64 value;
	struct {
		INT32 lo;
		INT32 hi;
	} ints;
};

///////////////////////////////////////////////////////////////////////////////
// ��������

const CCustomParams EMPTY_PARAMS;

///////////////////////////////////////////////////////////////////////////////
// class CBuffer

CBuffer::CBuffer()
{
	Init();
}

//-----------------------------------------------------------------------------

CBuffer::CBuffer(const CBuffer& src)
{
	Init();
	Assign(src);
}

//-----------------------------------------------------------------------------

CBuffer::CBuffer(int nSize)
{
	Init();
	SetSize(nSize);
}

//-----------------------------------------------------------------------------

CBuffer::CBuffer(const void *pBuffer, int nSize)
{
	Init();
	Assign(pBuffer, nSize);
}

//-----------------------------------------------------------------------------

CBuffer::~CBuffer()
{
	if (m_pBuffer)
		free(m_pBuffer);
}

//-----------------------------------------------------------------------------

void CBuffer::Assign(const CBuffer& src)
{
	SetSize(src.GetSize());
	SetPosition(src.GetPosition());
	if (m_nSize > 0)
		memmove(m_pBuffer, src.m_pBuffer, m_nSize);
}

//-----------------------------------------------------------------------------

void CBuffer::VerifyPosition()
{
	if (m_nPosition < 0) m_nPosition = 0;
	if (m_nPosition > m_nSize) m_nPosition = m_nSize;
}

//-----------------------------------------------------------------------------

CBuffer& CBuffer::operator = (const CBuffer& rhs)
{
	if (this != &rhs)
		Assign(rhs);
	return *this;
}

//-----------------------------------------------------------------------------
// ����: �� pBuffer �е� nSize ���ֽڸ��Ƶ� *this �У�������С����Ϊ nSize
//-----------------------------------------------------------------------------
void CBuffer::Assign(const void *pBuffer, int nSize)
{
	SetSize(nSize);
	if (m_nSize > 0)
		memmove(m_pBuffer, pBuffer, m_nSize);
	VerifyPosition();
}

//-----------------------------------------------------------------------------
// ����: ���û����С
// ����:
//   nSize     - �»�������С
//   bInitZero - ���»������Ⱦɻ��������Ƿ񽫶���Ŀռ���'\0'���
// ��ע:
//   �µĻ���ᱣ��ԭ������
//-----------------------------------------------------------------------------
void CBuffer::SetSize(int nSize, bool bInitZero)
{
	if (nSize <= 0)
	{
		if (m_pBuffer) free(m_pBuffer);
		m_pBuffer = NULL;
		m_nSize = 0;
		m_nPosition = 0;
	}
	else if (nSize != m_nSize)
	{
		void *pNewBuf;

		// ��� m_pBuffer == NULL���� realloc �൱�� malloc��
		pNewBuf = realloc(m_pBuffer, nSize + 1);  // �����һ���ֽ����� c_str()!

		if (pNewBuf)
		{
			if (bInitZero && (nSize > m_nSize))
				memset(((char*)pNewBuf) + m_nSize, 0, nSize - m_nSize);
			m_pBuffer = pNewBuf;
			m_nSize = nSize;
			VerifyPosition();
		}
		else
		{
			IseThrowMemoryException();
		}
	}
}

//-----------------------------------------------------------------------------
// ����: ���� Position
//-----------------------------------------------------------------------------
void CBuffer::SetPosition(int nPosition)
{
	m_nPosition = nPosition;
	VerifyPosition();
}

//-----------------------------------------------------------------------------
// ����: ���� C �����ַ��� (ĩ�˸��ӽ����� '\0')
//-----------------------------------------------------------------------------
char* CBuffer::c_str() const
{
	if (m_nSize <= 0 || !m_pBuffer)
		return (char*)"";
	else
	{
		((char*)m_pBuffer)[m_nSize] = 0;
		return (char*)m_pBuffer;
	}
}

//-----------------------------------------------------------------------------
// ����: �����������뵽������
//-----------------------------------------------------------------------------
bool CBuffer::LoadFromStream(CStream& Stream)
{
	try
	{
		INT64 nSize64 = Stream.GetSize() - Stream.GetPosition();
		ISE_ASSERT(nSize64 <= MAXLONG);
		int nSize = (int)nSize64;

		SetPosition(0);
		SetSize(nSize);
		Stream.ReadBuffer(Data(), nSize);
		return true;
	}
	catch (CException&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// ����: ���ļ����뵽������
//-----------------------------------------------------------------------------
bool CBuffer::LoadFromFile(const string& strFileName)
{
	CFileStream fs;
	bool bResult = fs.Open(strFileName, FM_OPEN_READ | FM_SHARE_DENY_WRITE);
	if (bResult)
		bResult = LoadFromStream(fs);
	return bResult;
}

//-----------------------------------------------------------------------------
// ����: �����汣�浽��������
//-----------------------------------------------------------------------------
bool CBuffer::SaveToStream(CStream& Stream)
{
	try
	{
		if (GetSize() > 0)
			Stream.WriteBuffer(Data(), GetSize());
		return true;
	}
	catch (CException&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// ����: �����汣�浽�ļ���
//-----------------------------------------------------------------------------
bool CBuffer::SaveToFile(const string& strFileName)
{
	CFileStream fs;
	bool bResult = fs.Open(strFileName, FM_CREATE);
	if (bResult)
		bResult = SaveToStream(fs);
	return bResult;
}

///////////////////////////////////////////////////////////////////////////////
// class CDateTime

//-----------------------------------------------------------------------------
// ����: ���ص�ǰʱ�� (��1970-01-01 00:00:00 ���������, UTCʱ��)
//-----------------------------------------------------------------------------
CDateTime CDateTime::CurrentDateTime()
{
	return CDateTime(time(NULL));
}

//-----------------------------------------------------------------------------
// ����: ���ַ���ת���� CDateTime
// ע��: strDateTime �ĸ�ʽ����Ϊ YYYY-MM-DD HH:MM:SS
//-----------------------------------------------------------------------------
CDateTime& CDateTime::operator = (const string& strDateTime)
{
	int nYear, nMonth, nDay, nHour, nMinute, nSecond;

	if (strDateTime.length() == 19)
	{
		nYear = StrToInt(strDateTime.substr(0, 4), 0);
		nMonth = StrToInt(strDateTime.substr(5, 2), 0);
		nDay = StrToInt(strDateTime.substr(8, 2), 0);
		nHour = StrToInt(strDateTime.substr(11, 2), 0);
		nMinute = StrToInt(strDateTime.substr(14, 2), 0);
		nSecond = StrToInt(strDateTime.substr(17, 2), 0);

		EncodeDateTime(nYear, nMonth, nDay, nHour, nMinute, nSecond);
		return *this;
	}
	else
	{
		IseThrowException(SEM_INVALID_DATETIME_STR);
		return *this;
	}
}

//-----------------------------------------------------------------------------
// ����: ����ʱ����룬������ *this
//-----------------------------------------------------------------------------
void CDateTime::EncodeDateTime(int nYear, int nMonth, int nDay,
	int nHour, int nMinute, int nSecond)
{
	struct tm tm;

	tm.tm_year = nYear - 1900;
	tm.tm_mon = nMonth - 1;
	tm.tm_mday = nDay;
	tm.tm_hour = nHour;
	tm.tm_min = nMinute;
	tm.tm_sec = nSecond;

	m_tTime = mktime(&tm);
}

//-----------------------------------------------------------------------------
// ����: ����ʱ����룬�����������
//-----------------------------------------------------------------------------
void CDateTime::DecodeDateTime(int *pYear, int *pMonth, int *pDay,
	int *pHour, int *pMinute, int *pSecond, int *pWeekDay, int *pYearDay) const
{
	struct tm tm;

#ifdef ISE_WIN32
#ifdef ISE_COMPILER_VC
#if (_MSC_VER >= 1400)  // >= VC8
	localtime_s(&tm, &m_tTime);
#else
	struct tm *ptm = localtime(&m_tTime);    // ��������������
	if (ptm) tm = *ptm;
#endif
#else
	struct tm *ptm = localtime(&m_tTime);    // ��������������
	if (ptm) tm = *ptm;
#endif
#endif

#ifdef ISE_LINUX
	localtime_r(&m_tTime, &tm);
#endif

	if (pYear) *pYear = tm.tm_year + 1900;
	if (pMonth) *pMonth = tm.tm_mon + 1;
	if (pDay) *pDay = tm.tm_mday;
	if (pHour) *pHour = tm.tm_hour;
	if (pMinute) *pMinute = tm.tm_min;
	if (pSecond) *pSecond = tm.tm_sec;
	if (pWeekDay) *pWeekDay = tm.tm_wday;
	if (pYearDay) *pYearDay = tm.tm_yday;
}

//-----------------------------------------------------------------------------
// ����: ���������ַ���
// ����:
//   strDateSep - ���ڷָ���
// ��ʽ:
//   YYYY-MM-DD
//-----------------------------------------------------------------------------
string CDateTime::DateString(const string& strDateSep) const
{
	string strResult;
	int nYear, nMonth, nDay;

	DecodeDateTime(&nYear, &nMonth, &nDay, NULL, NULL, NULL, NULL);
	strResult = FormatString("%04d%s%02d%s%02d",
		nYear, strDateSep.c_str(), nMonth, strDateSep.c_str(), nDay);

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: ��������ʱ���ַ���
// ����:
//   strDateSep     - ���ڷָ���
//   strDateTimeSep - ���ں�ʱ��֮��ķָ���
//   strTimeSep     - ʱ��ָ���
// ��ʽ:
//   YYYY-MM-DD HH:MM:SS
//-----------------------------------------------------------------------------
string CDateTime::DateTimeString(const string& strDateSep,
	const string& strDateTimeSep, const string& strTimeSep) const
{
	string strResult;
	int nYear, nMonth, nDay, nHour, nMinute, nSecond;

	DecodeDateTime(&nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond, NULL);
	strResult = FormatString("%04d%s%02d%s%02d%s%02d%s%02d%s%02d",
		nYear, strDateSep.c_str(), nMonth, strDateSep.c_str(), nDay,
		strDateTimeSep.c_str(),
		nHour, strTimeSep.c_str(), nMinute, strTimeSep.c_str(), nSecond);

	return strResult;
}

///////////////////////////////////////////////////////////////////////////////
// class CCriticalSection

CCriticalSection::CCriticalSection()
{
#ifdef ISE_WIN32
	InitializeCriticalSection(&m_Lock);
#endif
#ifdef ISE_LINUX
	pthread_mutexattr_t attr;

	// ������˵��:
	// PTHREAD_MUTEX_TIMED_NP:
	//   ��ͨ����ͬһ�߳��ڱ���ɶԵ��� Lock �� Unlock�������������ö�� Lock�������������
	// PTHREAD_MUTEX_RECURSIVE_NP:
	//   Ƕ�������߳��ڿ���Ƕ�׵��� Lock����һ����Ч��֮����������ͬ������ Unlock ���ɽ�����
	// PTHREAD_MUTEX_ERRORCHECK_NP:
	//   ����������ͬһ�߳�Ƕ�׵��� Lock ���������
	// PTHREAD_MUTEX_ADAPTIVE_NP:
	//   ��Ӧ����
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&m_Lock, &attr);
	pthread_mutexattr_destroy(&attr);
#endif
}

//-----------------------------------------------------------------------------

CCriticalSection::~CCriticalSection()
{
#ifdef ISE_WIN32
	DeleteCriticalSection(&m_Lock);
#endif
#ifdef ISE_LINUX
	// �����δ����������� destroy���˺����᷵�ش��� EBUSY��
	// �� linux �£���ʹ�˺������ش���Ҳ��������Դй©��
	pthread_mutex_destroy(&m_Lock);
#endif
}

//-----------------------------------------------------------------------------
// ����: ����
//-----------------------------------------------------------------------------
void CCriticalSection::Lock()
{
#ifdef ISE_WIN32
	EnterCriticalSection(&m_Lock);
#endif
#ifdef ISE_LINUX
	pthread_mutex_lock(&m_Lock);
#endif
}

//-----------------------------------------------------------------------------
// ����: ����
//-----------------------------------------------------------------------------
void CCriticalSection::Unlock()
{
#ifdef ISE_WIN32
	LeaveCriticalSection(&m_Lock);
#endif
#ifdef ISE_LINUX
	pthread_mutex_unlock(&m_Lock);
#endif
}

//-----------------------------------------------------------------------------
// ����: ���Լ��� (���Ѿ����ڼ���״̬����������)
// ����:
//   true   - �����ɹ�
//   false  - ʧ�ܣ������Ѿ����ڼ���״̬
//-----------------------------------------------------------------------------
bool CCriticalSection::TryLock()
{
#ifdef ISE_WIN32
	return TryEnterCriticalSection(&m_Lock) != 0;
#endif
#ifdef ISE_LINUX
	return pthread_mutex_trylock(&m_Lock) != EBUSY;
#endif
}

///////////////////////////////////////////////////////////////////////////////
// class CSemaphore

CSemaphore::CSemaphore(UINT nInitValue)
{
	m_nInitValue = nInitValue;
	DoCreateSem();
}

//-----------------------------------------------------------------------------

CSemaphore::~CSemaphore()
{
	DoDestroySem();
}

//-----------------------------------------------------------------------------

void CSemaphore::DoCreateSem()
{
#ifdef ISE_WIN32
	m_Sem = CreateSemaphore(NULL, m_nInitValue, 0x7FFFFFFF, NULL);
	if (!m_Sem)
		IseThrowException(SEM_SEM_INIT_ERROR);
#endif
#ifdef ISE_LINUX
	if (sem_init(&m_Sem, 0, m_nInitValue) != 0)
		IseThrowException(SEM_SEM_INIT_ERROR);
#endif
}

//-----------------------------------------------------------------------------

void CSemaphore::DoDestroySem()
{
#ifdef ISE_WIN32
	CloseHandle(m_Sem);
#endif
#ifdef ISE_LINUX
	sem_destroy(&m_Sem);
#endif
}

//-----------------------------------------------------------------------------
// ����: �����ֵԭ�ӵؼ�1����ʾ����һ���ɷ��ʵ���Դ
//-----------------------------------------------------------------------------
void CSemaphore::Increase()
{
#ifdef ISE_WIN32
	ReleaseSemaphore(m_Sem, 1, NULL);
#endif
#ifdef ISE_LINUX
	sem_post(&m_Sem);
#endif
}

//-----------------------------------------------------------------------------
// ����: �ȴ����(ֱ�����ֵ����0)��Ȼ�����ֵԭ�ӵؼ�1
//-----------------------------------------------------------------------------
void CSemaphore::Wait()
{
#ifdef ISE_WIN32
	WaitForSingleObject(m_Sem, INFINITE);
#endif
#ifdef ISE_LINUX
	int ret;
	do
	{
		ret = sem_wait(&m_Sem);
	}
	while (ret == -1 && (errno == EINTR || errno == EAGAIN));
#endif
}

//-----------------------------------------------------------------------------
// ����: ��������ã������ֵ��Ϊ��ʼ״̬
//-----------------------------------------------------------------------------
void CSemaphore::Reset()
{
	DoDestroySem();
	DoCreateSem();
}

///////////////////////////////////////////////////////////////////////////////
// class CSignalMasker

#ifdef ISE_LINUX

CSignalMasker::CSignalMasker(bool bAutoRestore) :
	m_bBlock(false),
	m_bAutoRestore(bAutoRestore)
{
	sigemptyset(&m_OldSet);
	sigemptyset(&m_NewSet);
}

//-----------------------------------------------------------------------------

CSignalMasker::~CSignalMasker()
{
	if (m_bAutoRestore) Restore();
}

//-----------------------------------------------------------------------------

int CSignalMasker::SigProcMask(int nHow, const sigset_t *pNewSet, sigset_t *pOldSet)
{
	int nRet;
	if ((nRet = sigprocmask(nHow, pNewSet, pOldSet)) < 0)
		IseThrowException(strerror(errno));

	return nRet;
}

//-----------------------------------------------------------------------------
// ����: ���� Block/UnBlock ����������źż���
//-----------------------------------------------------------------------------
void CSignalMasker::SetSignals(int nSigCount, va_list argList)
{
	sigemptyset(&m_NewSet);
	for (int i = 0; i < nSigCount; i++)
		sigaddset(&m_NewSet, va_arg(argList, int));
}

//-----------------------------------------------------------------------------

void CSignalMasker::SetSignals(int nSigCount, ...)
{
	va_list argList;
	va_start(argList, nSigCount);
	SetSignals(nSigCount, argList);
	va_end(argList);
}

//-----------------------------------------------------------------------------
// ����: �ڽ��̵�ǰ�����źż������ SetSignals ���õ��ź�
//-----------------------------------------------------------------------------
void CSignalMasker::Block()
{
	SigProcMask(SIG_BLOCK, &m_NewSet, &m_OldSet);
	m_bBlock = true;
}

//-----------------------------------------------------------------------------
// ����: �ڽ��̵�ǰ�����źż��н�� SetSignals ���õ��ź�
//-----------------------------------------------------------------------------
void CSignalMasker::UnBlock()
{
	SigProcMask(SIG_UNBLOCK, &m_NewSet, &m_OldSet);
	m_bBlock = true;
}

//-----------------------------------------------------------------------------
// ����: �����������źż��ָ�Ϊ Block/UnBlock ֮ǰ��״̬
//-----------------------------------------------------------------------------
void CSignalMasker::Restore()
{
	if (m_bBlock)
	{
		SigProcMask(SIG_SETMASK, &m_OldSet, NULL);
		m_bBlock = false;
	}
}

#endif

///////////////////////////////////////////////////////////////////////////////
// class CSeqNumberAlloc

//-----------------------------------------------------------------------------
// ����: ���캯��
// ����:
//   nStartId - ��ʼ���к�
//-----------------------------------------------------------------------------
CSeqNumberAlloc::CSeqNumberAlloc(UINT nStartId)
{
	m_nCurrentId = nStartId;
}

//-----------------------------------------------------------------------------
// ����: ����һ���·����ID
//-----------------------------------------------------------------------------
UINT CSeqNumberAlloc::AllocId()
{
	CAutoLocker Locker(m_Lock);
	return m_nCurrentId++;
}

///////////////////////////////////////////////////////////////////////////////
// class CStream

INT64 CStream::GetSize()
{
	INT64 nPos, nResult;

	nPos = Seek(0, SO_CURRENT);
	nResult = Seek(0, SO_END);
	Seek(nPos, SO_BEGINNING);

	return nResult;
}

///////////////////////////////////////////////////////////////////////////////
// class CMemoryStream

//-----------------------------------------------------------------------------
// ����: ���캯��
// ����:
//   nMemoryDelta - �ڴ��������� (�ֽ����������� 2 �� N �η�)
//-----------------------------------------------------------------------------
CMemoryStream::CMemoryStream(int nMemoryDelta) :
	m_pMemory(NULL),
	m_nCapacity(0),
	m_nSize(0),
	m_nPosition(0)
{
	SetMemoryDelta(nMemoryDelta);
}

//-----------------------------------------------------------------------------

CMemoryStream::~CMemoryStream()
{
	Clear();
}

//-----------------------------------------------------------------------------

void CMemoryStream::SetMemoryDelta(int nNewMemoryDelta)
{
	if (nNewMemoryDelta != DEFAULT_MEMORY_DELTA)
	{
		if (nNewMemoryDelta < MIN_MEMORY_DELTA)
			nNewMemoryDelta = MIN_MEMORY_DELTA;

		// ��֤ nNewMemoryDelta ��2��N�η�
		for (int i = sizeof(int) * 8 - 1; i >= 0; i--)
			if (((1 << i) & nNewMemoryDelta) != 0)
			{
				nNewMemoryDelta &= (1 << i);
				break;
			}
	}

	m_nMemoryDelta = nNewMemoryDelta;
}

//-----------------------------------------------------------------------------

void CMemoryStream::SetPointer(char *pMemory, int nSize)
{
	m_pMemory = pMemory;
	m_nSize = nSize;
}

//-----------------------------------------------------------------------------

void CMemoryStream::SetCapacity(int nNewCapacity)
{
	SetPointer(Realloc(nNewCapacity), m_nSize);
	m_nCapacity = nNewCapacity;
}

//-----------------------------------------------------------------------------

char* CMemoryStream::Realloc(int& nNewCapacity)
{
	char* pResult;

	if (nNewCapacity > 0 && nNewCapacity != m_nSize)
		nNewCapacity = (nNewCapacity + (m_nMemoryDelta - 1)) & ~(m_nMemoryDelta - 1);

	pResult = m_pMemory;
	if (nNewCapacity != m_nCapacity)
	{
		if (nNewCapacity == 0)
		{
			free(m_pMemory);
			pResult = NULL;
		}
		else
		{
			if (m_nCapacity == 0)
				pResult = (char*)malloc(nNewCapacity);
			else
				pResult = (char*)realloc(m_pMemory, nNewCapacity);

			if (!pResult)
				IseThrowMemoryException();
		}
	}

	return pResult;
}

//-----------------------------------------------------------------------------
// ����: ���ڴ���
//-----------------------------------------------------------------------------
int CMemoryStream::Read(void *pBuffer, int nCount)
{
	int nResult = 0;

	if (m_nPosition >= 0 && nCount >= 0)
	{
		nResult = m_nSize - m_nPosition;
		if (nResult > 0)
		{
			if (nResult > nCount) nResult = nCount;
			memmove(pBuffer, m_pMemory + (UINT)m_nPosition, nResult);
			m_nPosition += nResult;
		}
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: д�ڴ���
//-----------------------------------------------------------------------------
int CMemoryStream::Write(const void *pBuffer, int nCount)
{
	int nResult = 0;
	int nPos;

	if (m_nPosition >= 0 && nCount >= 0)
	{
		nPos = m_nPosition + nCount;
		if (nPos > 0)
		{
			if (nPos > m_nSize)
			{
				if (nPos > m_nCapacity)
					SetCapacity(nPos);
				m_nSize = nPos;
			}
			memmove(m_pMemory + (UINT)m_nPosition, pBuffer, nCount);
			m_nPosition = nPos;
			nResult = nCount;
		}
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: �ڴ���ָ�붨λ
//-----------------------------------------------------------------------------
INT64 CMemoryStream::Seek(INT64 nOffset, SEEK_ORIGIN nSeekOrigin)
{
	switch (nSeekOrigin)
	{
	case SO_BEGINNING:
		m_nPosition = (int)nOffset;
		break;
	case SO_CURRENT:
		m_nPosition += (int)nOffset;
		break;
	case SO_END:
		m_nPosition = m_nSize + (int)nOffset;
		break;
	}

	return m_nPosition;
}

//-----------------------------------------------------------------------------

void CStream::ReadBuffer(void *pBuffer, int nCount)
{
	if (nCount != 0 && Read(pBuffer, nCount) != nCount)
		IseThrowStreamException(SEM_STREAM_READ_ERROR);
}

//-----------------------------------------------------------------------------

void CStream::WriteBuffer(const void *pBuffer, int nCount)
{
	if (nCount != 0 && Write(pBuffer, nCount) != nCount)
		IseThrowStreamException(SEM_STREAM_WRITE_ERROR);
}

//-----------------------------------------------------------------------------
// ����: �����ڴ�����С
//-----------------------------------------------------------------------------
void CMemoryStream::SetSize(INT64 nSize)
{
	ISE_ASSERT(nSize <= MAXLONG);

	int nOldPos = m_nPosition;

	SetCapacity((int)nSize);
	m_nSize = (int)nSize;
	if (nOldPos > nSize) Seek(0, SO_END);
}

//-----------------------------------------------------------------------------
// ����: �����������뵽�ڴ�����
//-----------------------------------------------------------------------------
bool CMemoryStream::LoadFromStream(CStream& Stream)
{
	try
	{
		INT64 nCount = Stream.GetSize();
		ISE_ASSERT(nCount <= MAXLONG);

		Stream.SetPosition(0);
		SetSize(nCount);
		if (nCount != 0)
			Stream.ReadBuffer(m_pMemory, (int)nCount);
		return true;
	}
	catch (CException&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// ����: ���ļ����뵽�ڴ�����
//-----------------------------------------------------------------------------
bool CMemoryStream::LoadFromFile(const string& strFileName)
{
	CFileStream fs;
	bool bResult = fs.Open(strFileName, FM_OPEN_READ | FM_SHARE_DENY_WRITE);
	if (bResult)
		bResult = LoadFromStream(fs);
	return bResult;
}

//-----------------------------------------------------------------------------
// ����: ���ڴ������浽��������
//-----------------------------------------------------------------------------
bool CMemoryStream::SaveToStream(CStream& Stream)
{
	try
	{
		if (m_nSize != 0)
			Stream.WriteBuffer(m_pMemory, m_nSize);
		return true;
	}
	catch (CException&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// ����: ���ڴ������浽�ļ���
//-----------------------------------------------------------------------------
bool CMemoryStream::SaveToFile(const string& strFileName)
{
	CFileStream fs;
	bool bResult = fs.Open(strFileName, FM_CREATE);
	if (bResult)
		bResult = SaveToStream(fs);
	return bResult;
}

//-----------------------------------------------------------------------------
// ����: ����ڴ���
//-----------------------------------------------------------------------------
void CMemoryStream::Clear()
{
	SetCapacity(0);
	m_nSize = 0;
	m_nPosition = 0;
}

///////////////////////////////////////////////////////////////////////////////
// class CFileStream

//-----------------------------------------------------------------------------
// ����: ȱʡ���캯��
//-----------------------------------------------------------------------------
CFileStream::CFileStream()
{
	Init();
}

//-----------------------------------------------------------------------------
// ����: ���캯��
// ����:
//   strFileName - �ļ���
//   nOpenMode   - �ļ����򿪷�ʽ
//   nRights     - �ļ���ȡȨ��
//-----------------------------------------------------------------------------
CFileStream::CFileStream(const string& strFileName, UINT nOpenMode, UINT nRights)
{
	Init();

	CFileException e;
	if (!Open(strFileName, nOpenMode, nRights, &e))
		throw CFileException(e);
}

//-----------------------------------------------------------------------------
// ����: ��������
//-----------------------------------------------------------------------------
CFileStream::~CFileStream()
{
	Close();
}

//-----------------------------------------------------------------------------
// ����: ��ʼ��
//-----------------------------------------------------------------------------
void CFileStream::Init()
{
	m_strFileName.clear();
	m_hHandle = INVALID_HANDLE_VALUE;
}

//-----------------------------------------------------------------------------
// ����: �����ļ�
//-----------------------------------------------------------------------------
HANDLE CFileStream::FileCreate(const string& strFileName, UINT nRights)
{
#ifdef ISE_WIN32
	return CreateFileA(strFileName.c_str(), GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
#endif
#ifdef ISE_LINUX
	umask(0);  // ��ֹ nRights �� umask ֵ ����
	return open(strFileName.c_str(), O_RDWR | O_CREAT | O_TRUNC, nRights);
#endif
}

//-----------------------------------------------------------------------------
// ����: ���ļ�
//-----------------------------------------------------------------------------
HANDLE CFileStream::FileOpen(const string& strFileName, UINT nOpenMode)
{
#ifdef ISE_WIN32
	UINT nAccessModes[3] = {
		GENERIC_READ,
		GENERIC_WRITE,
		GENERIC_READ | GENERIC_WRITE
	};
	UINT nShareModes[5] = {
		0,
		0,
		FILE_SHARE_READ,
		FILE_SHARE_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE
	};

	HANDLE nFileHandle = INVALID_HANDLE_VALUE;

	if ((nOpenMode & 3) <= FM_OPEN_READ_WRITE &&
		(nOpenMode & 0xF0) <= FM_SHARE_DENY_NONE)
		nFileHandle = CreateFileA(strFileName.c_str(), nAccessModes[nOpenMode & 3],
			nShareModes[(nOpenMode & 0xF0) >> 4], NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	return nFileHandle;
#endif
#ifdef ISE_LINUX
	BYTE nShareModes[4] = {
		0,          // none
		F_WRLCK,    // FM_SHARE_EXCLUSIVE
		F_RDLCK,    // FM_SHARE_DENY_WRITE
		0           // FM_SHARE_DENY_NONE
	};

	HANDLE nFileHandle = INVALID_HANDLE_VALUE;
	BYTE nShareMode;

	if (FileExists(strFileName) &&
		(nOpenMode & 0x03) <= FM_OPEN_READ_WRITE &&
		(nOpenMode & 0xF0) <= FM_SHARE_DENY_NONE)
	{
		umask(0);  // ��ֹ nOpenMode �� umask ֵ����
		nFileHandle = open(strFileName.c_str(), (nOpenMode & 0x03), DEFAULT_FILE_ACCESS_RIGHTS);
		if (nFileHandle != INVALID_HANDLE_VALUE)
		{
			nShareMode = ((nOpenMode & 0xF0) >> 4);
			if (nShareModes[nShareMode] != 0)
			{
				struct flock flk;

				flk.l_type = nShareModes[nShareMode];
				flk.l_whence = SEEK_SET;
				flk.l_start = 0;
				flk.l_len = 0;

				if (fcntl(nFileHandle, F_SETLK, &flk) < 0)
				{
					FileClose(nFileHandle);
					nFileHandle = INVALID_HANDLE_VALUE;
				}
			}
		}
	}

	return nFileHandle;
#endif
}

//-----------------------------------------------------------------------------
// ����: �ر��ļ�
//-----------------------------------------------------------------------------
void CFileStream::FileClose(HANDLE hHandle)
{
#ifdef ISE_WIN32
	CloseHandle(hHandle);
#endif
#ifdef ISE_LINUX
	close(hHandle);
#endif
}

//-----------------------------------------------------------------------------
// ����: ���ļ�
//-----------------------------------------------------------------------------
int CFileStream::FileRead(HANDLE hHandle, void *pBuffer, int nCount)
{
#ifdef ISE_WIN32
	unsigned long nResult;
	if (!ReadFile(hHandle, pBuffer, nCount, &nResult, NULL))
		nResult = -1;
	return nResult;
#endif
#ifdef ISE_LINUX
	return read(hHandle, pBuffer, nCount);
#endif
}

//-----------------------------------------------------------------------------
// ����: д�ļ�
//-----------------------------------------------------------------------------
int CFileStream::FileWrite(HANDLE hHandle, const void *pBuffer, int nCount)
{
#ifdef ISE_WIN32
	unsigned long nResult;
	if (!WriteFile(hHandle, pBuffer, nCount, &nResult, NULL))
		nResult = -1;
	return nResult;
#endif
#ifdef ISE_LINUX
	return write(hHandle, pBuffer, nCount);
#endif
}

//-----------------------------------------------------------------------------
// ����: �ļ�ָ�붨λ
//-----------------------------------------------------------------------------
INT64 CFileStream::FileSeek(HANDLE hHandle, INT64 nOffset, SEEK_ORIGIN nSeekOrigin)
{
#ifdef ISE_WIN32
	INT64 nResult = nOffset;
	((Int64Rec*)&nResult)->ints.lo = SetFilePointer(
		hHandle, ((Int64Rec*)&nResult)->ints.lo,
		(PLONG)&(((Int64Rec*)&nResult)->ints.hi), nSeekOrigin);
	if (((Int64Rec*)&nResult)->ints.lo == -1 && GetLastError() != 0)
		((Int64Rec*)&nResult)->ints.hi = -1;
	return nResult;
#endif
#ifdef ISE_LINUX
	return lseek(hHandle, nOffset, nSeekOrigin);
#endif
}

//-----------------------------------------------------------------------------
// ����: ���ļ�
// ����:
//   strFileName - �ļ���
//   nOpenMode   - �ļ����򿪷�ʽ
//   nRights     - �ļ���ȡȨ��
//   pException  - ��������쳣���򴫻ظ��쳣
//-----------------------------------------------------------------------------
bool CFileStream::Open(const string& strFileName, UINT nOpenMode, UINT nRights,
	CFileException* pException)
{
	Close();

	if (nOpenMode == FM_CREATE)
		m_hHandle = FileCreate(strFileName, nRights);
	else
		m_hHandle = FileOpen(strFileName, nOpenMode);

	bool bResult = (m_hHandle != INVALID_HANDLE_VALUE);

	if (!bResult && pException != NULL)
	{
		if (nOpenMode == FM_CREATE)
			*pException = CFileException(strFileName.c_str(), GetLastSysError(),
				FormatString(SEM_CANNOT_CREATE_FILE, strFileName.c_str(),
				SysErrorMessage(GetLastSysError()).c_str()).c_str());
		else
			*pException = CFileException(strFileName.c_str(), GetLastSysError(),
				FormatString(SEM_CANNOT_OPEN_FILE, strFileName.c_str(),
				SysErrorMessage(GetLastSysError()).c_str()).c_str());
	}

	m_strFileName = strFileName;
	return bResult;
}

//-----------------------------------------------------------------------------
// ����: �ر��ļ�
//-----------------------------------------------------------------------------
void CFileStream::Close()
{
	if (m_hHandle != INVALID_HANDLE_VALUE)
	{
		FileClose(m_hHandle);
		m_hHandle = INVALID_HANDLE_VALUE;
	}
	m_strFileName.clear();
}

//-----------------------------------------------------------------------------
// ����: ���ļ���
//-----------------------------------------------------------------------------
int CFileStream::Read(void *pBuffer, int nCount)
{
	int nResult;

	nResult = FileRead(m_hHandle, pBuffer, nCount);
	if (nResult == -1) nResult = 0;

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: д�ļ���
//-----------------------------------------------------------------------------
int CFileStream::Write(const void *pBuffer, int nCount)
{
	int nResult;

	nResult = FileWrite(m_hHandle, pBuffer, nCount);
	if (nResult == -1) nResult = 0;

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: �ļ���ָ�붨λ
//-----------------------------------------------------------------------------
INT64 CFileStream::Seek(INT64 nOffset, SEEK_ORIGIN nSeekOrigin)
{
	return FileSeek(m_hHandle, nOffset, nSeekOrigin);
}

//-----------------------------------------------------------------------------
// ����: �����ļ�����С
//-----------------------------------------------------------------------------
void CFileStream::SetSize(INT64 nSize)
{
	bool bSuccess;
	Seek(nSize, SO_BEGINNING);

#ifdef ISE_WIN32
	bSuccess = SetEndOfFile(m_hHandle) != 0;
#endif
#ifdef ISE_LINUX
	bSuccess = (ftruncate(m_hHandle, GetPosition()) == 0);
#endif

	if (!bSuccess)
		IseThrowFileException(m_strFileName.c_str(), GetLastSysError(), SEM_SET_FILE_STREAM_SIZE_ERR);
}

//-----------------------------------------------------------------------------
// ����: �ж��ļ�����ǰ�Ƿ��״̬
//-----------------------------------------------------------------------------
bool CFileStream::IsOpen() const
{
	return (m_hHandle != INVALID_HANDLE_VALUE);
}

///////////////////////////////////////////////////////////////////////////////
// class CList

CList::CList() :
	m_pList(NULL),
	m_nCount(0),
	m_nCapacity(0)
{
	// nothing
}

//-----------------------------------------------------------------------------

CList::~CList()
{
	Clear();
}

//-----------------------------------------------------------------------------

void CList::Grow()
{
	int nDelta;

	if (m_nCapacity > 64)
		nDelta = m_nCapacity / 4;
	else if (m_nCapacity > 8)
		nDelta = 16;
	else
		nDelta = 4;

	SetCapacity(m_nCapacity + nDelta);
}

//-----------------------------------------------------------------------------

POINTER CList::Get(int nIndex) const
{
	if (nIndex < 0 || nIndex >= m_nCount)
		IseThrowException(SEM_INDEX_ERROR);

	return m_pList[nIndex];
}

//-----------------------------------------------------------------------------

void CList::Put(int nIndex, POINTER Item)
{
	if (nIndex < 0 || nIndex >= m_nCount)
		IseThrowException(SEM_INDEX_ERROR);

	m_pList[nIndex] = Item;
}

//-----------------------------------------------------------------------------

void CList::SetCapacity(int nNewCapacity)
{
	if (nNewCapacity < m_nCount)
		IseThrowException(SEM_LIST_CAPACITY_ERROR);

	if (nNewCapacity != m_nCapacity)
	{
		if (nNewCapacity > 0)
		{
			POINTER *p = (POINTER*)realloc(m_pList, nNewCapacity * sizeof(PVOID));
			if (p)
				m_pList = p;
			else
				IseThrowMemoryException();
		}
		else
		{
			if (m_pList)
			{
				free(m_pList);
				m_pList = NULL;
			}
		}

		m_nCapacity = nNewCapacity;
	}
}

//-----------------------------------------------------------------------------

void CList::SetCount(int nNewCount)
{
	if (nNewCount < 0)
		IseThrowException(SEM_LIST_COUNT_ERROR);

	if (nNewCount > m_nCapacity)
		SetCapacity(nNewCount);
	if (nNewCount > m_nCount)
		memset(&m_pList[m_nCount], 0, (nNewCount - m_nCount) * sizeof(POINTER));
	else
		for (int i = m_nCount - 1; i >= nNewCount; i--) Delete(i);

	m_nCount = nNewCount;
}

//-----------------------------------------------------------------------------
// ����: ���б������Ԫ��
//-----------------------------------------------------------------------------
int CList::Add(POINTER Item)
{
	if (m_nCount == m_nCapacity) Grow();
	m_pList[m_nCount] = Item;
	m_nCount++;

	return m_nCount - 1;
}

//-----------------------------------------------------------------------------
// ����: ���б��в���Ԫ��
// ����:
//   nIndex - ����λ���±��(0-based)
//-----------------------------------------------------------------------------
void CList::Insert(int nIndex, POINTER Item)
{
	if (nIndex < 0 || nIndex > m_nCount)
		IseThrowException(SEM_INDEX_ERROR);

	if (m_nCount == m_nCapacity) Grow();
	if (nIndex < m_nCount)
		memmove(&m_pList[nIndex + 1], &m_pList[nIndex], (m_nCount - nIndex) * sizeof(POINTER));
	m_pList[nIndex] = Item;
	m_nCount++;
}

//-----------------------------------------------------------------------------
// ����: ���б���ɾ��Ԫ��
// ����:
//   nIndex - �±��(0-based)
//-----------------------------------------------------------------------------
void CList::Delete(int nIndex)
{
	if (nIndex < 0 || nIndex >= m_nCount)
		IseThrowException(SEM_INDEX_ERROR);

	m_nCount--;
	if (nIndex < m_nCount)
		memmove(&m_pList[nIndex], &m_pList[nIndex + 1], (m_nCount - nIndex) * sizeof(POINTER));
}

//-----------------------------------------------------------------------------
// ����: ���б���ɾ��Ԫ��
// ����: ��ɾ��Ԫ�����б��е��±��(0-based)����δ�ҵ����򷵻� -1.
//-----------------------------------------------------------------------------
int CList::Remove(POINTER Item)
{
	int nResult;

	nResult = IndexOf(Item);
	if (nResult >= 0)
		Delete(nResult);

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ���б���ɾ��Ԫ��
// ����: ��ɾ����Ԫ��ֵ����δ�ҵ����򷵻� NULL.
//-----------------------------------------------------------------------------
POINTER CList::Extract(POINTER Item)
{
	int i;
	POINTER pResult = NULL;

	i = IndexOf(Item);
	if (i >= 0)
	{
		pResult = Item;
		m_pList[i] = NULL;
		Delete(i);
	}

	return pResult;
}

//-----------------------------------------------------------------------------
// ����: �ƶ�һ��Ԫ�ص��µ�λ��
//-----------------------------------------------------------------------------
void CList::Move(int nCurIndex, int nNewIndex)
{
	POINTER pItem;

	if (nCurIndex != nNewIndex)
	{
		if (nNewIndex < 0 || nNewIndex >= m_nCount)
			IseThrowException(SEM_INDEX_ERROR);

		pItem = Get(nCurIndex);
		m_pList[nCurIndex] = NULL;
		Delete(nCurIndex);
		Insert(nNewIndex, NULL);
		m_pList[nNewIndex] = pItem;
	}
}

//-----------------------------------------------------------------------------
// ����: �ı��б�Ĵ�С
//-----------------------------------------------------------------------------
void CList::Resize(int nCount)
{
	SetCount(nCount);
}

//-----------------------------------------------------------------------------
// ����: ����б�
//-----------------------------------------------------------------------------
void CList::Clear()
{
	SetCount(0);
	SetCapacity(0);
}

//-----------------------------------------------------------------------------
// ����: �����б��е��׸�Ԫ�� (���б�Ϊ�����׳��쳣)
//-----------------------------------------------------------------------------
POINTER CList::First() const
{
	return Get(0);
}

//-----------------------------------------------------------------------------
// ����: �����б��е����Ԫ�� (���б�Ϊ�����׳��쳣)
//-----------------------------------------------------------------------------
POINTER CList::Last() const
{
	return Get(m_nCount - 1);
}

//-----------------------------------------------------------------------------
// ����: ����Ԫ�����б��е��±�� (��δ�ҵ��򷵻� -1)
//-----------------------------------------------------------------------------
int CList::IndexOf(POINTER Item) const
{
	int nResult = 0;

	while (nResult < m_nCount && m_pList[nResult] != Item) nResult++;
	if (nResult == m_nCount)
		nResult = -1;

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: �����б���Ԫ������
//-----------------------------------------------------------------------------
int CList::GetCount() const
{
	return m_nCount;
}

//-----------------------------------------------------------------------------
// ����: ��ֵ������
//-----------------------------------------------------------------------------
CList& CList::operator = (const CList& rhs)
{
	if (this == &rhs) return *this;

	Clear();
	SetCapacity(rhs.m_nCapacity);
	for (int i = 0; i < rhs.GetCount(); i++)
		Add(rhs[i]);
	return *this;
}

//-----------------------------------------------------------------------------
// ����: ��ȡ�б�������Ԫ��
//-----------------------------------------------------------------------------
const POINTER& CList::operator[] (int nIndex) const
{
	if (nIndex < 0 || nIndex >= m_nCount)
		IseThrowException(SEM_INDEX_ERROR);

	return m_pList[nIndex];
}

//-----------------------------------------------------------------------------
// ����: ��ȡ�б�������Ԫ��
//-----------------------------------------------------------------------------
POINTER& CList::operator[] (int nIndex)
{
	return
		const_cast<POINTER&>(
			((const CList&)(*this))[nIndex]
		);
}

///////////////////////////////////////////////////////////////////////////////
// class CPropertyList

CPropertyList::CPropertyList()
{
	// nothing
}

//-----------------------------------------------------------------------------

CPropertyList::~CPropertyList()
{
	Clear();
}

//-----------------------------------------------------------------------------
// ����: ����ĳ��������Ŀ��û�ҵ��򷵻� NULL
//-----------------------------------------------------------------------------
CPropertyList::CPropertyItem* CPropertyList::Find(const string& strName)
{
	int i = IndexOf(strName);
	CPropertyItem *pResult = (i >= 0? (CPropertyItem*)m_Items[i] : NULL);
	return pResult;
}

//-----------------------------------------------------------------------------

bool CPropertyList::IsReservedChar(char ch)
{
	return
		((ch >= 0) && (ch <= 32)) ||
		(ch == (char)PROP_ITEM_SEP) ||
		(ch == (char)PROP_ITEM_QUOT);
}

//-----------------------------------------------------------------------------

bool CPropertyList::HasReservedChar(const string& str)
{
	for (UINT i = 0; i < str.length(); i++)
		if (IsReservedChar(str[i])) return true;
	return false;
}

//-----------------------------------------------------------------------------

char* CPropertyList::ScanStr(char *pStr, char ch)
{
	char *pResult = pStr;
	while ((*pResult != ch) && (*pResult != 0)) pResult++;
	return pResult;
}

//-----------------------------------------------------------------------------
// ����: ���ַ��� str ������(")������
// ʾ��:
//   abcdefg   ->  "abcdefg"
//   abc"efg   ->  "abc""efg"
//   abc""fg   ->  "abc""""fg"
//-----------------------------------------------------------------------------
string CPropertyList::MakeQuotedStr(const string& str)
{
	const char QUOT_CHAR = (char)PROP_ITEM_QUOT;
	string strResult;

	for (UINT i = 0; i < str.length(); i++)
	{
		if (str[i] == QUOT_CHAR)
			strResult += string(2, QUOT_CHAR);
		else
			strResult += str[i];
	}
	strResult = "\"" + strResult + "\"";

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: �� MakeQuotedStr ���ɵ��ַ�����ԭ
// ����:
//   pStr - ��������ַ���ָ�룬������󴫻ش����ֹλ��
// ����:
//   ��ԭ����ַ���
// ʾ��:
//   "abcde"xyz   ->  abcde       ��������ʱ pStr ָ�� x
//   "ab""cd"     ->  ab"cd       ��������ʱ pStr ָ�� '\0'
//   "ab""""cd"   ->  ab""cd      ��������ʱ pStr ָ�� '\0'
//-----------------------------------------------------------------------------
string CPropertyList::ExtractQuotedStr(char*& pStr)
{
	const char QUOT_CHAR = (char)PROP_ITEM_QUOT;
	string strResult;
	int nDropCount;
	char *p;

	if (pStr == NULL || *pStr != QUOT_CHAR) return strResult;

	pStr++;
	nDropCount = 1;
	p = pStr;
	pStr = ScanStr(pStr, QUOT_CHAR);
	while (*pStr)
	{
		pStr++;
		if (*pStr != QUOT_CHAR) break;
		pStr++;
		nDropCount++;
		pStr = ScanStr(pStr, QUOT_CHAR);
	}

	if (((pStr - p) <= 1) || ((pStr - p - nDropCount) == 0)) return strResult;

	if (nDropCount == 1)
		strResult.assign(p, pStr - p - 1);
	else
	{
		strResult.resize(pStr - p - nDropCount);
		char *pDest = const_cast<char*>(strResult.c_str());
		while (p < pStr)
		{
			if ((*p == QUOT_CHAR) && (p < pStr - 1) && (*(p+1) == QUOT_CHAR)) p++;
			*pDest++ = *p++;
		}
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: ���б������Ԫ��
//-----------------------------------------------------------------------------
void CPropertyList::Add(const string& strName, const string& strValue)
{
	if (strName.find(NAME_VALUE_SEP, 0) != string::npos)
		IseThrowException(SEM_PROPLIST_NAME_ERROR);

	CPropertyItem *pItem = Find(strName);
	if (!pItem)
	{
		pItem = new CPropertyItem(strName, strValue);
		m_Items.Add(pItem);
	}
	else
		pItem->strValue = strValue;
}

//-----------------------------------------------------------------------------
// ����: ���б���ɾ��Ԫ��
// ����:
//   true  - �ɹ�
//   false - ʧ��(δ�ҵ�)
//-----------------------------------------------------------------------------
bool CPropertyList::Remove(const string& strName)
{
	int i = IndexOf(strName);
	bool bResult = (i >= 0);

	if (bResult)
	{
		delete (CPropertyItem*)m_Items[i];
		m_Items.Delete(i);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// ����: ����б�
//-----------------------------------------------------------------------------
void CPropertyList::Clear()
{
	for (int i = 0; i < m_Items.GetCount(); i++)
		delete (CPropertyItem*)m_Items[i];
	m_Items.Clear();
}

//-----------------------------------------------------------------------------
// ����: ����ĳ���������б��е�λ��(0-based)
//-----------------------------------------------------------------------------
int CPropertyList::IndexOf(const string& strName) const
{
	int nResult = -1;

	for (int i = 0; i < m_Items.GetCount(); i++)
		if (SameText(strName, ((CPropertyItem*)m_Items[i])->strName))
		{
			nResult = i;
			break;
		}

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: �ж�ĳ�������Ƿ����
//-----------------------------------------------------------------------------
bool CPropertyList::NameExists(const string& strName) const
{
	return (IndexOf(strName) >= 0);
}

//-----------------------------------------------------------------------------
// ����: ���� Name ���б���ȡ�� Value
// ����: ���б��в����ڸ������ԣ��򷵻� False��
//-----------------------------------------------------------------------------
bool CPropertyList::GetValue(const string& strName, string& strValue) const
{
	int i = IndexOf(strName);
	bool bResult = (i >= 0);

	if (bResult)
		strValue = ((CPropertyItem*)m_Items[i])->strValue;

	return bResult;
}

//-----------------------------------------------------------------------------
// ����: �����±��(0-based)ȡ��������Ŀ
//-----------------------------------------------------------------------------
const CPropertyList::CPropertyItem& CPropertyList::GetItems(int nIndex) const
{
	if (nIndex < 0 || nIndex >= GetCount())
		IseThrowException(SEM_INDEX_ERROR);

	return *((CPropertyItem*)m_Items[nIndex]);
}

//-----------------------------------------------------------------------------
// ����: �������б�ת���� PropString ������
// ʾ��:
//   [<abc,123>, <def,456>]   ->  abc=123,def=456
//   [<abc,123>, <,456>]      ->  abc=123,=456
//   [<abc,123>, <",456>]     ->  abc=123,"""=456"
//   [<abc,123>, <',456>]     ->  abc=123,'=456
//   [<abc,123>, <def,">]     ->  abc=123,"def="""
//   [<abc,123>, < def,456>]  ->  abc=123," def=456"
//   [<abc,123>, <def,=>]     ->  abc=123,def==
//   [<abc,123>, <=,456>]     ->  �׳��쳣(Name�в�������ڵȺ�"=")
//-----------------------------------------------------------------------------
string CPropertyList::GetPropString() const
{
	string strResult;
	string strItem;

	for (int nIndex = 0; nIndex < GetCount(); nIndex++)
	{
		const CPropertyItem& Item = GetItems(nIndex);

		strItem = Item.strName + (char)NAME_VALUE_SEP + Item.strValue;
		if (HasReservedChar(strItem))
			strItem = MakeQuotedStr(strItem);

		strResult += strItem;
		if (nIndex < GetCount() - 1) strResult += (char)PROP_ITEM_SEP;
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: �� PropString ת���������б�
//-----------------------------------------------------------------------------
void CPropertyList::SetPropString(const string& strPropString)
{
	char *pStr = const_cast<char*>(strPropString.c_str());
	string strItem;

	Clear();
	while (*pStr)
	{
		if (*pStr == PROP_ITEM_QUOT)
			strItem = ExtractQuotedStr(pStr);
		else
		{
			char *p = pStr;
			pStr = ScanStr(pStr, PROP_ITEM_SEP);
			strItem.assign(p, pStr - p);
			if (*pStr == PROP_ITEM_SEP) pStr++;
		}

		string::size_type i = strItem.find(NAME_VALUE_SEP, 0);
		if (i != string::npos)
			Add(strItem.substr(0, i), strItem.substr(i + 1));
	}
}

//-----------------------------------------------------------------------------
// ����: ��ֵ������
//-----------------------------------------------------------------------------
CPropertyList& CPropertyList::operator = (const CPropertyList& rhs)
{
	if (this == &rhs) return *this;

	Clear();
	for (int i = 0; i < rhs.GetCount(); i++)
		Add(rhs.GetItems(i).strName, rhs.GetItems(i).strValue);

	return *this;
}

//-----------------------------------------------------------------------------
// ����: ��ȡ�б�������Ԫ��
// ע��: ���ô˺���ʱ���� strName �����ڣ���������ӵ��б��У�
//-----------------------------------------------------------------------------
string& CPropertyList::operator[] (const string& strName)
{
	int i = IndexOf(strName);
	if (i < 0)
	{
		Add(strName, "");
		i = GetCount() - 1;
	}

	return ((CPropertyItem*)m_Items[i])->strValue;
}

///////////////////////////////////////////////////////////////////////////////
// class CStrings

CStrings::CStrings()
{
	Init();
}

//-----------------------------------------------------------------------------

void CStrings::Assign(const CStrings& src)
{
	CAutoUpdater AutoUpdater(*this);
	Clear();

	m_nDefined = src.m_nDefined;
	m_chDelimiter = src.m_chDelimiter;
	m_strLineBreak = src.m_strLineBreak;
	m_chQuoteChar = src.m_chQuoteChar;
	m_chNameValueSeparator = src.m_chNameValueSeparator;

	AddStrings(src);
}

//-----------------------------------------------------------------------------

void CStrings::Init()
{
	m_nDefined = 0;
	m_chDelimiter = 0;
	m_chQuoteChar = 0;
	m_chNameValueSeparator = 0;
	m_nUpdateCount = 0;
}

//-----------------------------------------------------------------------------

void CStrings::Error(const char* lpszMsg, int nData) const
{
	IseThrowException(FormatString(lpszMsg, nData).c_str());
}

//-----------------------------------------------------------------------------

string CStrings::ExtractName(const char* lpszStr) const
{
	string strResult(lpszStr);

	string::size_type i = strResult.find(GetNameValueSeparator());
	if (i != string::npos)
		strResult = strResult.substr(0, i);
	else
		strResult.clear();

	return strResult;
}

//-----------------------------------------------------------------------------

int CStrings::CompareStrings(const char* str1, const char* str2) const
{
	int r = CompareText(str1, str2);

	if (r > 0) r = 1;
	else if (r < 0) r = -1;

	return r;
}

//-----------------------------------------------------------------------------

int CStrings::Add(const char* lpszStr)
{
	int nResult = GetCount();
	Insert(nResult, lpszStr);
	return nResult;
}

//-----------------------------------------------------------------------------

int CStrings::Add(const char* lpszStr, POINTER pData)
{
	int nResult = Add(lpszStr);
	SetData(nResult, pData);
	return nResult;
}

//-----------------------------------------------------------------------------

void CStrings::AddStrings(const CStrings& Strings)
{
	CAutoUpdater AutoUpdater(*this);

	for (int i = 0; i < Strings.GetCount(); i++)
		Add(Strings.GetString(i).c_str(), Strings.GetData(i));
}

//-----------------------------------------------------------------------------

void CStrings::Insert(int nIndex, const char* lpszStr, POINTER pData)
{
	Insert(nIndex, lpszStr);
	SetData(nIndex, pData);
}

//-----------------------------------------------------------------------------

bool CStrings::Equals(const CStrings& Strings)
{
	bool bResult;
	int nCount = GetCount();

	bResult = (nCount == Strings.GetCount());
	if (bResult)
	{
		for (int i = 0; i < nCount; i++)
			if (GetString(i) != Strings.GetString(i))
			{
				bResult = false;
				break;
			}
	}

	return bResult;
}

//-----------------------------------------------------------------------------

void CStrings::Exchange(int nIndex1, int nIndex2)
{
	CAutoUpdater AutoUpdater(*this);

	POINTER pTempData;
	string strTempStr;

	strTempStr = GetString(nIndex1);
	pTempData = GetData(nIndex1);
	SetString(nIndex1, GetString(nIndex2).c_str());
	SetData(nIndex1, GetData(nIndex2));
	SetString(nIndex2, strTempStr.c_str());
	SetData(nIndex2, pTempData);
}

//-----------------------------------------------------------------------------

void CStrings::Move(int nCurIndex, int nNewIndex)
{
	if (nCurIndex != nNewIndex)
	{
		CAutoUpdater AutoUpdater(*this);

		POINTER pTempData;
		string strTempStr;

		strTempStr = GetString(nCurIndex);
		pTempData = GetData(nCurIndex);
		Delete(nCurIndex);
		Insert(nNewIndex, strTempStr.c_str(), pTempData);
	}
}

//-----------------------------------------------------------------------------

bool CStrings::Exists(const char* lpszStr) const
{
	return (IndexOf(lpszStr) >= 0);
}

//-----------------------------------------------------------------------------

int CStrings::IndexOf(const char* lpszStr) const
{
	int nResult = -1;

	for (int i = 0; i < GetCount(); i++)
		if (CompareStrings(GetString(i).c_str(), lpszStr) == 0)
		{
			nResult = i;
			break;
		}

	return nResult;
}

//-----------------------------------------------------------------------------

int CStrings::IndexOfName(const char* lpszName) const
{
	int nResult = -1;

	for (int i = 0; i < GetCount(); i++)
		if (CompareStrings((ExtractName(GetString(i).c_str()).c_str()), lpszName) == 0)
		{
			nResult = i;
			break;
		}

	return nResult;
}

//-----------------------------------------------------------------------------

int CStrings::IndexOfData(POINTER pData) const
{
	int nResult = -1;

	for (int i = 0; i < GetCount(); i++)
		if (GetData(i) == pData)
		{
			nResult = i;
			break;
		}

	return nResult;
}

//-----------------------------------------------------------------------------

bool CStrings::LoadFromStream(CStream& Stream)
{
	try
	{
		CAutoUpdater AutoUpdater(*this);

		INT64 nSize64 = Stream.GetSize() - Stream.GetPosition();
		ISE_ASSERT(nSize64 <= MAXLONG);
		int nSize = (int)nSize64;

		CBuffer Buffer(nSize + sizeof(char));
		Stream.ReadBuffer(Buffer, nSize);
		*((char*)(Buffer.Data() + nSize)) = '\0';

		SetText(Buffer);
		return true;
	}
	catch (CException&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------

bool CStrings::LoadFromFile(const char* lpszFileName)
{
	CFileStream fs;
	bool bResult = fs.Open(lpszFileName, FM_OPEN_READ | FM_SHARE_DENY_WRITE);
	if (bResult)
		bResult = LoadFromStream(fs);
	return bResult;
}

//-----------------------------------------------------------------------------

bool CStrings::SaveToStream(CStream& Stream) const
{
	try
	{
		string strText = GetText();
		int nLen = (int)strText.length();
		Stream.WriteBuffer((char*)strText.c_str(), nLen * sizeof(char));
		return true;
	}
	catch (CException&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------

bool CStrings::SaveToFile(const char* lpszFileName) const
{
	CFileStream fs;
	bool bResult = fs.Open(lpszFileName, FM_CREATE);
	if (bResult)
		bResult = SaveToStream(fs);
	return bResult;
}

//-----------------------------------------------------------------------------

char CStrings::GetDelimiter() const
{
	if ((m_nDefined & SD_DELIMITER) == 0)
		const_cast<CStrings&>(*this).SetDelimiter(DEFAULT_DELIMITER);
	return m_chDelimiter;
}

//-----------------------------------------------------------------------------

void CStrings::SetDelimiter(char chValue)
{
	if (m_chDelimiter != chValue || (m_nDefined & SD_DELIMITER) == 0)
	{
		m_nDefined |= SD_DELIMITER;
		m_chDelimiter = chValue;
	}
}

//-----------------------------------------------------------------------------

string CStrings::GetLineBreak() const
{
	if ((m_nDefined & SD_LINE_BREAK) == 0)
		const_cast<CStrings&>(*this).SetLineBreak(DEFAULT_LINE_BREAK);
	return m_strLineBreak;
}

//-----------------------------------------------------------------------------

void CStrings::SetLineBreak(const char* lpszValue)
{
	if (m_strLineBreak != string(lpszValue) || (m_nDefined & SD_LINE_BREAK) == 0)
	{
		m_nDefined |= SD_LINE_BREAK;
		m_strLineBreak = lpszValue;
	}
}

//-----------------------------------------------------------------------------

char CStrings::GetQuoteChar() const
{
	if ((m_nDefined & SD_QUOTE_CHAR) == 0)
		const_cast<CStrings&>(*this).SetQuoteChar(DEFAULT_QUOTE_CHAR);
	return m_chQuoteChar;
}

//-----------------------------------------------------------------------------

void CStrings::SetQuoteChar(char chValue)
{
	if (m_chQuoteChar != chValue || (m_nDefined & SD_QUOTE_CHAR) == 0)
	{
		m_nDefined |= SD_QUOTE_CHAR;
		m_chQuoteChar = chValue;
	}
}

//-----------------------------------------------------------------------------

char CStrings::GetNameValueSeparator() const
{
	if ((m_nDefined & SD_NAME_VALUE_SEP) == 0)
		const_cast<CStrings&>(*this).SetNameValueSeparator(DEFAULT_NAME_VALUE_SEP);
	return m_chNameValueSeparator;
}

//-----------------------------------------------------------------------------

void CStrings::SetNameValueSeparator(char chValue)
{
	if (m_chNameValueSeparator != chValue || (m_nDefined & SD_NAME_VALUE_SEP) == 0)
	{
		m_nDefined |= SD_NAME_VALUE_SEP;
		m_chNameValueSeparator = chValue;
	}
}

//-----------------------------------------------------------------------------

string CStrings::CombineNameValue(const char* lpszName, const char* lpszValue) const
{
	string strName(lpszName);
	char chNameValueSep = GetNameValueSeparator();

	if (strName.find(chNameValueSep) != string::npos)
		Error(SEM_STRINGS_NAME_ERROR, 0);

	return strName + chNameValueSep + lpszValue;
}

//-----------------------------------------------------------------------------

string CStrings::GetName(int nIndex) const
{
	return ExtractName(GetString(nIndex).c_str());
}

//-----------------------------------------------------------------------------

string CStrings::GetValue(const char* lpszName) const
{
	string strName(lpszName);
	int i = IndexOfName(strName.c_str());
	if (i >= 0)
		return GetString(i).substr(strName.length() + 1);
	else
		return "";
}

//-----------------------------------------------------------------------------

string CStrings::GetValue(int nIndex) const
{
	if (nIndex >= 0)
	{
		string strName = GetName(nIndex);
		if (!strName.empty())
			return GetString(nIndex).substr(strName.length() + 1);
		else
		{
			string strItem = GetString(nIndex);
			if (!strItem.empty() && strItem[0] == GetNameValueSeparator())
				return strItem.substr(1);
			else
				return "";
		}
	}
	else
		return "";
}

//-----------------------------------------------------------------------------

void CStrings::SetValue(const char* lpszName, const char* lpszValue)
{
	string strName(lpszName);
	string strValue(lpszValue);

	int i = IndexOfName(lpszName);
	if (strValue.empty())
	{
		if (i >= 0) Delete(i);
	}
	else
	{
		if (i < 0)
			i = Add("");
		SetString(i, (strName + GetNameValueSeparator() + strValue).c_str());
	}
}

//-----------------------------------------------------------------------------

void CStrings::SetValue(int nIndex, const char* lpszValue)
{
	string strValue(lpszValue);

	if (strValue.empty())
	{
		if (nIndex >= 0) Delete(nIndex);
	}
	else
	{
		if (nIndex < 0)
			nIndex = Add("");
		SetString(nIndex, (GetName(nIndex) + GetNameValueSeparator() + strValue).c_str());
	}
}

//-----------------------------------------------------------------------------

string CStrings::GetText() const
{
	string strResult;
	int nCount = GetCount();
	string strLineBreak = GetLineBreak();

	for (int i = 0; i < nCount; i++)
	{
		strResult += GetString(i);
		strResult += strLineBreak;
	}

	return strResult;
}

//-----------------------------------------------------------------------------

void CStrings::SetText(const char* lpszValue)
{
	CAutoUpdater AutoUpdater(*this);

	string strValue(lpszValue);
	string strLineBreak = GetLineBreak();
	int nStart = 0;

	Clear();
	while (true)
	{
		string::size_type nPos = strValue.find(strLineBreak, nStart);
		if (nPos != string::npos)
		{
			Add(strValue.substr(nStart, nPos - nStart).c_str());
			nStart = (int)(nPos + strLineBreak.length());
		}
		else
		{
			string str = strValue.substr(nStart);
			if (!str.empty())
				Add(str.c_str());
			break;
		}
	}
}

//-----------------------------------------------------------------------------

string CStrings::GetCommaText() const
{
	UINT nBakDefined = m_nDefined;
	char chBakDelimiter = m_chDelimiter;
	char chBakQuoteChar = m_chQuoteChar;

	const_cast<CStrings&>(*this).SetDelimiter(DEFAULT_DELIMITER);
	const_cast<CStrings&>(*this).SetQuoteChar(DEFAULT_QUOTE_CHAR);

	string strResult = GetDelimitedText();  // �������׳��쳣

	const_cast<CStrings&>(*this).m_nDefined = nBakDefined;
	const_cast<CStrings&>(*this).m_chDelimiter = chBakDelimiter;
	const_cast<CStrings&>(*this).m_chQuoteChar = chBakQuoteChar;

	return strResult;
}

//-----------------------------------------------------------------------------

void CStrings::SetCommaText(const char* lpszValue)
{
	SetDelimiter(DEFAULT_DELIMITER);
	SetQuoteChar(DEFAULT_QUOTE_CHAR);

	SetDelimitedText(lpszValue);
}

//-----------------------------------------------------------------------------

string CStrings::GetDelimitedText() const
{
	string strResult;
	string strLine;
	int nCount = GetCount();
	char chQuoteChar = GetQuoteChar();
	char chDelimiter = GetDelimiter();

	if (nCount == 1 && GetString(0).empty())
		return string(GetQuoteChar(), 2);

	string strDelimiters;
	for (int i = 1; i <= 32; i++)
		strDelimiters += (char)i;
	strDelimiters += chQuoteChar;
	strDelimiters += chDelimiter;

	for (int i = 0; i < nCount; i++)
	{
		strLine = GetString(i);
		if (strLine.find_first_of(strDelimiters) != string::npos)
			strLine = GetQuotedStr((char*)strLine.c_str(), chQuoteChar);

		if (i > 0) strResult += chDelimiter;
		strResult += strLine;
	}

	return strResult;
}

//-----------------------------------------------------------------------------

void CStrings::SetDelimitedText(const char* lpszValue)
{
	CAutoUpdater AutoUpdater(*this);

	char chQuoteChar = GetQuoteChar();
	char chDelimiter = GetDelimiter();
	const char* pCur = lpszValue;
	string strLine;

	Clear();
	while (*pCur >= 1 && *pCur <= 32)
		pCur++;

	while (*pCur != '\0')
	{
		if (*pCur == chQuoteChar)
			strLine = ExtractQuotedStr(pCur, chQuoteChar);
		else
		{
			const char* p = pCur;
			while (*pCur > 32 && *pCur != chDelimiter)
				pCur++;
			strLine.assign(p, pCur - p);
		}

		Add(strLine.c_str());

		while (*pCur >= 1 && *pCur <= 32)
			pCur++;

		if (*pCur == chDelimiter)
		{
			const char* p = pCur;
			p++;
			if (*p == '\0')
				Add("");

			do pCur++; while (*pCur >= 1 && *pCur <= 32);
		}
	}
}

//-----------------------------------------------------------------------------

void CStrings::SetString(int nIndex, const char* lpszValue)
{
	POINTER pTempData = GetData(nIndex);
	Delete(nIndex);
	Insert(nIndex, lpszValue, pTempData);
}

//-----------------------------------------------------------------------------

void CStrings::BeginUpdate()
{
	if (m_nUpdateCount == 0)
		SetUpdateState(true);
	m_nUpdateCount++;
}

//-----------------------------------------------------------------------------

void CStrings::EndUpdate()
{
	m_nUpdateCount--;
	if (m_nUpdateCount == 0)
		SetUpdateState(false);
}

//-----------------------------------------------------------------------------

CStrings& CStrings::operator = (const CStrings& rhs)
{
	if (this != &rhs)
		Assign(rhs);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// class CStrList

//-----------------------------------------------------------------------------

CStrList::CStrList()
{
	Init();
}

//-----------------------------------------------------------------------------

CStrList::CStrList(const CStrList& src)
{
	Init();
	Assign(src);
}

//-----------------------------------------------------------------------------

CStrList::~CStrList()
{
	InternalClear();
}

//-----------------------------------------------------------------------------

void CStrList::Assign(const CStrList& src)
{
	CStrings::operator=(src);
}

//-----------------------------------------------------------------------------

void CStrList::Init()
{
	m_pList = NULL;
	m_nCount = 0;
	m_nCapacity = 0;
	m_nDupMode = DM_IGNORE;
	m_bSorted = false;
	m_bCaseSensitive = false;
}

//-----------------------------------------------------------------------------

void CStrList::InternalClear()
{
	SetCapacity(0);
}

//-----------------------------------------------------------------------------

string& CStrList::StringObjectNeeded(int nIndex) const
{
	if (m_pList[nIndex].pStr == NULL)
		m_pList[nIndex].pStr = new string();
	return *(m_pList[nIndex].pStr);
}

//-----------------------------------------------------------------------------

void CStrList::ExchangeItems(int nIndex1, int nIndex2)
{
	CStringItem Temp;

	Temp = m_pList[nIndex1];
	m_pList[nIndex1] = m_pList[nIndex2];
	m_pList[nIndex2] = Temp;
}

//-----------------------------------------------------------------------------

void CStrList::Grow()
{
	int nDelta;

	if (m_nCapacity > 64)
		nDelta = m_nCapacity / 4;
	else if (m_nCapacity > 8)
		nDelta = 16;
	else
		nDelta = 4;

	SetCapacity(m_nCapacity + nDelta);
}

//-----------------------------------------------------------------------------

void CStrList::QuickSort(int l, int r, STRINGLIST_COMPARE_PROC pfnCompareProc)
{
	int i, j, p;

	do
	{
		i = l;
		j = r;
		p = (l + r) >> 1;
		do
		{
			while (pfnCompareProc(*this, i, p) < 0) i++;
			while (pfnCompareProc(*this, j, p) > 0) j--;
			if (i <= j)
			{
				ExchangeItems(i, j);
				if (p == i)
					p = j;
				else if (p == j)
					p = i;
				i++;
				j--;
			}
		}
		while (i <= j);

		if (l < j)
			QuickSort(l, j, pfnCompareProc);
		l = i;
	}
	while (i < r);
}

//-----------------------------------------------------------------------------

void CStrList::SetUpdateState(bool bUpdating)
{
	if (bUpdating)
		OnChanging();
	else
		OnChanged();
}

//-----------------------------------------------------------------------------

int CStrList::CompareStrings(const char* str1, const char* str2) const
{
	if (m_bCaseSensitive)
		return strcmp(str1, str2);
	else
		return CompareText(str1, str2);
}

//-----------------------------------------------------------------------------

void CStrList::InsertItem(int nIndex, const char* lpszStr, POINTER pData)
{
	OnChanging();
	if (m_nCount == m_nCapacity) Grow();
	if (nIndex < m_nCount)
	{
		memmove(m_pList + nIndex + 1, m_pList + nIndex, (m_nCount - nIndex) * sizeof(CStringItem));
		m_pList[nIndex].pStr = NULL;
	}

	StringObjectNeeded(nIndex) = lpszStr;
	m_pList[nIndex].pData = pData;

	m_nCount++;
	OnChanged();
}

//-----------------------------------------------------------------------------

int CStrList::Add(const char* lpszStr)
{
	return Add(lpszStr, NULL);
}

//-----------------------------------------------------------------------------

int CStrList::Add(const char* lpszStr, POINTER pData)
{
	int nResult;

	if (!m_bSorted)
		nResult = m_nCount;
	else
	{
		if (Find(lpszStr, nResult))
			switch (m_nDupMode)
			{
			case DM_IGNORE:
				return nResult;
			case DM_ERROR:
				Error(SEM_DUPLICATE_STRING, 0);
			default:
				break;
			}
	}

	InsertItem(nResult, lpszStr, pData);
	return nResult;
}

//-----------------------------------------------------------------------------

void CStrList::Clear()
{
	InternalClear();
}

//-----------------------------------------------------------------------------

void CStrList::Delete(int nIndex)
{
	if (nIndex < 0 || nIndex >= m_nCount)
		Error(SEM_LIST_INDEX_ERROR, nIndex);

	OnChanging();

	delete m_pList[nIndex].pStr;
	m_pList[nIndex].pStr = NULL;

	m_nCount--;
	if (nIndex < m_nCount)
	{
		memmove(m_pList + nIndex, m_pList + nIndex + 1, (m_nCount - nIndex) * sizeof(CStringItem));
		m_pList[m_nCount].pStr = NULL;
	}

	OnChanged();
}

//-----------------------------------------------------------------------------

void CStrList::Exchange(int nIndex1, int nIndex2)
{
	if (nIndex1 < 0 || nIndex1 >= m_nCount)
		Error(SEM_LIST_INDEX_ERROR, nIndex1);
	if (nIndex2 < 0 || nIndex2 >= m_nCount)
		Error(SEM_LIST_INDEX_ERROR, nIndex2);

	OnChanging();
	ExchangeItems(nIndex1, nIndex2);
	OnChanged();
}

//-----------------------------------------------------------------------------

int CStrList::IndexOf(const char* lpszStr) const
{
	int nResult;

	if (!m_bSorted)
		nResult = CStrings::IndexOf(lpszStr);
	else if (!Find(lpszStr, nResult))
		nResult = -1;

	return nResult;
}

//-----------------------------------------------------------------------------

void CStrList::Insert(int nIndex, const char* lpszStr)
{
	Insert(nIndex, lpszStr, NULL);
}

//-----------------------------------------------------------------------------

void CStrList::Insert(int nIndex, const char* lpszStr, POINTER pData)
{
	if (m_bSorted)
		Error(SEM_SORTED_LIST_ERROR, 0);
	if (nIndex < 0 || nIndex > m_nCount)
		Error(SEM_LIST_INDEX_ERROR, nIndex);

	InsertItem(nIndex, lpszStr, pData);
}

//-----------------------------------------------------------------------------

void CStrList::SetCapacity(int nValue)
{
	if (nValue < 0) nValue = 0;

	for (int i = nValue; i < m_nCapacity; i++)
		delete m_pList[i].pStr;

	if (nValue > 0)
	{
		CStringItem *p = (CStringItem*)realloc(m_pList, nValue * sizeof(CStringItem));
		if (p)
			m_pList = p;
		else
			IseThrowMemoryException();
	}
	else
	{
		if (m_pList)
		{
			free(m_pList);
			m_pList = NULL;
		}
	}

	if (m_pList != NULL)
	{
		for (int i = m_nCapacity; i < nValue; i++)
		{
			m_pList[i].pStr = NULL;   // new string object when needed
			m_pList[i].pData = NULL;
		}
	}

	m_nCapacity = nValue;
	if (m_nCount > m_nCapacity)
		m_nCount = m_nCapacity;
}

//-----------------------------------------------------------------------------

POINTER CStrList::GetData(int nIndex) const
{
	if (nIndex < 0 || nIndex >= m_nCount)
		Error(SEM_LIST_INDEX_ERROR, nIndex);

	return m_pList[nIndex].pData;
}

//-----------------------------------------------------------------------------

void CStrList::SetData(int nIndex, POINTER pData)
{
	if (nIndex < 0 || nIndex >= m_nCount)
		Error(SEM_LIST_INDEX_ERROR, nIndex);

	OnChanging();
	m_pList[nIndex].pData = pData;
	OnChanged();
}

//-----------------------------------------------------------------------------

const string& CStrList::GetString(int nIndex) const
{
	if (nIndex < 0 || nIndex >= m_nCount)
		Error(SEM_LIST_INDEX_ERROR, nIndex);

	return StringObjectNeeded(nIndex);
}

//-----------------------------------------------------------------------------

void CStrList::SetString(int nIndex, const char* lpszValue)
{
	if (m_bSorted)
		Error(SEM_SORTED_LIST_ERROR, 0);
	if (nIndex < 0 || nIndex >= m_nCount)
		Error(SEM_LIST_INDEX_ERROR, nIndex);

	OnChanging();
	StringObjectNeeded(nIndex) = lpszValue;
	OnChanged();
}

//-----------------------------------------------------------------------------

bool CStrList::Find(const char* lpszStr, int& nIndex) const
{
	if (!m_bSorted)
	{
		nIndex = IndexOf(lpszStr);
		return (nIndex >= 0);
	}

	bool bResult = false;
	int l, h, i, c;

	l = 0;
	h = m_nCount - 1;
	while (l <= h)
	{
		i = (l + h) >> 1;
		c = CompareStrings(StringObjectNeeded(i).c_str(), lpszStr);
		if (c < 0)
			l = i + 1;
		else
		{
			h = i - 1;
			if (c == 0)
			{
				bResult = true;
				if (m_nDupMode != DM_ACCEPT)
					l = i;
			}
		}
	}

	nIndex = l;
	return bResult;
}

//-----------------------------------------------------------------------------

int StringListCompareProc(const CStrList& StringList, int nIndex1, int nIndex2)
{
	return StringList.CompareStrings(
		StringList.StringObjectNeeded(nIndex1).c_str(),
		StringList.StringObjectNeeded(nIndex2).c_str());
}

//-----------------------------------------------------------------------------

void CStrList::Sort()
{
	Sort(StringListCompareProc);
}

//-----------------------------------------------------------------------------

void CStrList::Sort(STRINGLIST_COMPARE_PROC pfnCompareProc)
{
	if (!m_bSorted && m_nCount > 1)
	{
		OnChanging();
		QuickSort(0, m_nCount - 1, pfnCompareProc);
		OnChanged();
	}
}

//-----------------------------------------------------------------------------

void CStrList::SetSorted(bool bValue)
{
	if (bValue != m_bSorted)
	{
		if (bValue) Sort();
		m_bSorted = bValue;
	}
}

//-----------------------------------------------------------------------------

void CStrList::SetCaseSensitive(bool bValue)
{
	if (bValue != m_bCaseSensitive)
	{
		m_bCaseSensitive = bValue;
		if (m_bSorted) Sort();
	}
}

//-----------------------------------------------------------------------------

CStrList& CStrList::operator = (const CStrList& rhs)
{
	if (this != &rhs)
		Assign(rhs);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// class CUrl

CUrl::CUrl(const string& strUrl)
{
	SetUrl(strUrl);
}

CUrl::CUrl(const CUrl& src)
{
	(*this) = src;
}

//-----------------------------------------------------------------------------

void CUrl::Clear()
{
	m_strProtocol.clear();
	m_strHost.clear();
	m_strPort.clear();
	m_strPath.clear();
	m_strFileName.clear();
	m_strBookmark.clear();
	m_strUserName.clear();
	m_strPassword.clear();
	m_strParams.clear();
}

//-----------------------------------------------------------------------------

CUrl& CUrl::operator = (const CUrl& rhs)
{
	if (this == &rhs) return *this;

	m_strProtocol = rhs.m_strProtocol;
	m_strHost = rhs.m_strHost;
	m_strPort = rhs.m_strPort;
	m_strPath = rhs.m_strPath;
	m_strFileName = rhs.m_strFileName;
	m_strBookmark = rhs.m_strBookmark;
	m_strUserName = rhs.m_strUserName;
	m_strPassword = rhs.m_strPassword;
	m_strParams = rhs.m_strParams;

	return *this;
}

//-----------------------------------------------------------------------------

string CUrl::GetUrl() const
{
	const char SEP_CHAR = '/';
	string strResult;

	if (!m_strProtocol.empty())
		strResult = m_strProtocol + "://";

	if (!m_strUserName.empty())
	{
		strResult += m_strUserName;
		if (!m_strPassword.empty())
			strResult = strResult + ":" + m_strPassword;
		strResult += "@";
	}

	strResult += m_strHost;

	if (!m_strPort.empty())
	{
		if (SameText(m_strProtocol, "HTTP"))
		{
			if (m_strPort != "80")
				strResult = strResult + ":" + m_strPort;
		}
		else if (SameText(m_strProtocol, "HTTPS"))
		{
			if (m_strPort != "443")
				strResult = strResult + ":" + m_strPort;
		}
		else if (SameText(m_strProtocol, "FTP"))
		{
			if (m_strPort != "21")
				strResult = strResult + ":" + m_strPort;
		}
	}

	// path and filename
	string str = m_strPath;
	if (!str.empty() && str[str.length()-1] != SEP_CHAR)
		str += SEP_CHAR;
	str += m_strFileName;

	if (!str.empty())
	{
		if (!strResult.empty() && str[0] == SEP_CHAR) str.erase(0, 1);
		if (!m_strHost.empty() && strResult[strResult.length()-1] != SEP_CHAR)
			strResult += SEP_CHAR;
		strResult += str;
	}

	if (!m_strParams.empty())
		strResult = strResult + "?" + m_strParams;

	if (!m_strBookmark.empty())
		strResult = strResult + "#" + m_strBookmark;

	return strResult;
}

//-----------------------------------------------------------------------------

string CUrl::GetUrl(UINT nParts)
{
	CUrl Url(*this);

	if (!(nParts & URL_PROTOCOL)) Url.SetProtocol("");
	if (!(nParts & URL_HOST)) Url.SetHost("");
	if (!(nParts & URL_PORT)) Url.SetPort("");
	if (!(nParts & URL_PATH)) Url.SetPath("");
	if (!(nParts & URL_FILENAME)) Url.SetFileName("");
	if (!(nParts & URL_BOOKMARK)) Url.SetBookmark("");
	if (!(nParts & URL_USERNAME)) Url.SetUserName("");
	if (!(nParts & URL_PASSWORD)) Url.SetPassword("");
	if (!(nParts & URL_PARAMS)) Url.SetParams("");

	return Url.GetUrl();
}

//-----------------------------------------------------------------------------

void CUrl::SetUrl(const string& strValue)
{
	Clear();

	string strUrl(strValue);
	if (strUrl.empty()) return;

	// get the bookmark
	string::size_type nPos = strUrl.rfind('#');
	if (nPos != string::npos)
	{
		m_strBookmark = strUrl.substr(nPos + 1);
		strUrl.erase(nPos);
	}

	// get the parameters
	nPos = strUrl.find('?');
	if (nPos != string::npos)
	{
		m_strParams = strUrl.substr(nPos + 1);
		strUrl = strUrl.substr(0, nPos);
	}

	string strBuffer;
	nPos = strUrl.find("://");
	if (nPos != string::npos)
	{
		m_strProtocol = strUrl.substr(0, nPos);
		strUrl.erase(0, nPos + 3);
		// get the user name, password, host and the port number
		strBuffer = FetchStr(strUrl, '/', true);
		// get username and password
		nPos = strBuffer.find('@');
		if (nPos != string::npos)
		{
			m_strPassword = strBuffer.substr(0, nPos);
			strBuffer.erase(0, nPos + 1);
			m_strUserName = FetchStr(m_strPassword, ':');
			if (m_strUserName.empty())
				m_strPassword.clear();
		}
		// get the host and the port number
		string::size_type p1, p2;
		if ((p1 = strBuffer.find('[')) != string::npos &&
			(p2 = strBuffer.find(']')) != string::npos &&
			p2 > p1)
		{
			// this is for IPv6 Hosts
			m_strHost = FetchStr(strBuffer, ']');
			FetchStr(m_strHost, '[');
			FetchStr(strBuffer, ':');
		}
		else
			m_strHost = FetchStr(strBuffer, ':', true);
		m_strPort = strBuffer;
		// get the path
		nPos = strUrl.rfind('/');
		if (nPos != string::npos)
		{
			m_strPath = "/" + strUrl.substr(0, nPos + 1);
			strUrl.erase(0, nPos + 1);
		}
		else
			m_strPath = "/";
	}
	else
	{
		// get the path
		nPos = strUrl.rfind('/');
		if (nPos != string::npos)
		{
			m_strPath = strUrl.substr(0, nPos + 1);
			strUrl.erase(0, nPos + 1);
		}
	}

	// get the filename
	m_strFileName = strUrl;
}

///////////////////////////////////////////////////////////////////////////////
// class CPacket

CPacket::CPacket()
{
	Init();
}

//-----------------------------------------------------------------------------

CPacket::~CPacket()
{
	Clear();
}

//-----------------------------------------------------------------------------

void CPacket::Init()
{
	m_pStream = NULL;
	m_bAvailable = false;
	m_bIsPacked = false;
}

//-----------------------------------------------------------------------------

void CPacket::ThrowUnpackError()
{
	IseThrowException(SEM_PACKET_UNPACK_ERROR);
}

//-----------------------------------------------------------------------------

void CPacket::ThrowPackError()
{
	IseThrowException(SEM_PACKET_PACK_ERROR);
}

//-----------------------------------------------------------------------------

void CPacket::CheckUnsafeSize(int nValue)
{
	const int MAX_UNSAFE_SIZE = 1024*1024*8;  // 8M

	if (nValue < 0 || nValue > MAX_UNSAFE_SIZE)
		IseThrowException(SEM_UNSAFE_VALUE_IN_PACKET);
}

//-----------------------------------------------------------------------------

void CPacket::ReadBuffer(void *pBuffer, int nBytes)
{
	if (m_pStream->Read(pBuffer, nBytes) != nBytes)
		ThrowUnpackError();
}

//-----------------------------------------------------------------------------

void CPacket::ReadString(std::string& str)
{
	DWORD nSize;

	str.clear();
	if (m_pStream->Read(&nSize, sizeof(DWORD)) == sizeof(DWORD))
	{
		CheckUnsafeSize(nSize);
		if (nSize > 0)
		{
			str.resize(nSize);
			if (m_pStream->Read((void*)str.data(), nSize) != nSize)
				ThrowUnpackError();
		}
	}
	else
	{
		ThrowUnpackError();
	}
}

//-----------------------------------------------------------------------------

void CPacket::ReadBlob(std::string& str)
{
	ReadString(str);
}

//-----------------------------------------------------------------------------

void CPacket::ReadBlob(CStream& Stream)
{
	std::string str;

	ReadBlob(str);
	Stream.SetSize(0);
	if (!str.empty())
		Stream.Write((void*)str.c_str(), (int)str.length());
}

//-----------------------------------------------------------------------------

void CPacket::ReadBlob(CBuffer& Buffer)
{
	std::string str;

	ReadBlob(str);
	Buffer.SetSize(0);
	if (!str.empty())
		Buffer.Assign((void*)str.c_str(), (int)str.length());
}

//-----------------------------------------------------------------------------

void CPacket::WriteBuffer(const void *pBuffer, int nBytes)
{
	ISE_ASSERT(pBuffer && nBytes >= 0);
	m_pStream->Write(pBuffer, nBytes);
}

//-----------------------------------------------------------------------------

void CPacket::WriteString(const std::string& str)
{
	DWORD nSize;

	nSize = (DWORD)str.length();
	m_pStream->Write(&nSize, sizeof(DWORD));
	if (nSize > 0)
		m_pStream->Write((void*)str.c_str(), nSize);
}

//-----------------------------------------------------------------------------

void CPacket::WriteBlob(void *pBuffer, int nBytes)
{
	DWORD nSize = 0;

	if (pBuffer && nBytes >= 0)
		nSize = (DWORD)nBytes;
	m_pStream->Write(&nSize, sizeof(DWORD));
	if (nSize > 0)
		m_pStream->Write(pBuffer, nSize);
}

//-----------------------------------------------------------------------------

void CPacket::WriteBlob(const CBuffer& Buffer)
{
	WriteBlob(Buffer.Data(), Buffer.GetSize());
}

//-----------------------------------------------------------------------------
// ����: �̶��ַ����ĳ���
//-----------------------------------------------------------------------------
void CPacket::FixStrLength(std::string& str, int nLength)
{
	if ((int)str.length() != nLength)
		str.resize(nLength, 0);
}

//-----------------------------------------------------------------------------
// ����: �����ַ�������󳤶�
//-----------------------------------------------------------------------------
void CPacket::TruncString(std::string& str, int nMaxLength)
{
	if ((int)str.length() > nMaxLength)
		str.resize(nMaxLength);
}

//-----------------------------------------------------------------------------
// ����: ���ݴ��
// ����:
//   true  - �ɹ�
//   false - ʧ��
// ��ע:
//   ���������ݿ��� CPacket.GetBuffer �� CPacket.GetSize ȡ����
//-----------------------------------------------------------------------------
bool CPacket::Pack()
{
	bool bResult;

	try
	{
		delete m_pStream;
		m_pStream = new CMemoryStream(DEFAULT_MEMORY_DELTA);
		DoPack();
		DoAfterPack();
		DoCompress();
		DoEncrypt();
		m_bAvailable = true;
		m_bIsPacked = true;
		bResult = true;
	}
	catch (CException&)
	{
		bResult = false;
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// ����: ���ݽ��
// ����:
//   true  - �ɹ�
//   false - ʧ�� (���ݲ��㡢��ʽ�����)
//-----------------------------------------------------------------------------
bool CPacket::Unpack(void *pBuffer, int nBytes)
{
	bool bResult;

	try
	{
		delete m_pStream;
		m_pStream = new CMemoryStream(DEFAULT_MEMORY_DELTA);
		m_pStream->SetSize(nBytes);
		memmove(m_pStream->GetMemory(), pBuffer, nBytes);
		DoDecrypt();
		DoDecompress();
		DoUnpack();
		m_bAvailable = true;
		m_bIsPacked = false;
		bResult = true;
	}
	catch (CException&)
	{
		bResult = false;
		Clear();
	}

	delete m_pStream;
	m_pStream = NULL;

	return bResult;
}

//-----------------------------------------------------------------------------
// ����: ���ݽ��
//-----------------------------------------------------------------------------
bool CPacket::Unpack(const CBuffer& Buffer)
{
	return Unpack(Buffer.Data(), Buffer.GetSize());
}

//-----------------------------------------------------------------------------
// ����: ������ݰ�����
//-----------------------------------------------------------------------------
void CPacket::Clear()
{
	delete m_pStream;
	m_pStream = NULL;
	m_bAvailable = false;
	m_bIsPacked = false;
}

//-----------------------------------------------------------------------------
// ����: ȷ�������Ѵ��
//-----------------------------------------------------------------------------
void CPacket::EnsurePacked()
{
	if (!IsPacked()) Pack();
}

///////////////////////////////////////////////////////////////////////////////
// class CCustomParams

CCustomParams::CCustomParams()
{
	Init();
}

//-----------------------------------------------------------------------------

CCustomParams::CCustomParams(const CCustomParams& src)
{
	Init();
	*this = src;
}

//-----------------------------------------------------------------------------

CCustomParams::CCustomParams(int nCount, ...)
{
	ISE_ASSERT(nCount >= 0 && nCount <= MAX_PARAM_COUNT);

	Init();

	va_list argList;
	va_start(argList, nCount);
	for (int i = 0; i < nCount; i++)
	{
		PVOID pArg = va_arg(argList, PVOID);
		Add(pArg);
	}
	va_end(argList);
}

//-----------------------------------------------------------------------------

void CCustomParams::Init()
{
	m_nCount = 0;
}

//-----------------------------------------------------------------------------

bool CCustomParams::Add(PVOID pValue)
{
	bool bResult = (m_nCount < MAX_PARAM_COUNT);
	if (bResult)
	{
		m_pParams[m_nCount] = pValue;
		m_nCount++;
	}

	return bResult;
}

//-----------------------------------------------------------------------------

void CCustomParams::Clear()
{
	Init();
}

//-----------------------------------------------------------------------------

CCustomParams& CCustomParams::operator = (const CCustomParams& rhs)
{
	if (this == &rhs) return *this;

	memmove(m_pParams, rhs.m_pParams, sizeof(m_pParams));
	m_nCount = rhs.m_nCount;

	return *this;
}

//-----------------------------------------------------------------------------

PVOID& CCustomParams::operator[] (int nIndex)
{
	return
		const_cast<PVOID&>(
			((const CCustomParams&)(*this))[nIndex]
		);
}

//-----------------------------------------------------------------------------

const PVOID& CCustomParams::operator[] (int nIndex) const
{
	if (nIndex < 0 || nIndex >= m_nCount)
		IseThrowException(FormatString(SEM_LIST_INDEX_ERROR, nIndex).c_str());

	return m_pParams[nIndex];
}

///////////////////////////////////////////////////////////////////////////////
// class CLogger

CLogger::CLogger() :
	m_bNewFileDaily(false)
{
	// nothing
}

//-----------------------------------------------------------------------------

CLogger& CLogger::Instance()
{
	static CLogger obj;
	return obj;
}

//-----------------------------------------------------------------------------

string CLogger::GetLogFileName()
{
	string strResult = m_strFileName;

	if (strResult.empty())
		strResult = GetAppPath() + "log.txt";

	if (m_bNewFileDaily)
	{
		string strFileExt = ExtractFileExt(strResult);
		strResult = strResult.substr(0, strResult.length() - strFileExt.length()) + ".";
		strResult += CDateTime::CurrentDateTime().DateString("");
		strResult += strFileExt;
	}

	return strResult;
}

//-----------------------------------------------------------------------------

bool CLogger::OpenFile(CFileStream& FileStream, const string& strFileName)
{
	return
		FileStream.Open(strFileName, FM_OPEN_WRITE | FM_SHARE_DENY_NONE) ||
		FileStream.Open(strFileName, FM_CREATE | FM_SHARE_DENY_NONE);
}

//-----------------------------------------------------------------------------
// ����: ���ַ���д���ļ�
//-----------------------------------------------------------------------------
void CLogger::WriteToFile(const string& strString)
{
	CAutoLocker Locker(m_Lock);

	string strFileName = GetLogFileName();
	CFileStream fs;
	if (!OpenFile(fs, strFileName))
	{
		string strPath = ExtractFilePath(strFileName);
		if (!strPath.empty())
		{
			ForceDirectories(strPath);
			OpenFile(fs, strFileName);
		}
	}

	if (fs.IsOpen())
	{
		fs.Seek(0, SO_END);
		fs.Write(strString.c_str(), (int)strString.length());
	}
}

//-----------------------------------------------------------------------------
// ����: ������־�ļ���
// ����:
//   strFileName   - ��־�ļ��� (��·��)
//   bNewFileDaily - ���Ϊtrue�������Զ����ļ�����(��׺��ǰ)���ϵ��������
//-----------------------------------------------------------------------------
void CLogger::SetFileName(const string& strFileName, bool bNewFileDaily)
{
	m_strFileName = strFileName;
	m_bNewFileDaily = bNewFileDaily;
}

//-----------------------------------------------------------------------------
// ����: ���ı�д����־
//-----------------------------------------------------------------------------
void CLogger::WriteStr(const char *sString)
{
	string strText;
	UINT nProcessId, nThreadId;

#ifdef ISE_WIN32
	nProcessId = GetCurrentProcessId();
	nThreadId = GetCurrentThreadId();
#endif
#ifdef ISE_LINUX
	nProcessId = getpid();
	nThreadId = pthread_self();
#endif

	strText = FormatString("[%s](%05d|%05u)<%s>\n",
		CDateTime::CurrentDateTime().DateTimeString().c_str(),
		nProcessId, nThreadId, sString);

	WriteToFile(strText);
}

//-----------------------------------------------------------------------------
// ����: ���ı�д����־
//-----------------------------------------------------------------------------
void CLogger::WriteFmt(const char *sFormatString, ...)
{
	string strText;

	va_list argList;
	va_start(argList, sFormatString);
	FormatStringV(strText, sFormatString, argList);
	va_end(argList);

	WriteStr(strText.c_str());
}

//-----------------------------------------------------------------------------
// ����: ���쳣��Ϣд����־
//-----------------------------------------------------------------------------
void CLogger::WriteException(const CException& e)
{
	WriteStr(e.MakeLogStr().c_str());
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
