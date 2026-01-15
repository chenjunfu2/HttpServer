#pragma once

#include "Windows_TcpSocket.h"
#include "VirtualFileSystem.hpp"
#include <ctype.h>

class HttpRequest;

class StateContext
{
	friend class HttpRequest;
public:
	enum class ParseState
	{
		READY = 0,		//就绪状态

		METHOD,			//解析方法
		METHOD_END,		//方法后空白
		PATH,			//解析路径
		PATH_END,		//路径后空白
		VERSION,		//解析版本
		VERSION_END,	//版本后空白与换行

		REQUEST_LINE_END,//请求行结束

		HEADER_KEY,		//解析header键
		HEADER_KEY_END,	//header键后空白
		HEADER_VAL,		//解析header值
		HEADER_VAL_END,	//header值后空白与换行

		HEADER_LINE_END,//解析header行结束

		HEADER_END,		//header解析结束

		BODY,			//解析body

		COMPLETE,		//解析完成

		ERROR,			//解析错误
	};

	enum class ParseError
	{
		NO_ERR = 0,
		INVALID_METHOD,//无效方法
		INVALID_PATH,//无效路径
		INVALID_VERSION,//无效版本
		HEADER_TOO_LARGE,//请求头过长
		BODY_TOO_LARGE,//请求体过长
		UNEXPECTED_CHAR,//非法字符
		INCOMPLETE_REQUEST,//不完整的请求
	};

protected:
	ParseState enParseState = ParseState::READY;
	ParseError enParseError = ParseError::NO_ERR;

	size_t szContentLength = 0;
	size_t szHeaderSize = 0;
	
	size_t szConsecutiveCRLF = 0;//连续的换行
	bool bWaitLF = false;//遇到CR，等待LF

	std::string strTempBuffer{};
	

protected:
	void Clear(void) noexcept
	{
		enParseState = ParseState::READY;

		szContentLength = 0;
		szHeaderSize = 0;

		szConsecutiveCRLF = 0;
		bWaitLF = false;

		strTempBuffer.clear();
	}

	DEFAULT_CSTC(StateContext);

	void SetParseError(ParseError _enParseError)
	{
		enParseState = ParseState::ERROR;
		enParseError = _enParseError;
	}


public:
	DELETE_COPY(StateContext);
	DEFAULT_MOVE(StateContext);
	DEFAULT_DSTC(StateContext);

	GETTER_COPY(ParseState, enParseState);
	GETTER_COPY(ParseError, enParseError);

	GETTER_COPY(ContentLength, szContentLength);
	GETTER_COPY(HeaderSize, szHeaderSize);

};

class HttpRequest
{
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

	enum class ConnectionType
	{
		UNKNOWN = 0,
		KEEP_ALIVE,
		CLOSE,
	};

private:
	Method enMethod = Method::UNKNOWN;
	ConnectionType enConnectionType = ConnectionType::UNKNOWN;

	std::string strPath{};
	std::string strVersion{};
	std::string strHost{};
	std::string strBody{};

	std::unordered_map<std::string, std::string> mapHeaders{};

private:
	bool SetMethod(const std::string &strMethod) noexcept
	{
		static inline const std::unordered_map<std::string, Method> mapMethod =
		{
			{"GET", Method::GET},
			{"HEAD", Method::HEAD},
			{"POST", Method::POST},
			{"PUT", Method::PUT},
			{"DELETE", Method::DELETE},
			{"CONNECT", Method::CONNECT},
			{"OPTIONS", Method::OPTIONS},
			{"TRACE", Method::TRACE},
			{"PATCH", Method::PATCH},
		};

		size_t strMethodSize = strMethod.size();
		if (strMethodSize < 3 || strMethodSize > 7)
		{
			return false;
		}

		auto it = mapMethod.find(strMethod);
		if (it == mapMethod.end())
		{
			return false;
		}

		enMethod = it->second;
		return true;
	}

