#pragma once

#include "CPP_Helper.h"
#include "Windows_ErrorMessage.h"

#include <stdint.h>
#include <stddef.h>

ErrorMessage OpenFile(const char *pcFileName);

ErrorMessage MappingFile(const char *pcFileName, void *&pFile, uint64_t &u64FileSize);
ErrorMessage UnMapFile(void *&pFileClose);


class FileSystem
{
public:
	using FILE_T = void *;

protected:
	FILE_T GetUnInitFile(void);

private:
	FILE_T fileData;

public:








};

