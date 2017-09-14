#include <efile_types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ElasticFile.h>
#include <TEFileException.h>

ElasticFile::ElasticFile()
	: m_handle(0)
	, m_modified(false)
	, m_cursor(*this)
	, m_sectors_table(*this)
{
}


ElasticFile::~ElasticFile()
{
	close();
}

void ElasticFile::SetHandle(TEFileHandle file)
{
	CheckHandle(file);
	m_handle = file;
}

const TEFileHandle& ElasticFile::GetHandle()
{
	return m_handle;
}

TEFileCursor& ElasticFile::GetCursor()
{
	return m_cursor;
}

bool ElasticFile::Modified()
{
	return m_modified;
}

void ElasticFile::SetModified()
{
	if(!m_modified)
		m_modified = true;
}

TEFileSectorsTable& ElasticFile::GetSectorsTable()
{
	return m_sectors_table;
}

const TEFileOpenMode& ElasticFile::GetMode()
{
	return m_mode;
}

TEFileHandle ElasticFile::Open(const std::string& file_name, const TEFileOpenMode& mode)
{
	m_handle = InitLow(file_name, mode);
	Init(m_handle, mode);
	return m_handle;
}

TEFileHandle ElasticFile::InitLow(const std::string& file_name, const TEFileOpenMode& mode)
{
	std::string low_open_mode;

	bool checked(false);
	if(mode & EF_MODE_APPEND)
	{
		low_open_mode = "rb+";
	}

	if(mode & EF_MODE_CREATE)
	{
		low_open_mode = "wb+";
	}
	else if(mode & EF_MODE_CREATENEW)
	{
		CheckFileExists(file_name);
		low_open_mode = "wb+";
		checked = true;
	}
	else if(mode & EF_MODE_OPEN)
	{
		low_open_mode = "rb+";
	}
	else if(mode & EF_MODE_OPEN_OR_CREATE)
	{
		CheckFileExists(file_name);
		low_open_mode = "rb+";
		checked = true;
	}

	if(mode & EF_MODE_TRUNCATE)
	{
		if(!checked)
			CheckFileExists(file_name);

		low_open_mode = "wb+";
	}

	return OpenLow(file_name, low_open_mode);
}

void ElasticFile::Init(const TEFileHandle& file_handle, const TEFileOpenMode& mode)
{
	// Initialize file info
	SetHandle(file_handle);
	m_mode = mode;

	// Trying to read and load sectors table from the file
	if(m_sectors_table.Load() == EF_IO_ERROR)
	{
		throw TEFileException(EF_IO_ERROR, "IO error");
	}

	// Initialize a cursor
	m_cursor.Update(m_sectors_table.List().begin(), 0, 0);
	m_cursor.SetPosition(0, mode & EF_MODE_APPEND ? EF_CURSOR_END : EF_CURSOR_CURRENT);
}

TEFileHandle ElasticFile::OpenLow(const std::string& file_name, const std::string& low_mode)
{
	TEFileHandle file_handle;
	// Open the file
	if(fopen_s(&file_handle, file_name.c_str(), low_mode.c_str()) != 0)
	{
		throw TEFileException(EF_OPEN_FILE_ERROR, STRING("Can't open file '" << file_name << "'"));
	}

	return file_handle;
}

void ElasticFile::Close()
{
	if(close() == EOF)
	{
		throw TEFileException(EF_CLOSE_FILE_ERROR, STRING("Can' t close file " << m_handle));
	}
}

int ElasticFile::close()
{
	if(Modified())
	{
		m_sectors_table.Write();
		m_modified = false;
	}

	m_sectors_table.Clear();
	return fclose(m_handle);
}

void ElasticFile::CheckHandle()
{
	CheckHandle(m_handle);
}

void ElasticFile::CheckHandle(const TEFileHandle& file_handle)
{
	if(file_handle == NULL)
	{
		throw(TEFileException(EF_NULL_HANDLE, "Handle is NULL"));
	}
}

