#include <ElasticFileAPI.h>
#include <TEFileException.h>
#include <EFileController.h>

#ifdef EF_EXCEPTIONS_ENABLED
#define THROW_EXCEPTION(ex) throw ex
#else
#define THROW_EXCEPTION(ex)
#endif

ElasticFileAPI::ElasticFileAPI(void)
{

}

ElasticFileAPI::~ElasticFileAPI(void)
{

}

#define UNKNOWN_EXCEPTION "unknown exception"
void ProcessException(TEFileException& ex)
{
	ERRLOG( EXCEPTION_PREFIX << ex.what() );
	THROW_EXCEPTION(ex);
}

void ProcessException(std::exception& ex)
{
	ERRLOG( "std::exception: " << ex.what() );
	THROW_EXCEPTION(ex);
}

void ProcessException(char* msg)
{
	ERRLOG( "exception: " << msg );
}


TEFileHandle ElasticFileAPI::FileOpen(const std::string& fileName, const TEFileOpenMode& openMode)
{
	try
	{
		return EFileController::Get().OpenFile(fileName, openMode);
	}
	catch(TEFileException& ex)
	{
		ProcessException(ex);
		return NULL;
	}
	catch(std::exception& ex)
	{
		ProcessException(ex);
		return NULL;
	}
	catch(...)
	{
		ProcessException(UNKNOWN_EXCEPTION);
		return NULL;
	}
}

bool ElasticFileAPI::FileSetCursor(const TEFileHandle& file, const size_file_t& offset, const TEFileCursorMoveMode& mode)
{
	try
	{
		EFileController::Get().GetFile(file).SetPosition(offset, mode);
	}
	catch(TEFileException& ex)
	{
		ProcessException(ex);
		return false;
	}
	catch(std::exception& ex)
	{
		ProcessException(ex);
		return false;
	}
	catch(...)
	{
		ProcessException(UNKNOWN_EXCEPTION);
		return false;
	}

	return true;
}

const size_file_t& ElasticFileAPI::FileGetCursor(const TEFileHandle& file)
{
	try
	{
		return EFileController::Get().GetFile(file).GetPosition();
	}
	catch(TEFileException& ex)
	{
		ProcessException(ex);
	}
	catch(std::exception& ex)
	{
		ProcessException(ex);
	}
	catch(...)
	{
		ProcessException(UNKNOWN_EXCEPTION);
	}

	return 0;
}

size_file_t ElasticFileAPI::FileRead(const TEFileHandle& file, PBYTE buffer, const size_file_t& size)
{
	try
	{
		return EFileController::Get().GetFile(file).Read(buffer, size);
	}
	catch(TEFileException& ex)
	{
		ProcessException(ex);
		if(ex.error() == EF_READ_DATA_ERROR)
			return ex.data();

		return 0;
	}
	catch(std::exception& ex)
	{
		ProcessException(ex);
		return 0;
	}
	catch(...)
	{
		ProcessException(UNKNOWN_EXCEPTION);
		return 0;
	}
}

size_file_t ElasticFileAPI::FileWrite(const TEFileHandle& file, const PBYTE buffer, const size_file_t& size, bool overwrite)
{
	try
	{
		return EFileController::Get().GetFile(file).Write(buffer, size, overwrite);
	}
	catch(TEFileException& ex)
	{
		ProcessException(ex);
		if(ex.error() == EF_WRITE_DATA_ERROR)
			return ex.data();

		return 0;
	}
	catch(std::exception& ex)
	{
		ProcessException(ex);
		return 0;
	}
	catch(...)
	{
		ProcessException(UNKNOWN_EXCEPTION);
		return 0;
	}
}


bool ElasticFileAPI::FileTruncate(const TEFileHandle& file, const size_file_t& cut_size)
{
	try
	{
		return EFileController::Get().GetFile(file).Truncate(cut_size) == cut_size;
	}
	catch(TEFileException& ex)
	{
		ProcessException(ex);
	}
	catch(std::exception& ex)
	{
		ProcessException(ex);
	}
	catch(...)
	{
		ProcessException(UNKNOWN_EXCEPTION);
	}

	return false;
}

bool ElasticFileAPI::FileClose(const TEFileHandle& file)
{
	try
	{
		EFileController::Get().CloseFile(file);
	}
	catch(TEFileException& ex)
	{
		ProcessException(ex);
		return false;
	}
	catch(std::exception& ex)
	{
		ProcessException(ex);
		return false;
	}
	catch(...)
	{
		ProcessException(UNKNOWN_EXCEPTION);
		return false;
	}

	return true;
}