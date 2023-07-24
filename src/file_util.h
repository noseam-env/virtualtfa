#pragma once

#include <filesystem>

void setFileMetadata(const char *filepath, std::filesystem::perms mode, std::uint64_t ctime, std::uint64_t mtime);
