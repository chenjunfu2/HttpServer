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
		PATH,			//解析路径
		VERSION,		//解析版本

		START_LINE_END,	//开始行结束

		FIELD_KEY,		//解析header键
		FIELD_KEY_END,	//header键后空白
		FIELD_VAL,		//解析header值
		FIELD_VAL_END,	//header值后空白
		FIELD_LINE_END,	//解析header行结束

		HEADER_FIELD_END,//header解析结束

		BODY,			//解析body(content)

		COMPLETE,		//解析完成

		ERROR,			//解析错误
	};

	enum class ParseError
	{
		NO_ERR = 0,

		UNEXPECTED_CHAR,//非法字符
		INVALID_FORMAT,//非法格式
		INCOMPLETE_REQUEST,//不完整的请求

		HEADER_TOO_LARGE,//请求头过长
		BODY_TOO_LARGE,//请求体过长

		INVALID_METHOD,//无效方法
		INVALID_PATH,//无效路径
		INVALID_VERSION,//无效版本
	};

protected:
	ParseState enParseState = ParseState::READY;
	ParseError enParseError = ParseError::NO_ERR;

	std::string strTempBuffer{};

	size_t szMaxPathLength = 0;//最大路径长度
	size_t szMaxHeaderLength = 0;//最大头部长度
	size_t szMaxContentLength = 0;//最大内容长度（Body长度）
	size_t szMaxRequestLength = 0;//完整请求最大长度
	
	size_t szConsecutiveCRLF = 0;//连续的换行
	bool bWaitLF = false;//遇到CR，等待LF

protected:
	DEFAULT_CSTC(StateContext);

	void Clear(void) noexcept
	{
		enParseState = ParseState::READY;
		enParseError = ParseError::NO_ERR;

		strTempBuffer.clear();

		szMaxPathLength = 0;
		szMaxHeaderLength = 0;
		szMaxContentLength = 0;
		szMaxRequestLength = 0;

		szConsecutiveCRLF = 0;
		bWaitLF = false;
	}

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

	GETTER_COPY(MaxPathLength, szMaxPathLength);
	GETTER_COPY(MaxHeaderLength, szMaxHeaderLength);
	GETTER_COPY(MaxContentLength, szMaxContentLength);
	GETTER_COPY(MaxRequestLength, szMaxRequestLength);

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

public:
	struct StartLine
	{
		Method enMethod = Method::UNKNOWN;
		std::string strPath{};
		std::string strVersion{};
		
	};
	
	struct HeaderField
	{
		ConnectionType enConnectionType = ConnectionType::UNKNOWN;
		size_t szContentLength = 0;//内容长度（Body长度）
		std::string strHost{};
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
		stStartLine.strVersion.clear();
		
		stHeaderField.enConnectionType = ConnectionType::UNKNOWN;
		stHeaderField.szContentLength = 0;
		stHeaderField.strHost.clear();
		stHeaderField.mapFields.clear();

		stMessageBody.strContent.clear();
	}

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

		stStartLine.enMethod = it->second;
		return true;
	}

