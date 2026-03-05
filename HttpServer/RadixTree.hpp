#pragma once

#include "CPP_Helper.h"

#include <string>
#include <unordered_map>
#include <memory>

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

	using PathSegList = std::vector<std::string>;

	struct Node
	{
		//节点类型枚举
		NodeType enNodeType = NodeType::NONE;

		//有值则代表这是已注册的完整路径，注意完整路径也有可能是另一个完整路径里的片段
		std::unique_ptr<ValType> pValData = {};
		
		/*
		如果是静态节点，那么存储可能折叠的静态路径后续部分，
			开头已经存储在进入当前节点之前的map中，
			由map匹配后调用本节点，然后匹配剩余路径
			否则为空且节点类型为NONE
		如果是参数节点，那么存储参数的名称
		如果是单段、多段节点，那么为空
		*/
		PathSegList listPathSeg{};

		//键永远只使用路径分隔符中间的一段，如果是折叠键，则在Node内再次匹配
		std::unordered_map<std::string, Node> mapStatic = {};
		std::unique_ptr<Node> pParam = {};
		std::unique_ptr<Node> pWildSingle = {};
		std::unique_ptr<Node> pWildMulti = {};
	};

private:
	Node stRoot{};

private:
	static bool EscapeRequestSeg(std::string& strEscape)
	{
		/*
		转义:
			%XX%
		*/




	}

	static bool EscapeRegisterSeg(std::string& strEscape)
	{
		/*
		转义:
			\\ = \
			\* = *
			\: = :
			\/ = /
		*/




	}



	static std::string TokenizeRequestPathNested(const std::string &strFullPath, size_t &szCurSegPos)
	{
		size_t szNextSegPos = strFullPath.find('/', szCurSegPos);
		if (szNextSegPos == strFullPath.npos)
		{
			size_t szOff = szCurSegPos, szCount = strFullPath.size() - szCurSegPos;
			szCurSegPos = strFullPath.size();
			return strFullPath.substr(szOff, szCount);//返回子串
		}
		
		//裁切路径部分
		size_t szOff = szCurSegPos, szCount = szNextSegPos - szCurSegPos;//szNextSegPos是/的位置，用它来当作size计算的上限不会包含尾部的/
		szCurSegPos = szNextSegPos + 1;//跳过下一个/
		return strFullPath.substr(szOff, szCount);//返回子串
	}

	static PathSegList TokenizeRequestPath(const std::string &strFullPath)
	{
		if (strFullPath.empty())
		{
			return {};
		}

		PathSegList ret{};
		size_t szCurSegPos = 0;
		do
		{
			ret.push_back(TokenizeRequestPathNested(strFullPath, szCurSegPos));
		} while (szCurSegPos < strFullPath.size());

		return ret;
	}

	static std::string TokenizeRegisterPathNested(const std::string &strFullPath, size_t &szCurSegPos)
	{
		//这里需要处理转义，因为注册中的\/不能当作/

		size_t szNewCurPos = szCurSegPos;
		size_t szNextSegPos = 0;
		do
		{
			szNextSegPos = strFullPath.find('/', szNewCurPos);
			if (szNextSegPos == strFullPath.npos)
			{
				size_t szOff = szCurSegPos, szCount = strFullPath.size() - szCurSegPos;
				szCurSegPos = strFullPath.size();
				return strFullPath.substr(szOff, szCount);//返回子串
			}

			//如果前一个是转义字符那就继续查找下一个
			auto *pPath = strFullPath.data();
			if (szNextSegPos != 0 && pPath[szNextSegPos - 1] == '\\')
			{
				//处理连续的转义，找到连续的/开头
				size_t i = szNextSegPos - 1;
				while (i != 0 && pPath[--i] == '\\')
				{
					continue;
				}

				//让i指向'\'
				if (pPath[i] != '\\')
				{
					++i;
				}

				//按2倍配对确认到底是\\/还是\/
				size_t szCount = szNextSegPos - 1 - i;
				if (szCount != 0 && (szCount + 1) % 2 != 0)//奇数个\，那么最后一个\与/配对
				{
					szNewCurPos = szNextSegPos + 1;//跳过此/
					continue;
				}
			}

			break;//否则直接离开
		} while (true);

		size_t szOff = szCurSegPos, szCount = szNextSegPos - szCurSegPos;//szNextSegPos是/的位置，用它来当作size计算的上限不会包含尾部的/
		szCurSegPos = szNextSegPos + 1;//跳过下一个/
		return strFullPath.substr(szOff, szCount);//返回子串
	}

	static PathSegList TokenizeRegisterPath(const std::string &strFullPath)
	{
		if (strFullPath.empty())
		{
			return {};
		}

		PathSegList ret{};
		size_t szCurSegPos = 0;
		size_t szFullPathSize = strFullPath.size();
		do
		{
			ret.push_back(TokenizeRegisterPathNested(strFullPath, szCurSegPos));
		} while (szCurSegPos < szFullPathSize);

		return ret;
	}


	//获取注册路径片段的类型
	static NodeType GetRegisterSegType(const std::string &strSegment)
	{
		if (strSegment.empty())
		{
			return NodeType::STATIC;//根结点或空节点
		}

		//遍历字符串，查看是否为参数或单段、多段匹配
		const char *pCur = strSegment.data();
		const char *pEnd = pCur + strSegment.size();
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
			return NodeType::NONE;//未知
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
	注册时的转义字符
	\\ = \
	\* = *
	\: = :
	\/ = /
	*/
	ValType *RegisterPath(const std::string &strPath, ValType tVal)
	{
		PathSegList listPathSeg = TokenizeRegisterPath(strPath);
		if (listPathSeg.empty())
		{
			return NULL;
		}











	}

	ValType *FindRegisterPath(const std::string &strPath)
	{
		PathSegList listPathSeg = TokenizeRegisterPath(strPath);
		if (listPathSeg.empty())
		{
			return NULL;
		}




	}





	ValType *FindRequestPath(const std::string &strPath)
	{
		PathSegList listPathSeg = TokenizeRequestPath(strPath);
		if (listPathSeg.empty())
		{
			return NULL;
		}




	}








};


