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
// ise_xml_doc.h
// Classes:
//   * XmlNode
//   * XmlNodeProps
//   * XmlDocument
//   * XmlReader
//   * XmlWriter
//   * XmlDocParser
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_XML_DOC_H_
#define _ISE_XML_DOC_H_

#include "ise/main/ise_options.h"
#include "ise/main/ise_global_defs.h"
#include "ise/main/ise_err_msgs.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_sys_utils.h"

using std::string;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class XmlNode;
class XmlNodeProps;

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

// XML��ǩ���
enum XML_TAG_TYPE
{
    XTT_START_TAG   = 0x01,      // ��ʼ��ǩ
    XTT_END_TAG     = 0x02,      // ������ǩ
};

typedef UINT XML_TAG_TYPES;

///////////////////////////////////////////////////////////////////////////////
// ��������

const int DEF_XML_INDENT_SPACES = 4;
const char* const S_DEF_XML_DOC_VER = "1.0";

///////////////////////////////////////////////////////////////////////////////
// �����

std::string strToXml(const std::string& str);
std::string xmlToStr(const std::string& str);

///////////////////////////////////////////////////////////////////////////////
// class XmlNode

class XmlNode
{
public:
    XmlNode();
    XmlNode(const XmlNode& src);
    virtual ~XmlNode();

    XmlNode& operator = (const XmlNode& rhs);

    void addNode(XmlNode *node);
    XmlNode* addNode();
    XmlNode* addNode(const std::string& name, const std::string& dataString);
    void insertNode(int index, XmlNode *node);

    XmlNode* findChildNode(const std::string& name);
    int indexOf(const std::string& name);
    void clear();

    XmlNode* getRootNode() const;
    XmlNode* getParentNode() const { return parentNode_; }
    int getChildCount() const;
    XmlNode* getChildNodes(int index) const;
    XmlNodeProps* getProps() const { return props_; }
    std::string getName() const { return name_; }
    std::string getDataString() const { return dataString_; }

    void setParentNode(XmlNode *node);
    void setName(const std::string& value) { name_ = value; }
    void setDataString(const std::string& value);

private:
    void initObject();
    void assignNode(XmlNode& src, XmlNode& dest);

private:
    XmlNode *parentNode_;       // ���ڵ�
    PointerList *childNodes_;   // �ӽڵ��б� (XmlNode*)[]
    XmlNodeProps *props_;       // ����ֵ�б�
    std::string name_;               // �ڵ�����
    std::string dataString_;         // �ڵ�����(xmlToStr֮�������)
};

///////////////////////////////////////////////////////////////////////////////
// class XmlNodeProps

struct XmlNodePropItem
{
    std::string name;
    std::string value;
};

class XmlNodeProps
{
public:
    XmlNodeProps();
    XmlNodeProps(const XmlNodeProps& src);
    virtual ~XmlNodeProps();

    XmlNodeProps& operator = (const XmlNodeProps& rhs);
    std::string& operator[](const std::string& name) { return valueOf(name); }

    bool add(const std::string& name, const std::string& value);
    void remove(const std::string& name);
    void clear();
    int indexOf(const std::string& name);
    bool propExists(const std::string& name);
    std::string& valueOf(const std::string& name);

    int getCount() const { return items_.getCount(); }
    XmlNodePropItem getItems(int index) const;
    std::string getPropString() const;
    void setPropString(const std::string& propString);

private:
    XmlNodePropItem* getItemPtr(int index) { return (XmlNodePropItem*)items_[index]; }
    void parsePropString(const std::string& propStr);

private:
    PointerList items_;      // (XmlNodePropItem*)[]
};

///////////////////////////////////////////////////////////////////////////////
// class XmlDocument

class XmlDocument
{
public:
    XmlDocument();
    XmlDocument(const XmlDocument& src);
    virtual ~XmlDocument();

    XmlDocument& operator = (const XmlDocument& rhs);

    bool saveToStream(Stream& stream);
    bool loadFromStream(Stream& stream);
    bool saveToString(std::string& str);
    bool loadFromString(const std::string& str);
    bool saveToFile(const std::string& fileName);
    bool loadFromFile(const std::string& fileName);
    void clear();

    bool getAutoIndent() const { return autoIndent_; }
    int getIndentSpaces() const { return indentSpaces_; }
    std::string getEncoding() const { return encoding_; }
    XmlNode* getRootNode() { return &rootNode_; }

    void setAutoIndent(bool value) { autoIndent_ = value; }
    void setIndentSpaces(int value) { indentSpaces_ = value; }
    void setEncoding(const std::string& value) { encoding_ = value; }

private:
    bool autoIndent_;     // �Ƿ��Զ�����
    int indentSpaces_;    // �����ո���
    XmlNode rootNode_;    // ���ڵ�
    std::string encoding_;     // XML����
};

///////////////////////////////////////////////////////////////////////////////
// class XmlReader

class XmlReader : boost::noncopyable
{
public:
    XmlReader(XmlDocument *owner, Stream *stream);
    virtual ~XmlReader();

    void readHeader(std::string& version, std::string& encoding);
    void readRootNode(XmlNode *node);

private:
    XML_TAG_TYPES readXmlData(std::string& name, std::string& prop, std::string& data);
    XML_TAG_TYPES readNode(XmlNode *node);

private:
    XmlDocument *owner_;
    Stream *stream_;
    Buffer buffer_;
    int position_;
};

///////////////////////////////////////////////////////////////////////////////
// class XmlWriter

class XmlWriter : boost::noncopyable
{
public:
    XmlWriter(XmlDocument *owner, Stream *stream);
    virtual ~XmlWriter();

    void writeHeader(const std::string& version = S_DEF_XML_DOC_VER, const std::string& encoding = "");
    void writeRootNode(XmlNode *node);

private:
    void flushBuffer();
    void writeLn(const std::string& str);
    void writeNode(XmlNode *node, int indent);

private:
    XmlDocument *owner_;
    Stream *stream_;
    std::string buffer_;
};

///////////////////////////////////////////////////////////////////////////////
// class XmlDocParser - XML�ĵ������
//
// ���ʽ���:
//   NamePath: XMLֵ������·��������һϵ��XML�ڵ�����ɣ�������֮���õ��(.)�ָ������
//   һ�����ƿ����ǽڵ����������XML�ĸ��ڵ�����������·���б�ʡ�ԡ�����·�������ִ�Сд��
//   ��ע����ǣ���������֮���õ�ŷָ������Ը������ڲ���Ӧ���е�ţ����������
//   ʾ��: "Database.MainDb.HostName"

class XmlDocParser : boost::noncopyable
{
public:
    XmlDocParser();
    virtual ~XmlDocParser();

    bool loadFromFile(const std::string& fileName);

    std::string getString(const std::string& namePath);
    int getInteger(const std::string& namePath, int defaultVal = 0);
    double getFloat(const std::string& namePath, double defaultVal = 0);
    bool getBoolean(const std::string& namePath, bool defaultVal = 0);

    XmlDocument& getXmlDoc() { return xmlDoc_; }

private:
    void splitNamePath(const std::string& namePath, StrList& result);
private:
    XmlDocument xmlDoc_;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_XML_DOC_H_
