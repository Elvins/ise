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
// ise_database.h
// Classes:
//   * CDbConnParams          - ���ݿ����Ӳ�����
//   * CDbOptions             - ���ݿ����ò�����
//   * CDbConnection          - ���ݿ����ӻ���
//   * CDbConnectionPool      - ���ݿ����ӳػ���
//   * CDbFieldDef            - �ֶζ�����
//   * CDbFieldDefList        - �ֶζ����б���
//   * CDbField               - �ֶ�������
//   * CDbFieldList           - �ֶ������б���
//   * CDbParam               - SQL������
//   * CDbParamList           - SQL�����б���
//   * CDbDataSet             - ���ݼ���
//   * CDbQuery               - ���ݲ�ѯ����
//   * CDbQueryWrapper        - ���ݲ�ѯ����װ��
//   * CDbDataSetWrapper      - ���ݼ���װ��
//   * CDatabase              - ���ݿ���
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_DATABASE_H_
#define _ISE_DATABASE_H_

#include "ise_options.h"
#include "ise_global_defs.h"
#include "ise_classes.h"
#include "ise_socket.h"
#include "ise_exceptions.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class CDbConnParams;
class CDbOptions;
class CDbConnection;
class CDbConnectionPool;
class CDbFieldDef;
class CDbFieldDefList;
class CDbField;
class CDbFieldList;
class CDbParam;
class CDbParamList;
class CDbDataSet;
class CDbQuery;
class CDbQueryWrapper;
class CDbDataSetWrapper;
class CDatabase;

///////////////////////////////////////////////////////////////////////////////
// class CDbConnParams - ���ݿ����Ӳ�����

class CDbConnParams
{
private:
	string m_strHostName;           // ������ַ
	string m_strUserName;           // �û���
	string m_strPassword;           // �û�����
	string m_strDbName;             // ���ݿ���
	int m_nPort;                    // ���Ӷ˿ں�

public:
	CDbConnParams();
	CDbConnParams(const CDbConnParams& src);
	CDbConnParams(const string& strHostName, const string& strUserName,
		const string& strPassword, const string& strDbName, const int nPort);

	string GetHostName() const { return m_strHostName; }
	string GetUserName() const { return m_strUserName; }
	string GetPassword() const { return m_strPassword; }
	string GetDbName() const { return m_strDbName; }
	int GetPort() const { return m_nPort; }

	void SetHostName(const string& strValue) { m_strHostName = strValue; }
	void SetUserName(const string& strValue) { m_strUserName = strValue; }
	void SetPassword(const string& strValue) { m_strPassword = strValue; }
	void SetDbName(const string& strValue) { m_strDbName = strValue; }
	void SetPort(const int nValue) { m_nPort = nValue; }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbOptions - ���ݿ����ò�����

class CDbOptions
{
public:
	enum { DEF_MAX_DB_CONNECTIONS = 100 };      // ���ӳ����������ȱʡֵ

private:
	int m_nMaxDbConnections;                    // ���ӳ�����������������
	CStrList m_InitialSqlCmdList;               // ���ݿ�ս�������ʱҪִ�е�����
	string m_strInitialCharSet;                 // ���ݿ�ս�������ʱҪ���õ��ַ���

public:
	CDbOptions();

	int GetMaxDbConnections() const { return m_nMaxDbConnections; }
	CStrList& InitialSqlCmdList() { return m_InitialSqlCmdList; }
	string GetInitialCharSet() { return m_strInitialCharSet; }

	void SetMaxDbConnections(int nValue);
	void SetInitialSqlCmd(const string& strValue);
	void SetInitialCharSet(const string& strValue);
};

///////////////////////////////////////////////////////////////////////////////
// class CDbConnection - ���ݿ����ӻ���

class CDbConnection
{
public:
	friend class CDbConnectionPool;

protected:
	CDatabase *m_pDatabase;         // CDatabase ���������
	bool m_bConnected;              // �Ƿ��ѽ�������
	bool m_bBusy;                   // �����ӵ�ǰ�Ƿ�����ռ��

protected:
	// �������ݿ����Ӳ������������ (��ʧ�����׳��쳣)
	void Connect();
	// �Ͽ����ݿ����Ӳ������������
	void Disconnect();
	// �ս�������ʱִ������
	void ExecCmdOnConnected();

