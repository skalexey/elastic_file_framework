#pragma once
#include <efile_types.h>
#include <TEFileSectorsTable.h>
#include <TEFileCursor.h>

class ElasticFile
{
public:
	friend class TEFileCursor;
	friend class TEFileSectorsTable;

	ElasticFile();
	~ElasticFile();

	size_file_t Write(const PBYTE buffer, const size_file_t& size, bool overwrite);
	size_file_t Read(PBYTE buffer, const size_file_t& size);
	size_file_t Truncate(const size_file_t& cut_size);
	TEFileHandle Open(const std::string& file_name, const TEFileOpenMode& mode);
	void Close();
	void SetPosition(const size_file_t& offset, const TEFileCursorMoveMode& mode);
	const size_file_t& GetPosition();
	TEFileSectorsList::iterator Extend(const size_file_t& size_to_extend);

protected:
	TEFileSectorsTable& GetSectorsTable();
	const TEFileHandle& GetHandle();
	TEFileCursor& GetCursor();
	const TEFileOpenMode& GetMode();
	void SetHandle(TEFileHandle file);
	bool Modified();
	void SetModified();
	int close();
	size_file_t WriteSector(TEFileSectorsList::iterator sector_it, PBYTE buffer, const size_file_t& from, const size_file_t& sizeToWrite);
	size_file_t WriteOverwrite(const PBYTE buffer, const size_file_t& size);
	size_file_t WriteInsert(const PBYTE buffer, const size_file_t& size);
	TEFileHandle OpenLow(const std::string& file_name, const std::string& lowMode);
	void Init(const TEFileHandle& file_handle, const TEFileOpenMode& mode);
	TEFileHandle InitLow(const std::string& file_name, const TEFileOpenMode& mode);
	void CheckHandle();
	void CheckHandle(const TEFileHandle& file_handle);
	void CheckSize(const size_file_t& size);
	void CheckFileExists(const std::string& file_name);

private:
	TEFileHandle m_handle;
	TEFileCursor m_cursor;
	TEFileSectorsTable m_sectors_table;
	bool m_modified;
	TEFileOpenMode m_mode;
};

typedef std::shared_ptr<ElasticFile> ElasticFilePtr;