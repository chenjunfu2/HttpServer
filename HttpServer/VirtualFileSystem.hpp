#pragma once
#include <unordered_map>
#include <filesystem>

#include "CPP_Helper.h"

struct File
{
private:
	enum class StorageMode : size_t
	{
		UNDEFINE = 0,
		DISK_ONLY,
		MEMORY_CACHED,
		ENUM_END,
	};

	class MemoryCache
	{
	private:
		void *pFileData;
		size_t szFileSize;

	private:
		void Clear(void)
		{
			if (pFileData != NULL)
			{
				free(pFileData);
				szFileSize = 0;
			}
		}

	public:
		MemoryCache(void):
			pFileData(NULL),
			szFileSize(0)
		{}
		~MemoryCache(void)
		{
			Clear();
		}

		DELETE_COPY(MemoryCache);

		MemoryCache(MemoryCache &&_Move) noexcept :
			pFileData(_Move.pFileData),
			szFileSize(_Move.szFileSize)
		{
			_Move.pFileData = NULL;
			_Move.szFileSize = 0;
		}

		MemoryCache &operator=(MemoryCache &&_Move) noexcept
		{
			Clear();

			pFileData = _Move.pFileData;
			szFileSize = _Move.szFileSize;

			_Move.pFileData = NULL;
			_Move.szFileSize = 0;

			return *this;
		}

		GETTER_COPY(FileData, pFileData);
		GETTER_COPY(FileSize, szFileSize);

		bool CacheFileToMemory(std::filesystem::path &pathFile)
		{
			//open file
			//malloc memory
			//load file
			//close file

			return true;
		}

	};

public:
	StorageMode modeStorage;
	std::filesystem::path pathFile;
	MemoryCache cacheFile;
private:



public:
	File(void) :
		modeStorage(StorageMode::UNDEFINE),
		pathFile(),
		cacheFile()
	{}

	~File(void)
	{}

	DELETE_COPY(File);

	void LoadFile(size_t szCacheSizeMax)
	{




	}









};

class VirtualFileSystem
{
private:



	std::unordered_map<std::string, File> mapFile;//映射http请求key到实际文件路径path











































};




