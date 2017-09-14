#include <TEFileCursor.h>
#include <TEFileException.h>
#include <ElasticFile.h>

TEFileCursor::TEFileCursor(ElasticFile& file)
	: m_position(0)
	, m_offset_in_sector(0)
	, m_file(file)
{
}


TEFileCursor::~TEFileCursor(void)
{
}

TEFileSectorsList::iterator TEFileCursor::GetCurrentSector()
{
	return m_sector;
}

const size_file_t& TEFileCursor::GetOffsetInSector()
{
	return m_offset_in_sector;
}

void TEFileCursor::SetPosition(const size_file_t& offset, TEFileCursorMoveMode mode)
{
	TEFileSectorsTable& sectors_table = m_file.GetSectorsTable();

	const size_file_t& data_size = sectors_table.GetDataSize();
	
	size_file_t new_position(0);

	switch(mode)
	{
	case EF_CURSOR_BEGIN:
		new_position = offset;
		break;

	case EF_CURSOR_CURRENT:
		new_position = m_position + offset;
		break;

	case EF_CURSOR_END:
		new_position = data_size - offset;
		break;
	}

	if (m_file.GetMode() & EF_MODE_APPEND)
	{
		if(new_position < m_position)
			throw TEFileException(EF_SET_POSITION_ERROR, "Can't go back with append mode");
	}

	size_file_t offset_in_sector(0);
	m_sector = GetSectorInPosition(new_position, offset_in_sector);
	if(m_sector == sectors_table.List().end() && offset_in_sector > 0)
		m_sector = m_file.Extend(offset_in_sector);

	m_position = new_position;
	m_offset_in_sector = offset_in_sector;

	size_file_t position_to_move;
	if(m_sector == sectors_table.List().end())
		position_to_move = sectors_table.GetFileSize();
	else
		position_to_move = m_sector->SectorAddr + m_offset_in_sector;

	if(_fseeki64(m_file.GetHandle(), position_to_move, SEEK_SET) != 0)
		throw TEFileException(EF_IO_ERROR, STRING("IO error when seek to " << position_to_move));
}

TEFileSectorsList::iterator TEFileCursor::GetSectorInPosition(const size_file_t& position, size_file_t& offset_in_sector)
{
	TEFileSectorsTable& sectors_table = m_file.GetSectorsTable();
	TEFileSectorsList& sectors_list = sectors_table.List();

	CURRLOG( "Get sector in position " << position << ": " );

	// Resolve simple situations
	if(position == m_position && m_sector != sectors_list.end() && !sectors_list.empty())
	{
		// Current position
		offset_in_sector = m_offset_in_sector;
		DEVLOG( m_sector->SectorAddr << ", " << offset_in_sector );
		return m_sector;
	}
	else if(m_sector == sectors_list.end() && sectors_list.empty())
	{
		// No sectors
		offset_in_sector = position;
		DEVLOG( -1 << ", " << offset_in_sector );
		return m_sector;
	}
	else if(position >= sectors_table.GetDataSize())
	{
		// Beyond or end of file
		offset_in_sector = position - sectors_table.GetDataSize();
		DEVLOG( -1 << ", " << offset_in_sector );
		return sectors_list.end();
	}
	else if(position == 0)
	{
		// Begin of the file
		offset_in_sector = 0;
		DEVLOG( sectors_list.begin()->SectorAddr << ", " << offset_in_sector );
		return sectors_list.begin();
	}
	else if(m_sector != sectors_list.end() && position > m_position && m_position - m_offset_in_sector + m_sector->SectorSize > position)
	{
		// If offset in current sector
		offset_in_sector = position - m_position;
		DEVLOG( m_sector->SectorAddr << ", " << offset_in_sector );
		return m_sector;
	}
	// So position somewhere between begin and end of the sectors list
	// Calculate optimal start point
	size_file_t distance_from_current = max(m_position, position) - min(m_position, position);
	size_file_t distance_from_begin = position;
	size_file_t distance_from_end = sectors_table.GetDataSize() - position;

	size_file_t min_distance = min(min(distance_from_current, distance_from_begin), distance_from_end);
	char vect = 1;

	TEFileSectorsList::iterator begin_it;
	size_file_t virtual_cursor(0);
	if(min_distance == distance_from_end)
	{
		CURRLOG( " end " );
		vect = -1;
		begin_it = sectors_list.end();
		virtual_cursor = sectors_table.GetDataSize();
	}
	else if(min_distance == distance_from_begin)
	{
		CURRLOG( " begin " );
		vect = 1;
		begin_it = sectors_list.begin();
		virtual_cursor = 0;
	}
	else if(min_distance == distance_from_current)
	{
		CURRLOG( " current " );
		vect = (position > m_position ? 1 : -1);
		begin_it = m_sector;
		virtual_cursor = m_position - m_offset_in_sector;
	}

	// Go to the sector
	if(vect > 0)
	{
		CURRLOG( " > " );
		// Go right
		for(TEFileSectorsList::iterator it = begin_it; it != sectors_list.end(); ++it)
		{
			TEFileSector& sector(*it);

			if(sector.Free)
				continue;

			if(virtual_cursor + sector.SectorSize > position)
			{
				offset_in_sector = position - virtual_cursor;
				DEVLOG( it->SectorAddr << ", " << offset_in_sector );
				return it;
			}

			virtual_cursor += sector.SectorSize;
		}
	}
	else if(vect < 0)
	{
		CURRLOG( " < " );
		// Go left
		TEFileSectorsList::reverse_iterator reverse_begin_it(begin_it);
		if(begin_it != sectors_list.end()) // Skip right-end iterator
		{
			--reverse_begin_it;
			virtual_cursor += begin_it->SectorSize;
		}

		for(TEFileSectorsList::reverse_iterator it = reverse_begin_it; it != sectors_list.rend(); ++it)
		{
			TEFileSector& sector(*it);

			virtual_cursor -= sector.SectorSize;

			if(sector.Free)
				continue;

			if(virtual_cursor <= position)
			{
				offset_in_sector = position - virtual_cursor;
				DEVLOG( it->SectorAddr << ", " << offset_in_sector );
				return --it.base();
			}
		}
	}

	throw TEFileException(EF_CURSOR_ERROR, STRING("Can't find sector in position " << position << ". Unknown case"));
}

const size_file_t& TEFileCursor::GetPosition()
{
	return m_position;
}

void TEFileCursor::Update(TEFileSectorsList::iterator sector)
{
	m_sector = sector;
}

void TEFileCursor::Update(TEFileSectorsList::iterator sector, const size_file_t& offset_in_sector)
{
	m_sector = sector;
	m_offset_in_sector = offset_in_sector;
}

void TEFileCursor::Update(TEFileSectorsList::iterator sector, const size_file_t& offset_in_sector, const size_file_t& position)
{
	m_sector = sector;
	m_position = position;
	m_offset_in_sector = offset_in_sector;
}