size_file_t ElasticFile::WriteOverwrite(const PBYTE buffer, const size_file_t& buffer_size)
{
	DEVLOG( std::endl << "write overwrite '" << std::string((char*)buffer, buffer_size).c_str() << "' from position " << m_cursor.GetPosition() << std::endl);

	TEFileSectorsList::iterator sector_it = m_cursor.GetCurrentSector();
	TEFileSectorsList::iterator end_it = m_sectors_table.List().end();

	// Overwrite existing data
	size_file_t bytes_written(0);
	for(; sector_it != end_it; ++sector_it)
	{
		TEFileSector& sector(*sector_it);

		if(sector.Free)
			continue;

		size_file_t from = m_cursor.GetOffsetInSector();
		size_file_t bytes_to_write = min(buffer_size - bytes_written, sector.SectorSize - from);

		bytes_written += WriteSector(sector_it, buffer + bytes_written, from, bytes_to_write);

		if(bytes_written == buffer_size)
			return bytes_written;
	}

	// Write rest data to the end
	size_file_t bytes_left = buffer_size - bytes_written;
	if(bytes_left > 0 && m_cursor.GetPosition() == m_sectors_table.GetDataSize())
		bytes_written += WriteInsert(buffer + bytes_written, bytes_left);

	return bytes_written;
}

size_file_t ElasticFile::WriteInsert(const PBYTE buffer, const size_file_t& bytes_count_to_write)
{
	DEVLOG(std::endl << "write insert '" << std::string((char*)buffer, bytes_count_to_write).c_str() << "' to position " << m_cursor.GetPosition() << std::endl);

	TEFileSectorsList::iterator current_sector_it = m_cursor.GetCurrentSector();
	TEFileSectorsList& sectors_list = m_sectors_table.List();

	// If just need to write in the end and the virtual end equals the real end in the file. Just extend the end sector and write to extended space
	if(m_cursor.GetOffsetInSector() == 0 && current_sector_it != sectors_list.begin() && !sectors_list.empty() && m_sectors_table.FreeSectors().empty())
	{
		// Cursor points to offset 0 of the next after extended sector now. So, go to the previous sector in order to extend it
		--current_sector_it;

		// If the sector is located in the end of the file
		if(current_sector_it == m_sectors_table.Map().rbegin()->second)
		{
			size_file_t bytes_written = WriteSector(current_sector_it, buffer, current_sector_it->SectorSize, bytes_count_to_write);
			return bytes_written;
		}
	}

	// If only one sector needs to allocate (fast allocating without containers)
	if(m_sectors_table.FreeSectors().empty())
	{
		TEFileSectorsList::iterator new_sector_it = m_sectors_table.AllocateNewSector(bytes_count_to_write);
		m_sectors_table.MoveSectorTo(new_sector_it, m_cursor.GetPosition());

		size_file_t bytes_written = WriteSector(new_sector_it, buffer, 0, bytes_count_to_write);

		TUniteResult unite_result;
		if(m_sectors_table.CheckAndUniteSector(new_sector_it, unite_result) == EF_UNITE_RIGHT)
			m_cursor.Update(unite_result.first, unite_result.second);

		return bytes_written;
	}

	// If need to allocate more than one new sectors
	TEFileSectorsIterators allocated_sectors_iterators;
	m_sectors_table.Allocate(bytes_count_to_write, allocated_sectors_iterators);

	if(allocated_sectors_iterators.empty())
	{
		throw TEFileException(EF_ALLOCATE_ERROR, STRING("Can't allocate " << bytes_count_to_write << " bytes"));
	}

	// Move allocated sectors to the current position
	m_sectors_table.MoveSectorsTo(allocated_sectors_iterators, m_cursor.GetPosition());

	size_file_t bytes_written(0);
	TEFileSectorsList::iterator first_it = *allocated_sectors_iterators.begin();
	TEFileSectorsList::iterator last_it = *std::prev(allocated_sectors_iterators.end());
	TEFileSectorsList::iterator sector_it = first_it;
	do
	{
		bool is_last = sector_it == last_it;

		size_file_t bytes_to_write = sector_it->SectorSize;
		bytes_written += WriteSector(sector_it, buffer + bytes_written, 0, bytes_to_write);

		TUniteResult unite_result;
		if(m_sectors_table.CheckAndUniteSector(sector_it, unite_result) == EF_UNITE_RIGHT)
			m_cursor.Update(unite_result.first, unite_result.second);

		if(!is_last)
			sector_it = m_cursor.GetCurrentSector(); // The next sector after WriteSector
		else
			break;
	}
	while(sector_it != sectors_list.end());

	return bytes_written;
}

