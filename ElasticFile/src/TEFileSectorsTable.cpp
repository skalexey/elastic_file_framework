#include <TEFileSectorsTable.h>
#include <TEFileException.h>
#include <ElasticFile.h>

TEFileSectorsTable::TEFileSectorsTable(ElasticFile& file)
	: m_file_size(0)
	, m_data_size(0)
	, m_sectors_count(0)
	, m_file(file)
{
}

TEFileSectorsTable::~TEFileSectorsTable()
{
	m_sectors_map.clear();
	m_free_sectors_map.clear();
	m_sectors_list.clear();
}

TEFileSectorsList::iterator TEFileSectorsTable::GetFirstFreeSector() 
{
	if(!m_free_sectors_map.empty())
		return m_free_sectors_map.begin()->second;

	return m_sectors_list.end();
}

size_file_t TEFileSectorsTable::GetTableSize() const
{
	return m_sectors_count * sizeof(TEFileSector) + sizeof(TEFileSectorsCount);
}

size_file_t TEFileSectorsTable::MinFileSize() const
{
	return m_sectors_count * sizeof(BYTE);
}

TEFileSectorsList::iterator TEFileSectorsTable::AllocateNewSector(size_file_t size_to_allocate)
{
	TEFileSector sector;
	sector.Free = 1;
	sector.SectorAddr = m_file_size;
	sector.SectorSize = size_to_allocate;

	return InsertSector(sector, m_sectors_list.end());
}

void TEFileSectorsTable::Allocate(size_file_t size_to_allocate, TEFileSectorsIterators& allocated_sectors_iterators)
{
	TEFileSectorsList::iterator firstFreeSector_it = GetFirstFreeSector();
	size_file_t bytes_allocated(0);

	// Try to fill holes in file space
	TEFileSectorsList::iterator sector_it;
	for(sector_it = firstFreeSector_it; sector_it != m_sectors_list.end(); ++sector_it)
	{
		TEFileSector sector = *sector_it;
		if(!sector.Free)
			continue;

		size_file_t bytes_to_allocate = size_to_allocate - bytes_allocated;
		if(bytes_to_allocate < sector.SectorSize)
			SplitSector(sector_it, bytes_to_allocate);
		else if(bytes_to_allocate > sector.SectorSize)
			bytes_to_allocate = sector.SectorSize;
			
		allocated_sectors_iterators.push_back(sector_it);

		bytes_allocated += bytes_to_allocate;

		if(bytes_allocated == size_to_allocate)
			return;
	}

	size_file_t bytes_to_allocate = size_to_allocate - bytes_allocated;

	// Create a new sector
	if(bytes_to_allocate > 0)
	{
		sector_it = AllocateNewSector(bytes_to_allocate);
		allocated_sectors_iterators.push_back(sector_it);
	}
}

TEFileSectorsList::iterator TEFileSectorsTable::InsertSector(const TEFileSector& sector, TEFileSectorsList::iterator before)
{
	TEFileSectorsList::iterator new_sector_it = m_sectors_list.insert(before, sector);
	m_sectors_map[sector.SectorAddr] = new_sector_it;

	if(!sector.Free)
		m_data_size += sector.SectorSize;
	else
		m_free_sectors_map[sector.SectorAddr] = new_sector_it;

	m_file_size += sector.SectorSize;

	m_sectors_count++;

	return new_sector_it;
}

void TEFileSectorsTable::MoveSectorsBefore(TEFileSectorsIterators& sectors_to_move, TEFileSectorsList::iterator before_it, bool)
{
	if(sectors_to_move.empty())
		return;

	for(TEFileSectorsIterators::iterator it = sectors_to_move.begin(); it != sectors_to_move.end(); ++it)
	{
		if(*it == before_it)
			continue;

		MoveSector(*it, before_it);
	}
}

