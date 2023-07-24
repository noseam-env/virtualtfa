#ifndef VIRTUALTFA_VIRTUALTFA_HPP
#define VIRTUALTFA_VIRTUALTFA_HPP

#include <functional>

struct TfaHeader;
struct VirtualTfaArchive;
struct VirtualTfaEntry;

class IProgressListener {
public:
    virtual ~IProgressListener() = default;

    virtual void totalProgress(std::uint64_t currentSize) = 0;
    virtual void fileStart(char *fileName, std::uint64_t fileSize) = 0;
    virtual void fileProgress(char *fileName, std::uint64_t fileSize, std::uint64_t currentSize) = 0;
    virtual void fileEnd(char *fileName, std::uint64_t fileSize) = 0;
};

class VirtualTfaWriter {
public:
    VirtualTfaWriter(VirtualTfaArchive *archive, IProgressListener *listener);
    ~VirtualTfaWriter();

    std::uint64_t writeTo(char *buffer, std::uint64_t bufferSize);
    std::uint64_t calcSize();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

class VirtualTfaReader {
public:
    VirtualTfaReader(std::filesystem::path destDir, IProgressListener *listener);
    ~VirtualTfaReader();

    std::uint64_t addReadData(char *buffer, std::uint64_t bufferSize);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

class VirtualFile {
public:
    virtual ~VirtualFile() = default;
    [[nodiscard]] virtual std::string getRelativePath() const = 0;
    [[nodiscard]] virtual std::uint64_t getSize() const = 0;
    [[nodiscard]] virtual std::uint64_t getCreatedTime() const = 0; // UNIX time
    [[nodiscard]] virtual std::uint64_t getModifiedTime() const = 0; // UNIX time
    [[nodiscard]] virtual std::filesystem::file_status getStatus() const = 0;
    virtual void seek(std::uint64_t pos) = 0;
    virtual std::uint64_t read(char* buffer, std::uint64_t count) = 0;
};

VirtualTfaArchive *virtual_tfa_archive_new();

VirtualTfaEntry *virtual_tfa_entry_new(VirtualFile &virtualFile);

void virtual_tfa_archive_add(VirtualTfaArchive *archive, VirtualTfaEntry *entry);

void virtual_tfa_archive_close(VirtualTfaArchive *archive);

#endif //VIRTUALTFA_VIRTUALTFA_HPP
