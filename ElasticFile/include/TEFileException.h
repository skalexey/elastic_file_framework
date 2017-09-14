#define EXCEPTION_PREFIX "EFileException: "

class TEFileException : public std::exception
{
public: 
	TEFileException(int error_code, const std::string& str, size_file_t data = 0)throw()
		: m_error_message(str)
		, m_error_code(error_code)
		, m_data(data)

	{
	}

	virtual ~TEFileException()throw()
	{
	}

	virtual const char* what()throw()
	{
		return m_error_message.c_str();
	}

	int error()
	{
		return m_error_code;
	}

	size_file_t data()
	{
		return m_data;
	}

private: 
	std::string m_error_message;
	int m_error_code;
	size_file_t m_data;
};