std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator> TEFileSectorsTable::MoveSectorsBefore(TEFileSectorsIterators& sectors_to_move, TEFileSectorsList::iterator before_it)
{
	if(sectors_to_move.empty())
		return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(m_sectors_list.end(), m_sectors_list.end());

	TEFileSectorsList::iterator first_it = before_it;
	TEFileSectorsList::iterator last_it = before_it;

	for(TEFileSectorsIterators::iterator it = sectors_to_move.begin(); it != sectors_to_move.end(); ++it)
	{
		if(*it == before_it)
			continue;

		MoveSector(*it, before_it);

		if(it == sectors_to_move.begin())
			first_it = *it;

		last_it = *it;
	}

	return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(first_it, last_it);
}

std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator> TEFileSectorsTable::TruncateSector(TEFileSectorsList::iterator sector, size_file_t from, size_file_t truncation_size)
{
	if(sector == m_sectors_list.end())
		return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(m_sectors_list.end(), m_sectors_list.end());

	if(from > sector->SectorSize || from < 0)
		return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(m_sectors_list.end(), m_sectors_list.end());

	if(from + truncation_size > sector->SectorSize)
		return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(m_sectors_list.end(), m_sectors_list.end());

	if(sector->Free)
		return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(m_sectors_list.end(), m_sectors_list.end());

	// Split sector twice
	std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator> first_split = SplitSector(sector, from);
	std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator> second_split = SplitSector(first_split.second, truncation_size);

	TEFileSectorsList::iterator free_part_it = first_split.second;
	SetSectorFree(free_part_it, 1);

	TEFileSectorsList::iterator next_sector_it = std::next(free_part_it);
	while(next_sector_it != m_sectors_list.end() && next_sector_it->Free)
		++next_sector_it;

	return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(free_part_it, next_sector_it);
}

std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator> TEFileSectorsTable::SplitSector(TEFileSectorsList::iterator sector_it, size_file_t offset_in_sector)
{
	if(sector_it == m_sectors_list.end())
		return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(m_sectors_list.end(), m_sectors_list.end());

	TEFileSector& sector(*sector_it);

	if(offset_in_sector > sector.SectorSize)
		return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(m_sectors_list.end(), m_sectors_list.end());

	if(offset_in_sector == 0)
		return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(m_sectors_list.end(), sector_it);

	if(offset_in_sector == sector.SectorSize)
		return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(sector_it, m_sectors_list.end());

	TEFileSector secondPart;

	secondPart.Free = sector.Free;
	secondPart.SectorAddr = sector.SectorAddr + offset_in_sector;
	secondPart.SectorSize = sector.SectorSize - offset_in_sector;

	sector.SectorSize = offset_in_sector;

	m_file_size -= secondPart.SectorSize;

	if(!sector.Free)
		m_data_size -= secondPart.SectorSize;


	TEFileSectorsList::iterator next_sector_it = std::next(sector_it);

	TEFileSectorsList::iterator second_part_it = InsertSector(secondPart, next_sector_it);

	return std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator>(sector_it, second_part_it);
}

void TEFileSectorsTable::MoveSectorTo(TEFileSectorsList::iterator sector_it, size_file_t position)
{
	size_file_t offset_in_sector;
	TEFileSectorsList::iterator sectorInPosition_it = m_file.GetCursor().GetSectorInPosition(position, offset_in_sector);

	if(position != m_data_size && offset_in_sector != 0)
		sectorInPosition_it = SplitSector(sectorInPosition_it, offset_in_sector).second;

	MoveSector(sector_it, sectorInPosition_it);
}

void TEFileSectorsTable::MoveSectorsTo(TEFileSectorsIterators& sectorsToMove, size_file_t position)
{
	size_file_t offset_in_sector;
	TEFileSectorsList::iterator sector_in_position_it = m_file.GetCursor().GetSectorInPosition(position, offset_in_sector);

	if(position != m_data_size && offset_in_sector != 0)
		sector_in_position_it = SplitSector(sector_in_position_it, offset_in_sector).second;

	MoveSectorsBefore(sectorsToMove, sector_in_position_it);
}

