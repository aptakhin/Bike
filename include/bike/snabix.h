// snabix is about bin and sax
//

#include <cstring>
#include <stdio.h>

namespace bike {

class IWriter {
public:
	virtual void write(void* buf, size_t size) = 0;
};

class IReader {
public:
	virtual size_t read(void* buf, size_t size) = 0;
};

template <size_t BufSize>
class BufferedWriter : public IWriter
{
public:
	BufferedWriter(IWriter* backend)
	:	backend_(backend),
		size_(0) {}

	void write(void* buf, size_t size) /* override */ {
		char* ptr = reinterpret_cast<char*>(buf);
		while (size > 0) {
			size_t write_bytes = std::min(size, BufSize - size_);
			memcpy(buf_ + size_, ptr, write_bytes);
			size_ += write_bytes;
			ptr   += write_bytes;
			size  -= write_bytes;
			if (size_ == BufSize)
				flush();
		}
	}

	~BufferedWriter() {
		flush();
	}

	void flush() {
		backend_->write(buf, size_);
		size_ = 0;
	}
	
protected:
	IWriter* backend_;
	char buf_[BufSize];
	size_t size_;
};

class FileWriter : public IWriter
{
public:
	FileWriter(const char* filename);

	void write(void* buf, size_t size) /* override */;

	~FileWriter();

protected:
	FILE* fout_;
};



} // namespace bike {