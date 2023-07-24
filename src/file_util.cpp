#include "file_util.h"

#include <iostream>

#if defined(_WIN32)

#include <Windows.h>

FILETIME* convertTimeToFILETIME(const std::uint64_t& time) {
    const std::uint64_t ticksPerSecond = 10000000ULL;
    std::uint64_t fileTimeTicks = time * ticksPerSecond;

    const std::uint64_t epochOffset = 116444736000000000ULL;
    std::uint64_t fileTimeValue = fileTimeTicks + epochOffset;

    auto* fileTime = new FILETIME;
    fileTime->dwLowDateTime = static_cast<DWORD>(fileTimeValue);
    fileTime->dwHighDateTime = static_cast<DWORD>(fileTimeValue >> 32);

    return fileTime;
}

// not working, ill try NtSetInformationFile next time
void setFileMetadata(const char *filepath, std::filesystem::perms mode, std::uint64_t ctime, std::uint64_t mtime) {
    HANDLE fileHandle = CreateFileA(filepath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "setFileMetadata: Error opening file" << std::endl;
        return;
    }

    if (!SetFileTime(fileHandle, convertTimeToFILETIME(ctime), nullptr, convertTimeToFILETIME(mtime))) {
        std::cerr << "setFileMetadata: Error setting file time" << std::endl;
        CloseHandle(fileHandle);
        return;
    }

    CloseHandle(fileHandle);

    /*DWORD attributes = GetFileAttributesA(filepath);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        std::cout << "Cannot get file attributes" << std::endl;
        return;
    }

    if ((mode & std::filesystem::perms::owner_read) != std::filesystem::perms::none) {
        attributes &= ~FILE_ATTRIBUTE_READONLY;
    } else {
        attributes |= FILE_ATTRIBUTE_READONLY;
    }

    if ((mode & std::filesystem::perms::owner_write) != std::filesystem::perms::none) {
        attributes &= ~FILE_ATTRIBUTE_READONLY;
    }

    if ((mode & std::filesystem::perms::group_read) != std::filesystem::perms::none) {
        // Setting hidden file attribute
        attributes |= FILE_ATTRIBUTE_HIDDEN;
    } else {
        attributes &= ~FILE_ATTRIBUTE_HIDDEN;
    }

    if ((mode & std::filesystem::perms::others_read) != std::filesystem::perms::none) {
        // Setting system file attribute
        attributes |= FILE_ATTRIBUTE_SYSTEM;
    } else {
        attributes &= ~FILE_ATTRIBUTE_SYSTEM;
    }

    if (!SetFileAttributesA(filepath, attributes)) {
        std::cout << "Cannot set file permissions" << std::endl;
        return;
    }*/
}

#elif defined(__linux__)

void setFileMetadata(const char *filepath, std::filesystem::perms mode, std::uint64_t ctime, std::uint64_t mtime) {
    // TODO: implement
}

#elif defined(__APPLE__)

void setFileMetadata(const char *filepath, std::filesystem::perms mode, std::uint64_t ctime, std::uint64_t mtime) {
    // TODO: implement
}

#endif
