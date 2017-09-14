#pragma once
#include <list>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <windows.h>

#define LOG_ERROR
//#define LOG_DEV
//#define LOG_INFO

typedef unsigned int size_file_t;

struct TEFileSector
{
	TEFileSector()
		: SectorAddr(0)
		, SectorSize(0)
		, Free(0)
	{
	}

	BYTE Free;
	size_file_t SectorAddr; // Real file offset
	size_file_t SectorSize; // Size of the sector in bytes
};

typedef std::shared_ptr<TEFileSector> TEFileSectorPtr;

typedef size_file_t TEFileSectorsCount;

typedef FILE* TEFileHandle;

typedef DWORD TEFileOpenMode;
#define EF_MODE_APPEND			0x000001
#define EF_MODE_CREATE			0x000010
#define EF_MODE_CREATENEW		0x000100
#define EF_MODE_OPEN			0x001000
#define EF_MODE_OPEN_OR_CREATE	0x010000
#define EF_MODE_TRUNCATE		0x100000

// Errors and statuses
enum
{
	EF_SUCCESS,
	EF_NULL_HANDLE,
	EF_HANDLE_NOT_FOUND,
	EF_UNCORRECT_PARAMETER,
	EF_CANNOT_READ_SECTORS_COUNT,
	EF_CANNOT_READ_SECTORS,
	EF_OPEN_FILE_ERROR,
	EF_IO_ERROR,
	EF_ALLOCATE_ERROR,
	EF_WRITE_DATA_ERROR,
	EF_FILE_DATA_LESS,
	EF_CLOSE_FILE_ERROR,
	EF_FILE_NOT_EXISTS,
	EF_SET_POSITION_ERROR,
	EF_READ_DATA_ERROR,
	EF_READ_ON_APPEND,
	EF_TRUNCATE_ON_APPEND,
	EF_TRUNCATE_ERROR,
	EF_CURSOR_ERROR,
	EF_UNKNOWN_ERROR
};

typedef std::list<TEFileSector> TEFileSectorsList;
typedef std::vector<TEFileSectorsList::iterator> TEFileSectorsIterators;

typedef std::map<size_file_t, TEFileSectorsList::iterator> TEFileSectorsMap;

enum TEFileCursorMoveMode
{
	EF_CURSOR_BEGIN,
	EF_CURSOR_CURRENT,
	EF_CURSOR_END
};

typedef std::pair<TEFileSectorsList::iterator, size_file_t> TUniteResult;
	
enum TUniteStatus
{
	EF_UNITE_NONE,
	EF_UNITE_LEFT,
	EF_UNITE_RIGHT,
	EF_UNITE_BOTH
};

#define STRING(expr) (static_cast<std::ostringstream*>(&(std::ostringstream().flush() << expr))->str())

#ifdef LOG_DEV
	#define devlog std::cout << " [D] "
	#define DEVLOG(expr) devlog << expr << std::endl
	#define CURRLOG(expr) std::cout << expr
#else
	#define devlog std::stringstream()
	#define DEVLOG(expr)
	#define CURRLOG(expr)
#endif
	
#ifdef LOG_ERROR
	#define errlog std::cout << " [E] "
	#define ERRLOG(expr) errlog << expr << std::endl
#else
	#define errlog std::stringstream()
	#define ERRLOG(expr)
#endif

#ifdef LOG_INFO
	#define infolog std::cout << " [I] "
	#define INFO(expr) infolog << expr << std::endl
#else
	#define infolog std::stringstream()
	#define INFO(expr)
#endif
