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

class IseScheduleTask;
class IseScheduleTaskMgr;

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

typedef boost::function<void (UINT taskId, const Context& context)> SchTaskTriggerCallback;

///////////////////////////////////////////////////////////////////////////////
// ��������

const int SECONDS_PER_MINUTE = 60;
const int SECONDS_PER_HOUR   = 60*60;
const int SECONDS_PER_DAY    = 60*60*24;

///////////////////////////////////////////////////////////////////////////////
// class IseScheduleTask

class IseScheduleTask : boost::noncopyable
{
public:
    IseScheduleTask(UINT taskId, ISE_SCHEDULE_TASK_TYPE taskType,
        UINT afterSeconds, const SchTaskTriggerCallback& onTrigger,
        const Context& context);
    ~IseScheduleTask() {}

    void process();

    UINT getTaskId() const { return taskId_; }
    ISE_SCHEDULE_TASK_TYPE getTaskType() const { return taskType_; }
    UINT getAfterSeconds() const { return afterSeconds_; }

private:
    UINT taskId_;                         // ����ID
    ISE_SCHEDULE_TASK_TYPE taskType_;     // ��������
    UINT afterSeconds_;                   // ���������͵���ָ��ʱ�����Ӻ�����봥���¼�
    time_t lastTriggerTime_;              // �������ϴδ����¼���ʱ��
    Context context_;                     // �Զ���������
    SchTaskTriggerCallback onTrigger_;    // �����¼��ص�
};

///////////////////////////////////////////////////////////////////////////////
// class IseScheduleTaskMgr

class IseScheduleTaskMgr : boost::noncopyable
{
private:
    IseScheduleTaskMgr();
public:
    ~IseScheduleTaskMgr();
    static IseScheduleTaskMgr& instance();

    void execute(Thread& ExecutorThread);

    UINT addTask(ISE_SCHEDULE_TASK_TYPE taskType, UINT afterSeconds,
        const SchTaskTriggerCallback& onTrigger,
        const Context& context = EMPTY_CONTEXT);
    bool removeTask(UINT taskId);
    void clear();

private:
    typedef ObjectList<IseScheduleTask> IseScheduleTaskList;

    IseScheduleTaskList taskList_;
    SeqNumberAlloc taskIdAlloc_;
    CriticalSection lock_;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SCHEDULER_H_