	bool ParseChar(StateContext &contextState, char c) noexcept
	{
		switch (contextState.enParseState)
		{
		case StateContext::ParseState::READY:
			return ParseReady(contextState, c, StateContext::ParseState::METHOD);
			break;
		case StateContext::ParseState::METHOD:
			return ParseMethod(contextState, c);
			break;
		case StateContext::ParseState::METHOD_END:
			return ParseSkipSpace(contextState, c, StateContext::ParseState::PATH);
			break;
		case StateContext::ParseState::PATH:
			return ParsePath(contextState, c);
			break;
		case StateContext::ParseState::PATH_END:
			return ParseSkipSpace(contextState, c, StateContext::ParseState::VERSION);
			break;
		case StateContext::ParseState::VERSION:
			return ParseVersion(contextState, c);
			break;
		case StateContext::ParseState::VERSION_END:
			//return;
			break;
		case StateContext::ParseState::REQUEST_LINE_END:

			break;
		case StateContext::ParseState::HEADER_KEY:
			return ParseHeaderKey(contextState, c);
			break;
		case StateContext::ParseState::HEADER_KEY_END:

			break;
		case StateContext::ParseState::HEADER_VAL:
			return ParseHeaderVal(contextState, c);
			break;
		case StateContext::ParseState::HEADER_VAL_END:

			break;
		case StateContext::ParseState::HEADER_LINE_END:

			break;
		case StateContext::ParseState::HEADER_END:
			return ParseHeaderEnd(contextState, c);
			break;
		case StateContext::ParseState::BODY:
			return ParseBody(contextState, c);
			break;
		case StateContext::ParseState::COMPLETE:
		case StateContext::ParseState::ERROR:
		default:
			return false;
			break;
		}
	}

	//-1->其它 0->失败 1->CR 2->LF
	int32_t ParseCRLF(StateContext &contextState, char c) noexcept
	{
		if (c == '\r')//保证\r\n成对出现
		{
			if (contextState.bWaitLF == false)
			{
				contextState.bWaitLF = true;
				return 1;
			}
			else
			{
				contextState.SetParseError(StateContext::ParseError::UNEXPECTED_CHAR);
				return 0;
			}
		}
		else if (c == '\n')
		{
			if (contextState.bWaitLF == true)
			{
				contextState.bWaitLF = false;
				++contextState.szConsecutiveCRLF;
				return 2;
			}
			else
			{
				contextState.SetParseError(StateContext::ParseError::UNEXPECTED_CHAR);
				return 0;
			}
		}
		else//其它字符
		{
			return -1;
		}
	}

	bool ParseSkipSpace(StateContext &contextState, char c, StateContext::ParseState enNextState) noexcept
	{
		if (c == '\r' || c == '\n')//非法
		{
			contextState.SetParseError(StateContext::ParseError::UNEXPECTED_CHAR);
			return false;
		}

		if (isspace(c))//跳过非CRLF空白
		{
			return true;
		}

		if (!isalpha(c))//确保字符正确
		{
			contextState.SetParseError(StateContext::ParseError::UNEXPECTED_CHAR);
			return false;
		}

		contextState.strTempBuffer.push_back(c);
		contextState.enParseState = enNextState;
		return true;
	}

	bool ParseReady(StateContext &contextState, char c, StateContext::ParseState enNextState) noexcept
	{
		int32_t i32Ret = ParseCRLF(contextState, c);
		if (i32Ret >= 0)
		{
			return (bool)i32Ret;
		}
		//else -1其他字符，走下面处理

		if (isspace(c))//跳过空白（前面已经筛掉CRLF，放心跳过剩余非CRLF空白）
		{
			return true;
		}

		if (!isalpha(c))//确保字符正确
		{
			contextState.SetParseError(StateContext::ParseError::UNEXPECTED_CHAR);
			return false;
		}

		contextState.strTempBuffer.push_back(c);
		contextState.enParseState = enNextState;
		return true;
	}

