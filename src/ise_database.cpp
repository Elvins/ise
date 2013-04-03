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
// �ļ�����: ise_database.cpp
// ��������: ���ݿ�ӿ�(DBI: ISE Database Interface)
///////////////////////////////////////////////////////////////////////////////

#include "ise_database.h"
#include "ise_sysutils.h"
#include "ise_errmsgs.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class CDbConnParams

CDbConnParams::CDbConnParams() :
	m_nPort(0)
{
	// nothing
}

CDbConnParams::CDbConnParams(const CDbConnParams& src)
{
	m_strHostName = src.m_strHostName;
	m_strUserName = src.m_strUserName;
	m_strPassword = src.m_strPassword;
	m_strDbName = src.m_strDbName;
	m_nPort = src.m_nPort;
}

CDbConnParams::CDbConnParams(const string& strHostName, const string& strUserName,
	const string& strPassword, const string& strDbName, int nPort)
{
	m_strHostName = strHostName;
	m_strUserName = strUserName;
	m_strPassword = strPassword;
	m_strDbName = strDbName;
	m_nPort = nPort;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbOptions

CDbOptions::CDbOptions()
{
	SetMaxDbConnections(DEF_MAX_DB_CONNECTIONS);
}

void CDbOptions::SetMaxDbConnections(int nValue)
{
	if (nValue < 1) nValue = 1;
	m_nMaxDbConnections = nValue;
}

void CDbOptions::SetInitialSqlCmd(const string& strValue)
{
	m_InitialSqlCmdList.Clear();
	m_InitialSqlCmdList.Add(strValue.c_str());
}

void CDbOptions::SetInitialCharSet(const string& strValue)
{
	m_strInitialCharSet = strValue;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbConnection

CDbConnection::CDbConnection(CDatabase *pDatabase)
{
	m_pDatabase = pDatabase;
	m_bConnected = false;
	m_bBusy = false;
}

CDbConnection::~CDbConnection()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: �������ݿ����Ӳ������������ (��ʧ�����׳��쳣)
//-----------------------------------------------------------------------------
void CDbConnection::Connect()
{
	if (!m_bConnected)
	{
		DoConnect();
		ExecCmdOnConnected();
		m_bConnected = true;
	}
}

//-----------------------------------------------------------------------------
// ����: �Ͽ����ݿ����Ӳ������������
//-----------------------------------------------------------------------------
void CDbConnection::Disconnect()
{
	if (m_bConnected)
	{
		DoDisconnect();
		m_bConnected = false;
	}
}

//-----------------------------------------------------------------------------
// ����: �ս�������ʱִ������
//-----------------------------------------------------------------------------
void CDbConnection::ExecCmdOnConnected()
{
	try
	{
		CStrList& CmdList = m_pDatabase->GetDbOptions()->InitialSqlCmdList();
		if (!CmdList.IsEmpty())
		{
			CDbQueryWrapper Query(m_pDatabase->CreateDbQuery());
			Query->m_pDbConnection = this;

			for (int i = 0; i < CmdList.GetCount(); ++i)
			{
				try
				{
					Query->SetSql(CmdList[i]);
					Query->Execute();
				}
				catch (...)
				{}
			}

			Query->m_pDbConnection = NULL;
		}
	}
	catch (...)
	{}
}

//-----------------------------------------------------------------------------
// ����: �������� (�� ConnectionPool ����)
//-----------------------------------------------------------------------------
bool CDbConnection::GetDbConnection()
{
	if (!m_bBusy)
	{
		ActivateConnection();
		m_bBusy = true;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// ����: �黹���� (�� ConnectionPool ����)
//-----------------------------------------------------------------------------
void CDbConnection::ReturnDbConnection()
{
	m_bBusy = false;
}

//-----------------------------------------------------------------------------
// ����: ���������Ƿ񱻽��� (�� ConnectionPool ����)
//-----------------------------------------------------------------------------
bool CDbConnection::IsBusy()
{
	return m_bBusy;
}

//-----------------------------------------------------------------------------
// ����: �������ݿ�����
// ����:
//   bForce - �Ƿ�ǿ�Ƽ���
//-----------------------------------------------------------------------------
void CDbConnection::ActivateConnection(bool bForce)
{
	// û���������ݿ���������
	if (!m_bConnected || bForce)
	{
		Disconnect();
		Connect();
		return;
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CDbConnectionPool

CDbConnectionPool::CDbConnectionPool(CDatabase *pDatabase) :
	m_pDatabase(pDatabase)
{
	// nothing
}

CDbConnectionPool::~CDbConnectionPool()
{
	ClearPool();
}

//-----------------------------------------------------------------------------
// ����: ������ӳ�
//-----------------------------------------------------------------------------
void CDbConnectionPool::ClearPool()
{
	CAutoLocker Locker(m_Lock);

	for (int i = 0; i < m_DbConnectionList.GetCount(); i++)
	{
		CDbConnection *pDbConnection;
		pDbConnection = (CDbConnection*)m_DbConnectionList[i];
		pDbConnection->DoDisconnect();
		delete pDbConnection;
	}

	m_DbConnectionList.Clear();
}

//-----------------------------------------------------------------------------
// ����: ����һ�����õĿ������� (��ʧ�����׳��쳣)
// ����: ���Ӷ���ָ��
//-----------------------------------------------------------------------------
CDbConnection* CDbConnectionPool::GetConnection()
{
	CDbConnection *pDbConnection = NULL;
	bool bResult = false;

	{
		CAutoLocker Locker(m_Lock);

		// ������е������Ƿ�����
		for (int i = 0; i < m_DbConnectionList.GetCount(); i++)
		{
			pDbConnection = (CDbConnection*)m_DbConnectionList[i];
			bResult = pDbConnection->GetDbConnection();  // �������
			if (bResult) break;
		}

		// ������ʧ�ܣ��������µ����ݿ����ӵ����ӳ�
		if (!bResult && (m_DbConnectionList.GetCount() < m_pDatabase->GetDbOptions()->GetMaxDbConnections()))
		{
			pDbConnection = m_pDatabase->CreateDbConnection();
			m_DbConnectionList.Add(pDbConnection);
			bResult = pDbConnection->GetDbConnection();
		}
	}

	if (!bResult)
		IseThrowDbException(SEM_GET_CONN_FROM_POOL_ERROR);

	return pDbConnection;
}

//-----------------------------------------------------------------------------
// ����: �黹���ݿ�����
//-----------------------------------------------------------------------------
void CDbConnectionPool::ReturnConnection(CDbConnection *pDbConnection)
{
	CAutoLocker Locker(m_Lock);
	pDbConnection->ReturnDbConnection();
}

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldDef

CDbFieldDef::CDbFieldDef(const string& strName, int nType)
{
	m_strName = strName;
	m_nType = nType;
}

CDbFieldDef::CDbFieldDef(const CDbFieldDef& src)
{
	m_strName = src.m_strName;
	m_nType = src.m_nType;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldDefList

CDbFieldDefList::CDbFieldDefList()
{
	// nothing
}

CDbFieldDefList::~CDbFieldDefList()
{
	Clear();
}

//-----------------------------------------------------------------------------
// ����: ���һ���ֶζ������
//-----------------------------------------------------------------------------
void CDbFieldDefList::Add(CDbFieldDef *pFieldDef)
{
	if (pFieldDef != NULL)
		m_Items.Add(pFieldDef);
}

//-----------------------------------------------------------------------------
// ����: �ͷŲ���������ֶζ������
//-----------------------------------------------------------------------------
void CDbFieldDefList::Clear()
{
	for (int i = 0; i < m_Items.GetCount(); i++)
		delete (CDbFieldDef*)m_Items[i];
	m_Items.Clear();
}

//-----------------------------------------------------------------------------
// ����: �����ֶ�����Ӧ���ֶ����(0-based)����δ�ҵ��򷵻�-1.
//-----------------------------------------------------------------------------
int CDbFieldDefList::IndexOfName(const string& strName)
{
	int nIndex = -1;

	for (int i = 0; i < m_Items.GetCount(); i++)
	{
		if (SameText(((CDbFieldDef*)m_Items[i])->GetName(), strName))
		{
			nIndex = i;
			break;
		}
	}

	return nIndex;
}

//-----------------------------------------------------------------------------
// ����: ����ȫ���ֶ���
//-----------------------------------------------------------------------------
void CDbFieldDefList::GetFieldNameList(CStrList& List)
{
	List.Clear();
	for (int i = 0; i < m_Items.GetCount(); i++)
		List.Add(((CDbFieldDef*)m_Items[i])->GetName().c_str());
}

//-----------------------------------------------------------------------------
// ����: �����±�ŷ����ֶζ������ (nIndex: 0-based)
//-----------------------------------------------------------------------------
CDbFieldDef* CDbFieldDefList::operator[] (int nIndex)
{
	if (nIndex >= 0 && nIndex < m_Items.GetCount())
		return (CDbFieldDef*)m_Items[nIndex];
	else
		return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbField

CDbField::CDbField()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: �����ͷ����ֶ�ֵ (��ת��ʧ���򷵻�ȱʡֵ)
//-----------------------------------------------------------------------------
int CDbField::AsInteger(int nDefault) const
{
	return StrToInt(AsString(), nDefault);
}

//-----------------------------------------------------------------------------
// ����: ��64λ���ͷ����ֶ�ֵ (��ת��ʧ���򷵻�ȱʡֵ)
//-----------------------------------------------------------------------------
INT64 CDbField::AsInt64(INT64 nDefault) const
{
	return StrToInt64(AsString(), nDefault);
}

//-----------------------------------------------------------------------------
// ����: �Ը����ͷ����ֶ�ֵ (��ת��ʧ���򷵻�ȱʡֵ)
//-----------------------------------------------------------------------------
double CDbField::AsFloat(double fDefault) const
{
	return StrToFloat(AsString(), fDefault);
}

//-----------------------------------------------------------------------------
// ����: �Բ����ͷ����ֶ�ֵ (��ת��ʧ���򷵻�ȱʡֵ)
//-----------------------------------------------------------------------------
bool CDbField::AsBoolean(bool bDefault) const
{
	return AsInteger(bDefault? 1 : 0) != 0;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldList

CDbFieldList::CDbFieldList()
{
	// nothing
}

CDbFieldList::~CDbFieldList()
{
	Clear();
}

//-----------------------------------------------------------------------------
// ����: ���һ���ֶ����ݶ���
//-----------------------------------------------------------------------------
void CDbFieldList::Add(CDbField *pField)
{
	m_Items.Add(pField);
}

//-----------------------------------------------------------------------------
// ����: �ͷŲ���������ֶ����ݶ���
//-----------------------------------------------------------------------------
void CDbFieldList::Clear()
{
	for (int i = 0; i < m_Items.GetCount(); i++)
		delete (CDbField*)m_Items[i];
	m_Items.Clear();
}

//-----------------------------------------------------------------------------
// ����: �����±�ŷ����ֶ����ݶ��� (nIndex: 0-based)
//-----------------------------------------------------------------------------
CDbField* CDbFieldList::operator[] (int nIndex)
{
	if (nIndex >= 0 && nIndex < m_Items.GetCount())
		return (CDbField*)m_Items[nIndex];
	else
		return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbParamList

CDbParamList::CDbParamList(CDbQuery *pDbQuery) :
	m_pDbQuery(pDbQuery)
{
	// nothing
}

CDbParamList::~CDbParamList()
{
	Clear();
}

//-----------------------------------------------------------------------------
// ����: ���ݲ����������б��в��Ҳ�������
//-----------------------------------------------------------------------------
CDbParam* CDbParamList::FindParam(const string& strName)
{
	CDbParam *pResult = NULL;

	for (int i = 0; i < m_Items.GetCount(); i++)
		if (SameText(((CDbParam*)m_Items[i])->m_strName, strName))
		{
			pResult = (CDbParam*)m_Items[i];
			break;
		}

	return pResult;
}

//-----------------------------------------------------------------------------
// ����: ���ݲ���������б��в��Ҳ�������
//-----------------------------------------------------------------------------
CDbParam* CDbParamList::FindParam(int nNumber)
{
	CDbParam *pResult = NULL;

	for (int i = 0; i < m_Items.GetCount(); i++)
		if (((CDbParam*)m_Items[i])->m_nNumber == nNumber)
		{
			pResult = (CDbParam*)m_Items[i];
			break;
		}

	return pResult;
}

//-----------------------------------------------------------------------------
// ����: ����һ���������󲢷���
//-----------------------------------------------------------------------------
CDbParam* CDbParamList::CreateParam(const string& strName, int nNumber)
{
	CDbParam *pResult = m_pDbQuery->GetDatabase()->CreateDbParam();

	pResult->m_pDbQuery = m_pDbQuery;
	pResult->m_strName = strName;
	pResult->m_nNumber = nNumber;

	return pResult;
}

//-----------------------------------------------------------------------------
// ����: �������Ʒ��ض�Ӧ�Ĳ������������򷵻�NULL
//-----------------------------------------------------------------------------
CDbParam* CDbParamList::ParamByName(const string& strName)
{
	CDbParam *pResult = FindParam(strName);
	if (!pResult)
	{
		pResult = CreateParam(strName, 0);
		m_Items.Add(pResult);
	}

	return pResult;
}

//-----------------------------------------------------------------------------
// ����: �������(1-based)���ض�Ӧ�Ĳ������������򷵻�NULL
//-----------------------------------------------------------------------------
CDbParam* CDbParamList::ParamByNumber(int nNumber)
{
	CDbParam *pResult = FindParam(nNumber);
	if (!pResult)
	{
		pResult = CreateParam("", nNumber);
		m_Items.Add(pResult);
	}

	return pResult;
}

//-----------------------------------------------------------------------------
// ����: �ͷŲ���������ֶ����ݶ���
//-----------------------------------------------------------------------------
void CDbParamList::Clear()
{
	for (int i = 0; i < m_Items.GetCount(); i++)
		delete (CDbParam*)m_Items[i];
	m_Items.Clear();
}

///////////////////////////////////////////////////////////////////////////////
// class CDbDataSet

CDbDataSet::CDbDataSet(CDbQuery *pDbQuery) :
	m_pDbQuery(pDbQuery)
{
	// nothing
}

CDbDataSet::~CDbDataSet()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: ȡ�õ�ǰ���ݼ��еļ�¼����
//-----------------------------------------------------------------------------
UINT64 CDbDataSet::GetRecordCount()
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return 0;
}

//-----------------------------------------------------------------------------
// ����: �������ݼ��Ƿ�Ϊ��
//-----------------------------------------------------------------------------
bool CDbDataSet::IsEmpty()
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return true;
}

//-----------------------------------------------------------------------------
// ����: ȡ�õ�ǰ��¼�е��ֶ�����
//-----------------------------------------------------------------------------
int CDbDataSet::GetFieldCount()
{
	return m_DbFieldDefList.GetCount();
}

//-----------------------------------------------------------------------------
// ����: ȡ�õ�ǰ��¼��ĳ���ֶεĶ��� (nIndex: 0-based)
//-----------------------------------------------------------------------------
CDbFieldDef* CDbDataSet::GetFieldDefs(int nIndex)
{
	if (nIndex >= 0 && nIndex < m_DbFieldDefList.GetCount())
		return m_DbFieldDefList[nIndex];
	else
		IseThrowDbException(SEM_INDEX_ERROR);

	return NULL;
}

//-----------------------------------------------------------------------------
// ����: ȡ�õ�ǰ��¼��ĳ���ֶε����� (nIndex: 0-based)
//-----------------------------------------------------------------------------
CDbField* CDbDataSet::GetFields(int nIndex)
{
	if (nIndex >= 0 && nIndex < m_DbFieldList.GetCount())
		return m_DbFieldList[nIndex];
	else
		IseThrowDbException(SEM_INDEX_ERROR);

	return NULL;
}

//-----------------------------------------------------------------------------
// ����: ȡ�õ�ǰ��¼��ĳ���ֶε�����
// ����:
//   strName - �ֶ���
//-----------------------------------------------------------------------------
CDbField* CDbDataSet::GetFields(const string& strName)
{
	int nIndex = m_DbFieldDefList.IndexOfName(strName);

	if (nIndex >= 0)
		return GetFields(nIndex);
	else
	{
		CStrList FieldNames;
		m_DbFieldDefList.GetFieldNameList(FieldNames);
		string strFieldNameList = FieldNames.GetCommaText();

		string strErrMsg = FormatString(SEM_FIELD_NAME_ERROR, strName.c_str(), strFieldNameList.c_str());
		IseThrowDbException(strErrMsg.c_str());
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbQuery

CDbQuery::CDbQuery(CDatabase *pDatabase) :
	m_pDatabase(pDatabase),
	m_pDbConnection(NULL),
	m_pDbParamList(NULL)
{
	m_pDbParamList = pDatabase->CreateDbParamList(this);
}

CDbQuery::~CDbQuery()
{
	delete m_pDbParamList;

	if (m_pDbConnection)
		m_pDatabase->GetDbConnectionPool()->ReturnConnection(m_pDbConnection);
}

void CDbQuery::EnsureConnected()
{
	if (!m_pDbConnection)
		m_pDbConnection = m_pDatabase->GetDbConnectionPool()->GetConnection();
}

//-----------------------------------------------------------------------------
// ����: ����SQL���
//-----------------------------------------------------------------------------
void CDbQuery::SetSql(const string& strSql)
{
	m_strSql = strSql;
	m_pDbParamList->Clear();

	DoSetSql(strSql);
}

//-----------------------------------------------------------------------------
// ����: ��������ȡ�ò�������
// ��ע:
//   ȱʡ����´˹��ܲ����ã�������Ҫ���ô˹��ܣ��ɵ��ã�
//   return m_pDbParamList->ParamByName(strName);
//-----------------------------------------------------------------------------
CDbParam* CDbQuery::ParamByName(const string& strName)
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return NULL;
}

//-----------------------------------------------------------------------------
// ����: �������(1-based)ȡ�ò�������
// ��ע:
//   ȱʡ����´˹��ܲ����ã�������Ҫ���ô˹��ܣ��ɵ��ã�
//   return m_pDbParamList->ParamByNumber(nNumber);
//-----------------------------------------------------------------------------
CDbParam* CDbQuery::ParamByNumber(int nNumber)
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return NULL;
}

//-----------------------------------------------------------------------------
// ����: ִ��SQL (�޷��ؽ��)
//-----------------------------------------------------------------------------
void CDbQuery::Execute()
{
	EnsureConnected();
	DoExecute(NULL);
}

//-----------------------------------------------------------------------------
// ����: ִ��SQL (�������ݼ�)
//-----------------------------------------------------------------------------
CDbDataSet* CDbQuery::Query()
{
	EnsureConnected();

	CDbDataSet *pDataSet = m_pDatabase->CreateDbDataSet(this);
	try
	{
		// ִ�в�ѯ
		DoExecute(pDataSet);
		// ��ʼ�����ݼ�
		pDataSet->InitDataSet();
		// ��ʼ�����ݼ����ֶεĶ���
		pDataSet->InitFieldDefs();
	}
	catch (CException&)
	{
		delete pDataSet;
		pDataSet = NULL;
		throw;
	}

	return pDataSet;
}

//-----------------------------------------------------------------------------
// ����: ת���ַ���ʹ֮��SQL�кϷ� (str �пɺ� '\0' �ַ�)
//-----------------------------------------------------------------------------
string CDbQuery::EscapeString(const string& str)
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return "";
}

//-----------------------------------------------------------------------------
// ����: ȡ��ִ��SQL����Ӱ�������
//-----------------------------------------------------------------------------
UINT CDbQuery::GetAffectedRowCount()
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return 0;
}

//-----------------------------------------------------------------------------
// ����: ȡ�����һ��������������ID��ֵ
//-----------------------------------------------------------------------------
UINT64 CDbQuery::GetLastInsertId()
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return 0;
}

//-----------------------------------------------------------------------------
// ����: ȡ�ò�ѯ�����õ����ݿ�����
//-----------------------------------------------------------------------------
CDbConnection* CDbQuery::GetDbConnection()
{
	EnsureConnected();
	return m_pDbConnection;
}

///////////////////////////////////////////////////////////////////////////////
// class CDatabase

CDatabase::CDatabase()
{
	m_pDbConnParams = NULL;
	m_pDbOptions = NULL;
	m_pDbConnectionPool = NULL;
}

CDatabase::~CDatabase()
{
	delete m_pDbConnParams;
	delete m_pDbOptions;
	delete m_pDbConnectionPool;
}

void CDatabase::EnsureInited()
{
	if (!m_pDbConnParams)
	{
		m_pDbConnParams = CreateDbConnParams();
		m_pDbOptions = CreateDbOptions();
		m_pDbConnectionPool = CreateDbConnectionPool();
	}
}

CDbConnParams* CDatabase::GetDbConnParams()
{
	EnsureInited();
	return m_pDbConnParams;
}

CDbOptions* CDatabase::GetDbOptions()
{
	EnsureInited();
	return m_pDbOptions;
}

CDbConnectionPool* CDatabase::GetDbConnectionPool()
{
	EnsureInited();
	return m_pDbConnectionPool;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
