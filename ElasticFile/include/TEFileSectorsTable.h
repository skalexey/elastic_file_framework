#pragma once
#include <efile_types.h>

class ElasticFile;
class TEFileSectorsTable
{
public:
	TEFileSectorsTable(ElasticFile& file);
	~TEFileSectorsTable();

	int Load();
	bool Write();
	void Allocate(size_file_t size_to_allocate, TEFileSectorsIterators& allocated_sectors_iterators);
	TEFileSectorsList::iterator AllocateNewSector(size_file_t size_to_allocate);

	size_file_t GetSectorsCount() const;
	const size_file_t& GetDataSize() const;
	const size_file_t& GetFileSize() const;
	TEFileSectorsList& List();
	TEFileSectorsMap& Map();
	TEFileSectorsMap& FreeSectors();

	void SetSectorFree(TEFileSectorsList::iterator sector, bool free);

	std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator> TruncateSector(TEFileSectorsList::iterator sector, size_file_t from, size_file_t truncation_size);
	void MoveSectorsTo(TEFileSectorsIterators& sectors_to_move, size_file_t position);
	void MoveSectorTo(TEFileSectorsList::iterator sector_it, size_file_t position);
	std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator> MoveSectorsBefore(TEFileSectorsIterators& sectors_to_move, TEFileSectorsList::iterator before_it);
	void MoveSectorsBefore(TEFileSectorsIterators& sectors_to_move, TEFileSectorsList::iterator before_it, bool);
	TUniteStatus CheckAndUniteSector(TEFileSectorsList::iterator sector_it, TUniteResult& unite_result);

	void ExtendSector(TEFileSectorsList::iterator sector_it, size_file_t size_to_extend);
	size_file_t CalculateGarbageSize();

	void Clear();

private:
	TEFileSectorsList::iterator UniteSectors(TEFileSectorsList::iterator first_it, TEFileSectorsList::iterator second_it);
	TUniteStatus CheckAndUniteFreeSector(TEFileSectorsList::iterator sector_it, TUniteResult& unite_result);
	TUniteStatus CheckAndUniteDataSector(TEFileSectorsList::iterator sector_it, TUniteResult& unite_result);
	void RemoveSector(TEFileSectorsList::iterator sector_it);
	void MoveSector(TEFileSectorsList::iterator sector_it, TEFileSectorsList::iterator before_it);
	
	TEFileSectorsList::iterator InsertSector(const TEFileSector& sector, TEFileSectorsList::iterator before);
	std::pair<TEFileSectorsList::iterator, TEFileSectorsList::iterator> SplitSector(TEFileSectorsList::iterator sector_it, size_file_t offset_in_sector);

	int Parse();
	void Create();
	
	TEFileSectorsList::iterator GetFirstFreeSector();
	size_file_t GetTableSize() const;
	size_file_t MinFileSize() const;


private:
	TEFileSectorsList m_sectors_list;
	TEFileSectorsMap m_sectors_map;
	TEFileSectorsCount m_sectors_count;
	size_file_t m_file_size;
	size_file_t m_data_size;
	ElasticFile& m_file;
	TEFileSectorsMap m_free_sectors_map;
};