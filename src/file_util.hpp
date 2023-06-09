#ifndef LIBFLOWDROP_FILE_UTIL_HPP
#define LIBFLOWDROP_FILE_UTIL_HPP

#include <filesystem>

void setFileMetadata(const char *filepath, std::filesystem::perms mode, std::time_t ctime, std::time_t mtime);

#endif //LIBFLOWDROP_FILE_UTIL_HPP
