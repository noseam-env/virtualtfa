#include "file_util.h"

#include <iostream>

#if defined(_WIN32)

#include <windows.h>
#include <fileapi.h>
#include <handleapi.h>

FILETIME *convertTimeToFILETIME(const std::time_t &time) {
    // Convert std::time_t to __int64
    __int64 fileTimeValue = static_cast<__int64>(time) * 10000000 + 116444736000000000;

    // Create a FILETIME structure
    FILETIME *fileTime = new FILETIME;
    fileTime->dwLowDateTime = static_cast<DWORD>(fileTimeValue);
    fileTime->dwHighDateTime = static_cast<DWORD>(fileTimeValue >> 32);

    return fileTime;
}

void setFileMetadata(const char *filepath, std::filesystem::perms mode, std::time_t ctime, std::time_t mtime) {
    HANDLE fileHandle = CreateFileA(filepath, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        std::cout << "Cannot open file" << std::endl;
        return;
    }

    if (!SetFileTime(fileHandle, convertTimeToFILETIME(ctime), nullptr, convertTimeToFILETIME(mtime))) {
        std::cout << "Cannot update file metadata" << std::endl;
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

void setFileMetadata(const char *filepath, std::filesystem::perms mode, std::time_t ctime, std::time_t mtime) {
    // TODO: implement
}

#elif defined(__APPLE__)

void setFileMetadata(const char *filepath, std::filesystem::perms mode, std::time_t ctime, std::time_t mtime) {
    // TODO: implement
}

#endif