TEFileSectorsList::iterator ElasticFile::Extend(const size_file_t& size_to_extend)
{
	DEVLOG( "extend file to " << size_to_extend );

	TEFileSectorsIterators allocated_sectors_iterators;
	m_sectors_table.Allocate(size_to_extend, allocated_sectors_iterators);
	std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator> moved_allocated_sectors_range = m_sectors_table.MoveSectorsBefore(allocated_sectors_iterators, m_sectors_table.List().end());

	// Calculate size of trash to clear it
	size_file_t size_to_clear = min(m_sectors_table.CalculateGarbageSize(), size_to_extend);
	size_file_t size_cleared(0);

	// Value which will be filled with garbage
	byte initial_value = 0;
	TEFileSectorsList::iterator sector_it = moved_allocated_sectors_range.first;
	do
	{
		bool last = sector_it == moved_allocated_sectors_range.second;

		// Mark a sector as free
		m_sectors_table.SetSectorFree(sector_it, 0);

		// Try to unite sectors
		TUniteResult unite_result;
		if(m_sectors_table.CheckAndUniteSector(sector_it, unite_result) != EF_UNITE_NONE)
		{
			sector_it = unite_result.first;
			m_cursor.Update(sector_it, unite_result.second);
		}

		// Increase real file size
		TEFileSector& sector = *sector_it;
		size_file_t clear_left = size_to_clear - size_cleared;
		if(clear_left == 0)
		{
			// Just write a single byte to the end of a sector for increase file by SectorSize and fill it with zero
			_fseeki64(m_handle, sector.SectorAddr + sector.SectorSize-1, SEEK_SET);
			fwrite(&initial_value, 1, 1, m_handle);
			++sector_it;
			continue;
		}

		// Clear garbage in free file space
		size_file_t local_size_to_clear = sector.SectorSize;
		if(local_size_to_clear > clear_left)
			local_size_to_clear = clear_left;

		_fseeki64(m_handle, sector.SectorAddr, SEEK_SET);
		for(size_file_t virtual_cursor = 0; virtual_cursor != local_size_to_clear; ++virtual_cursor)
			fwrite(&initial_value, 1, 1, m_handle);

		size_cleared += local_size_to_clear;

		if(last)
			break;

		++sector_it;
	}
	while(sector_it != m_sectors_table.List().end());
	fflush(m_handle);

	SetModified();

	return sector_it;
}

void ElasticFile::CheckSize(const size_file_t& size)
{
	if(size < 0)
	{
		throw TEFileException(EF_UNCORRECT_PARAMETER, "Uncorrect parameter value", size);
	}
}

// Main interface
size_file_t ElasticFile::Write(const PBYTE buffer, const size_file_t& size, bool overwrite)
{
	CheckSize(size);
	CheckHandle();
	return overwrite ? WriteOverwrite(buffer, size) : WriteInsert(buffer, size);
}

size_file_t ElasticFile::Read(PBYTE buffer, const size_file_t& size)
{
	CheckSize(size);
	CheckHandle();

	if (m_mode & EF_MODE_APPEND)
	{
		throw TEFileException(EF_READ_ON_APPEND, "Can't read in append mode");
	}

	TEFileSectorsList::iterator first_sector_it = m_cursor.GetCurrentSector();
	TEFileSectorsList::iterator end_it = m_sectors_table.List().end();

	TEFileSector sector;
	size_file_t bytes_read(0);
	for(TEFileSectorsList::iterator sector_it = first_sector_it; sector_it != end_it; ++sector_it)
	{
		TEFileSector& sector(*sector_it);

		if(sector.Free)
			continue;

		// Use sector offset only at first sector because cursor moves only in the end of reading in order to faster read
		if(sector_it == first_sector_it) 
			_fseeki64(m_handle, sector.SectorAddr + m_cursor.GetOffsetInSector(), SEEK_SET);
		else
			_fseeki64(m_handle, sector.SectorAddr, SEEK_SET);

		// Read sector
		size_file_t bytes_to_read = min(sector.SectorAddr + sector.SectorSize - ftell(m_handle), size - bytes_read);
		size_file_t local_bytes_read = fread(buffer + bytes_read, sizeof(BYTE), bytes_to_read, m_handle);
		bytes_read += local_bytes_read;

		if(local_bytes_read != bytes_to_read)
		{
			m_cursor.SetPosition(bytes_read, EF_CURSOR_CURRENT);
			throw TEFileException(EF_READ_DATA_ERROR, STRING("Read " << bytes_read << " bytes of " << size), bytes_read);
		}

		if(bytes_read == size)
			break;
	}

	if(bytes_read < size)
	{
		m_cursor.SetPosition(bytes_read, EF_CURSOR_CURRENT);
		throw TEFileException(EF_READ_DATA_ERROR, STRING("Read " << bytes_read << " bytes of " << size << ". End of file reached"), bytes_read);
	}

	m_cursor.SetPosition(bytes_read, EF_CURSOR_CURRENT);

	return bytes_read;
}