private:
	//--------------------------------------------------------------------------//

	//-1->其它字符 0->失败 1->CR 2->LF
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

	//-1->遇到非空白  0->出错 1->成功
	int32_t ParseSkipSpace(StateContext &contextState, char c) noexcept
	{
		if (c == '\r' || c == '\n')//非法
		{
			contextState.SetParseError(StateContext::ParseError::UNEXPECTED_CHAR);
			return 0;
		}

		if (isspace(c))//跳过非CRLF空白
		{
			return 1;
		}

		return -1;
	}

	//--------------------------------------------------------------------------//


	/*
		|------------------------------------------------------------------
		|HTTP-message = 
		|				start-line
		|				*( header-field CRLF )
		|				CRLF
		|				[ message-body ]
		|------------------------------------------------------------------
		|
		|	|------------------------------------------------------------------
		|	|start-line = request-line / status-line
		|	|------------------------------------------------------------------
		|	|
		|	|	|------------------------------------------------------------------
		|	|	|request-line =
		|	|	|				method SP request-target SP HTTP-version
		|	|	|				CRLF
		|	|	|
		|	|	|	method = token
		|	|	|------------------------------------------------------------------
		|	|	|
		|	|	|------------------------------------------------------------------
		|	|	|status-line =
		|	|	|				HTTP-version SP status-code SP reason-phrase
		|	|	|				CRLF
		|	|	|
		|	|	|	status-code = 3DIGIT
		|	|	|	reason-phrase  = *( HTAB / SP / VCHAR / obs-text )
		|	|	|------------------------------------------------------------------
		|	|
		|	|------------------------------------------------------------------
		|	|header-field = field-name ":" OWS field-value OWS
		|	|
		|	|	field-name		= token
		|	|	field-value		= *( field-content / obs-fold )
		|	|	field-content	= field-vchar [ 1*( SP / HTAB ) field-vchar ]
		|	|	field-vchar		= VCHAR / obs-text
		|	|
		|	|		obs-fold	= CRLF 1*( SP / HTAB )
		|	|					; obsolete line folding
		|	|					; see Section 3.2.4
		|	|------------------------------------------------------------------
		|
		|------------------------------------------------------------------
		|OWS	= *( SP / HTAB )
		|	; optional whitespace
		|RWS	= 1*( SP / HTAB )
		|	; required whitespace
		|BWS	= OWS
		|	; "bad" whitespace
		|------------------------------------------------------------------
	*/
	
	//在Start-Line之前，至多出现一个CRLF，且不能有多余的空白
	bool ParseReady(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		if (contextState.szConsecutiveCRLF >= 1)
		{
			contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
			return false;
		}


		int32_t i32Ret = ParseCRLF(contextState, c);
		if (i32Ret == 0)
		{
			return false;
		}
		else if (i32Ret == 1)
		{
			++contextState.szConsecutiveCRLF;
			return true;
		}
		//else (i32Ret == -1)其他字符，走下面处理

		//遇到第一个非空白，转到METHOD处理
		contextState.enParseState = StateContext::ParseState::METHOD;
		bReuseChar = true;//重用字符，交给METHOD解析

		return true;
	}

	//请求行以方法标识符起始，后接单个空格(SP)、请求目标、再一个单个空格(SP)、协议版本，最终以CRLF结尾。

	bool ParseMethod(StateContext &contextState, char c, bool &bReuseChar) noexcept
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

			//清理缓冲区
			contextState.strTempBuffer.clear();

			//转换并重用字符
			contextState.enParseState = StateContext::ParseState::PATH;
			bReuseChar = true;
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

	bool ParsePath(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{

		return true;
	}

	//拒绝所有以空白起始行的消息
	bool ParseVersion(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{

		return true;
	}

	bool ParseStartLineEnd(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{

		return true;
	}

	bool ParseFieldKey(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{

		return true;
	}

	bool ParseFieldKeyEnd(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{

		return true;
	}

	bool ParseFieldVal(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{

		return true;
	}

	bool ParseFieldValEnd(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{

		return true;
	}

	bool ParseFieldLineEnd(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{

		return true;
	}

	bool ParseHeaderFieldEnd(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{

		return true;
	}

	bool ParseBody(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{

		return true;
	}

	//--------------------------------------------------------------------------//

	bool ParseChar(StateContext &contextState, char c) noexcept
	{
		bool bReuseChar{};
		bool bRet = false;
		do
		{
			bReuseChar = false;//每次重置

			switch (contextState.enParseState)
			{
			case StateContext::ParseState::READY:
				bRet = ParseReady(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::METHOD:
				bRet = ParseMethod(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::PATH:
				bRet = ParsePath(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::VERSION:
				bRet = ParseVersion(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::START_LINE_END:
				bRet = ParseStartLineEnd(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::FIELD_KEY:
				bRet = ParseFieldKey(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::FIELD_KEY_END:
				bRet = ParseFieldKeyEnd(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::FIELD_VAL:
				bRet = ParseFieldVal(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::FIELD_VAL_END:
				bRet = ParseFieldValEnd(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::FIELD_LINE_END:
				bRet = ParseFieldLineEnd(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::HEADER_FIELD_END:
				bRet = ParseHeaderFieldEnd(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::BODY:
				bRet = ParseBody(contextState, c, bReuseChar);
				break;
			case StateContext::ParseState::COMPLETE:
			case StateContext::ParseState::ERROR:
			default:
				bRet = false;
				break;
			}
		} while (bReuseChar);

		return bRet;
	}

	//--------------------------------------------------------------------------//
	
	static StateContext GetNewContext(
		size_t szMaxPathLength,
		size_t szMaxHeaderLength,
		size_t szMaxContentLength,
		size_t szMaxRequestLength) noexcept
	{
		StateContext ctxNew{};

		ctxNew.szMaxPathLength = szMaxPathLength;
		ctxNew.szMaxHeaderLength = szMaxHeaderLength;
		ctxNew.szMaxContentLength = szMaxContentLength;
		ctxNew.szMaxRequestLength = szMaxRequestLength;

		return ctxNew;
	}

	static void ReuseContext(
		StateContext &ctxReuse,
		size_t szMaxPathLength,
		size_t szMaxHeaderLength,
		size_t szMaxContentLength,
		size_t szMaxRequestLength) noexcept
	{
		ctxReuse.Clear();

		ctxReuse.szMaxPathLength = szMaxPathLength;
		ctxReuse.szMaxHeaderLength = szMaxHeaderLength;
		ctxReuse.szMaxContentLength = szMaxContentLength;
		ctxReuse.szMaxRequestLength = szMaxRequestLength;
	}

	//--------------------------------------------------------------------------//

public:
	DEFAULT_CSTC(HttpRequest);
	DEFAULT_DSTC(HttpRequest);

	DEFAULT_MOVE(HttpRequest);
	DELETE_COPY(HttpRequest);

	bool ParsingStream(StateContext &contextState, const std::string &strStream, size_t &szConsumedLength)
	{
		szConsumedLength = 0;

		if (strStream.empty())
		{
			return false;
		}

		for (auto it : strStream)
		{
			if (!ParseChar(contextState, it))
			{
				break;
			}

			++szConsumedLength;
		}

		//出错或完成
		//返回退出状态
		return contextState.enParseState == StateContext::ParseState::COMPLETE;
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


public:
	DELETE_CPMV(HttpServer);













};





