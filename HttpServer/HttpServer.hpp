#pragma once

#include "Windows_TcpSocket.h"
#include "VirtualFileSystem.hpp"

#include "MyAssert.hpp"

#include <ctype.h>
#include <charconv>


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
		BODY_END,		//body解析完成

		COMPLETE,		//解析完成

		ERROR,			//解析错误
	};

	enum class ParseError
	{
		NO_ERR = 0,

		UNEXPECTED_CHAR,//非法字符
		INVALID_FORMAT,//非法格式
		DUPLICATE_FIELDS,//重复的字段
		INCOMPLETE_REQUEST,//不完整的请求

		STARTLINE_TOO_LARGE,//起始行过长
		HEADER_TOO_LARGE,//请求头过长
		BODY_TOO_LARGE,//请求体过长
	};

protected:
	ParseState enParseState = ParseState::READY;
	ParseError enParseError = ParseError::NO_ERR;

	std::string strTempBuffer{};
	std::string strTempBuffer2{};

	size_t szMaxPathLength = 0;//最大路径长度
	size_t szMaxHeaderLength = 0;//最大头部长度
	size_t szMaxContentLength = 0;//最大内容长度（Body长度）

	size_t szCurHeaderLenght = 0;//当前头部长度
	
	size_t szConsecutiveRWS = 0;//连续的RWS空白数
	size_t szConsecutiveCRLF = 0;//连续的换行
	bool bWaitLF = false;//遇到CR，等待LF

