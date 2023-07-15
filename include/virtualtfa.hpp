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
    ~VirtualTfaWriter();

    std::size_t writeTo(char *buffer, std::size_t bufferSize);

    std::size_t calcSize();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

class VirtualTfaReader {
public:
    VirtualTfaReader(std::filesystem::path destDir, IProgressListener *listener);
    ~VirtualTfaReader();

    std::size_t addReadData(char *buffer, std::size_t bufferSize);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

VirtualTfaArchive *virtual_tfa_archive_new();

VirtualTfaEntry *virtual_tfa_entry_new(const std::filesystem::path &filePath);

void virtual_tfa_archive_add(VirtualTfaArchive *archive, VirtualTfaEntry *entry);

void virtual_tfa_archive_close(VirtualTfaArchive *archive);

#endif //VIRTUALTFA_VIRTUALTFA_HPP