void TEFileSectorsTable::Create()
{
	const TEFileHandle& file_handle = m_file.GetHandle();

	Clear();

	_fseeki64(file_handle, 0, SEEK_END);

	size_file_t file_size = ftell(file_handle);

	if(file_size == 0)
		return;

	TEFileSector sector;
	sector.Free = 0;
	sector.SectorAddr = 0;
	sector.SectorSize = file_size;

	InsertSector(sector, m_sectors_list.end());
}

void TEFileSectorsTable::Clear()
{
	m_sectors_list.clear();
	m_sectors_map.clear();
	m_free_sectors_map.clear();
	m_sectors_count = 0;
	m_file_size = 0;
	m_data_size = 0;
}

int TEFileSectorsTable::Load()
{
	int result = Parse();

	if(result == EF_IO_ERROR) // If some low-level error
		return result;
	else if(result != 0) // If table is corrupted - create new
		Create();

	return result;
}

int TEFileSectorsTable::Parse()
{
	const TEFileHandle& file_handle = m_file.GetHandle();

	if(_fseeki64(file_handle, 0, SEEK_END) != 0)
		return EF_IO_ERROR;

	size_file_t file_size = ftell(file_handle);

	TEFileSectorsCount sectors_count(0);

	if(file_size < sizeof(TEFileSectorsCount))
		return EF_CANNOT_READ_SECTORS_COUNT;

	if(_fseeki64(file_handle, file_size - sizeof(TEFileSectorsCount), SEEK_SET) != 0)
		return EF_IO_ERROR;

	
	DEVLOG( "read sectors table:" );
	CURRLOG( "sectors count: " );

	if(fread(&sectors_count, sizeof(TEFileSectorsCount), 1, file_handle) != 1)
	{
		DEVLOG( "er" << std::endl );
		return EF_IO_ERROR;
	}
	DEVLOG( sectors_count );

	if(_fseeki64(file_handle, file_size - sizeof(TEFileSector)*sectors_count - sizeof(TEFileSectorsCount), SEEK_SET) != 0)
		return EF_CANNOT_READ_SECTORS;

	m_data_size = 0;

	DEVLOG( "read sectors: " );
	TEFileSector sector;
	size_t elements_to_read = 1;
	for(size_file_t sector_number = 0; sector_number < sectors_count; ++sector_number)
	{
		if(fread(&sector, sizeof(TEFileSector), elements_to_read, file_handle) != elements_to_read)
		{
			DEVLOG( "error: " << errno << ", " << feof(file_handle) << ", " << ferror(file_handle));
			return EF_CANNOT_READ_SECTORS;
		}

		DEVLOG( "[" << sector.SectorAddr << "," << sector.SectorSize << "," << (int)sector.Free << "]" );

		InsertSector(sector, m_sectors_list.end());
	}

	size_file_t file_space = file_size - GetTableSize();
	if(m_file_size > file_space || m_file_size < MinFileSize())
	{
		DEVLOG( "data corrupted" << std::endl );
		return EF_FILE_DATA_LESS;
	}

	DEVLOG( "success" << std::endl );
	return 0;
}

const size_file_t& TEFileSectorsTable::GetDataSize() const
{
	return m_data_size;
}

const size_file_t& TEFileSectorsTable::GetFileSize() const
{
	return m_file_size;
}

TEFileSectorsList& TEFileSectorsTable::List()
{
	return m_sectors_list;
}
TEFileSectorsMap& TEFileSectorsTable::Map()
{
	return m_sectors_map;
}

TEFileSectorsMap& TEFileSectorsTable::FreeSectors()
{
	return m_free_sectors_map;
}

