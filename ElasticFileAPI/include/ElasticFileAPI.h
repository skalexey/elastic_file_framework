#pragma once
#include <ElasticFile.h>

class ElasticFileAPI
{
public:
	ElasticFileAPI(void);
	~ElasticFileAPI(void);

	static TEFileHandle FileOpen(const std::string& file_name, const TEFileOpenMode& open_mode);
	static bool FileSetCursor(const TEFileHandle& file, const size_file_t& offset, const TEFileCursorMoveMode& mode);
	static const size_file_t& FileGetCursor(const TEFileHandle& file);
	static size_file_t FileRead(const TEFileHandle& file, PBYTE buffer, const size_file_t& size);
	static size_file_t FileWrite(const TEFileHandle& file, const PBYTE buffer, const size_file_t& size, bool overwrite = false);
	static bool FileTruncate(const TEFileHandle& file, const size_file_t& cut_size);
	static bool FileClose(const TEFileHandle& file);
};



