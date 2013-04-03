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
// ise_dbi_mysql.h
// Classes:
//   * CMySqlConnection       - ���ݿ�������
//   * CMySqlField            - �ֶ�������
//   * CMySqlDataSet          - ���ݼ���
//   * CMySqlQuery            - ���ݲ�ѯ����
//   * CMySqlDatabase         - ���ݿ���
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_DBI_MYSQL_H_
#define _ISE_DBI_MYSQL_H_

#include "ise_database.h"

#include <errmsg.h>
#include <mysql.h>

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class CMySqlConnection;
class CMySqlField;
class CMySqlDataSet;
class CMySqlQuery;
class CMySqlDatabase;

///////////////////////////////////////////////////////////////////////////////
// class CMySqlConnection - ���ݿ�������

class CMySqlConnection : public CDbConnection
{
private:
	MYSQL m_ConnObject;            // ���Ӷ���

public:
	CMySqlConnection(CDatabase *pDatabase);
	virtual ~CMySqlConnection();

	// �������� (��ʧ�����׳��쳣)
	virtual void DoConnect();
	// �Ͽ�����
	virtual void DoDisconnect();

	// ȡ��MySQL���Ӷ���
	MYSQL& GetConnObject() { return m_ConnObject; }
};

///////////////////////////////////////////////////////////////////////////////
// class CMySqlField - �ֶ�������

class CMySqlField : public CDbField
{
private:
	char* m_pDataPtr;               // ָ���ֶ�����
	int m_nDataSize;                // �ֶ����ݵ����ֽ���
public:
	CMySqlField();
	virtual ~CMySqlField() {}

	void SetData(void *pDataPtr, int nDataSize);
	virtual bool IsNull() const { return (m_pDataPtr == NULL); }
	virtual string AsString() const;
};

///////////////////////////////////////////////////////////////////////////////
// class CMySqlDataSet - ���ݼ���

class CMySqlDataSet : public CDbDataSet
{
private:
	MYSQL_RES* m_pRes;      // MySQL��ѯ�����
	MYSQL_ROW m_pRow;       // MySQL��ѯ�����

private:
	MYSQL& GetConnObject();
	void FreeDataSet();

protected:
	virtual void InitDataSet();
	virtual void InitFieldDefs();

public:
	CMySqlDataSet(CDbQuery* pDbQuery);
	virtual ~CMySqlDataSet();

	virtual bool Rewind();
	virtual bool Next();

	virtual UINT64 GetRecordCount();
	virtual bool IsEmpty();
};

///////////////////////////////////////////////////////////////////////////////
// class CMySqlQuery - ��ѯ����

class CMySqlQuery : public CDbQuery
{
private:
	MYSQL& GetConnObject();

protected:
	virtual void DoExecute(CDbDataSet *pResultDataSet);

public:
	CMySqlQuery(CDatabase *pDatabase);
	virtual ~CMySqlQuery();

	virtual string EscapeString(const string& str);
	virtual UINT GetAffectedRowCount();
	virtual UINT64 GetLastInsertId();
};

///////////////////////////////////////////////////////////////////////////////
// class CMySqlDatabase

class CMySqlDatabase : public CDatabase
{
public:
	virtual CDbConnection* CreateDbConnection() { return new CMySqlConnection(this); }
	virtual CDbDataSet* CreateDbDataSet(CDbQuery* pDbQuery) { return new CMySqlDataSet(pDbQuery); }
	virtual CDbQuery* CreateDbQuery() { return new CMySqlQuery(this); }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_DBI_MYSQL_H_
