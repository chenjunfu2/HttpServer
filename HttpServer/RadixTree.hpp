#pragma once

#include "CPP_Helper.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <variant>

template<typename ValType>
class RadixTree
{
public:
	enum class NodeType : uint64_t
	{
		NONE,			//非匹配节点（上一节点匹配成功）
		STATIC,			//折叠的静态节点（强匹配）
		PARAM,			//参数节点（:param）
		WILD_SINGLE,	//单段通配符（*）
		WILD_MULTI,		//多段通配符（**）
	};

	struct Node;

	struct SubNode
	{
		//键永远只使用路径分隔符中间的一段，如果是折叠键，则在Node内再次匹配
		std::unordered_map<std::string, Node> mapStatic = {};
		std::unique_ptr<Node> pParam = {};
		std::unique_ptr<Node> pWildSingle = {};
		std::unique_ptr<Node> pWildMulti = {};
	};

	struct Node
	{
		//节点类型枚举
		NodeType enNodeType = NodeType::NONE;
		
		/*
		如果是静态节点，那么存储可能折叠的静态路径后续部分，
			开头已经存储在进入当前节点之前的map中，
			由map匹配后调用本节点，然后匹配剩余路径
			否则为空且节点类型为NONE
		如果是参数节点，那么存储参数的名称
		如果是单段、多段节点，那么为空
		*/

		std::string strSegment;

		/*
		所有节点都需要判断vData是否为SubNode类型

		如果不是SubNode类型，则当前节点即为叶子节点
		要么匹配并返回ValType，要么失败

		如果是SubNode类型，那么假设匹配到一个NONE子节点，
		则代表上一节点为尾部，返回当前节点的ValType，
		否则按照参数类型继续匹配

		（NONE节点仅用于替代上一节点存储ValType，因为
		存在以参数结尾，但是可以追加操作方法的情况，
		匹配到参数后，继续匹配是否有追加方法，如果是空字符串
		匹配则节点为NONE并存储ValType，否则匹配方法并返回
		方法对应ValType）
		*/
		std::variant<SubNode, ValType> vData;
	};

private:
	Node stRoot{};

private:
	//借用NodeType类型实现，解释方式类似但不完全相同
	NodeType GetSegmentStrType(const char *pBeg, const char *pEnd)
	{
		if (pBeg == pEnd)
		{
			return NodeType::NONE;//出错
		}

		//遍历字符串，查看是否为参数或单段、多段匹配
		const char *pCur = pBeg;
		if (*pCur == ':')//参数
		{
			if (++pCur == pEnd)//参数名呢？
			{
				return NodeType::NONE;
			}

			return NodeType::PARAM;
		}
		else if (*pCur == '*')//单段或多段
		{
			if (++pCur == pEnd)
			{
				return NodeType::WILD_SINGLE;
			}

			if (*pCur == '*' && ++pCur == pEnd)//仅连续两个**
			{
				return NodeType::WILD_MULTI;
			}

			//还有东西
			return NodeType::NONE;
		}
		else//其它字符，静态段
		{
			return NodeType::STATIC;
		}
	}

public:
	DEFAULT_CSTC(RadixTree);
	DEFAULT_DSTC(RadixTree);
	DEFAULT_MOVE(RadixTree);
	DELETE_COPY(RadixTree);

public:
	/*
	转义字符（仅在开头出现时需要）
	\\ = \
	\ * = *
	\: = :
	*/
	bool RegisterPath(const std::string &strPath, ValType tVal)
	{
		if (strPath.empty() || strPath[0] != '/')
		{
			return false;
		}

		//查找下一个'/'
		size_t szSegNext = strPath.find_first_of('/', 1);
		if (szSegNext == strPath.npos)//当前是完整路径
		{
			szSegNext = strPath.size();
		}

		//得到类型
		auto curType = GetSegmentStrType(strPath.data(), strPath.data() + szSegNext);


		//可能的\开头则丢弃
		size_t szSegBeg = 0;
		if (strPath.size() == 1)
		{
			szSegBeg = 0;
		}
		else
		{
			szSegBeg = strPath[1] == '\\'
				? 2
				: 1;
		}
		 
		switch (curType)
		{
		case NodeType::STATIC:
			break;
		case NodeType::PARAM:
			break;
		case NodeType::WILD_SINGLE:
			break;
		case NodeType::WILD_MULTI:
			break;
		case NodeType::NONE:
		default:
			return false;
			break;
		}








	}











};