	// ConnectionPool ���������к������������ӵ�ʹ�����
	bool GetDbConnection();         // ��������
	void ReturnDbConnection();      // �黹����
	bool IsBusy();                  // �����Ƿ񱻽���

protected:
	// �����ݿ⽨������(��ʧ�����׳��쳣)
	virtual void DoConnect() {}
	// �����ݿ�Ͽ�����
	virtual void DoDisconnect() {}

public:
	CDbConnection(CDatabase *pDatabase);
	virtual ~CDbConnection();

	// �������ݿ�����
	void ActivateConnection(bool bForce = false);
};

///////////////////////////////////////////////////////////////////////////////
// class CDbConnectionPool - ���ݿ����ӳػ���
//
// ����ԭ��:
// 1. ����ά��һ�������б�����ǰ����������(�������ӡ�æ����)����ʼΪ�գ�����һ���������ޡ�
// 2. ��������:
//    ���ȳ��Դ������б����ҳ�һ���������ӣ����ҵ������ɹ���ͬʱ��������Ϊæ״̬����û�ҵ�
//    ��: (1)��������δ�ﵽ���ޣ��򴴽�һ���µĿ������ӣ�(2)���������Ѵﵽ���ޣ������ʧ�ܡ�
//    ������ʧ��(�������������޷����������ӵ�)�����׳��쳣��
// 3. �黹����:
//    ֻ�轫������Ϊ����״̬���ɣ�����(Ҳ����)�Ͽ����ݿ����ӡ�

class CDbConnectionPool
{
protected:
	CDatabase *m_pDatabase;         // ���� CDatabase ����
	CList m_DbConnectionList;       // ��ǰ�����б� (CDbConnection*[])�������������Ӻ�æ����
	CCriticalSection m_Lock;        // ������

protected:
	void ClearPool();

public:
	CDbConnectionPool(CDatabase *pDatabase);
	virtual ~CDbConnectionPool();

	// ����һ�����õĿ������� (��ʧ�����׳��쳣)
	virtual CDbConnection* GetConnection();
	// �黹���ݿ�����
	virtual void ReturnConnection(CDbConnection *pDbConnection);
};

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldDef - �ֶζ�����

class CDbFieldDef
{
protected:
	string m_strName;               // �ֶ�����
	int m_nType;                    // �ֶ�����(���������ඨ��)

public:
	CDbFieldDef() {}
	CDbFieldDef(const string& strName, int nType);
	CDbFieldDef(const CDbFieldDef& src);
	virtual ~CDbFieldDef() {}

	void SetData(char *sName, int nType) { m_strName = sName; m_nType = nType; }

	string GetName() const { return m_strName; }
	int GetType() const { return m_nType; }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldDefList - �ֶζ����б���

class CDbFieldDefList
{
private:
	CList m_Items;                  // (CDbFieldDef* [])

public:
	CDbFieldDefList();
	virtual ~CDbFieldDefList();

	// ���һ���ֶζ������
	void Add(CDbFieldDef *pFieldDef);
	// �ͷŲ���������ֶζ������
	void Clear();
	// �����ֶ�����Ӧ���ֶ����(0-based)
	int IndexOfName(const string& strName);
	// ����ȫ���ֶ���
	void GetFieldNameList(CStrList& List);

