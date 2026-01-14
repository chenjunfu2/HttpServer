#pragma once
#include <unordered_map>
#include <filesystem>
#include <variant>

#include "CPP_Helper.h"
#include "Windows_FileSystem.h"

struct VirtualFile
{
private:
	enum class StorageMode : size_t
	{
		UNDEFINE = 0,
		DISK_ONLY,
		MEMORY_CACHED,
		MEMORY_MAPPED,
		NO_FOUND,
		ACCESS_DENIED,
		UNKNOWN_ERROR,
		ENUM_END,
	};

	class MemoryCache
	{
	private:
		const void *pFileData;
		int64_t i64FileSize;

	public:
		MemoryCache(void):
			pFileData(NULL),
			i64FileSize(0)
		{}
		~MemoryCache(void)
		{
			Clear();
		}

		DELETE_COPY(MemoryCache);

		MemoryCache(MemoryCache &&_Move) noexcept :
			pFileData(_Move.pFileData),
			i64FileSize(_Move.i64FileSize)
		{
			_Move.pFileData = NULL;
			_Move.i64FileSize = 0;
		}

		MemoryCache &operator=(MemoryCache &&_Move) noexcept
		{
			Clear();

			pFileData = _Move.pFileData;
			i64FileSize = _Move.i64FileSize;

			_Move.pFileData = NULL;
			_Move.i64FileSize = 0;

			return *this;
		}

		void Clear(void)
		{
			if (pFileData != NULL)
			{
				free((void *)pFileData);
				i64FileSize = 0;
			}
		}

		GETTER_COPY(FileData, pFileData);
		GETTER_COPY(FileSize, i64FileSize);

		SystemError CacheFileToMemory(File &f)
		{
			if (!f.GetSize(i64FileSize))
			{
				return f.GetFileError();
			}

			if (i64FileSize > UINT32_MAX)
			{
				i64FileSize = 0;
				return SystemError(SystemError::ErrorCode::MORE_DATA);
			}
			
			pFileData = malloc(i64FileSize);
			if (pFileData == NULL)
			{
				i64FileSize = 0;
				return SystemError(SystemError::ErrorCode::OUTOFMEMORY);
			}

			uint32_t u32ReadSize = (uint32_t)i64FileSize;
			if (!f.Read((void *)pFileData, u32ReadSize))
			{
				i64FileSize = 0;
				free((void *)pFileData), pFileData = NULL;
				
				return f.GetFileError();
			}

			return {};
		}

	};

private:
	StorageMode enStorageMode;
	std::filesystem::path pathFile;
	std::string strMimeType;
	std::variant<std::monostate, MemoryCache, File::MemoryView> cacheFile;

protected:
	//static std::string GetMimeType(std::filesystem::path &p)
	//{
	//	auto u8Str = p.extension().u8string();
	//	
	//
	//
	//
	//}

public:
	VirtualFile(void) :
		enStorageMode(StorageMode::UNDEFINE),
		pathFile(),
		strMimeType(),
		cacheFile()
	{}

	VirtualFile(std::filesystem::path _pathFile, std::string _strMimeType) :
		enStorageMode(StorageMode::DISK_ONLY),
		pathFile(std::move(_pathFile)),
		strMimeType(std::move(_strMimeType)),
		cacheFile()
	{}

	~VirtualFile(void)
	{}

	DELETE_COPY(VirtualFile);

	GETTER_COPY(StorageMode, enStorageMode);

	GETTER_CREF(FilePath, pathFile);

	const void *GetData(void) const noexcept
	{
		switch (enStorageMode)
		{
		case VirtualFile::StorageMode::MEMORY_CACHED:
			return std::get<MemoryCache>(cacheFile).GetFileData();
			break;
		case VirtualFile::StorageMode::MEMORY_MAPPED:
			return std::get<File::MemoryView>(cacheFile).GetViewData();
			break;
		default:
			return NULL;
			break;
		}

	}
	int64_t GetSize(void) const noexcept
	{
		switch (enStorageMode)
		{
		case VirtualFile::StorageMode::MEMORY_CACHED:
			return std::get<MemoryCache>(cacheFile).GetFileSize();
			break;
		case VirtualFile::StorageMode::MEMORY_MAPPED:
			return std::get<File::MemoryView>(cacheFile).GetViewSize();
			break;
		default:
			return 0;
			break;
		}
	}

	//void SetFilePath(std::filesystem::path &pathNew)
	//{
	//	if (enStorageMode == StorageMode::MEMORY_CACHED ||
	//		enStorageMode == StorageMode::MEMORY_MAPPED)
	//	{
	//		cacheFile.emplace<std::monostate>();//析构原先对象
	//	}
	//
	//	pathFile = pathNew;
	//	strMimeType = GetMimeType(pathFile);
	//	enStorageMode = StorageMode::DISK_ONLY;
	//}

	SystemError Flush(int64_t i64MemCacheMaxSize)
	{
		File f;
		if (!f.Open(pathFile.c_str(), File::AccessMode::READ, File::ShareMode::READ, File::CreationMode::OPEN_EXISTING))
		{
			switch (f.GetFileError().GetFileErrorCode())
			{
			case FileError::ErrorCode::ACCESS_DENIED:
				enStorageMode = StorageMode::ACCESS_DENIED;
				break;
			case FileError::ErrorCode::NO_FOUND:
				enStorageMode = StorageMode::NO_FOUND;
				break;
			default:
				enStorageMode = StorageMode::UNKNOWN_ERROR;
				return f.GetFileError();
				break;
			}
		}

		int64_t i64FileSize{};
		if (!f.GetSize(i64FileSize))
		{
			enStorageMode = StorageMode::UNKNOWN_ERROR;
			return f.GetFileError();
		}

		if (i64FileSize > i64MemCacheMaxSize)//to mem view
		{
			if (!f.MappingToMemoryView(cacheFile.emplace<File::MemoryView>()))
			{
				enStorageMode = StorageMode::UNKNOWN_ERROR;
				return f.GetFileError();
			}
		}
		else//to mem cache
		{
			auto &refMemoryCache = cacheFile.emplace<MemoryCache>();
			return refMemoryCache.CacheFileToMemory(f);
		}

		return {};
	}

};

class VirtualFileSystem
{
private:
	std::unordered_map<std::string, VirtualFile> mapFile;//映射http请求key到实际文件路径path

public:
	DELETE_COPY(VirtualFileSystem);
	DEFAULT_CSTC(VirtualFileSystem);
	DEFAULT_DSTC(VirtualFileSystem);

	VirtualFileSystem(VirtualFileSystem &&_Move) :
		mapFile(std::move(_Move.mapFile))
	{}

	VirtualFileSystem &operator=(VirtualFileSystem &&_Move)
	{
		mapFile = std::move(_Move.mapFile);
	}

	void Register(std::string strHttpPath, std::filesystem::path pathFileSystem, std::string strMimeType)
	{
		mapFile.insert_or_assign(std::move(strHttpPath), VirtualFile{ std::move(pathFileSystem), std::move(strMimeType) });
	}

	void UnRegister(const std::string &strHttpPath)
	{
		mapFile.erase(strHttpPath);
	}

	void Load(int64_t i64MemCacheMaxSize)
	{
		for (auto &[key, val] : mapFile)
		{
			val.Flush(i64MemCacheMaxSize);
		}
	}

	VirtualFile *GetFile(const std::string &strHttpPath)
	{
		auto it = mapFile.find(strHttpPath);
		if (it == mapFile.end())
		{
			return NULL;
		}

		return &(it->second);
	}

};

