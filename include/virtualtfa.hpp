#ifndef VIRTUALTFA_VIRTUALTFA_HPP
#define VIRTUALTFA_VIRTUALTFA_HPP

#include <functional>

struct TfaHeader;
struct VirtualTfaArchive;
struct VirtualTfaEntry;

class IProgressListener {
public:
    virtual ~IProgressListener() = default;

    virtual void totalProgress(std::size_t currentSize) = 0;
    virtual void fileStart(char *fileName, std::size_t fileSize) = 0;
    virtual void fileProgress(char *fileName, std::size_t fileSize, std::size_t currentSize) = 0;
    virtual void fileEnd(char *fileName, std::size_t fileSize) = 0;
};

class VirtualTfaWriter {
public:
    VirtualTfaWriter(VirtualTfaArchive *archive, IProgressListener *listener);

    std::size_t writeTo(char *buffer, std::size_t bufferSize);

    std::size_t calcSize();

private:
    VirtualTfaArchive *m_archive;
    IProgressListener *m_listener;

    std::size_t m_pointer = 0;
};

class VirtualTfaReader {
public:
    VirtualTfaReader(std::filesystem::path destDir, IProgressListener *listener);

    std::size_t addReadData(char *buffer, std::size_t bufferSize);

private:
    std::filesystem::path m_destDir;
    IProgressListener *m_listener;

    char *m_cur_headerBuf;
    std::filesystem::perms m_cur_h_mode;
    std::time_t m_cur_h_ctime = 0;
    std::time_t m_cur_h_mtime = 0;
    std::size_t m_cur_h_namesize = 0;
    std::size_t m_cur_h_filesize = 0;
    char *m_cur_name = nullptr;
    char *m_cur_FILEname = nullptr;
    FILE *m_cur_FILE = nullptr;
    std::size_t m_cur_remainHeaderSize;
    std::size_t m_cur_remainNameSize = 0;
    std::size_t m_cur_remainFileSize = 0;
    std::size_t m_totalRead = 0;
};

VirtualTfaArchive *virtual_tfa_archive_new();

VirtualTfaEntry *virtual_tfa_entry_new(const std::filesystem::path &filePath);

void virtual_tfa_archive_add(VirtualTfaArchive *archive, VirtualTfaEntry *entry);

void virtual_tfa_archive_close(VirtualTfaArchive *archive);

#endif //VIRTUALTFA_VIRTUALTFA_HPP
