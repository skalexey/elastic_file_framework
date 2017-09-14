#include <iostream>
#include <string>
#include <cstdlib>

#define LOG_INFO

#include <ElasticFileAPI.h>

static const char alphanum[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz";

int stringLength = sizeof(alphanum) - 1;

char genRandomChar()
{
	return alphanum[rand() % stringLength];
}

void genRandomString(size_file_t len, std::string& out)
{
	out.clear();
	out.resize(len);
	for(size_file_t i = 0; i != len; i++)
		out[i] = genRandomChar();
}

__int64 input_number()
{
	__int64 value;

	while(true)
	{
		std::cin >> value;
		if (std::cin.fail())
		{
			std::cin.clear();
			char c;
			std::cin >> c;
			continue;
		}

		if(value < 0)
			continue;

		break;
	}

	return value;
}

void test1(const std::string& fileName, size_file_t m_length, size_file_t N)
{
	// [m][m][m]...[m]

	INFO("Test 1. Write " << N << " records with length " << m_length);

	DWORD time = GetTickCount();
	DWORD gentime(0);
	DWORD local_time(0);
	TEFileHandle file = ElasticFileAPI::FileOpen(fileName, EF_MODE_CREATE | EF_MODE_APPEND);

	INFO("Time to open: " << GetTickCount() - time);
	
	time = GetTickCount();

	std::string m;
	for(size_file_t i = 0; i != N; i++)
	{
		local_time = GetTickCount();
		genRandomString(m_length, m);
		gentime += GetTickCount() - local_time;
		ElasticFileAPI::FileWrite(file, (PBYTE)m.c_str(), m_length);
	}

	INFO("Complete. Time to write: " << GetTickCount() - time - gentime << ". Time to generate random strings: " << gentime);
	INFO("Close file and save sectors table");

	time = GetTickCount();

	ElasticFileAPI::FileClose(file);

	INFO("Time passed: " << GetTickCount() - time << std::endl);
}

void test2(const std::string& fileName, size_file_t m_length, size_file_t M_length, size_file_t N)
{
	// [m][M][m][M][m][M]...[m][M]

	INFO("Test 2. Write " << N << " records with length " << M_length << " between previously added records");

	DWORD time = GetTickCount();

	INFO("Open file and load sectors table");

	TEFileHandle file = ElasticFileAPI::FileOpen(fileName, EF_MODE_OPEN);
	
	INFO("Time to open: " << GetTickCount() - time);
	
	DWORD write_time = GetTickCount();
	DWORD gentime(0);
	DWORD local_time(0);
	std::string M;
	ElasticFileAPI::FileSetCursor(file, 0, EF_CURSOR_BEGIN);
	for(size_file_t i = 0; i != N; i++)
	{
		local_time = GetTickCount();
		genRandomString(M_length, M);
		gentime += GetTickCount() - local_time;
		ElasticFileAPI::FileSetCursor(file, m_length, EF_CURSOR_CURRENT);
		ElasticFileAPI::FileWrite(file, (PBYTE)M.c_str(), M_length);
	}

	INFO("Complete. Time to write: " << GetTickCount() - time - gentime << ". Time to generate random strings: " << gentime);
	INFO("Close file and save sectors table");

	time = GetTickCount();

	ElasticFileAPI::FileClose(file);

	INFO("Time to close: " << GetTickCount() - time << std::endl);
}

void test3(const std::string& fileName, size_file_t M_length,  size_file_t m_length, size_file_t N)
{
	// Read every 10th record

	INFO("Test 3. Read evety 10th record and print the minimal (in alphabetical order) string");
	
	DWORD time(0);
	DWORD start_time = GetTickCount();

	INFO("Open file and load sectors table");

	TEFileHandle file = ElasticFileAPI::FileOpen(fileName, EF_MODE_OPEN);

	INFO("Time to open: " << GetTickCount() - start_time);

	start_time = GetTickCount();

	// Get file size
	ElasticFileAPI::FileSetCursor(file, 0, EF_CURSOR_END);
	size_file_t file_size = ElasticFileAPI::FileGetCursor(file);

	ElasticFileAPI::FileSetCursor(file, 0, EF_CURSOR_BEGIN);

	// Read
	std::string read_string;
	std::string max_string;
	size_file_t offset = m_length*5 + M_length*4;

	if(file_size < offset + M_length)
	{
		INFO("There are less then 10 records in the file");
		ElasticFileAPI::FileClose(file);
		return;
	}

	PBYTE buff = (PBYTE)malloc(M_length);
	while(true)
	{
		if(ElasticFileAPI::FileGetCursor(file) + offset + M_length >= file_size)
		{
			INFO("Last 10'th record has read");
			break;
		}

		ElasticFileAPI::FileSetCursor(file, offset, EF_CURSOR_CURRENT);

		size_file_t bytes_to_read = M_length;
		size_file_t bytes_read = ElasticFileAPI::FileRead(file, buff, bytes_to_read);
		if(bytes_read != bytes_to_read)
		{
			INFO("Can't read " << bytes_to_read << " bytes. " << bytes_read << " bytes read");
			break;
		}

		read_string.assign((char*)buff, M_length);

		time = GetTickCount();

		if(max_string.empty() || _strcmpi(read_string.c_str(), max_string.c_str()) > 0)
			max_string = read_string;
	}

	delete buff;

	DWORD time_to_read_and_compare = GetTickCount() - start_time;

	INFO( "Maximal string: " << std::endl << max_string.c_str() << std::endl );

	INFO("Complete. Time to read and compare: " << time_to_read_and_compare);
	INFO("Close file and destroy sectors table");

	time = GetTickCount();

	ElasticFileAPI::FileClose(file);

	INFO("Time to close: " << GetTickCount() - time << std::endl);
}

void test4(const std::string& fileName, size_file_t M_length,  size_file_t m_length, size_file_t N)
{
	// Delete every even pair
	INFO("Test 4. Delete every even pair");

	DWORD time = GetTickCount();

	INFO("Open file and load sectors table");

	TEFileHandle file = ElasticFileAPI::FileOpen(fileName, EF_MODE_OPEN);

	INFO("Time to open: " << GetTickCount() - time);

	time = GetTickCount();

	// Get file size
	ElasticFileAPI::FileSetCursor(file, 0, EF_CURSOR_END);
	size_file_t file_size = ElasticFileAPI::FileGetCursor(file);

	ElasticFileAPI::FileSetCursor(file, 0, EF_CURSOR_BEGIN);
	while(true)
	{
		size_file_t pair_size = m_length + M_length;
		if(ElasticFileAPI::FileGetCursor(file) + pair_size >= file_size)
		{
			INFO("Last even pair has deleted");
			break;
		}

		ElasticFileAPI::FileSetCursor(file, pair_size, EF_CURSOR_CURRENT);
		if(!ElasticFileAPI::FileTruncate(file, pair_size))
		{
			INFO("Can't truncate " << pair_size << " bytes");
			break;
		}
		file_size -= pair_size;

	}

	INFO("Complete. Time to delete: " << (time = GetTickCount() - time));
	INFO("Close file and save sectors table");
	
	time = GetTickCount();

	ElasticFileAPI::FileClose(file);

	INFO("Time to close: " << GetTickCount() - time << std::endl);
}

int main(int argc, void* argv[])
{
	std::string file_name("test_data_1");
	
	size_file_t m_length(0);
	size_file_t M_length(0);
	size_file_t N(0);
	
	std::cout << std::endl << " Enter 3 numbers (m, M and N)" << std::endl << std::endl;
	std::cout << " m: "; m_length = (size_file_t)input_number();
	std::cout << " M: "; M_length = (size_file_t)input_number();
	std::cout << " N: "; N = (size_file_t)input_number();
	std::cout << std::endl;

	test1(file_name, m_length, N);
	std::system("PAUSE");

	test2(file_name, m_length, M_length, N);
	std::system("PAUSE");

	test3(file_name, M_length, m_length, N);
	std::system("PAUSE");

	test4(file_name, M_length, m_length, N);
	std::system("PAUSE");

	return 0;
}

