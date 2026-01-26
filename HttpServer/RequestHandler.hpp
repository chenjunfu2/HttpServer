#pragma once

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class RequestHandler
{
    bool HandleRequest(const HttpRequest &req, HttpResponse &res)
    {
        // 1. 调用路由层，匹配路由
        // 2. 执行匹配到的控制器/中间件
        // 3. 将结果写入res
    }


};