bool TEFileSectorsTable::Write()
{
	const TEFileHandle& file_handle = m_file.GetHandle();

	_fseeki64(file_handle, 0, SEEK_END);
	size_file_t file_size = ftell(file_handle);

	size_file_t table_size = GetTableSize();
	size_file_t free_space = file_size - m_file_size;
	size_file_t table_position = free_space <= table_size ? m_file_size : file_size - table_size;

	DEVLOG( "write sectors table (" << m_sectors_count << "):" ); 

	if(_fseeki64(file_handle, table_position, SEEK_SET) != 0)
		return false;

	for(TEFileSectorsList::iterator sector_it = m_sectors_list.begin(); sector_it != m_sectors_list.end(); ++sector_it)
	{
		CURRLOG( "[" << sector_it->SectorAddr << "," << sector_it->SectorSize << "," << (int)sector_it->Free << "] " );
		if(fwrite(&(*sector_it), sizeof(TEFileSector), 1, file_handle) != 1)
		{
			CURRLOG( "er" << std::endl );
			return false;
		}

		CURRLOG( "ok" << std::endl );
	}

	CURRLOG( "write size: " );
	if(fwrite(&m_sectors_count, sizeof(TEFileSectorsCount), 1, file_handle) != 1)
	{
		DEVLOG( "er" );
		return false;
	}

	DEVLOG( "success" );
	return true;
}

void TEFileSectorsTable::RemoveSector(TEFileSectorsList::iterator sector_it)
{
	if(sector_it == m_sectors_list.end())
		return;

	if(!sector_it->Free)
		m_data_size -= sector_it->SectorSize;
	else
		m_free_sectors_map.erase(sector_it->SectorAddr);

	m_file_size -= sector_it->SectorSize;

	m_sectors_map.erase(sector_it->SectorAddr);
	m_sectors_list.erase(sector_it);
	
	m_sectors_count--;
}


size_file_t TEFileSectorsTable::GetSectorsCount() const
{
	return m_sectors_count;
}

void TEFileSectorsTable::SetSectorFree(TEFileSectorsList::iterator sector, bool free)
{
	if(free && sector->Free == 1)
		return;

	if(!free)
		m_free_sectors_map.erase(sector->SectorAddr);
	else
		m_free_sectors_map[sector->SectorAddr] = sector;

	sector->Free = free;
	m_data_size += sector->SectorSize * (free ? -1 : 1);
}

void TEFileSectorsTable::MoveSector(TEFileSectorsList::iterator sector_it, TEFileSectorsList::iterator before_it)
{
	if(sector_it == before_it)
		return;

	if(before_it != m_sectors_list.begin() && std::prev(before_it) == sector_it)
		return;

	m_sectors_list.splice(before_it, m_sectors_list, sector_it);
}

TUniteStatus TEFileSectorsTable::CheckAndUniteSector(TEFileSectorsList::iterator sector_it, TUniteResult& uniteResult)
{
	if(sector_it == m_sectors_list.end())
		return EF_UNITE_NONE;
	else if(sector_it->Free)
		return CheckAndUniteFreeSector(sector_it, uniteResult);
	else
		return CheckAndUniteDataSector(sector_it, uniteResult);
}

TUniteStatus TEFileSectorsTable::CheckAndUniteDataSector(TEFileSectorsList::iterator sector_it, TUniteResult& unite_result)
{
	TEFileSectorsList::iterator result_it = sector_it;
	size_file_t offset_in_sector(0);

	TUniteStatus unite_status = EF_UNITE_NONE;
	
	if(sector_it != m_sectors_list.begin()) // If there is a sector before this one
	{
		// Get left sector in the sectors list
		TEFileSectorsList::iterator sector_left_it = std::prev(sector_it);

		TEFileSector& sector = *sector_it;
		TEFileSector& sector_left = *sector_left_it;

		if(sector_left.Free == sector.Free && sector_left.Free == 0 && sector_left.SectorAddr + sector_left.SectorSize == sector.SectorAddr)
		{
			offset_in_sector = sector_left.SectorSize;
			result_it = UniteSectors(sector_left_it, sector_it);
			unite_status = EF_UNITE_LEFT; 
		}
	}

	if(result_it != --(m_sectors_list.rbegin().base())) // If there is a sector after this one
	{
		// Get right sector in the sectors list
		TEFileSectorsList::iterator sector_right_it = std::next(result_it);

		TEFileSector& sector = *result_it;
		TEFileSector& sector_right = *sector_right_it;

		if(sector.Free == sector_right.Free && sector.Free == 0 && sector.SectorAddr + sector.SectorSize == sector_right.SectorAddr)
		{
			result_it = UniteSectors(result_it, sector_right_it);
			unite_status = unite_status == EF_UNITE_LEFT ? EF_UNITE_BOTH : EF_UNITE_RIGHT;
		}
	}

	unite_result.first = result_it;
	unite_result.second = offset_in_sector;

	return unite_status;
}