protected:
	DEFAULT_CSTC(StateContext);

	void Clear(void) noexcept
	{
		enParseState = ParseState::READY;
		enParseError = ParseError::NO_ERR;

		strTempBuffer.clear();
		strTempBuffer2.clear();

		szMaxPathLength = 0;
		szMaxHeaderLength = 0;
		szMaxContentLength = 0;

		szCurHeaderLenght = 0;

		szConsecutiveRWS = 0;
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

private:
	bool SetMethod(const std::string &strMethod) noexcept
	{
		static const std::unordered_map<std::string, Method> mapMethod =
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

		//先初始化为未知
		stStartLine.enMethod = Method::UNKNOWN;

		//检查长度
		size_t strMethodSize = strMethod.size();
		if (strMethodSize < 3 || strMethodSize > 7)//短于最短，长于最长
		{
			return false;
		}

		auto it = mapMethod.find(strMethod);
		if (it == mapMethod.end())
		{
			return false;
		}

		stStartLine.enMethod = it->second;//设置实际查找值
		return true;
	}

	void SetConnectionType(void) noexcept
	{
		static const std::unordered_map<std::string, ConnectionType> mapConnectionType =
		{
			{"keep-alive", ConnectionType::KEEP_ALIVE},
			{"close", ConnectionType::CLOSE},
		};


		//先设置未知
		stHeaderField.enConnectionType = ConnectionType::UNKNOWN;

		auto itConnection = stHeaderField.mapFields.find("Connection");
		if (itConnection == stHeaderField.mapFields.end())
		{
			return;
		}

		//已找到
		auto &strConnectionType = itConnection->second;

		//检查长度
		size_t strConnectionTypeSize = strConnectionType.size();
		if (strConnectionTypeSize < 5 || strConnectionTypeSize > 10)//短于最短，长于最长
		{
			return;
		}

		auto it = mapConnectionType.find(strConnectionType);
		if (it == mapConnectionType.end())
		{
			return;
		}

		stHeaderField.enConnectionType = it->second;//设置实际查找值

		return;
	}


	bool SetContentLength(StateContext &contextState)//注意，这里是解析发生错误返回false，而非找不到返回false
	{
		//先设置长度为0
		stHeaderField.szContentLength = 0;

		auto itContentLength = stHeaderField.mapFields.find("Content-Length");
		if (itContentLength == stHeaderField.mapFields.end())
		{
			return true;//找不到直接true，值为0
		}

		auto &strContentLength = itContentLength->second;

		//解析为数值（严格解析）
		const char *pBeg = strContentLength.data();
		const char *pEnd = strContentLength.data() + strContentLength.size();

		auto [pUse, errCode] = std::from_chars(pBeg, pEnd, stHeaderField.szContentLength);
		if (errCode != std::errc() || pUse != pEnd)
		{
			contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
			return false;
		}

		return true;
	}

	bool SetHost(StateContext &contextState)//标准要求Host必须要存在，否则失败
	{
		//先设置为NULL
		stHeaderField.pstrHost = NULL;

		auto itHost = stHeaderField.mapFields.find("Host");
		if (itHost == stHeaderField.mapFields.end())
		{
			contextState.SetParseError(StateContext::ParseError::INCOMPLETE_REQUEST);//必须字段缺少，请求不完整
			return false;
		}

		stHeaderField.pstrHost = &itHost->second;

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
			if (contextState.bWaitLF == true)//什么？CR之后不是LF？
			{
				contextState.bWaitLF = false;//清除标志并设置错误

				contextState.SetParseError(StateContext::ParseError::UNEXPECTED_CHAR);
				return 0;
			}

			return -1;
		}
	}

	bool IsNoSpacePrintChar(char c) noexcept
	{
		return c >= 0x21 && c <= 0x7E;
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
		|	|	|	request-target = origin-form / absolute-form / authority-form / asterisk-form
		|	|	|	HTTP-version  = HTTP-name "/" DIGIT "." DIGIT
		|	|	|		HTTP-name     = %x48.54.54.50 ; "HTTP", case-sensitive
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
		|
		|------------------------------------------------------------------
		|token	=	1*tchar
		|
		|tchar	=	"!" / "#" / "$" / "%" / "&" / "'" / "*"
		|			/ "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
		|			/ DIGIT / ALPHA
		|			; any VCHAR, except delimiters
		|------------------------------------------------------------------
		|
		|------------------------------------------------------------------
		|quoted-string	= DQUOTE *( qdtext / quoted-pair ) DQUOTE
		|qdtext			= HTAB / SP /%x21 / %x23-5B / %x5D-7E / obs-text
		|obs-text		= %x80-FF
		|comment		= "(" *( ctext / quoted-pair / comment ) ")"
		|ctext			= HTAB / SP / %x21-27 / %x2A-5B / %x5D-7E / obs-text
		|quoted-pair	= "\" ( HTAB / SP / VCHAR / obs-text )
		|------------------------------------------------------------------
	*/
	
	//在Start-Line之前，至多出现一个CRLF，且不能有多余的空白
	/*
		In the interest of robustness, a server that is expecting to receive and parse 
		a request-line SHOULD ignore at least one empty line (CRLF) received prior 
		to the request-line.
	*/
	bool ParseReady(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		int32_t i32Ret = ParseCRLF(contextState, c);
		if (i32Ret == 0)
		{
			return false;//ParseCRLF已设置错误
		}
		else if (i32Ret == 1)//遇到CR不处理
		{
			return true;
		}
		else if (i32Ret == 2)
		{
			if (++contextState.szConsecutiveCRLF > 1)//多个CRLF？
			{
				contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
				return false;
			}

			return true;
		}
		//else //(i32Ret == -1)其他字符，走下面处理

		//遇到第一个非空白，迁移状态
		contextState.enParseState = StateContext::ParseState::METHOD;
		contextState.szConsecutiveCRLF = 0;//清理CRLF计数器
		bReuseChar = true;//重用字符

		return true;
	}

	//请求行以方法标识符起始，后接单个空格(SP)、请求目标、再一个单个空格(SP)、协议版本，最终以CRLF结尾。
	/*
		A request-line begins with a method token, followed by a single space (SP), 
		the request-target, another single space (SP), the protocol version, and ends 
		with CRLF.
	*/

	bool ParseMethod(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		if (c == ' ')//遇到空白
		{
			//尝试解析并设置方法
			if (!SetMethod(contextState.strTempBuffer))
			{
				contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
				return false;
			}

			//成功后清理缓冲区
			contextState.strTempBuffer.clear();

			//转换到下一状态（注意此空白字符被丢弃而非重用，不赋值bReuseChar=true）
			contextState.enParseState = StateContext::ParseState::PATH;
			
			return true;
		}

		if (!isalpha(c))//非法字符（方法只能是字母）
		{
			contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
			return false;
		}

		if (contextState.strTempBuffer.size() >= MAX_METHOD_LENGTH)//过长请求方法
		{
			contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
			return false;
		}

		//临时缓存+1
		contextState.strTempBuffer.push_back(c);

		return true;
	}


	bool ParsePath(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		if (c == ' ')//遇到空白
		{
			//设置路径字符串
			stStartLine.strPath = std::move(contextState.strTempBuffer);
			contextState.strTempBuffer.clear();
			//丢弃空白，然后迁移到http版本状态
			contextState.enParseState = StateContext::ParseState::VERSION;

			return true;
		}

		//字符合法性
		if (!IsNoSpacePrintChar(c))
		{
			contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
			return false;
		}

		if (contextState.strTempBuffer.size() >= contextState.szMaxPathLength)
		{
			contextState.SetParseError(StateContext::ParseError::STARTLINE_TOO_LARGE);
			return false;
		}

		//临时缓存+1
		contextState.strTempBuffer.push_back(c);

		return true;
	}


	//标准要求版本号必须以"HTTP/"开头且只有两个"数字"，并以"."分割
	/*
		The HTTP version number consists of two decimal digits separated by a "." 
		(period or decimal point). The first digit ("major version") indicates the HTTP 
		messaging syntax, whereas the second digit ("minor version") indicates the 
		highest minor version within that major version to which the sender is 
		conformant and able to understand for future communication.
	*/
	bool ParseVersion(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		int32_t i32Ret = ParseCRLF(contextState, c);
		if (i32Ret != -1)
		{
			if (i32Ret == 0)
			{
				return false;//错误已在ParseCRLF内设置
			}

			//第一次到这里必须要是遇到CR，因为如果第一次直接遇到LF那么ParseCRLF必须正确处理错误
			MyAssert(i32Ret == 1, "Not CR, is LF, what the fuck?");

			//必须相等
			if (contextState.strTempBuffer.size() != VERSION_LENGTH)
			{
				contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
				return false;
			}

			//版本必须以"HTTP/"开头，否则错误
			if (!contextState.strTempBuffer.starts_with("HTTP/"))
			{
				contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
				return false;
			}

			//解析出"数字"."数字"形式的主次版本号

			//格式强制判断
			if (!isdigit(contextState.strTempBuffer[5]) ||
				contextState.strTempBuffer[6] != '.' ||
				!isdigit(contextState.strTempBuffer[7]))
			{
				contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
				return false;
			}

			//获取主次版本
			stStartLine.u8MajorVersion = contextState.strTempBuffer[5] - '0';
			stStartLine.u8MinorVersion = contextState.strTempBuffer[7] - '0';

			//清理，丢弃空白并迁移
			contextState.strTempBuffer.clear();
			contextState.enParseState = StateContext::ParseState::START_LINE_END;

			return true;
		}

		//字符合法性
		if (!IsNoSpacePrintChar(c))//隐含空白约束
		{
			contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
			return false;
		}

		//过长版本号
		if (contextState.strTempBuffer.size() >= VERSION_LENGTH)
		{
			contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
			return false;
		}

		//临时缓存+1
		contextState.strTempBuffer.push_back(c);

		return true;
	}

	bool ParseStartLineEnd(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		int32_t i32Ret = ParseCRLF(contextState, c);
		if (i32Ret == 0)
		{
			return false;//ParseCRLF已设置错误
		}
		else if (i32Ret == 1)//遇到CR不处理
		{
			return true;
		}
		else if (i32Ret == 2)
		{
			if (++contextState.szConsecutiveCRLF > 1)//多个CRLF？
			{
				contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
				return false;
			}

			return true;
		}
		//else//(i32Ret == -1)遇到CRLF以外

		//迁移到Key状态
		contextState.enParseState = StateContext::ParseState::FIELD_KEY;
		contextState.szConsecutiveCRLF = 0;//清理CRLF计数器
		bReuseChar = true;//重用字符

		return true;
	}

	bool ParseFieldKey(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		if (++contextState.szCurHeaderLenght > contextState.szMaxHeaderLength)
		{
			contextState.SetParseError(StateContext::ParseError::HEADER_TOO_LARGE);
			return false;
		}

		if (c == ':')
		{
			//已存在字段重复，请求有误
			if (stHeaderField.mapFields.contains(contextState.strTempBuffer))
			{
				contextState.SetParseError(StateContext::ParseError::DUPLICATE_FIELDS);
				return false;
			}

			//字段无误，等待值
			//直接迁移到下一状态，不清理buffer，不重用字符
			contextState.enParseState = StateContext::ParseState::FIELD_KEY_END;

			return true;
		}

		//字符合法性
		if (!IsNoSpacePrintChar(c))//注意此处隐含空白约束
		{
			contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
			return false;
		}

		//临时缓存+1
		contextState.strTempBuffer.push_back(c);

		return true;
	}

	bool ParseFieldKeyEnd(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		if (++contextState.szCurHeaderLenght > contextState.szMaxHeaderLength)
		{
			contextState.SetParseError(StateContext::ParseError::HEADER_TOO_LARGE);
			return false;
		}

		//根据标准描述，':'后面可以是OWS，也就是多个SP（空格）或者HTAB（\t水平制表符）
		if (c == ' ' || c == '\t')//处理前导OWS
		{
			return true;
		}

		//遇到第一个非此类字符，重用字符并迁移到下一状态，由下一状态判断合法性
		contextState.enParseState = StateContext::ParseState::FIELD_VAL;
		--contextState.szCurHeaderLenght;//重用字符撤销一个长度
		bReuseChar = true;

		return true;
	}

	//字段值中可能含有OWS空白，一个或多个，根据标准要求，需要保留，但是需要去掉尾部空白
	bool ParseFieldVal(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		if (++contextState.szCurHeaderLenght > contextState.szMaxHeaderLength)
		{
			contextState.SetParseError(StateContext::ParseError::HEADER_TOO_LARGE);
			return false;
		}

		int32_t i32Ret = ParseCRLF(contextState, c);
		if (i32Ret != -1)
		{
			if (i32Ret == 0)
			{
				return false;//ParseCRLF已设置错误
			}

			//第一次到这里必须要是遇到CR，因为如果第一次直接遇到LF那么ParseCRLF必须正确处理错误
			MyAssert(i32Ret == 1, "Not CR, is LF, what the fuck?");

			//在这里，检测szConsecutiveRWS，如果不为0，说明刚才遇到的一系列空白是OWS而非RWS，从字符串中删去
			if (contextState.szConsecutiveRWS != 0)
			{
				contextState.strTempBuffer2.erase(contextState.strTempBuffer2.size() - contextState.szConsecutiveRWS);
			}

			//插入map中
			auto [itCurrent, isOk] = stHeaderField.mapFields.try_emplace(
				std::move(contextState.strTempBuffer),
				std::move(contextState.strTempBuffer2)
			);

			//明明已经做过约束，怎么能在这里插入失败？
			MyAssert(isOk, "try_emplace fail, what happend?");

			//清理两个缓存
			contextState.strTempBuffer.clear();
			contextState.strTempBuffer2.clear();

			//丢弃字符并迁移到下一状态
			contextState.enParseState = StateContext::ParseState::FIELD_VAL_END;

			return true;
		}

		if (c == ' ' || c == '\t')//遇到RWS或尾随OWS
		{
			//插入值中
			contextState.strTempBuffer2.push_back(c);//注意这里是缓存2，因为缓存1已经是key，2里面才是val
			++contextState.szConsecutiveRWS;//统计遇到的空白数
			return true;
		}

		//一切正常，判断字符合法性，然后插入缓存2
		if (!IsNoSpacePrintChar(c))//注意此处隐含空白约束
		{
			contextState.SetParseError(StateContext::ParseError::INVALID_FORMAT);
			return false;
		}

		//注意这里是缓存2，因为缓存1已经是key，2里面才是val
		contextState.strTempBuffer2.push_back(c);
		contextState.szConsecutiveRWS = 0;//遇到了一个非空白，重置计数

		return true;
	}

	bool ParseFieldValEnd(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		if (++contextState.szCurHeaderLenght > contextState.szMaxHeaderLength)
		{
			contextState.SetParseError(StateContext::ParseError::HEADER_TOO_LARGE);
			return false;
		}

		if (c == ' ' || c == '\t')//处理后置OWS
		{
			return true;
		}

		//遇到其他字符
		contextState.enParseState = StateContext::ParseState::FIELD_LINE_END;//处理单行结束
		--contextState.szCurHeaderLenght;//重用字符撤销一个长度
		bReuseChar = true;//重用字符

		return true;
	}

	bool ParseFieldLineEnd(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		if (++contextState.szCurHeaderLenght > contextState.szMaxHeaderLength)
		{
			contextState.SetParseError(StateContext::ParseError::HEADER_TOO_LARGE);
			return false;
		}

		//处理换行
		int32_t i32Ret = ParseCRLF(contextState, c);
		if (i32Ret == 0)
		{
			return false;//ParseCRLF已设置错误
		}
		else if (i32Ret == 1)//遇到CR不处理
		{
			return true;
		}
		else if (i32Ret == 2)
		{
			if (++contextState.szConsecutiveCRLF > 1)//多个CRLF，Header已结束，迁移到结束状态
			{
				contextState.enParseState = StateContext::ParseState::HEADER_FIELD_END;
				contextState.szConsecutiveCRLF = 0;//清理计数器
				bReuseChar = true;//注意这里的重用字符，并不会真的被用到，仅仅是不要读取下一字符，防止只有头部的情况（当前头部已结束）
				return true;
			}

			return true;
		}
		//else//(i32Ret == -1)遇到CRLF以外

		//遇到其他字符
		contextState.enParseState = StateContext::ParseState::FIELD_KEY;//回到Key处理
		contextState.szConsecutiveCRLF = 0;//清理计数器
		--contextState.szCurHeaderLenght;//重用字符撤销一个长度
		bReuseChar = true;//重用字符


		return true;
	}

	//字段结束，解析部分需要的固定字段，但是不从map中删除，放入指定解析块中，如果还有Body的长度，则转到Body读取
	bool ParseHeaderFieldEnd(StateContext &contextState, char c, bool &bReuseChar) noexcept
	{
		if (++contextState.szCurHeaderLenght > contextState.szMaxHeaderLength)
		{
			contextState.SetParseError(StateContext::ParseError::HEADER_TOO_LARGE);
			return false;
		}

		//迁移到这里，为了防止超额读取，会重用字符，c必然是LF，否则错误
		MyAssert(c == '\n', "c not LF, what the fuck?");

		//解析所有的部分，把特定值拷贝或解析一份到特定结构体域上方便后续处理
		//注意部分值可能不存在，以及部分值必须存在，否则出错
		
		//这个选项可有可无，无所谓，哪怕未知，也不用失败
		SetConnectionType();
		
		//尝试获取body大小，注意函数失败返回false，不存在则无视
		if (!SetContentLength(contextState))
		{
			return false;//SetContentLength内部设置错误码
		}

		if (!SetHost(contextState))
		{
			return false;//SetHost内部设置错误码
		}

		//解析完成，判断ContentLength是否有长度，有则转入Body解析
		if (stHeaderField.szContentLength != 0)
		{
			//长度溢出
			if (stHeaderField.szContentLength > contextState.szMaxContentLength)
			{
				contextState.SetParseError(StateContext::ParseError::BODY_TOO_LARGE);
				return false;
			}

			//不重用字符（获取下一个）
			contextState.enParseState = StateContext::ParseState::BODY;
			return true;
		}

		contextState.enParseState = StateContext::ParseState::COMPLETE;
		return true;
	}

	void ParseBody(StateContext &contextState, const std::string &strStream, size_t &szConsumedLength) noexcept
	{
		//计算已读字节
		size_t szCompletedLength = stHeaderField.szContentLength - contextState.strTempBuffer.size();

		//计算剩余字节（取剩余大小或流大小中的较小值）
		size_t szStreamRemaining = strStream.size() - szConsumedLength;
		size_t szNeedLenght = szCompletedLength < szStreamRemaining
							? szCompletedLength
							: szStreamRemaining;

		//直接合并
		contextState.strTempBuffer.assign(strStream.data() + szConsumedLength, szNeedLenght);
		szConsumedLength += szNeedLenght;

		//读取字节数足够，切换状态
		if (stHeaderField.szContentLength == contextState.strTempBuffer.size())
		{
			contextState.enParseState = StateContext::ParseState::BODY_END;
			return;
		}

		return;
	}

	void ParseBodyEnd(StateContext &contextState) noexcept
	{
		//设置数据并清理缓存
		stMessageBody.strContent = std::move(contextState.strTempBuffer);
		contextState.strTempBuffer.clear();

		//成功后迁移到状态COMPLETE
		contextState.enParseState = StateContext::ParseState::COMPLETE;
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
			case StateContext::ParseState::COMPLETE:
			case StateContext::ParseState::ERROR:
			default:
				bRet = false;
				break;
			}
		} while (bReuseChar);

		return bRet;
	}

