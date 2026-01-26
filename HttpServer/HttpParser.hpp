#pragma once

#include "CPP_Helper.h"
#include "MyAssert.hpp"

#include "HttpRequest.hpp"

#include <ctype.h>
#include <string>
#include <charconv>
#include <unordered_map>

class HttpParser
{
public:
	enum class State
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

	enum class Error
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
	struct Limits
	{
		size_t szMaxPathLength = 0;//最大路径长度
		size_t szMaxHeaderLength = 0;//最大头部长度
		size_t szMaxContentLength = 0;//最大内容长度（Body长度）
	}stLimits;
	
	struct Context
	{
		HttpRequest *pCurRequest = NULL;

		State enState = State::READY;
		Error enError = Error::NO_ERR;

		size_t szCurHeaderLenght = 0;//当前头部长度

		size_t szConsecutiveRWS = 0;//连续的RWS空白数
		size_t szConsecutiveCRLF = 0;//连续的换行
		bool bWaitLF = false;//遇到CR，等待LF

		std::string strTempBuffer{};
		std::string strTempBuffer2{};
	}stContext;

private:
	void SetParseError(Error _enParseError)
	{
		stContext.enState = State::ERROR;
		stContext.enError = _enParseError;
	}

	bool SetMethod(const std::string &strMethod) noexcept
	{
		static const std::unordered_map<std::string, HttpRequest::Method> mapMethod =
		{
			{"GET",		HttpRequest::Method::GET},
			{"HEAD",	HttpRequest::Method::HEAD},
			{"POST",	HttpRequest::Method::POST},
			{"PUT",		HttpRequest::Method::PUT},
			{"DELETE",	HttpRequest::Method::DELETE},
			{"CONNECT",	HttpRequest::Method::CONNECT},
			{"OPTIONS",	HttpRequest::Method::OPTIONS},
			{"TRACE",	HttpRequest::Method::TRACE},
			{"PATCH",	HttpRequest::Method::PATCH},
		};

		//先初始化为未知
		stContext.pCurRequest->stStartLine.enMethod = HttpRequest::Method::UNKNOWN;

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

		stContext.pCurRequest->stStartLine.enMethod = it->second;//设置实际查找值
		return true;
	}

	void SetConnectionType(void) noexcept
	{
		static const std::unordered_map<std::string, HttpRequest::ConnectionType> mapConnectionType =
		{
			{"keep-alive",	HttpRequest::ConnectionType::KEEP_ALIVE},
			{"close",		HttpRequest::ConnectionType::CLOSE},
		};


		//先设置未知
		stContext.pCurRequest->stHeaderField.enConnectionType = HttpRequest::ConnectionType::UNKNOWN;

		auto itConnection = stContext.pCurRequest->stHeaderField.mapFields.find("Connection");
		if (itConnection == stContext.pCurRequest->stHeaderField.mapFields.end())
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

		stContext.pCurRequest->stHeaderField.enConnectionType = it->second;//设置实际查找值

		return;
	}


	bool SetContentLength(void)//注意，这里是解析发生错误返回false，而非找不到返回false
	{
		//先设置长度为0
		stContext.pCurRequest->stHeaderField.szContentLength = 0;

		auto itContentLength = stContext.pCurRequest->stHeaderField.mapFields.find("Content-Length");
		if (itContentLength == stContext.pCurRequest->stHeaderField.mapFields.end())
		{
			return true;//找不到直接true，值为0
		}

		auto &strContentLength = itContentLength->second;

		//解析为数值（严格解析）
		const char *pBeg = strContentLength.data();
		const char *pEnd = strContentLength.data() + strContentLength.size();

		auto [pUse, errCode] = std::from_chars(pBeg, pEnd, stContext.pCurRequest->stHeaderField.szContentLength);
		if (errCode != std::errc() || pUse != pEnd)
		{
			SetParseError(Error::INVALID_FORMAT);
			return false;
		}

		return true;
	}

