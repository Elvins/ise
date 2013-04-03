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
// �ļ�����: ise_sysutils.cpp
// ��������: ϵͳ�����
///////////////////////////////////////////////////////////////////////////////

#include "ise_sysutils.h"
#include "ise_classes.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// �����

//-----------------------------------------------------------------------------
// ����: �ж�һ���ַ����ǲ���һ������
//-----------------------------------------------------------------------------
bool IsIntStr(const string& str)
{
	bool bResult;
	int nLen = (int)str.size();
	char *pStr = (char*)str.c_str();

	bResult = (nLen > 0) && !isspace(pStr[0]);

	if (bResult)
	{
		char *endp;
		strtol(pStr, &endp, 10);
		bResult = (endp - pStr == nLen);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// ����: �ж�һ���ַ����ǲ���һ������
//-----------------------------------------------------------------------------
bool IsInt64Str(const string& str)
{
	bool bResult;
	int nLen = (int)str.size();
	char *pStr = (char*)str.c_str();

	bResult = (nLen > 0) && !isspace(pStr[0]);

	if (bResult)
	{
		char *endp;
#ifdef ISE_WIN32
		_strtoi64(pStr, &endp, 10);
#endif
#ifdef ISE_LINUX
		strtoll(pStr, &endp, 10);
#endif
		bResult = (endp - pStr == nLen);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// ����: �ж�һ���ַ����ǲ���һ��������
//-----------------------------------------------------------------------------
bool IsFloatStr(const string& str)
{
	bool bResult;
	int nLen = (int)str.size();
	char *pStr = (char*)str.c_str();

	bResult = (nLen > 0) && !isspace(pStr[0]);

	if (bResult)
	{
		char *endp;
		strtod(pStr, &endp);
		bResult = (endp - pStr == nLen);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// ����: �ж�һ���ַ����ɷ�ת���ɲ�����
//-----------------------------------------------------------------------------
bool IsBoolStr(const string& str)
{
	return SameText(str, TRUE_STR) || SameText(str, FALSE_STR) || IsFloatStr(str);
}

//-----------------------------------------------------------------------------
// ����: �ַ���ת��������(��ת��ʧ�ܣ��򷵻� nDefault)
//-----------------------------------------------------------------------------
int StrToInt(const string& str, int nDefault)
{
	if (IsIntStr(str))
		return strtol(str.c_str(), NULL, 10);
	else
		return nDefault;
}

//-----------------------------------------------------------------------------
// ����: �ַ���ת����64λ����(��ת��ʧ�ܣ��򷵻� nDefault)
//-----------------------------------------------------------------------------
INT64 StrToInt64(const string& str, INT64 nDefault)
{
	if (IsInt64Str(str))
#ifdef ISE_WIN32
		return _strtoi64(str.c_str(), NULL, 10);
#endif
#ifdef ISE_LINUX
		return strtol(str.c_str(), NULL, 10);
#endif
	else
		return nDefault;
}

//-----------------------------------------------------------------------------
// ����: ����ת�����ַ���
//-----------------------------------------------------------------------------
string IntToStr(int nValue)
{
	char sTemp[64];
	sprintf(sTemp, "%d", nValue);
	return &sTemp[0];
}

//-----------------------------------------------------------------------------
// ����: 64λ����ת�����ַ���
//-----------------------------------------------------------------------------
string IntToStr(INT64 nValue)
{
	char sTemp[64];
#ifdef ISE_WIN32
	sprintf(sTemp, "%I64d", nValue);
#endif
#ifdef ISE_LINUX
	sprintf(sTemp, "%lld", nValue);
#endif
	return &sTemp[0];
}

//-----------------------------------------------------------------------------
// ����: �ַ���ת���ɸ�����(��ת��ʧ�ܣ��򷵻� fDefault)
//-----------------------------------------------------------------------------
double StrToFloat(const string& str, double fDefault)
{
	if (IsFloatStr(str))
		return strtod(str.c_str(), NULL);
	else
		return fDefault;
}

//-----------------------------------------------------------------------------
// ����: ������ת�����ַ���
//-----------------------------------------------------------------------------
string FloatToStr(double fValue, const char *sFormat)
{
	char sTemp[256];
	sprintf(sTemp, sFormat, fValue);
	return &sTemp[0];
}

//-----------------------------------------------------------------------------
// ����: �ַ���ת���ɲ�����
//-----------------------------------------------------------------------------
bool StrToBool(const string& str, bool bDefault)
{
	if (IsBoolStr(str))
	{
		if (SameText(str, TRUE_STR))
			return true;
		else if (SameText(str, FALSE_STR))
			return false;
		else
			return (StrToFloat(str, 0) != 0);
	}
	else
		return bDefault;
}

//-----------------------------------------------------------------------------
// ����: ������ת�����ַ���
//-----------------------------------------------------------------------------
string BoolToStr(bool bValue, bool bUseBoolStrs)
{
	if (bUseBoolStrs)
		return (bValue? TRUE_STR : FALSE_STR);
	else
		return (bValue? "1" : "0");
}

//-----------------------------------------------------------------------------
// ����: ��ʽ���ַ��� (��FormatString��������)
//-----------------------------------------------------------------------------
void FormatStringV(string& strResult, const char *sFormatString, va_list argList)
{
#if defined(ISE_COMPILER_VC)

	int nSize = _vscprintf(sFormatString, argList);
	char *pBuffer = (char *)malloc(nSize + 1);
	if (pBuffer)
	{
		vsprintf(pBuffer, sFormatString, argList);
		strResult = pBuffer;
		free(pBuffer);
	}
	else
		strResult = "";

#else

	int nSize = 100;
	char *pBuffer = (char *)malloc(nSize);
	va_list args;

	while (pBuffer)
	{
		int nChars;

		va_copy(args, argList);
		nChars = vsnprintf(pBuffer, nSize, sFormatString, args);
		va_end(args);

		if (nChars > -1 && nChars < nSize)
			break;
		if (nChars > -1)
			nSize = nChars + 1;
		else
			nSize *= 2;
		pBuffer = (char *)realloc(pBuffer, nSize);
	}

	if (pBuffer)
	{
		strResult = pBuffer;
		free(pBuffer);
	}
	else
		strResult = "";

#endif
}

//-----------------------------------------------------------------------------
// ����: ���ظ�ʽ������ַ���
// ����:
//   sFormatString  - ��ʽ���ַ���
//   ...            - ��ʽ������
//-----------------------------------------------------------------------------
string FormatString(const char *sFormatString, ...)
{
	string strResult;

	va_list argList;
	va_start(argList, sFormatString);
	FormatStringV(strResult, sFormatString, argList);
	va_end(argList);

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: �ж������ַ����Ƿ���ͬ (�����ִ�Сд)
//-----------------------------------------------------------------------------
bool SameText(const string& str1, const string& str2)
{
	return CompareText(str1.c_str(), str2.c_str()) == 0;
}

//-----------------------------------------------------------------------------
// ����: �Ƚ������ַ��� (�����ִ�Сд)
//-----------------------------------------------------------------------------
int CompareText(const char* str1, const char* str2)
{
#ifdef ISE_COMPILER_VC
	return _stricmp(str1, str2);
#endif
#ifdef ISE_COMPILER_BCB
	return stricmp(str1, str2);
#endif
#ifdef ISE_COMPILER_GCC
	return strcasecmp(str1, str2);
#endif
}

//-----------------------------------------------------------------------------
// ����: ȥ���ַ���ͷβ�Ŀհ��ַ� (ASCII <= 32)
//-----------------------------------------------------------------------------
string TrimString(const string& str)
{
	string strResult;
	int i, nLen;

	nLen = (int)str.size();
	i = 0;
	while (i < nLen && (BYTE)str[i] <= 32) i++;
	if (i < nLen)
	{
		while ((BYTE)str[nLen-1] <= 32) nLen--;
		strResult = str.substr(i, nLen - i);
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: �ַ������д
//-----------------------------------------------------------------------------
string UpperCase(const string& str)
{
	string strResult = str;
	int nLen = (int)strResult.size();
	char c;

	for (int i = 0; i < nLen; i++)
	{
		c = strResult[i];
		if (c >= 'a' && c <= 'z')
			strResult[i] = c - 32;
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: �ַ�����Сд
//-----------------------------------------------------------------------------
string LowerCase(const string& str)
{
	string strResult = str;
	int nLen = (int)strResult.size();
	char c;

	for (int i = 0; i < nLen; i++)
	{
		c = strResult[i];
		if (c >= 'A' && c <= 'Z')
			strResult[i] = c + 32;
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: �ַ����滻
// ����:
//   strSource        - Դ��
//   strOldPattern    - Դ���н����滻���ַ���
//   strNewPattern    - ȡ�� strOldPattern ���ַ���
//   bReplaceAll      - �Ƿ��滻Դ��������ƥ����ַ���(��Ϊfalse����ֻ�滻��һ��)
//   bCaseSensitive   - �Ƿ����ִ�Сд
// ����:
//   �����滻����֮����ַ���
//-----------------------------------------------------------------------------
string RepalceString(const string& strSource, const string& strOldPattern,
	const string& strNewPattern, bool bReplaceAll, bool bCaseSensitive)
{
	string strResult = strSource;
	string strSearch, strPattern;
	string::size_type nOffset, nIndex;
	int nOldPattLen, nNewPattLen;

	if (!bCaseSensitive)
	{
		strSearch = UpperCase(strSource);
		strPattern = UpperCase(strOldPattern);
	}
	else
	{
		strSearch = strSource;
		strPattern = strOldPattern;
	}

	nOldPattLen = (int)strOldPattern.size();
	nNewPattLen = (int)strNewPattern.size();
	nIndex = 0;

	while (nIndex < strSearch.size())
	{
		nOffset = strSearch.find(strPattern, nIndex);
		if (nOffset == string::npos) break;  // ��û�ҵ�

		strSearch.replace(nOffset, nOldPattLen, strNewPattern);
		strResult.replace(nOffset, nOldPattLen, strNewPattern);
		nIndex = (nOffset + nNewPattLen);

		if (!bReplaceAll) break;
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: �ָ��ַ���
// ����:
//   strSource   - Դ��
//   chSplitter  - �ָ���
//   StrList     - ��ŷָ�֮����ַ����б�
//   bTrimResult - �Ƿ�Էָ��Ľ������ trim ����
// ʾ��:
//   ""          -> []
//   " "         -> [" "]
//   ","         -> ["", ""]
//   "a,b,c"     -> ["a", "b", "c"]
//   ",a,,b,c,"  -> ["", "a", "", "b", "c", ""]
//-----------------------------------------------------------------------------
void SplitString(const string& strSource, char chSplitter, CStrList& StrList,
	bool bTrimResult)
{
	string::size_type nOffset, nIndex = 0;

	StrList.Clear();
	if (strSource.empty()) return;

	while (true)
	{
		nOffset = strSource.find(chSplitter, nIndex);
		if (nOffset == string::npos)   // ��û�ҵ�
		{
			StrList.Add(strSource.substr(nIndex).c_str());
			break;
		}
		else
		{
			StrList.Add(strSource.substr(nIndex, nOffset - nIndex).c_str());
			nIndex = nOffset + 1;
		}
	}

	if (bTrimResult)
	{
		for (int i = 0; i < StrList.GetCount(); i++)
			StrList.SetString(i, TrimString(StrList[i]).c_str());
	}
}

//-----------------------------------------------------------------------------
// ����: �ָ��ַ�����ת�����������б�
// ����:
//   strSource  - Դ��
//   chSplitter - �ָ���
//   IntList    - ��ŷָ�֮����������б�
//-----------------------------------------------------------------------------
void SplitStringToInt(const string& strSource, char chSplitter, IntegerArray& IntList)
{
	CStrList StrList;
	SplitString(strSource, chSplitter, StrList, true);

	IntList.clear();
	for (int i = 0; i < StrList.GetCount(); i++)
		IntList.push_back(atoi(StrList[i].c_str()));
}

//-----------------------------------------------------------------------------
// ����: ���ƴ� pSource �� pDest ��
// ��ע:
//   1. ���ֻ���� nMaxBytes ���ֽڵ� pDest �У�����������'\0'��
//   2. ��� pSource ��ʵ�ʳ���(strlen)С�� nMaxBytes�����ƻ���ǰ������
//      pDest ��ʣ�ಿ���� '\0' ��䡣
//   3. ��� pSource ��ʵ�ʳ���(strlen)���� nMaxBytes������֮��� pDest û�н�������
//-----------------------------------------------------------------------------
char *StrNCopy(char *pDest, const char *pSource, int nMaxBytes)
{
	if (nMaxBytes > 0)
	{
		if (pSource)
			return strncpy(pDest, pSource, nMaxBytes);
		else
			return strcpy(pDest, "");
	}

	return pDest;
}

//-----------------------------------------------------------------------------
// ����: ���ƴ� pSource �� pDest ��
// ��ע: ���ֻ���� nDestSize ���ֽڵ� pDest �С����� pDest ������ֽ���Ϊ'\0'��
// ����:
//   nDestSize - pDest�Ĵ�С
//-----------------------------------------------------------------------------
char *StrNZCopy(char *pDest, const char *pSource, int nDestSize)
{
	if (nDestSize > 0)
	{
		if (pSource)
		{
			char *p;
			p = strncpy(pDest, pSource, nDestSize);
			pDest[nDestSize - 1] = '\0';
			return p;
		}
		else
			return strcpy(pDest, "");
	}
	else
		return pDest;
}

//-----------------------------------------------------------------------------
// ����: ��Դ���л�ȡһ���Ӵ�
//
// For example:
//   strInput(before)   chDelimiter  bDelete       strInput(after)   Result(after)
//   ----------------   -----------  ----------    ---------------   -------------
//   "abc def"           ' '         false         "abc def"         "abc"
//   "abc def"           ' '         true          "def"             "abc"
//   " abc"              ' '         false         " abc"            ""
//   " abc"              ' '         true          "abc"             ""
//   ""                  ' '         true/false    ""                ""
//-----------------------------------------------------------------------------
string FetchStr(string& strInput, char chDelimiter, bool bDelete)
{
	string strResult;

	string::size_type nPos = strInput.find(chDelimiter, 0);
	if (nPos == string::npos)
	{
		strResult = strInput;
		if (bDelete)
			strInput.clear();
	}
	else
	{
		strResult = strInput.substr(0, nPos);
		if (bDelete)
			strInput = strInput.substr(nPos + 1);
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: �������м���붺�Ž������ݷ���
//-----------------------------------------------------------------------------
string AddThousandSep(const INT64& nNumber)
{
	string strResult = IntToStr(nNumber);
	for (int i = (int)strResult.length() - 3; i > 0; i -= 3)
		strResult.insert(i, ",");
	return strResult;
}

//-----------------------------------------------------------------------------
// Converts a string to a quoted string.
// For example:
//    abc         ->     "abc"
//    ab'c        ->     "ab'c"
//    ab"c        ->     "ab""c"
//    (empty)     ->     ""
//-----------------------------------------------------------------------------
string GetQuotedStr(const char* lpszStr, char chQuote)
{
	string strResult;
	string strSrc(lpszStr);

	strResult = chQuote;

	string::size_type nStart = 0;
	while (true)
	{
		string::size_type nPos = strSrc.find(chQuote, nStart);
		if (nPos != string::npos)
		{
			strResult += strSrc.substr(nStart, nPos - nStart) + chQuote + chQuote;
			nStart = nPos + 1;
		}
		else
		{
			strResult += strSrc.substr(nStart);
			break;
		}
	}

	strResult += chQuote;

	return strResult;
}

//-----------------------------------------------------------------------------
// Converts a quoted string to an unquoted string.
//
// ExtractQuotedStr removes the quote characters from the beginning and end of a quoted string,
// and reduces pairs of quote characters within the string to a single quote character.
// The @a chQuote parameter defines what character to use as a quote character. If the first
// character in @a lpszStr is not the value of the @a chQuote parameter, ExtractQuotedStr returns
// an empty string.
//
// The function copies characters from @a lpszStr to the result string until the second solitary
// quote character or the first null character in @a lpszStr. The @a lpszStr parameter is updated
// to point to the first character following the quoted string. If @a lpszStr does not contain a
// matching end quote character, the @a lpszStr parameter is updated to point to the terminating
// null character.
//
// For example:
//    lpszStr(before)    Returned string        lpszStr(after)
//    ---------------    ---------------        ---------------
//    "abc"               abc                    '\0'
//    "ab""c"             ab"c                   '\0'
//    "abc"123            abc                    123
//    abc"                (empty)                abc"
//    "abc                abc                    '\0'
//    (empty)             (empty)                '\0'
//-----------------------------------------------------------------------------
string ExtractQuotedStr(const char*& lpszStr, char chQuote)
{
	string strResult;
	const char* lpszStart = lpszStr;

	if (lpszStr == NULL || *lpszStr != chQuote)
		return "";

	// Calc the character count after converting.

	int nSize = 0;
	lpszStr++;
	while (*lpszStr != '\0')
	{
		if (lpszStr[0] == chQuote)
		{
			if (lpszStr[1] == chQuote)
			{
				nSize++;
				lpszStr += 2;
			}
			else
			{
				lpszStr++;
				break;
			}
		}
		else
		{
			const char* p = lpszStr;
			lpszStr++;
			nSize += (int)(lpszStr - p);
		}
	}

	// Begin to retrieve the characters.

	strResult.resize(nSize);
	char* pResult = (char*)strResult.c_str();
	lpszStr = lpszStart;
	lpszStr++;
	while (*lpszStr != '\0')
	{
		if (lpszStr[0] == chQuote)
		{
			if (lpszStr[1] == chQuote)
			{
				*pResult++ = *lpszStr;
				lpszStr += 2;
			}
			else
			{
				lpszStr++;
				break;
			}
		}
		else
		{
			const char* p = lpszStr;
			lpszStr++;
			while (p < lpszStr)
				*pResult++ = *p++;
		}
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// Converts a quoted string to an unquoted string.
//
// GetDequotedStr removes the quote characters from the beginning and end of a quoted string, and
// reduces pairs of quote characters within the string to a single quote character. The chQuote
// parameter defines what character to use as a quote character. If the @a lpszStr parameter does
// not begin and end with the quote character, GetDequotedStr returns @a lpszStr unchanged.
//
// For example:
//    "abc"     ->     abc
//    "ab""c"   ->     ab"c
//    "abc      ->     "abc
//    abc"      ->     abc"
//    (empty)   ->    (empty)
//-----------------------------------------------------------------------------
string GetDequotedStr(const char* lpszStr, char chQuote)
{
	const char* lpszStart = lpszStr;
	int nStrLen = (int)strlen(lpszStr);

	string strResult = ExtractQuotedStr(lpszStr, chQuote);

	if ( (strResult.empty() || *lpszStr == '\0') &&
		nStrLen > 0 && (lpszStart[0] != chQuote || lpszStart[nStrLen-1] != chQuote) )
		strResult = lpszStart;

	return strResult;
}

//-----------------------------------------------------------------------------

#ifdef ISE_WIN32
static bool GetFileFindData(const string& strFileName, WIN32_FIND_DATAA& FindData)
{
	HANDLE hFindHandle = ::FindFirstFileA(strFileName.c_str(), &FindData);
	bool bResult = (hFindHandle != INVALID_HANDLE_VALUE);
	if (bResult) ::FindClose(hFindHandle);
	return bResult;
}
#endif

//-----------------------------------------------------------------------------
// ����: ����ļ��Ƿ����
//-----------------------------------------------------------------------------
bool FileExists(const string& strFileName)
{
#ifdef ISE_WIN32
	DWORD nFileAttr = ::GetFileAttributesA(strFileName.c_str());
	if (nFileAttr != (DWORD)(-1))
		return ((nFileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0);
	else
	{
		WIN32_FIND_DATAA FindData;
		DWORD nLastError = ::GetLastError();
		return
			(nLastError != ERROR_FILE_NOT_FOUND) &&
			(nLastError != ERROR_PATH_NOT_FOUND) &&
			(nLastError != ERROR_INVALID_NAME) &&
			GetFileFindData(strFileName, FindData) &&
			(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}
#endif
#ifdef ISE_LINUX
	return (euidaccess(strFileName.c_str(), F_OK) == 0);
#endif
}

//-----------------------------------------------------------------------------
// ����: ���Ŀ¼�Ƿ����
//-----------------------------------------------------------------------------
bool DirectoryExists(const string& strDir)
{
#ifdef ISE_WIN32
	int nCode;
	nCode = GetFileAttributesA(strDir.c_str());
	return (nCode != -1) && ((FILE_ATTRIBUTE_DIRECTORY & nCode) != 0);
#endif
#ifdef ISE_LINUX
	string strPath = PathWithSlash(strDir);
	struct stat st;
	bool bResult;

	if (stat(strPath.c_str(), &st) == 0)
		bResult = ((st.st_mode & S_IFDIR) == S_IFDIR);
	else
		bResult = false;

	return bResult;
#endif
}

//-----------------------------------------------------------------------------
// ����: ����Ŀ¼
// ʾ��:
//   CreateDir("C:\\test");
//   CreateDir("/home/test");
//-----------------------------------------------------------------------------
bool CreateDir(const string& strDir)
{
#ifdef ISE_WIN32
	return CreateDirectoryA(strDir.c_str(), NULL) != 0;
#endif
#ifdef ISE_LINUX
	return mkdir(strDir.c_str(), (mode_t)(-1)) == 0;
#endif
}

//-----------------------------------------------------------------------------
// ����: ɾ��Ŀ¼
// ����:
//   strDir     - ��ɾ����Ŀ¼
//   bRecursive - �Ƿ�ݹ�ɾ��
// ����:
//   true   - �ɹ�
//   false  - ʧ��
//-----------------------------------------------------------------------------
bool DeleteDir(const string& strDir, bool bRecursive)
{
	if (!bRecursive)
	{
#ifdef ISE_WIN32
		return RemoveDirectoryA(strDir.c_str()) != 0;
#endif
#ifdef ISE_LINUX
		return rmdir(strDir.c_str()) == 0;
#endif
	}

#ifdef ISE_WIN32
	const char* const ALL_FILE_WILDCHAR = "*.*";
#endif
#ifdef ISE_LINUX
	const char* const ALL_FILE_WILDCHAR = "*";
#endif

	bool bResult = true;
	string strPath = PathWithSlash(strDir);
	FILE_FIND_RESULT fr;
	FindFiles(strPath + ALL_FILE_WILDCHAR, FA_ANY_FILE, fr);

	for (int i = 0; i < (int)fr.size() && bResult; i++)
	{
		string strFullName = strPath + fr[i].strFileName;
		if (fr[i].nAttr & FA_DIRECTORY)
			bResult = DeleteDir(strFullName, true);
		else
			RemoveFile(strFullName);
	}

	bResult = DeleteDir(strPath, false);

	return bResult;
}

//-----------------------------------------------------------------------------
// ����: ȡ���ļ��������һ���ָ�����λ��(0-based)����û�У��򷵻�-1
//-----------------------------------------------------------------------------
int GetLastDelimPos(const string& strFileName, const string& strDelims)
{
	int nResult = (int)strFileName.size() - 1;

	for (; nResult >= 0; nResult--)
		if (strDelims.find(strFileName[nResult], 0) != string::npos)
			break;

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ���ļ����ַ�����ȡ���ļ�·��
// ����:
//   strFileName - ����·�����ļ���
// ����:
//   �ļ���·��
// ʾ��:
//   ExtractFilePath("C:\\MyDir\\test.c");         ����: "C:\\MyDir\\"
//   ExtractFilePath("C:");                        ����: "C:\\"
//   ExtractFilePath("/home/user1/data/test.c");   ����: "/home/user1/data/"
//-----------------------------------------------------------------------------
string ExtractFilePath(const string& strFileName)
{
	string strDelims;
	strDelims += PATH_DELIM;
#ifdef ISE_WIN32
	strDelims += DRIVER_DELIM;
#endif

	int nPos = GetLastDelimPos(strFileName, strDelims);
	return PathWithSlash(strFileName.substr(0, nPos + 1));
}

//-----------------------------------------------------------------------------
// ����: ���ļ����ַ�����ȡ���������ļ���
// ����:
//   strFileName - ����·�����ļ���
// ����:
//   �ļ���
// ʾ��:
//   ExtractFileName("C:\\MyDir\\test.c");         ����: "test.c"
//   ExtractFilePath("/home/user1/data/test.c");   ����: "test.c"
//-----------------------------------------------------------------------------
string ExtractFileName(const string& strFileName)
{
	string strDelims;
	strDelims += PATH_DELIM;
#ifdef ISE_WIN32
	strDelims += DRIVER_DELIM;
#endif

	int nPos = GetLastDelimPos(strFileName, strDelims);
	return strFileName.substr(nPos + 1, strFileName.size() - nPos - 1);
}

//-----------------------------------------------------------------------------
// ����: ���ļ����ַ�����ȡ���ļ���չ��
// ����:
//   strFileName - �ļ��� (�ɰ���·��)
// ����:
//   �ļ���չ��
// ʾ��:
//   ExtractFileExt("C:\\MyDir\\test.txt");         ����:  ".txt"
//   ExtractFileExt("/home/user1/data/test.c");     ����:  ".c"
//-----------------------------------------------------------------------------
string ExtractFileExt(const string& strFileName)
{
	string strDelims;
	strDelims += PATH_DELIM;
#ifdef ISE_WIN32
	strDelims += DRIVER_DELIM;
#endif
	strDelims += FILE_EXT_DELIM;

	int nPos = GetLastDelimPos(strFileName, strDelims);
	if (nPos >= 0 && strFileName[nPos] == FILE_EXT_DELIM)
		return strFileName.substr(nPos, strFileName.length());
	else
		return "";
}

//-----------------------------------------------------------------------------
// ����: �ı��ļ����ַ����е��ļ���չ��
// ����:
//   strFileName - ԭ�ļ��� (�ɰ���·��)
//   strExt      - �µ��ļ���չ��
// ����:
//   �µ��ļ���
// ʾ��:
//   ChangeFileExt("c:\\test.txt", ".new");        ����:  "c:\\test.new"
//   ChangeFileExt("test.txt", ".new");            ����:  "test.new"
//   ChangeFileExt("test", ".new");                ����:  "test.new"
//   ChangeFileExt("test.txt", "");                ����:  "test"
//   ChangeFileExt("test.txt", ".");               ����:  "test."
//   ChangeFileExt("/home/user1/test.c", ".new");  ����:  "/home/user1/test.new"
//-----------------------------------------------------------------------------
string ChangeFileExt(const string& strFileName, const string& strExt)
{
	string strResult(strFileName);
	string strNewExt(strExt);

	if (!strResult.empty())
	{
		if (!strNewExt.empty() && strNewExt[0] != FILE_EXT_DELIM)
			strNewExt = FILE_EXT_DELIM + strNewExt;

		string strOldExt = ExtractFileExt(strResult);
		if (!strOldExt.empty())
			strResult.erase(strResult.length() - strOldExt.length());
		strResult += strNewExt;
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// ����: ǿ�ƴ���Ŀ¼
// ����:
//   strDir - ��������Ŀ¼ (�����Ƕ༶Ŀ¼)
// ����:
//   true   - �ɹ�
//   false  - ʧ��
// ʾ��:
//   ForceDirectories("C:\\MyDir\\Test");
//   ForceDirectories("/home/user1/data");
//-----------------------------------------------------------------------------
bool ForceDirectories(string strDir)
{
	int nLen = (int)strDir.length();

	if (strDir.empty()) return false;
	if (strDir[nLen-1] == PATH_DELIM)
		strDir.resize(nLen - 1);

#ifdef ISE_WIN32
	if (strDir.length() < 3 || DirectoryExists(strDir) ||
		ExtractFilePath(strDir) == strDir) return true;    // avoid 'xyz:\' problem.
#endif
#ifdef ISE_LINUX
	if (strDir.empty() || DirectoryExists(strDir)) return true;
#endif
	return ForceDirectories(ExtractFilePath(strDir)) && CreateDir(strDir);
}

//-----------------------------------------------------------------------------
// ����: ɾ���ļ�
//-----------------------------------------------------------------------------
bool DeleteFile(const string& strFileName)
{
#ifdef ISE_WIN32
	DWORD nFileAttr = ::GetFileAttributesA(strFileName.c_str());
	if (nFileAttr != (DWORD)(-1) && (nFileAttr & FILE_ATTRIBUTE_READONLY) != 0)
	{
		nFileAttr &= ~FILE_ATTRIBUTE_READONLY;
		::SetFileAttributesA(strFileName.c_str(), nFileAttr);
	}

	return ::DeleteFileA(strFileName.c_str()) != 0;
#endif
#ifdef ISE_LINUX
	return (unlink(strFileName.c_str()) == 0);
#endif
}

//-----------------------------------------------------------------------------
// ����: ͬ DeleteFile()
//-----------------------------------------------------------------------------
bool RemoveFile(const string& strFileName)
{
	return DeleteFile(strFileName);
}

//-----------------------------------------------------------------------------
// ����: �ļ�������
//-----------------------------------------------------------------------------
bool RenameFile(const string& strOldFileName, const string& strNewFileName)
{
#ifdef ISE_WIN32
	return ::MoveFileA(strOldFileName.c_str(), strNewFileName.c_str()) != 0;
#endif
#ifdef ISE_LINUX
	return (rename(strOldFileName.c_str(), strNewFileName.c_str()) == 0);
#endif
}

//-----------------------------------------------------------------------------
// ����: ȡ���ļ��Ĵ�С����ʧ���򷵻�-1
//-----------------------------------------------------------------------------
INT64 GetFileSize(const string& strFileName)
{
	INT64 nResult;

	try
	{
		CFileStream FileStream(strFileName, FM_OPEN_READ | FM_SHARE_DENY_NONE);
		nResult = FileStream.GetSize();
	}
	catch (CException&)
	{
		nResult = -1;
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// ����: ��ָ��·���²��ҷ����������ļ�
// ����:
//   strPath    - ָ�����ĸ�·���½��в��ң�������ָ��ͨ���
//   nAttr      - ֻ���ҷ��ϴ����Ե��ļ�
//   FindResult - ���ز��ҽ��
// ʾ��:
//   FindFiles("C:\\test\\*.*", FA_ANY_FILE & ~FA_HIDDEN, fr);
//   FindFiles("/home/*.log", FA_ANY_FILE & ~FA_SYM_LINK, fr);
//-----------------------------------------------------------------------------
void FindFiles(const string& strPath, UINT nAttr, FILE_FIND_RESULT& FindResult)
{
	const UINT FA_SPECIAL = FA_HIDDEN | FA_SYS_FILE | FA_VOLUME_ID | FA_DIRECTORY;
	UINT nExcludeAttr = ~nAttr & FA_SPECIAL;
	FindResult.clear();

#ifdef ISE_WIN32
	HANDLE nFindHandle;
	WIN32_FIND_DATAA FindData;

	nFindHandle = FindFirstFileA(strPath.c_str(), &FindData);
	if (nFindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			DWORD nAttr = FindData.dwFileAttributes;
			string strName = FindData.cFileName;
			bool bIsSpecDir = (nAttr & FA_DIRECTORY) && (strName == "." || strName == "..");

			if ((nAttr & nExcludeAttr) == 0 && !bIsSpecDir)
			{
				FILE_FIND_ITEM Item;
				Item.nFileSize = FindData.nFileSizeHigh;
				Item.nFileSize = (Item.nFileSize << 32) | FindData.nFileSizeLow;
				Item.strFileName = strName;
				Item.nAttr = nAttr;

				FindResult.push_back(Item);
			}
		}
		while (FindNextFileA(nFindHandle, &FindData));

		FindClose(nFindHandle);
	}
#endif

#ifdef ISE_LINUX
	string strPathOnly = ExtractFilePath(strPath);
	string strPattern = ExtractFileName(strPath);
	string strFullName, strName;
	DIR *pDir;
	struct dirent DirEnt, *pDirEnt = NULL;
	struct stat StatBuf, LinkStatBuf;
	UINT nFileAttr, nFileMode;

	if (strPathOnly.empty()) strPathOnly = "/";

	pDir = opendir(strPathOnly.c_str());
	if (pDir)
	{
		while ((readdir_r(pDir, &DirEnt, &pDirEnt) == 0) && pDirEnt)
		{
			if (!fnmatch(strPattern.c_str(), pDirEnt->d_name, 0) == 0) continue;

			strName = pDirEnt->d_name;
			strFullName = strPathOnly + strName;

			if (lstat(strFullName.c_str(), &StatBuf) == 0)
			{
				nFileAttr = 0;
				nFileMode = StatBuf.st_mode;

				if (S_ISDIR(nFileMode))
					nFileAttr |= FA_DIRECTORY;
				else if (!S_ISREG(nFileMode))
				{
					if (S_ISLNK(nFileMode))
					{
						nFileAttr |= FA_SYM_LINK;
						if ((stat(strFullName.c_str(), &LinkStatBuf) == 0) &&
							(S_ISDIR(LinkStatBuf.st_mode)))
							nFileAttr |= FA_DIRECTORY;
					}
					nFileAttr |= FA_SYS_FILE;
				}

				if (pDirEnt->d_name[0] == '.' && pDirEnt->d_name[1])
					if (!(pDirEnt->d_name[1] == '.' && !pDirEnt->d_name[2]))
						nFileAttr |= FA_HIDDEN;

				if (euidaccess(strFullName.c_str(), W_OK) != 0)
					nFileAttr |= FA_READ_ONLY;

				bool bIsSpecDir = (nFileAttr & FA_DIRECTORY) && (strName == "." || strName == "..");

				if ((nFileAttr & nExcludeAttr) == 0 && !bIsSpecDir)
				{
					FILE_FIND_ITEM Item;
					Item.nFileSize = StatBuf.st_size;
					Item.strFileName = strName;
					Item.nAttr = nFileAttr;

					FindResult.push_back(Item);
				}
			}
		} // while

		closedir(pDir);
	}
#endif
}

//-----------------------------------------------------------------------------
// ����: ��ȫ·���ַ�������� "\" �� "/"
//-----------------------------------------------------------------------------
string PathWithSlash(const string& strPath)
{
	string strResult = TrimString(strPath);
	int nLen = (int)strResult.size();
	if (nLen > 0 && strResult[nLen-1] != PATH_DELIM)
		strResult += PATH_DELIM;
	return strResult;
}

//-----------------------------------------------------------------------------
// ����: ȥ��·���ַ�������� "\" �� "/"
//-----------------------------------------------------------------------------
string PathWithoutSlash(const string& strPath)
{
	string strResult = TrimString(strPath);
	int nLen = (int)strResult.size();
	if (nLen > 0 && strResult[nLen-1] == PATH_DELIM)
		strResult.resize(nLen - 1);
	return strResult;
}

//-----------------------------------------------------------------------------
// ����: ȡ�ÿ�ִ���ļ���ȫ��(������·��)
//-----------------------------------------------------------------------------
string GetAppExeName()
{
#ifdef ISE_WIN32
	char szBuffer[MAX_PATH];
	::GetModuleFileNameA(NULL, szBuffer, MAX_PATH);
	return string(szBuffer);
#endif
#ifdef ISE_LINUX
	const int BUFFER_SIZE = 1024;

	int r;
	char sBuffer[BUFFER_SIZE];
	string strResult;

	r = readlink("/proc/self/exe", sBuffer, BUFFER_SIZE);
	if (r != -1)
	{
		if (r >= BUFFER_SIZE) r = BUFFER_SIZE - 1;
		sBuffer[r] = 0;
		strResult = sBuffer;
	}
	else
	{
		IseThrowException(SEM_NO_PERM_READ_PROCSELFEXE);
	}

	return strResult;
#endif
}

//-----------------------------------------------------------------------------
// ����: ȡ�ÿ�ִ���ļ����ڵ�·��
//-----------------------------------------------------------------------------
string GetAppPath()
{
	return ExtractFilePath(GetAppExeName());
}

//-----------------------------------------------------------------------------
// ����: ȡ�ÿ�ִ���ļ����ڵ�·������Ŀ¼
//-----------------------------------------------------------------------------
string GetAppSubPath(const string& strSubDir)
{
	return PathWithSlash(GetAppPath() + strSubDir);
}

//-----------------------------------------------------------------------------
// ����: ���ز���ϵͳ�������
//-----------------------------------------------------------------------------
int GetLastSysError()
{
#ifdef ISE_WIN32
	return ::GetLastError();
#endif
#ifdef ISE_LINUX
	return errno;
#endif
}

//-----------------------------------------------------------------------------
// ����: ���ز���ϵͳ��������Ӧ�Ĵ�����Ϣ
//-----------------------------------------------------------------------------
string SysErrorMessage(int nErrorCode)
{
#ifdef ISE_WIN32
	char *pErrorMsg;

	pErrorMsg = strerror(nErrorCode);
	return pErrorMsg;
#endif
#ifdef ISE_LINUX
	const int ERROR_MSG_SIZE = 256;
	char sErrorMsg[ERROR_MSG_SIZE];
	string strResult;

	sErrorMsg[0] = 0;
	strerror_r(nErrorCode, sErrorMsg, ERROR_MSG_SIZE);
	if (sErrorMsg[0] == 0)
		strResult = FormatString("System error: %d", nErrorCode);
	else
		strResult = sErrorMsg;

	return strResult;
#endif
}

//-----------------------------------------------------------------------------
// ����: ˯�� fSeconds �룬�ɾ�ȷ�����롣
// ����:
//   fSeconds       - ˯�ߵ���������ΪС�����ɾ�ȷ������ (ʵ�ʾ�ȷ��ȡ���ڲ���ϵͳ)
//   AllowInterrupt - �Ƿ������ź��ж�
//-----------------------------------------------------------------------------
void SleepSec(double fSeconds, bool AllowInterrupt)
{
#ifdef ISE_WIN32
	Sleep((UINT)(fSeconds * 1000));
#endif
#ifdef ISE_LINUX
	const UINT NANO_PER_SEC = 1000000000;  // һ����ڶ�������
	struct timespec req, remain;
	int r;

	req.tv_sec = (UINT)fSeconds;
	req.tv_nsec = (UINT)((fSeconds - req.tv_sec) * NANO_PER_SEC);

	while (true)
	{
		r = nanosleep(&req, &remain);
		if (r == -1 && errno == EINTR && !AllowInterrupt)
			req = remain;
		else
			break;
	}
#endif
}

//-----------------------------------------------------------------------------
// ����: ȡ�õ�ǰ Ticks����λ:����
//-----------------------------------------------------------------------------
UINT GetCurTicks()
{
#ifdef ISE_WIN32
	return GetTickCount();
#endif
#ifdef ISE_LINUX
	timeval tv;
	gettimeofday(&tv, NULL);
	return INT64(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
#endif
}

//-----------------------------------------------------------------------------
// ����: ȡ������ Ticks ֮��
//-----------------------------------------------------------------------------
UINT GetTickDiff(UINT nOldTicks, UINT nNewTicks)
{
	if (nNewTicks >= nOldTicks)
		return (nNewTicks - nOldTicks);
	else
		return (UINT(-1) - nOldTicks + nNewTicks);
}

//-----------------------------------------------------------------------------
// ����: ����� "���������"
//-----------------------------------------------------------------------------
void Randomize()
{
	srand((unsigned int)time(NULL));
}

//-----------------------------------------------------------------------------
// ����: ���� [nMin..nMax] ֮���һ��������������߽�
//-----------------------------------------------------------------------------
int GetRandom(int nMin, int nMax)
{
	ISE_ASSERT((nMax - nMin) < MAXLONG);
	return nMin + (int)(((double)rand() / ((double)RAND_MAX + 0.1)) * (nMax - nMin + 1));
}

//-----------------------------------------------------------------------------
// ����: ����һ��������� [nStartNumber, nEndNumber] �ڵĲ��ظ��������
// ע��: ���� pRandomList ���������� >= (nEndNumber - nStartNumber + 1)
//-----------------------------------------------------------------------------
void GenerateRandomList(int nStartNumber, int nEndNumber, int *pRandomList)
{
	if (nStartNumber > nEndNumber || !pRandomList) return;

	int nCount = nEndNumber - nStartNumber + 1;

	if (rand() % 2 == 0)
		for (int i = 0; i < nCount; i++)
			pRandomList[i] = nStartNumber + i;
	else
		for (int i = 0; i < nCount; i++)
			pRandomList[nCount - i - 1] = nStartNumber + i;

	for (int i = nCount - 1; i >= 1; i--)
		Swap(pRandomList[i], pRandomList[rand()%i]);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