public:
	//--------------------------------------------------------------------------//
	
	static StateContext GetNewContext(
		size_t szMaxPathLength,
		size_t szMaxHeaderLength,
		size_t szMaxContentLength) noexcept
	{
		StateContext ctxNew{};

		ctxNew.szMaxPathLength = szMaxPathLength;
		ctxNew.szMaxHeaderLength = szMaxHeaderLength;
		ctxNew.szMaxContentLength = szMaxContentLength;

		return ctxNew;
	}

	static void ReuseContext(
		StateContext &ctxReuse,
		size_t szMaxPathLength,
		size_t szMaxHeaderLength,
		size_t szMaxContentLength) noexcept
	{
		ctxReuse.Clear();

		ctxReuse.szMaxPathLength = szMaxPathLength;
		ctxReuse.szMaxHeaderLength = szMaxHeaderLength;
		ctxReuse.szMaxContentLength = szMaxContentLength;
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

		//特殊外层状态机（内层不处理）
		switch(contextState.enParseState)
		{
		case StateContext::ParseState::BODY:
			ParseBody(contextState, strStream, szConsumedLength);
			break;
		case StateContext::ParseState::BODY_END:
			ParseBodyEnd(contextState);
			break;
		default:
			break;
		}

		//出错或完成
		//返回退出状态
		return contextState.enParseState != StateContext::ParseState::ERROR;
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





