#include "file_util.h"

#if defined(_WIN32)

#include <Windows.h>

FILETIME* virtualtfa_util_convert_filetime(tfa_utime_t time) {
    const tfa_utime_t ticksPerSecond = 10000000ULL;
    tfa_utime_t fileTimeTicks = time * ticksPerSecond;

    const tfa_utime_t epochOffset = 116444736000000000ULL;
    tfa_utime_t fileTimeValue = fileTimeTicks + epochOffset;

    FILETIME* fileTime = (FILETIME*)malloc(sizeof(FILETIME));
    fileTime->dwLowDateTime = (DWORD)(fileTimeValue);
    fileTime->dwHighDateTime = (DWORD)(fileTimeValue >> 32);

    return fileTime;
}

void virtualtfa_util_set_file_metadata(const char *filepath, tfa_mode_t mode, tfa_utime_t ctime, tfa_utime_t mtime) {
    HANDLE fileHandle = CreateFileA(filepath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "set_file_metadata: error opening file");
        return;
    }

    if (!SetFileTime(fileHandle, virtualtfa_util_convert_filetime(ctime), NULL, virtualtfa_util_convert_filetime(mtime))) {
        fprintf(stderr, "set_file_metadata: error setting file time");
        CloseHandle(fileHandle);
        return;
    }

    CloseHandle(fileHandle);
}

#elif defined(__linux__)

void virtualtfa_util_set_file_metadata(const char *filepath, tfa_mode_t mode, tfa_utime_t ctime, tfa_utime_t mtime) {
    // TODO: implement
}

#elif defined(__APPLE__)

void virtualtfa_util_set_file_metadata(const char *filepath, tfa_mode_t mode, tfa_utime_t ctime, tfa_utime_t mtime) {
    // TODO: implement
}

#endif