TUniteStatus TEFileSectorsTable::CheckAndUniteFreeSector(TEFileSectorsList::iterator sector_it, TUniteResult& unite_result)
{
	TEFileSectorsList::iterator result_it = sector_it;
	size_file_t offset_in_sector(0);

	TUniteStatus unite_status = EF_UNITE_NONE;

	if(sector_it != m_sectors_map.begin()->second) // If there is a sector before this one
	{
		// Get left sector in the file map
		TEFileSectorsList::iterator sector_left_it = std::prev(m_sectors_map.find(sector_it->SectorAddr))->second;

		TEFileSector& sector = *sector_it;
		TEFileSector& sector_left = *sector_left_it;

		if(sector_left.Free == sector.Free && sector.Free == 1)
		{
			offset_in_sector = sector_left.SectorSize;
			result_it = UniteSectors(sector_left_it, sector_it);
			unite_status = EF_UNITE_LEFT; 
		}
	}

	if(result_it != m_sectors_map.rbegin()->second) // If there is a sector after this one
	{
		// Get right sector in the file map
		TEFileSectorsList::iterator sector_right_it = m_sectors_map.find(result_it->SectorAddr+result_it->SectorSize)->second;

		TEFileSector& sector = *result_it;
		TEFileSector& sector_right = *sector_right_it;

		if(sector.Free == sector_right.Free && sector.Free == 1)
		{
			result_it = UniteSectors(result_it, sector_right_it);
			unite_status = unite_status == EF_UNITE_LEFT ? EF_UNITE_BOTH : EF_UNITE_RIGHT;
		}
	}

	unite_result.first = result_it;
	unite_result.second = offset_in_sector;

	return unite_status;
}

TEFileSectorsList::iterator TEFileSectorsTable::UniteSectors(TEFileSectorsList::iterator first_it, TEFileSectorsList::iterator second_it)
{
	TEFileSectorsList::iterator sector_left_it;
	TEFileSectorsList::iterator sector_right_it;

	if(second_it->SectorAddr > first_it->SectorAddr)
	{
		sector_left_it = first_it;
		sector_right_it = second_it;
	}
	else
	{
		sector_left_it = second_it;
		sector_right_it = first_it;
	}
	
	sector_left_it->SectorSize += sector_right_it->SectorSize;

	DEVLOG( "unite sectors " << sector_left_it->SectorAddr << " and " << sector_right_it->SectorAddr );

	if(sector_right_it->Free)
		m_free_sectors_map.erase(sector_right_it->SectorAddr);

	m_sectors_map.erase(sector_right_it->SectorAddr);
	m_sectors_list.erase(sector_right_it);

	m_sectors_count--;
	
	return sector_left_it;
}

void TEFileSectorsTable::ExtendSector(TEFileSectorsList::iterator sector_it, size_file_t size_to_extend)
{
	TEFileSector& sector = *sector_it;
	sector.SectorSize += size_to_extend;
	if(!sector.Free)
		m_data_size += size_to_extend;

	m_file_size += size_to_extend;
}

size_file_t TEFileSectorsTable::CalculateGarbageSize()
{
	const TEFileHandle& file_handle = m_file.GetHandle();
	_fseeki64(file_handle, 0, SEEK_END);
	return ftell(file_handle) - m_data_size;
}