	bool ParseMethod(StateContext &contextState, char c) noexcept
	{
		if (c == '\r' || c == '\n')//非法
		{
			contextState.SetParseError(StateContext::ParseError::UNEXPECTED_CHAR);
			return false;
		}

		if (isspace(c))//遇到空白，请求方法结束
		{
			if (!SetMethod(contextState.strTempBuffer))
			{
				contextState.SetParseError(StateContext::ParseError::INVALID_METHOD);
				return false;
			}

			contextState.strTempBuffer.clear();
			contextState.enParseState = StateContext::ParseState::PATH;
			return true;
		}

		if (!isalpha(c))//非法字符
		{
			contextState.SetParseError(StateContext::ParseError::INVALID_METHOD);
			return false;
		}

		if (contextState.strTempBuffer.size() >= MAX_METHOD_LENGTH)//过长请求方法
		{
			contextState.SetParseError(StateContext::ParseError::INVALID_METHOD);
			return false;
		}

		contextState.strTempBuffer.push_back(c);
		return true;
	}

	bool ParsePath(StateContext &contextState, char c) noexcept
	{

		return true;
	}

	bool ParseVersion(StateContext &contextState, char c) noexcept
	{

		return true;
	}

	bool ParseVersionEnd(StateContext &contextState, char c) noexcept
	{

		return true;
	}

	bool ParseHeaderKey(StateContext &contextState, char c) noexcept
	{

		return true;
	}

	bool ParseHeaderVal(StateContext &contextState, char c) noexcept
	{

		return true;
	}

	bool ParseHeaderEnd(StateContext &contextState, char c) noexcept
	{

		return true;
	}

	bool ParseBody(StateContext &contextState, char c) noexcept
	{

		return true;
	}

public:
	void Clear(void) noexcept
	{
		enMethod = Method::UNKNOWN;
		enConnectionType = ConnectionType::UNKNOWN;

		strPath.clear();
		strVersion.clear();
		strHost.clear();
		strBody.clear();

		mapHeaders.clear();
	}

	DEFAULT_CSTC(HttpRequest);
	DEFAULT_DSTC(HttpRequest);

	DEFAULT_MOVE(HttpRequest);
	DELETE_COPY(HttpRequest);

	static StateContext GetNewContext(void) noexcept
	{
		return {};
	}

	static void ClearContext(StateContext &contextState) noexcept
	{
		contextState.Clear();
	}

	bool ParsingStream(StateContext &contextState, const std::string &strStream, size_t &szConsumedLength)
	{
		size_t szCurrent = 0;

		if (strStream.empty())
		{
			szConsumedLength = 0;
			return false;
		}








	}







};

class HttpResponse
{
public:
	enum class Status : uint32_t
	{
		OK								= 200,	//请求成功

		NOT_MODIFIED					= 304,	//之前请求的内容未改变，无需重复发送

		BAD_REQUEST						= 400,	//非法的请求，服务端拒绝处理
		FORBIDDEN						= 403,	//没有访问权限
		NOT_FOUND						= 404,	//未找到请求的资源
		METHOD_NOT_ALLOWED				= 405,	//不支持、禁止的请求方法
		REQUEST_TIMEOUT					= 408,	//请求超时（服务器关闭无用链接）

		CONTENT_TOO_LARGE				= 413,	//请求体过大（也可以叫PAYLOAD_TOO_LARGE）
		URI_TOO_LONG					= 414,	//请求URL过长
		TOO_MANY_REQUESTS				= 429,	//短时间发送过多请求
		REQUEST_HEADER_FIELDS_TOO_LARGE	= 431,	//请求头过大

		INTERNAL_ERROR					= 500,	//服务器意外
		NOT_IMPLEMENTED					= 501,	//不支持的请求方法
		SERVICE_UNAVAILABLE				= 503,	//服务器维护
		VERSION_NOT_SUPPORTED			= 505,	//不支持的http版本
		INSUFFICIENT_STORAGE			= 507,	//空间不足
	};

private:





};

class Connection
{

};


class HttpServer
{
private:
	size_t szMaxHeaderSize;//请求头最大值
	size_t szMaxBodySize;//请求体最大值



public:
	DELETE_CPMV(HttpServer);













};