size_file_t ElasticFile::Truncate(const size_file_t& cut_size)
{
	CheckSize(cut_size);
	CheckHandle();

	if(m_mode & EF_MODE_APPEND)
	{
		throw TEFileException(EF_TRUNCATE_ON_APPEND, "Can't truncate in append mode");
	}

	DEVLOG( std::endl << "truncate from " << m_cursor.GetPosition() << " by " << cut_size << std::endl );

	TEFileSectorsList::iterator sector_it = m_cursor.GetCurrentSector();
	TEFileSectorsList::iterator end_it = m_sectors_table.List().end();

	size_file_t bytes_truncated(0);
	for(; sector_it != end_it;)
	{
		TEFileSector& sector(*sector_it);

		if(sector.Free)
		{
			++sector_it;
			continue;
		}

		// Truncate
		size_file_t bytes_to_truncate = min(cut_size - bytes_truncated, sector.SectorSize - m_cursor.GetOffsetInSector());
		std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator> truncation_result;
		truncation_result = m_sectors_table.TruncateSector(sector_it, m_cursor.GetOffsetInSector(), bytes_to_truncate);
		bytes_truncated += bytes_to_truncate;
		SetModified();

		// Check for unite free (truncated) part
		TUniteResult unite_result;
		m_sectors_table.CheckAndUniteSector(truncation_result.first, unite_result);

		// If truncation complete
		if(bytes_truncated == cut_size)	
		{
			// Check can we unite two sectors between which we have truncated data
			if(m_sectors_table.CheckAndUniteSector(truncation_result.second, unite_result) != EF_UNITE_NONE)
				m_cursor.Update(unite_result.first, unite_result.second);
			return bytes_truncated;
		}

		// Go to next sector with data
		sector_it = truncation_result.second;	
		m_cursor.Update(sector_it, 0);
	}

	if(bytes_truncated != cut_size)
	{
		throw TEFileException(EF_TRUNCATE_ERROR, STRING("Truncated " << bytes_truncated << " of " << cut_size << ". End of file reached"), bytes_truncated);
	}

	return bytes_truncated;
}

void ElasticFile::CheckFileExists(const std::string& file_name)
{
	struct _stat64 filestat;
	int result = _stati64(file_name.c_str(), &filestat);

	if(result != 0)
	{
		throw (TEFileException(EF_FILE_NOT_EXISTS, STRING("File " << file_name << " does not exists")));
	}
}

void ElasticFile::SetPosition(const size_file_t& offset, const TEFileCursorMoveMode& mode)
{
	CheckSize(offset);
	m_cursor.SetPosition(offset, mode);
}

const size_file_t& ElasticFile::GetPosition()
{
	return m_cursor.GetPosition();
}

size_file_t ElasticFile::WriteSector(TEFileSectorsList::iterator sector_it, PBYTE buffer, const size_file_t& from, const size_file_t& size_to_write)
{
	TEFileSector& sector = *sector_it;

	_fseeki64(m_handle, sector.SectorAddr + from, SEEK_SET);
	size_file_t bytes_written = fwrite(buffer, sizeof(BYTE), size_to_write, m_handle);

	m_sectors_table.SetSectorFree(sector_it, 0);

	if(from + size_to_write > sector.SectorSize)
		m_sectors_table.ExtendSector(sector_it, bytes_written);

	m_cursor.Update(std::next(sector_it), 0, m_cursor.GetPosition() + bytes_written);

	SetModified();

	if(bytes_written != size_to_write)
	{
		throw TEFileException(EF_WRITE_DATA_ERROR, STRING(bytes_written << " of " << size_to_write << " bytes have been written"), bytes_written);
	}

	return size_to_write;
}
