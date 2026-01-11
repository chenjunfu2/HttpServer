#include "Windows_SystemError.h"

#include <Windows.h>

uint32_t SystemError::GetNoErrorCode(void) noexcept
{
	return (uint32_t)NO_ERROR;
}