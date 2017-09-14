#pragma once
#include <efile_types.h>

class ElasticFile;

class TEFileCursor
{
public:
	TEFileCursor(ElasticFile& file);
	~TEFileCursor(void);

	void SetPosition(const size_file_t& offset, TEFileCursorMoveMode mode);
	const size_file_t& GetPosition();
	void Update(TEFileSectorsList::iterator sector);
	void Update(TEFileSectorsList::iterator sector, const size_file_t& offset_in_sector);
	void Update(TEFileSectorsList::iterator sector, const size_file_t& offset_in_sector, const size_file_t& position);
	TEFileSectorsList::iterator GetCurrentSector();
	TEFileSectorsList::iterator GetSectorInPosition(const size_file_t& position, size_file_t& offset_in_sector);
	TEFileSectorsList::iterator GetSectorInPosition2(const size_file_t& position, size_file_t& offset_in_sector);
	const size_file_t& GetOffsetInSector();

private:
	size_file_t m_position;
	TEFileSectorsList::iterator m_sector;
	size_file_t m_offset_in_sector;
	ElasticFile& m_file;
};