	bool SetHost(void)//标准要求Host必须要存在，否则失败
	{
		//先设置为NULL
		stContext.pCurRequest->stHeaderField.pstrHost = NULL;

		auto itHost = stContext.pCurRequest->stHeaderField.mapFields.find("Host");
		if (itHost == stContext.pCurRequest->stHeaderField.mapFields.end())
		{
			SetParseError(Error::INCOMPLETE_REQUEST);//必须字段缺少，请求不完整
			return false;
		}

		stContext.pCurRequest->stHeaderField.pstrHost = &itHost->second;

		return true;
	}

private:
	//--------------------------------------------------------------------------//

	//-1->其它字符 0->失败 1->CR 2->LF
	int32_t ParseCRLF(char c) noexcept
	{
		if (c == '\r')//保证\r\n成对出现
		{
			if (stContext.bWaitLF == false)
			{
				stContext.bWaitLF = true;
				return 1;
			}
			else
			{
				SetParseError(Error::UNEXPECTED_CHAR);
				return 0;
			}
		}
		else if (c == '\n')
		{
			if (stContext.bWaitLF == true)
			{
				stContext.bWaitLF = false;
				return 2;
			}
			else
			{
				SetParseError(Error::UNEXPECTED_CHAR);
				return 0;
			}
		}
		else//其它字符
		{
			if (stContext.bWaitLF == true)//什么？CR之后不是LF？
			{
				stContext.bWaitLF = false;//清除标志并设置错误

				SetParseError(Error::UNEXPECTED_CHAR);
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
	bool ParseReady(char c, bool &bReuseChar) noexcept
	{
		int32_t i32Ret = ParseCRLF(c);
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
			if (++stContext.szConsecutiveCRLF > 1)//多个CRLF？
			{
				SetParseError(Error::INVALID_FORMAT);
				return false;
			}

			return true;
		}
		//else //(i32Ret == -1)其他字符，走下面处理

		//遇到第一个非空白，迁移状态
		stContext.enState = State::METHOD;
		stContext.szConsecutiveCRLF = 0;//清理CRLF计数器
		bReuseChar = true;//重用字符

		return true;
	}

	//请求行以方法标识符起始，后接单个空格(SP)、请求目标、再一个单个空格(SP)、协议版本，最终以CRLF结尾。
	/*
		A request-line begins with a method token, followed by a single space (SP),
		the request-target, another single space (SP), the protocol version, and ends
		with CRLF.
	*/

	bool ParseMethod(char c, bool &bReuseChar) noexcept
	{
		if (c == ' ')//遇到空白
		{
			//尝试解析并设置方法
			if (!SetMethod(stContext.strTempBuffer))
			{
				SetParseError(Error::INVALID_FORMAT);
				return false;
			}

			//成功后清理缓冲区
			stContext.strTempBuffer.clear();

			//转换到下一状态（注意此空白字符被丢弃而非重用，不赋值bReuseChar=true）
			stContext.enState = State::PATH;

			return true;
		}

		if (!isalpha(c))//非法字符（方法只能是字母）
		{
			SetParseError(Error::INVALID_FORMAT);
			return false;
		}

		if (stContext.strTempBuffer.size() >= stContext.pCurRequest->MAX_METHOD_LENGTH)//过长请求方法
		{
			SetParseError(Error::INVALID_FORMAT);
			return false;
		}

		//临时缓存+1
		stContext.strTempBuffer.push_back(c);

		return true;
	}


	bool ParsePath(char c, bool &bReuseChar) noexcept
	{
		if (c == ' ')//遇到空白
		{
			//设置路径字符串
			stContext.pCurRequest->stStartLine.strPath = std::move(stContext.strTempBuffer);
			stContext.strTempBuffer.clear();
			//丢弃空白，然后迁移到http版本状态
			stContext.enState = State::VERSION;

			return true;
		}

		//字符合法性
		if (!IsNoSpacePrintChar(c))
		{
			SetParseError(Error::INVALID_FORMAT);
			return false;
		}

		if (stContext.strTempBuffer.size() >= stLimits.szMaxPathLength)
		{
			SetParseError(Error::STARTLINE_TOO_LARGE);
			return false;
		}

		//临时缓存+1
		stContext.strTempBuffer.push_back(c);

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
	bool ParseVersion(char c, bool &bReuseChar) noexcept
	{
		int32_t i32Ret = ParseCRLF(c);
		if (i32Ret != -1)
		{
			if (i32Ret == 0)
			{
				return false;//错误已在ParseCRLF内设置
			}

			//第一次到这里必须要是遇到CR，因为如果第一次直接遇到LF那么ParseCRLF必须正确处理错误
			MyAssert(i32Ret == 1, "Not CR, is LF, what the fuck?");

			//必须相等
			if (stContext.strTempBuffer.size() != stContext.pCurRequest->VERSION_LENGTH)
			{
				SetParseError(Error::INVALID_FORMAT);
				return false;
			}

			//版本必须以"HTTP/"开头，否则错误
			if (!stContext.strTempBuffer.starts_with("HTTP/"))
			{
				SetParseError(Error::INVALID_FORMAT);
				return false;
			}

			//解析出"数字"."数字"形式的主次版本号

			//格式强制判断
			if (!isdigit(stContext.strTempBuffer[5]) ||
				stContext.strTempBuffer[6] != '.' ||
				!isdigit(stContext.strTempBuffer[7]))
			{
				SetParseError(Error::INVALID_FORMAT);
				return false;
			}

			//获取主次版本
			stContext.pCurRequest->stStartLine.u8MajorVersion = stContext.strTempBuffer[5] - '0';
			stContext.pCurRequest->stStartLine.u8MinorVersion = stContext.strTempBuffer[7] - '0';

			//清理，丢弃空白并迁移
			stContext.strTempBuffer.clear();
			stContext.enState = State::START_LINE_END;

			return true;
		}

		//字符合法性
		if (!IsNoSpacePrintChar(c))//隐含空白约束
		{
			SetParseError(Error::INVALID_FORMAT);
			return false;
		}

		//过长版本号
		if (stContext.strTempBuffer.size() >= stContext.pCurRequest->VERSION_LENGTH)
		{
			SetParseError(Error::INVALID_FORMAT);
			return false;
		}

		//临时缓存+1
		stContext.strTempBuffer.push_back(c);

		return true;
	}

	bool ParseStartLineEnd(char c, bool &bReuseChar) noexcept
	{
		int32_t i32Ret = ParseCRLF(c);
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
			if (++stContext.szConsecutiveCRLF > 1)//多个CRLF？
			{
				SetParseError(Error::INVALID_FORMAT);
				return false;
			}

			return true;
		}
		//else//(i32Ret == -1)遇到CRLF以外

		//迁移到Key状态
		stContext.enState = State::FIELD_KEY;
		stContext.szConsecutiveCRLF = 0;//清理CRLF计数器
		bReuseChar = true;//重用字符

		return true;
	}

	bool ParseFieldKey(char c, bool &bReuseChar) noexcept
	{
		if (++stContext.szCurHeaderLenght > stLimits.szMaxHeaderLength)
		{
			SetParseError(Error::HEADER_TOO_LARGE);
			return false;
		}

		if (c == ':')
		{
			//已存在字段重复，请求有误
			if (stContext.pCurRequest->stHeaderField.mapFields.contains(stContext.strTempBuffer))
			{
				SetParseError(Error::DUPLICATE_FIELDS);
				return false;
			}

			//字段无误，等待值
			//直接迁移到下一状态，不清理buffer，不重用字符
			stContext.enState = State::FIELD_KEY_END;

			return true;
		}

		//字符合法性
		if (!IsNoSpacePrintChar(c))//注意此处隐含空白约束
		{
			SetParseError(Error::INVALID_FORMAT);
			return false;
		}

		//临时缓存+1
		stContext.strTempBuffer.push_back(c);

		return true;
	}

	bool ParseFieldKeyEnd(char c, bool &bReuseChar) noexcept
	{
		if (++stContext.szCurHeaderLenght > stLimits.szMaxHeaderLength)
		{
			SetParseError(Error::HEADER_TOO_LARGE);
			return false;
		}

		//根据标准描述，':'后面可以是OWS，也就是多个SP（空格）或者HTAB（\t水平制表符）
		if (c == ' ' || c == '\t')//处理前导OWS
		{
			return true;
		}

		//遇到第一个非此类字符，重用字符并迁移到下一状态，由下一状态判断合法性
		stContext.enState = State::FIELD_VAL;
		--stContext.szCurHeaderLenght;//重用字符撤销一个长度
		bReuseChar = true;

		return true;
	}

	//字段值中可能含有OWS空白，一个或多个，根据标准要求，需要保留，但是需要去掉尾部空白
	bool ParseFieldVal(char c, bool &bReuseChar)
	{
		if (++stContext.szCurHeaderLenght > stLimits.szMaxHeaderLength)
		{
			SetParseError(Error::HEADER_TOO_LARGE);
			return false;
		}

		int32_t i32Ret = ParseCRLF(c);
		if (i32Ret != -1)
		{
			if (i32Ret == 0)
			{
				return false;//ParseCRLF已设置错误
			}

			//第一次到这里必须要是遇到CR，因为如果第一次直接遇到LF那么ParseCRLF必须正确处理错误
			MyAssert(i32Ret == 1, "Not CR, is LF, what the fuck?");

			//在这里，检测stContext.szConsecutiveRWS，如果不为0，说明刚才遇到的一系列空白是OWS而非RWS，从字符串中删去
			if (stContext.szConsecutiveRWS != 0)
			{
				stContext.strTempBuffer2.erase(stContext.strTempBuffer2.size() - stContext.szConsecutiveRWS);
			}

			//插入map中
			auto [itCurrent, isOk] = 
				stContext.pCurRequest->stHeaderField.mapFields.try_emplace(
					std::move(stContext.strTempBuffer),
					std::move(stContext.strTempBuffer2)
				);

			//明明已经做过约束，怎么能在这里插入失败？
			MyAssert(isOk, "try_emplace fail, what happend?");

			//清理两个缓存
			stContext.strTempBuffer.clear();
			stContext.strTempBuffer2.clear();

			//丢弃字符并迁移到下一状态
			stContext.enState = State::FIELD_VAL_END;

			return true;
		}

		if (c == ' ' || c == '\t')//遇到RWS或尾随OWS
		{
			//插入值中
			stContext.strTempBuffer2.push_back(c);//注意这里是缓存2，因为缓存1已经是key，2里面才是val
			++stContext.szConsecutiveRWS;//统计遇到的空白数
			return true;
		}

		//一切正常，判断字符合法性，然后插入缓存2
		if (!IsNoSpacePrintChar(c))//注意此处隐含空白约束
		{
			SetParseError(Error::INVALID_FORMAT);
			return false;
		}

		//注意这里是缓存2，因为缓存1已经是key，2里面才是val
		stContext.strTempBuffer2.push_back(c);
		stContext.szConsecutiveRWS = 0;//遇到了一个非空白，重置计数

		return true;
	}

	bool ParseFieldValEnd(char c, bool &bReuseChar) noexcept
	{
		if (++stContext.szCurHeaderLenght > stLimits.szMaxHeaderLength)
		{
			SetParseError(Error::HEADER_TOO_LARGE);
			return false;
		}

		if (c == ' ' || c == '\t')//处理后置OWS
		{
			return true;
		}

		//遇到其他字符
		stContext.enState = State::FIELD_LINE_END;//处理单行结束
		--stContext.szCurHeaderLenght;//重用字符撤销一个长度
		bReuseChar = true;//重用字符

		return true;
	}

	bool ParseFieldLineEnd(char c, bool &bReuseChar) noexcept
	{
		if (++stContext.szCurHeaderLenght > stLimits.szMaxHeaderLength)
		{
			SetParseError(Error::HEADER_TOO_LARGE);
			return false;
		}

		//处理换行
		int32_t i32Ret = ParseCRLF(c);
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
			if (++stContext.szConsecutiveCRLF > 1)//多个CRLF，Header已结束，迁移到结束状态
			{
				stContext.enState = State::HEADER_FIELD_END;
				stContext.szConsecutiveCRLF = 0;//清理计数器
				bReuseChar = true;//注意这里的重用字符，并不会真的被用到，仅仅是不要读取下一字符，防止只有头部的情况（当前头部已结束）
				return true;
			}

			return true;
		}
		//else//(i32Ret == -1)遇到CRLF以外

		//遇到其他字符
		stContext.enState = State::FIELD_KEY;//回到Key处理
		stContext.szConsecutiveCRLF = 0;//清理计数器
		--stContext.szCurHeaderLenght;//重用字符撤销一个长度
		bReuseChar = true;//重用字符

		return true;
	}

	//字段结束，解析部分需要的固定字段，但是不从map中删除，放入指定解析块中，如果还有Body的长度，则转到Body读取
	bool ParseHeaderFieldEnd(char c, bool &bReuseChar) noexcept
	{
		if (++stContext.szCurHeaderLenght > stLimits.szMaxHeaderLength)
		{
			SetParseError(Error::HEADER_TOO_LARGE);
			return false;
		}

		//迁移到这里，为了防止超额读取，会重用字符，c必然是LF，否则错误
		MyAssert(c == '\n', "c not LF, what the fuck?");

		//解析所有的部分，把特定值拷贝或解析一份到特定结构体域上方便后续处理
		//注意部分值可能不存在，以及部分值必须存在，否则出错

		//这个选项可有可无，无所谓，哪怕未知，也不用失败
		SetConnectionType();

		//尝试获取body大小，注意函数失败返回false，不存在则无视
		if (!SetContentLength())
		{
			return false;//SetContentLength内部设置错误码
		}

		if (!SetHost())
		{
			return false;//SetHost内部设置错误码
		}

		//解析完成，判断ContentLength是否有长度，有则转入Body解析
		if (stContext.pCurRequest->stHeaderField.szContentLength != 0)
		{
			//长度溢出
			if (stContext.pCurRequest->stHeaderField.szContentLength > stLimits.szMaxContentLength)
			{
				SetParseError(Error::BODY_TOO_LARGE);
				return false;
			}

			//不重用字符（获取下一个）
			stContext.enState = State::BODY;
			return true;
		}

		stContext.enState = State::COMPLETE;
		return true;
	}

	void ParseBody(const char *pStream, size_t szStreamLength, size_t &szConsumedLength) noexcept
	{
		//计算已读字节
		size_t szCompletedLength = stContext.pCurRequest->stHeaderField.szContentLength - stContext.strTempBuffer.size();

		//计算剩余字节（取剩余大小或流大小中的较小值）
		size_t szStreamRemaining = szStreamLength - szConsumedLength;
		size_t szNeedLenght = szCompletedLength < szStreamRemaining
			? szCompletedLength
			: szStreamRemaining;

		//直接合并
		stContext.strTempBuffer.assign(pStream + szConsumedLength, szNeedLenght);
		szConsumedLength += szNeedLenght;

		//读取字节数足够，切换状态
		if (stContext.pCurRequest->stHeaderField.szContentLength == stContext.strTempBuffer.size())
		{
			stContext.enState = State::BODY_END;
			return;
		}

		return;
	}

	void ParseBodyEnd(void) noexcept
	{
		//设置数据并清理缓存
		stContext.pCurRequest->stMessageBody.strContent = std::move(stContext.strTempBuffer);
		stContext.strTempBuffer.clear();

		//成功后迁移到状态COMPLETE
		stContext.enState = State::COMPLETE;
	}

	//--------------------------------------------------------------------------//

	bool ParseChar(char c) noexcept
	{
		bool bReuseChar{};
		bool bRet = false;
		do
		{
			bReuseChar = false;//每次重置

			switch (stContext.enState)
			{
			case State::READY:
				bRet = ParseReady(c, bReuseChar);
				break;
			case State::METHOD:
				bRet = ParseMethod(c, bReuseChar);
				break;
			case State::PATH:
				bRet = ParsePath(c, bReuseChar);
				break;
			case State::VERSION:
				bRet = ParseVersion(c, bReuseChar);
				break;
			case State::START_LINE_END:
				bRet = ParseStartLineEnd(c, bReuseChar);
				break;
			case State::FIELD_KEY:
				bRet = ParseFieldKey(c, bReuseChar);
				break;
			case State::FIELD_KEY_END:
				bRet = ParseFieldKeyEnd(c, bReuseChar);
				break;
			case State::FIELD_VAL:
				bRet = ParseFieldVal(c, bReuseChar);
				break;
			case State::FIELD_VAL_END:
				bRet = ParseFieldValEnd(c, bReuseChar);
				break;
			case State::FIELD_LINE_END:
				bRet = ParseFieldLineEnd(c, bReuseChar);
				break;
			case State::HEADER_FIELD_END:
				bRet = ParseHeaderFieldEnd(c, bReuseChar);
				break;
			case State::BODY:
			case State::COMPLETE:
			case State::ERROR:
			default:
				bRet = false;
				break;
			}
		} while (bReuseChar);

		return bRet;
	}

public:
	//--------------------------------------------------------------------------//

	void Clear(void) noexcept
	{
		{
			stContext.pCurRequest = NULL;

			stContext.enState = State::READY;
			stContext.enError = Error::NO_ERR;

			stContext.szCurHeaderLenght = 0;

			stContext.szConsecutiveRWS = 0;
			stContext.szConsecutiveCRLF = 0;

			stContext.bWaitLF = false;

			stContext.strTempBuffer.clear();
			stContext.strTempBuffer2.clear();
		}
		
		{
			stLimits.szMaxPathLength = 0;
			stLimits.szMaxHeaderLength = 0;
			stLimits.szMaxContentLength = 0;
		}
	}

	//--------------------------------------------------------------------------//

	void SetLimits(
		size_t _szMaxPathLength,
		size_t _szMaxHeaderLength,
		size_t _szMaxContentLength) noexcept
	{
		
		stLimits.szMaxPathLength = _szMaxPathLength;
		stLimits.szMaxHeaderLength = _szMaxHeaderLength;
		stLimits.szMaxContentLength = _szMaxContentLength;
	}

	void BindRequest(HttpRequest *_pCurRequest) noexcept
	{
		stContext.pCurRequest = _pCurRequest;
	}

	HttpRequest *GetRequest(void) const noexcept
	{
		return stContext.pCurRequest;
	}

	void ResetContext(void) noexcept
	{
		stContext.pCurRequest = NULL;

		stContext.enState = State::READY;
		stContext.enError = Error::NO_ERR;

		stContext.strTempBuffer.clear();
		stContext.strTempBuffer2.clear();

		stContext.szCurHeaderLenght = 0;

		stContext.szConsecutiveRWS = 0;
		stContext.szConsecutiveCRLF = 0;

		stContext.bWaitLF = false;
	}

	//--------------------------------------------------------------------------//

	DEFAULT_CSTC(HttpParser);
	DEFAULT_DSTC(HttpParser);
	DEFAULT_MOVE(HttpParser);
	DELETE_COPY(HttpParser);

	GETTER_COPY(State, stContext.enState);
	GETTER_COPY(Error, stContext.enError);

	GETTER_COPY(MaxPathLength, stLimits.szMaxPathLength);
	GETTER_COPY(MaxHeaderLength, stLimits.szMaxHeaderLength);
	GETTER_COPY(MaxContentLength, stLimits.szMaxContentLength);

	//--------------------------------------------------------------------------//

	bool ParsingStream(const char *pStream, size_t szStreamLength, size_t &szConsumedLength)
	{
		if (stContext.pCurRequest == NULL || szStreamLength == 0)
		{
			return false;
		}
		
		const char *p = pStream + szConsumedLength;//从已消耗长度开始
		const char *end = pStream + szStreamLength;//直到当前数据长度结束
		while (p < end)
		{
			if (!ParseChar(*p))
			{
				break;
			}

			++p;
		}

		//更新消耗的长度
		szConsumedLength += end - p;

		//特殊外层状态机（内层不处理）
		switch (stContext.enState)
		{
		case State::BODY:
			ParseBody(pStream, szStreamLength, szConsumedLength);
			break;
		case State::BODY_END:
			ParseBodyEnd();
			break;
		default:
			break;
		}

		//出错或完成
		//返回退出状态
		return stContext.enState != State::ERROR;
	}

	bool IsComplete(void) const noexcept
	{
		return stContext.enState == State::COMPLETE;
	}

	//--------------------------------------------------------------------------//
};

