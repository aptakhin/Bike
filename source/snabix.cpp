// snabix
//

#include "../include/bike/snabix.h"

namespace bike {

FileWriter::FileWriter(const char* filename) {
	fout_ = fopen(filename, "wb");
}

void FileWriter::write(void* buf, size_t size) {
	fwrite(buf, 1, size, fout_);
}

FileWriter::~FileWriter() {
	fclose(fout_);
}

} // namespace bike {