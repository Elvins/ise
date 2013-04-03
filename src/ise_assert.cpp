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
// �ļ�����: ise_assert.cpp
// ��������: ����֧��
///////////////////////////////////////////////////////////////////////////////

#include "ise_assert.h"
#include "ise_exceptions.h"
#include "ise_classes.h"
#include "ise_sysutils.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// ����: �ڲ�ʹ�õĶ��Ժ����������־�����׳��쳣��
//-----------------------------------------------------------------------------
void InternalAssert(const char *pCondition, const char *pFileName, int nLineNumber)
{
	CSimpleException e(FormatString("Assertion failed: %s", pCondition).c_str(), pFileName, nLineNumber);
	Logger().WriteException(e);
	throw e;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
