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
// �ļ�����: ise_dbi_mysql.cpp
// ��������: MySQL���ݿ����
///////////////////////////////////////////////////////////////////////////////

#include "ise_dbi_mysql.h"
#include "ise_sysutils.h"
#include "ise_errmsgs.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class CMySqlConnection

CMySqlConnection::CMySqlConnection(CDatabase* pDatabase) :
	CDbConnection(pDatabase)
{
	memset(&m_ConnObject, 0, sizeof(m_ConnObject));
}

CMySqlConnection::~CMySqlConnection()
{
	// nothing
}

//-----------------------------------------------------------------------------
// ����: �������� (��ʧ�����׳��쳣)
//-----------------------------------------------------------------------------
void CMySqlConnection::DoConnect()
{
	static CCriticalSection s_Lock;
	CAutoLocker Locker(s_Lock);

	if (mysql_init(&m_ConnObject) == NULL)
		IseThrowDbException(SEM_MYSQL_INIT_ERROR);

	Logger().WriteFmt(SEM_MYSQL_CONNECTING, &m_ConnObject);

	int nValue = 0;
	mysql_options(&m_ConnObject, MYSQL_OPT_RECONNECT, (char*)&nValue);

	if (mysql_real_connect(&m_ConnObject,
		m_pDatabase->GetDbConnParams()->GetHostName().c_str(),
		m_pDatabase->GetDbConnParams()->GetUserName().c_str(),
		m_pDatabase->GetDbConnParams()->GetPassword().c_str(),
		m_pDatabase->GetDbConnParams()->GetDbName().c_str(),
		m_pDatabase->GetDbConnParams()->GetPort(), NULL, 0) == NULL)
	{
		mysql_close(&m_ConnObject);
		IseThrowDbException(FormatString(SEM_MYSQL_REAL_CONNECT_ERROR, mysql_error(&m_ConnObject)).c_str());
	}

	// for MYSQL 5.0.7 or higher
	string strInitialCharSet = m_pDatabase->GetDbOptions()->GetInitialCharSet();
	if (!strInitialCharSet.empty())
		mysql_set_character_set(&m_ConnObject, strInitialCharSet.c_str());

	Logger().WriteFmt(SEM_MYSQL_CONNECTED, &m_ConnObject);
}

//-----------------------------------------------------------------------------
// ����: �Ͽ�����
//-----------------------------------------------------------------------------
void CMySqlConnection::DoDisconnect()
{
	mysql_close(&m_ConnObject);
}

///////////////////////////////////////////////////////////////////////////////
// class CMySqlField

CMySqlField::CMySqlField()
{
	m_pDataPtr = NULL;
	m_nDataSize = 0;
}

void CMySqlField::SetData(void *pDataPtr, int nDataSize)
{
	m_pDataPtr = (char*)pDataPtr;
	m_nDataSize = nDataSize;
}

//-----------------------------------------------------------------------------
// ����: ���ַ����ͷ����ֶ�ֵ
//-----------------------------------------------------------------------------
string CMySqlField::AsString() const
{
	string strResult;

	if (m_pDataPtr && m_nDataSize > 0)
		strResult.assign(m_pDataPtr, m_nDataSize);

	return strResult;
}

///////////////////////////////////////////////////////////////////////////////
// class CMySqlDataSet

CMySqlDataSet::CMySqlDataSet(CDbQuery* pDbQuery) :
	CDbDataSet(pDbQuery),
	m_pRes(NULL),
	m_pRow(NULL)
{
	// nothing
}

CMySqlDataSet::~CMySqlDataSet()
{
	FreeDataSet();
}

MYSQL& CMySqlDataSet::GetConnObject()
{
	return ((CMySqlConnection*)m_pDbQuery->GetDbConnection())->GetConnObject();
}