	CDbFieldDef* operator[] (int nIndex);
	int GetCount() const { return m_Items.GetCount(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbField - �ֶ�������

class CDbField
{
public:
	CDbField();
	virtual ~CDbField() {}

	virtual bool IsNull() const { return false; }
	virtual int AsInteger(int nDefault = 0) const;
	virtual INT64 AsInt64(INT64 nDefault = 0) const;
	virtual double AsFloat(double fDefault = 0) const;
	virtual bool AsBoolean(bool bDefault = false) const;
	virtual string AsString() const { return ""; };
};

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldList - �ֶ������б���

class CDbFieldList
{
private:
	CList m_Items;       // (CDbField* [])

public:
	CDbFieldList();
	virtual ~CDbFieldList();

	// ���һ���ֶ����ݶ���
	void Add(CDbField *pField);
	// �ͷŲ���������ֶ����ݶ���
	void Clear();

	CDbField* operator[] (int nIndex);
	int GetCount() const { return m_Items.GetCount(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbParam - SQL������

class CDbParam
{
public:
	friend class CDbParamList;

protected:
	CDbQuery *m_pDbQuery;   // CDbQuery ��������
	string m_strName;       // ��������
	int m_nNumber;          // �������(1-based)

public:
	CDbParam() : m_pDbQuery(NULL), m_nNumber(0) {}
	virtual ~CDbParam() {}

	virtual void SetInt(int nValue) {}
	virtual void SetFloat(double fValue) {}
	virtual void SetString(const string& strValue) {}
};

///////////////////////////////////////////////////////////////////////////////
// class CDbParamList - SQL�����б���

class CDbParamList
{
protected:
	CDbQuery *m_pDbQuery;   // CDbQuery ��������
	CList m_Items;          // (CDbParam* [])

protected:
	virtual CDbParam* FindParam(const string& strName);
	virtual CDbParam* FindParam(int nNumber);
	virtual CDbParam* CreateParam(const string& strName, int nNumber);

public:
	CDbParamList(CDbQuery *pDbQuery);
	virtual ~CDbParamList();

	virtual CDbParam* ParamByName(const string& strName);
	virtual CDbParam* ParamByNumber(int nNumber);

	void Clear();
};

///////////////////////////////////////////////////////////////////////////////
// class CDbDataSet - ���ݼ���
//
// ˵��:
// 1. ����ֻ�ṩ����������ݼ��Ĺ��ܡ�
// 2. ���ݼ���ʼ��(InitDataSet)���α�ָ���һ����¼֮ǰ������� Next() ��ָ���һ����¼��
//    ���͵����ݼ���������Ϊ: while(DataSet.Next()) { ... }
//
// Լ��:
// 1. CDbQuery.Query() ����һ���µ� CDbDataSet ����֮����������� CDbDataSet ����
//    �������� CDbQuery ����
// 2. CDbQuery ��ִ�в�ѯA�󴴽���һ�����ݼ�A��֮����ִ�в�ѯBǰӦ�ر����ݼ�A��

class CDbDataSet
{
public:
	friend class CDbQuery;

protected:
	CDbQuery *m_pDbQuery;             // CDbQuery ��������
	CDbFieldDefList m_DbFieldDefList; // �ֶζ�������б�
	CDbFieldList m_DbFieldList;       // �ֶ����ݶ����б�

protected:
	// ��ʼ�����ݼ� (��ʧ�����׳��쳣)
	virtual void InitDataSet() = 0;
	// ��ʼ�����ݼ����ֶεĶ���
	virtual void InitFieldDefs() = 0;

public:
	CDbDataSet(CDbQuery *pDbQuery);
	virtual ~CDbDataSet();

	// ���α�ָ����ʼλ��(��һ����¼֮ǰ)
	virtual bool Rewind() = 0;
	// ���α�ָ����һ����¼
	virtual bool Next() = 0;

	// ȡ�õ�ǰ���ݼ��еļ�¼����
	virtual UINT64 GetRecordCount();
	// �������ݼ��Ƿ�Ϊ��
	virtual bool IsEmpty();

	// ȡ�õ�ǰ��¼�е��ֶ�����
	int GetFieldCount();
	// ȡ�õ�ǰ��¼��ĳ���ֶεĶ��� (nIndex: 0-based)
	CDbFieldDef* GetFieldDefs(int nIndex);
	// ȡ�õ�ǰ��¼��ĳ���ֶε����� (nIndex: 0-based)
	CDbField* GetFields(int nIndex);
	CDbField* GetFields(const string& strName);
};

///////////////////////////////////////////////////////////////////////////////
// class CDbQuery - ���ݲ�ѯ����
//
// ����ԭ��:
// 1. ִ��SQL: �����ӳ�ȡ��һ���������ӣ�Ȼ�����ô�����ִ��SQL�����黹���ӡ�

class CDbQuery
{
public:
	friend class CDbConnection;

protected:
	CDatabase *m_pDatabase;         // CDatabase ��������
	CDbConnection *m_pDbConnection; // CDbConnection ��������
	CDbParamList *m_pDbParamList;   // SQL �����б�
	string m_strSql;                // ��ִ�е�SQL���

protected:
	void EnsureConnected();

protected:
	// ����SQL���
	virtual void DoSetSql(const string& strSql) {}
	// ִ��SQL (�� pResultDataSet Ϊ NULL�����ʾ�����ݼ����ء���ʧ�����׳��쳣)
	virtual void DoExecute(CDbDataSet *pResultDataSet) {}

public:
	CDbQuery(CDatabase *pDatabase);
	virtual ~CDbQuery();

	// ����SQL���
	void SetSql(const string& strSql);

	// ��������ȡ�ò�������
	virtual CDbParam* ParamByName(const string& strName);
	// �������(1-based)ȡ�ò�������
	virtual CDbParam* ParamByNumber(int nNumber);

	// ִ��SQL (�޷��ؽ��, ��ʧ�����׳��쳣)
	void Execute();
	// ִ��SQL (�������ݼ�, ��ʧ�����׳��쳣)
	CDbDataSet* Query();

	// ת���ַ���ʹ֮��SQL�кϷ� (str �пɺ� '\0' �ַ�)
	virtual string EscapeString(const string& str);
	// ȡ��ִ��SQL����Ӱ�������
	virtual UINT GetAffectedRowCount();
	// ȡ�����һ��������������ID��ֵ
	virtual UINT64 GetLastInsertId();

	// ȡ�ò�ѯ�����õ����ݿ�����
	CDbConnection* GetDbConnection();
	// ȡ�� CDatabase ����
	CDatabase* GetDatabase() { return m_pDatabase; }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbQueryWrapper - ��ѯ����װ��
//
// ˵��: �������ڰ�װ CDbQuery �����Զ��ͷű���װ�Ķ��󣬷�ֹ��Դй©��
// ʾ��:
//      int main()
//      {
//          CDbQueryWrapper qry( MyDatabase.CreateDbQuery() );
//          qry->SetSql("select * from users");
//          /* ... */
//          // ջ���� qry ���Զ����٣����ͬʱ����װ�ĶѶ���Ҳ�Զ��ͷš�
//      }

class CDbQueryWrapper
{
private:
	CDbQuery *m_pDbQuery;

public:
	CDbQueryWrapper(CDbQuery *pDbQuery) : m_pDbQuery(pDbQuery) {}
	virtual ~CDbQueryWrapper() { delete m_pDbQuery; }

	CDbQuery* operator -> () { return m_pDbQuery; }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbDataSetWrapper - ���ݼ���װ��
//
// ˵��:
// 1. �������ڰ�װ CDbDataSet �����Զ��ͷű���װ�Ķ��󣬷�ֹ��Դй©��
// 2. ÿ�θ���װ����ֵ(Wrapper = DataSet)���ϴα���װ�Ķ����Զ��ͷš�
// ʾ��:
//      int main()
//      {
//          CDbQueryWrapper qry( MyDatabase.CreateDbQuery() );
//          CDbDataSetWrapper ds;
//
//          qry->SetSql("select * from users");
//          ds = qry->Query();
//          /* ... */
//
//          // ջ���� qry �� ds ���Զ����٣����ͬʱ����װ�ĶѶ���Ҳ�Զ��ͷš�
//      }

class CDbDataSetWrapper
{
private:
	CDbDataSet *m_pDbDataSet;

public:
	CDbDataSetWrapper() : m_pDbDataSet(NULL) {}
	virtual ~CDbDataSetWrapper() { delete m_pDbDataSet; }

	CDbDataSetWrapper& operator = (CDbDataSet *pDataSet)
	{
		if (m_pDbDataSet) delete m_pDbDataSet;
		m_pDbDataSet = pDataSet;
		return *this;
	}

	CDbDataSet* operator -> () { return m_pDbDataSet; }
};

///////////////////////////////////////////////////////////////////////////////
// class CDatabase - ���ݿ���

class CDatabase
{
protected:
	CDbConnParams *m_pDbConnParams;             // ���ݿ����Ӳ���
	CDbOptions *m_pDbOptions;                   // ���ݿ����ò���
	CDbConnectionPool *m_pDbConnectionPool;     // ���ݿ����ӳ�
private:
	void EnsureInited();
public:
	CDatabase();
	virtual ~CDatabase();

	// �๤������:
	virtual CDbConnParams* CreateDbConnParams() { return new CDbConnParams(); }
	virtual CDbOptions* CreateDbOptions() { return new CDbOptions(); }
	virtual CDbConnection* CreateDbConnection() = 0;
	virtual CDbConnectionPool* CreateDbConnectionPool() { return new CDbConnectionPool(this); }
	virtual CDbParam* CreateDbParam() { return new CDbParam(); }
	virtual CDbParamList* CreateDbParamList(CDbQuery* pDbQuery) { return new CDbParamList(pDbQuery); }
	virtual CDbDataSet* CreateDbDataSet(CDbQuery* pDbQuery) = 0;
	virtual CDbQuery* CreateDbQuery() = 0;

	CDbConnParams* GetDbConnParams();
	CDbOptions* GetDbOptions();
	CDbConnectionPool* GetDbConnectionPool();
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_DATABASE_H_
