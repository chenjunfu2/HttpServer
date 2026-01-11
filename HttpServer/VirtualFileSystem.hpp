#pragma once
#include <unordered_map>
#include <filesystem>

#include "CPP_Helper.h"

struct ReadOnlyFile
{
public:
	struct FileData
	{
		const void *pData;
		const size_t szSize;
	};

private:
	enum class StorageMode : size_t
	{
		UNDEFINE = 0,
		DISK_ONLY,
		MEMORY_CACHED,
		MEMORY_MAPPED,
		NO_FOUND,
		ACCESS_DENIED,
		ENUM_END,
	};

	class MemoryCache
	{
	private:
		void *pFileData;
		size_t szFileSize;

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

		void Clear(void)
		{
			if (pFileData != NULL)
			{
				free(pFileData);
				szFileSize = 0;
			}
		}

		GETTER_COPY(FileData, pFileData);
		GETTER_COPY(FileSize, szFileSize);

		bool CacheFileToMemory(std::filesystem::path &pathFile)
		{
			//open file
			//malloc memory
			//read file to memory
			//close file

			return true;
		}

	};

	class MemoryMapping
	{

	};


private:
	StorageMode modeStorage;
	std::filesystem::path pathFile;
	MemoryCache cacheFile;

public:
	ReadOnlyFile(void) :
		modeStorage(StorageMode::UNDEFINE),
		pathFile(),
		cacheFile()
	{}

	~ReadOnlyFile(void)
	{}

	DELETE_COPY(ReadOnlyFile);

	GETTER_COPY(StorageMode, modeStorage);

	GETTER_CREF(FilePath, pathFile);

	void SetFilePath(std::filesystem::path &pathNew)
	{
		if (modeStorage == StorageMode::MEMORY_CACHED)
		{
			cacheFile.Clear();
		}

		pathFile = pathNew;
		modeStorage = StorageMode::DISK_ONLY;
	}



	FileData GetFileData(void)
	{




	}

	void FlushFileCache(void)
	{

	}






};

class VirtualFileSystem
{
private:
	std::unordered_map<std::string, File> mapFile;//映射http请求key到实际文件路径path




};




