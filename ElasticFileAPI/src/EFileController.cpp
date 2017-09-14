#include <EFileController.h>
#include <TEFileException.h>

EFileController::EFileController(void)
{
}

EFileController::~EFileController(void)
{
}

EFileController& EFileController::Get()
{
	static EFileController controller;
	return controller;
}

ElasticFile& EFileController::GetFile(const TEFileHandle& file_handle)
{
	return *GetFileEntrance(file_handle)->second;
}

std::map<TEFileHandle, ElasticFilePtr>::iterator EFileController::GetFileEntrance(const TEFileHandle& file_handle)
{
	CheckHandle(file_handle);
	std::map<TEFileHandle, ElasticFilePtr>::iterator file_entrance = m_files.find(file_handle);
	if(file_entrance == m_files.end())
	{
		throw(TEFileException(EF_HANDLE_NOT_FOUND, "Handle is useless"));
	}
	return file_entrance;
}

TEFileHandle EFileController::OpenFile(const std::string& file_name, const TEFileOpenMode& mode)
{
	ElasticFilePtr new_file(new ElasticFile());
	TEFileHandle file_handle = new_file->Open(file_name, mode);
	m_files[file_handle] = new_file;
	return file_handle;
}

void EFileController::CloseFile(const TEFileHandle& file)
{
	m_files.erase(GetFileEntrance(file));
}

void EFileController::CheckHandle(const TEFileHandle& file_handle)
{
	if(file_handle == NULL)
	{
		throw(TEFileException(EF_NULL_HANDLE, "Handle is NULL"));
	}
}

