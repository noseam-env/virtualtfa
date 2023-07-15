#pragma once

#include <filesystem>

void setFileMetadata(const char *filepath, std::filesystem::perms mode, std::time_t ctime, std::time_t mtime);
