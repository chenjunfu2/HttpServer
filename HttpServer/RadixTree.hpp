#pragma once

#include <string>


template<typename VAL>
class RadixTree
{
public:
	enum class NodeType : uint64_t
	{
		STATIC,      // 静态节点（强匹配）
		PARAM,       // 参数节点（:param）
		WILD_SINGLE, // 单段通配符（*）
		WILD_MULTI,  // 多段通配符（**）
	};

	struct Node
	{





	};



public:





};


