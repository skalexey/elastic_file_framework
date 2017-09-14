#pragma once
#include <map>
#include <ElasticFile.h>

class EFileController
{
public:
	EFileController(void);
	~EFileController(void);

	ElasticFile& GetFile(const TEFileHandle& file_handle);
	TEFileHandle OpenFile(const std::string& file_name, const TEFileOpenMode& mode);
	void CloseFile(const TEFileHandle& file);
	
	static EFileController& Get();
	
protected:
	std::map<TEFileHandle, ElasticFilePtr>::iterator GetFileEntrance(const TEFileHandle& file_handle);
	void CheckHandle(const TEFileHandle& file_handle);

private:
	std::map<TEFileHandle, ElasticFilePtr> m_files;
};


