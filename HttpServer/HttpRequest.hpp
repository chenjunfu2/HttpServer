#pragma once

#include "CPP_Helper.h"

#include <ctype.h>
#include <string>
#include <unordered_map>

class HttpRequest
{
	friend class HttpParser;
public:
	enum class Method
	{
		UNKNOWN = 0,	//未知请求
		GET,			//获取数据，包括响应头和响应体
		HEAD,			//获取数据，仅响应头
		POST,
		PUT,
		DELETE,
		CONNECT,
		OPTIONS,
		TRACE,
		PATCH,
	};

	static inline constexpr size_t MIN_METHOD_LENGTH = 3;
	static inline constexpr size_t MAX_METHOD_LENGTH = 7;
	static inline constexpr size_t VERSION_LENGTH = sizeof("HTTP/x.x") - 1;

	enum class ConnectionType
	{
		UNKNOWN = 0,
		KEEP_ALIVE,
		CLOSE,
	};

public:
	struct StartLine
	{
		Method enMethod = Method::UNKNOWN;//请求方法
		std::string strPath{};//请求路径
		uint8_t u8MajorVersion;//主版本号
		uint8_t u8MinorVersion;//次版本号
	};

	struct HeaderField
	{
		ConnectionType enConnectionType = ConnectionType::UNKNOWN;
		size_t szContentLength = 0;//内容长度（Body长度）
		std::string *pstrHost = NULL;
		std::unordered_map<std::string, std::string> mapFields{};
	};

	struct MessageBody
	{
		std::string strContent{};//Body
	};

private:
	StartLine stStartLine;
	HeaderField stHeaderField;
	MessageBody stMessageBody;

public:
	void Clear(void) noexcept
	{
		stStartLine.enMethod = Method::UNKNOWN;
		stStartLine.strPath.clear();
		stStartLine.u8MajorVersion = 0;
		stStartLine.u8MinorVersion = 0;

		stHeaderField.enConnectionType = ConnectionType::UNKNOWN;
		stHeaderField.szContentLength = 0;
		stHeaderField.pstrHost = NULL;
		stHeaderField.mapFields.clear();

		stMessageBody.strContent.clear();
	}

	DEFAULT_CSTC(HttpRequest);
	DEFAULT_DSTC(HttpRequest);

	DEFAULT_MOVE(HttpRequest);
	DELETE_COPY(HttpRequest);
};
