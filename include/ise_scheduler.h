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
// ise_scheduler.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SCHEDULER_H_
#define _ISE_SCHEDULER_H_

#include "ise_options.h"
#include "ise_global_defs.h"
#include "ise_classes.h"
#include "ise_thread.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class CIseScheduleTask;
class CIseScheduleTaskMgr;

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

enum ISE_SCHEDULE_TASK_TYPE
{
	STT_EVERY_HOUR,
	STT_EVERY_DAY,
	STT_EVERY_WEEK,
	STT_EVERY_MONTH,
	STT_EVERY_YEAR,
};

typedef void (*SCH_TASK_TRIGGER_PROC)(void *pParam, UINT nTaskId, const CCustomParams& CustomParams);
typedef CCallBackDef<SCH_TASK_TRIGGER_PROC> SCH_TASK_TRIGGRE_CALLBACK;

///////////////////////////////////////////////////////////////////////////////
// ��������

const int SECONDS_PER_MINUTE = 60;
const int SECONDS_PER_HOUR   = 60*60;
const int SECONDS_PER_DAY    = 60*60*24;

///////////////////////////////////////////////////////////////////////////////
// class CIseScheduleTask

class CIseScheduleTask
{
private:
	UINT m_nTaskId;                         // ����ID
	ISE_SCHEDULE_TASK_TYPE m_nTaskType;     // ��������
	UINT m_nAfterSeconds;                   // ���������͵���ָ��ʱ�����Ӻ�����봥���¼�
	time_t m_nLastTriggerTime;              // �������ϴδ����¼���ʱ��
	SCH_TASK_TRIGGRE_CALLBACK m_OnTrigger;  // �����¼��ص�
	CCustomParams m_CustomParams;           // �Զ������
public:
	CIseScheduleTask(UINT nTaskId, ISE_SCHEDULE_TASK_TYPE nTaskType,
		UINT nAfterSeconds, const SCH_TASK_TRIGGRE_CALLBACK& OnTrigger,
		const CCustomParams& CustomParams);
	~CIseScheduleTask() {}

	void Process();

	UINT GetTaskId() const { return m_nTaskId; }
	ISE_SCHEDULE_TASK_TYPE GetTaskType() const { return m_nTaskType; }
	UINT GetAfterSeconds() const { return m_nAfterSeconds; }
};

///////////////////////////////////////////////////////////////////////////////
// class CIseScheduleTaskMgr

class CIseScheduleTaskMgr
{
private:
	typedef CObjectList<CIseScheduleTask> CIseScheduleTaskList;

	CIseScheduleTaskList m_TaskList;
	CSeqNumberAlloc m_TaskIdAlloc;
	CCriticalSection m_Lock;
private:
	CIseScheduleTaskMgr();
public:
	~CIseScheduleTaskMgr();
	static CIseScheduleTaskMgr& Instance();

	void Execute(CThread& ExecutorThread);

	UINT AddTask(ISE_SCHEDULE_TASK_TYPE nTaskType, UINT nAfterSeconds,
		const SCH_TASK_TRIGGRE_CALLBACK& OnTrigger,
		const CCustomParams& CustomParams = EMPTY_PARAMS);
	bool RemoveTask(UINT nTaskId);
	void Clear();
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SCHEDULER_H_