//-----------------------------------------------------------------------------
// ����: �ͷ����ݼ�
//-----------------------------------------------------------------------------
void CMySqlDataSet::FreeDataSet()
{
	if (m_pRes)
		mysql_free_result(m_pRes);
	m_pRes = NULL;
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�����ݼ� (����ʼ��ʧ�����׳��쳣)
//-----------------------------------------------------------------------------
void CMySqlDataSet::InitDataSet()
{
	// ��MySQL������һ���Ի�ȡ������
	m_pRes = mysql_store_result(&GetConnObject());

	// �����ȡʧ��
	if (!m_pRes)
	{
		IseThrowDbException(FormatString(SEM_MYSQL_STORE_RESULT_ERROR,
			mysql_error(&GetConnObject())).c_str());
	}
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�����ݼ����ֶεĶ���
//-----------------------------------------------------------------------------
void CMySqlDataSet::InitFieldDefs()
{
	MYSQL_FIELD *pMySqlFields;
	CDbFieldDef* pFieldDef;
	int nFieldCount;

	m_DbFieldDefList.Clear();
	nFieldCount = mysql_num_fields(m_pRes);
	pMySqlFields = mysql_fetch_fields(m_pRes);

	if (nFieldCount <= 0)
		IseThrowDbException(SEM_MYSQL_NUM_FIELDS_ERROR);

	for (int i = 0; i < nFieldCount; i++)
	{
		pFieldDef = new CDbFieldDef();
		pFieldDef->SetData(pMySqlFields[i].name, pMySqlFields[i].type);
		m_DbFieldDefList.Add(pFieldDef);
	}
}

//-----------------------------------------------------------------------------
// ����: ���α�ָ����ʼλ��(��һ����¼֮ǰ)
//-----------------------------------------------------------------------------
bool CMySqlDataSet::Rewind()
{
	if (GetRecordCount() > 0)
	{
		mysql_data_seek(m_pRes, 0);
		return true;
	}
	else
		return false;
}

//-----------------------------------------------------------------------------
// ����: ���α�ָ����һ����¼
//-----------------------------------------------------------------------------
bool CMySqlDataSet::Next()
{
	m_pRow = mysql_fetch_row(m_pRes);
	if (m_pRow)
	{
		CMySqlField* pField;
		int nFieldCount;
		unsigned long* pLengths;

		nFieldCount = mysql_num_fields(m_pRes);
		pLengths = (unsigned long*)mysql_fetch_lengths(m_pRes);

		for (int i = 0; i < nFieldCount; i++)
		{
			if (i < m_DbFieldList.GetCount())
			{
				pField = (CMySqlField*)m_DbFieldList[i];
			}
			else
			{
				pField = new CMySqlField();
				m_DbFieldList.Add(pField);
			}

			pField->SetData(m_pRow[i], pLengths[i]);
		}
	}

	return (m_pRow != NULL);
}

//-----------------------------------------------------------------------------
// ����: ȡ�ü�¼����
// ��ע: mysql_num_rows ʵ����ֻ��ֱ�ӷ��� m_pRes->row_count������Ч�ʺܸߡ�
//-----------------------------------------------------------------------------
UINT64 CMySqlDataSet::GetRecordCount()
{
	if (m_pRes)
		return (UINT64)mysql_num_rows(m_pRes);
	else
		return 0;
}

//-----------------------------------------------------------------------------
// ����: �������ݼ��Ƿ�Ϊ��
//-----------------------------------------------------------------------------
bool CMySqlDataSet::IsEmpty()
{
	return (GetRecordCount() == 0);
}

///////////////////////////////////////////////////////////////////////////////
// class CMySqlQuery

CMySqlQuery::CMySqlQuery(CDatabase* pDatabase) :
	CDbQuery(pDatabase)
{
	// nothing
}

CMySqlQuery::~CMySqlQuery()
{
	// nothing
}

MYSQL& CMySqlQuery::GetConnObject()
{
	return ((CMySqlConnection*)m_pDbConnection)->GetConnObject();
}

//-----------------------------------------------------------------------------
// ����: ִ��SQL (�� pResultDataSet Ϊ NULL�����ʾ�����ݼ����ء���ʧ�����׳��쳣)
//-----------------------------------------------------------------------------
void CMySqlQuery::DoExecute(CDbDataSet *pResultDataSet)
{
	/*
	ժ��MYSQL�ٷ��ֲ�:
	Upon connection, mysql_real_connect() sets the reconnect flag (part of the
	MYSQL structure) to a value of 1 in versions of the API older than 5.0.3,
	or 0 in newer versions. A value of 1 for this flag indicates that if a
	statement cannot be performed because of a lost connection, to try reconnecting
	to the server before giving up. You can use the MYSQL_OPT_RECONNECT option
	to mysql_options() to control reconnection behavior.

	����ֻҪ�� mysql_real_connect() ���ӵ����ݿ�(������ mysql_connect())����ô
	�� mysql_real_query() ����ʱ��ʹ���Ӷ�ʧ(����TCP���ӶϿ�)���õ���Ҳ�᳢��
	ȥ�����������ݿ⡣�����Ծ����Ա�֤����ʵ��

	ע:
	1. ���� <= 5.0.3 �İ汾��MySQLĬ�ϻ��������˺�İ汾����� mysql_options()
	   ��ȷָ�� MYSQL_OPT_RECONNECT Ϊ true���Ż�������
	2. Ϊ�������Ӷ�ʧ����������ִ�С����ݿ�ս�������ʱҪִ�е������������ȷ
	   ָ������ mysql_real_query() �Զ������������ɳ�����ʽ������
	*/

	for (int nTimes = 0; nTimes < 2; nTimes++)
	{
		int r = mysql_real_query(&GetConnObject(), m_strSql.c_str(), (int)m_strSql.length());

		// ���ִ��SQLʧ��
		if (r != 0)
		{
			// ������״Σ����Ҵ�������Ϊ���Ӷ�ʧ������������
			if (nTimes == 0)
			{
				int nErrNo = mysql_errno(&GetConnObject());
				if (nErrNo == CR_SERVER_GONE_ERROR || nErrNo == CR_SERVER_LOST)
				{
					Logger().WriteStr(SEM_MYSQL_LOST_CONNNECTION);

					// ǿ����������
					GetDbConnection()->ActivateConnection(true);
					continue;
				}
			}

			// �����׳��쳣
			string strSql(m_strSql);
			if (strSql.length() > 1024*2)
			{
				strSql.resize(100);
				strSql += "...";
			}

			string strErrMsg = FormatString("%s; Error: %s", strSql.c_str(), mysql_error(&GetConnObject()));
			IseThrowDbException(strErrMsg.c_str());
		}
		else break;
	}
}

//-----------------------------------------------------------------------------
// ����: ת���ַ���ʹ֮��SQL�кϷ�
//-----------------------------------------------------------------------------
string CMySqlQuery::EscapeString(const string& str)
{
	if (str.empty()) return "";

	int nSrcLen = (int)str.size();
	CBuffer Buffer(nSrcLen * 2 + 1);
	char *pEnd;

	EnsureConnected();

	pEnd = (char*)Buffer.Data();
	pEnd += mysql_real_escape_string(&GetConnObject(), pEnd, str.c_str(), nSrcLen);
	*pEnd = '\0';

	return (char*)Buffer.Data();
}

//-----------------------------------------------------------------------------
// ����: ��ȡִ��SQL����Ӱ�������
//-----------------------------------------------------------------------------
UINT CMySqlQuery::GetAffectedRowCount()
{
	UINT nResult = 0;

	if (m_pDbConnection)
		nResult = (UINT)mysql_affected_rows(&GetConnObject());

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ��ȡ���һ��������������ID��ֵ
//-----------------------------------------------------------------------------
UINT64 CMySqlQuery::GetLastInsertId()
{
	UINT64 nResult = 0;

	if (m_pDbConnection)
		nResult = (UINT64)mysql_insert_id(&GetConnObject());

	return nResult;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
