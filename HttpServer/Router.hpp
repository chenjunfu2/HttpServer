#pragma once

#include "CPP_Helper.h"

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#include "RadixTree.hpp"

#include <unordered_map>

class Router
{
public:
	class Route
	{








	};

private:
	RadixTree<Route> Routes;

public:
	DEFAULT_CSTC(Router);
	DEFAULT_DSTC(Router);
	DEFAULT_MOVE(Router);
	DELETE_COPY(Router);











};



//TODO:使用Radix Tree存储路径，支持正则表达式





