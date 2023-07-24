#include <fstream>
#include <utility>
#include <vector>
#include <filesystem>
#include <bitset>
#include <string>
#include <sstream>
#include <cstring>
#include <iostream>
#include "virtualtfa.hpp"
#include "file_util.h"

struct TfaHeader {
    char magic[4];
    char version;
    char typeflag;
    char unused[4]; // reserved
    char mode[8];
    char ctime[8];
    char mtime[8];
    char namesize[6];
    char filesize[8];
};

struct VirtualTfaArchive {
    std::vector<VirtualTfaEntry *> entries;
};

struct VirtualTfaEntry {
    TfaHeader header;
    std::uint64_t nameSize;
    std::uint64_t fileSize;
    std::string name;
    VirtualFile *file;
};

static std::uint64_t headerSize = sizeof(TfaHeader); // 48

/*struct ParsedTfaHeader {
    char typeflag;
    std::uint16_t mode;
    std::time_t ctime;
    std::time_t mtime;
    std::uint64_t namesize;
    std::uint64_t filesize;
};*/

std::ptrdiff_t calc_part_written_bytes(std::uint64_t pointer, std::uint64_t partStartPos, std::uint64_t partSize) {
    //
    // where is | this is the beginning of the part (the part is considered to be the 'header', 'name', 'file')
    // (not binary, just for example)
    //
    //                      ↓   part_size   ↓
    // totalSize:    1 1 0 1|1 1 0 1 0 1 1 1|0 0 1 1 0|1 1 0 1
    // pointer:      1 1 0 1|1 1 0
    // partStartPos: 1 1 0 1|
    //
    std::uint64_t partEndPos = partStartPos + partSize;
    if (pointer >= partEndPos) { // skip this part
        return -1;
    }
    std::uint64_t partWrittenBytes = pointer - partStartPos;
    return static_cast<std::ptrdiff_t>(partWrittenBytes);
}

std::uint64_t calc_bytes_to_write(std::uint64_t partSize, std::uint64_t partWrittenBytes, std::uint64_t bufferSizeLeft) {
    /*std::uint64_t toWrite = partSize - partWrittenBytes;
    if (toWrite > bufferSizeLeft) {
        toWrite = bufferSizeLeft;
    }
    return toWrite;*/
    return std::min(partSize - partWrittenBytes, bufferSizeLeft);
}

class VirtualTfaWriter::Impl {
public:
    Impl(VirtualTfaArchive *archive, IProgressListener *listener) : m_archive(archive), m_listener(listener) {}
    ~Impl() = default;

    std::uint64_t writeTo(char *buffer, std::uint64_t bufferSize) {
        std::uint64_t partStartPos = 0;
        std::uint64_t writtenBytes = 0;
        for (const VirtualTfaEntry *entry: m_archive->entries) {
            std::uint64_t bufferSizeLeft;
            std::uint64_t partSize;
            std::ptrdiff_t partWrittenBytes;

            bufferSizeLeft = bufferSize - writtenBytes;
            partSize = headerSize;
            partWrittenBytes = calc_part_written_bytes(m_pointer, partStartPos, partSize);
            if (partWrittenBytes != -1) {
                std::uint64_t toWrite = calc_bytes_to_write(partSize, partWrittenBytes, bufferSizeLeft);
                TfaHeader header = entry->header;
                char *headerBufferPtr = reinterpret_cast<char *>(&header) + partWrittenBytes; // offset
                std::copy(headerBufferPtr, headerBufferPtr + toWrite, buffer + writtenBytes);
                writtenBytes += toWrite;
                m_pointer += toWrite;
                if (writtenBytes == bufferSize) break;
            }
            partStartPos += partSize;

            bufferSizeLeft = bufferSize - writtenBytes;
            partSize = entry->nameSize;
            partWrittenBytes = calc_part_written_bytes(m_pointer, partStartPos, partSize);
            if (partWrittenBytes != -1) {
                std::uint64_t toWrite = calc_bytes_to_write(partSize, partWrittenBytes, bufferSizeLeft);
                const char *name = entry->name.c_str();
                const char *nameBufferPtr = name + partWrittenBytes; // offset
                std::copy(nameBufferPtr, nameBufferPtr + toWrite - 1, buffer + writtenBytes);
                buffer[writtenBytes + toWrite - 1] = '\0';
                writtenBytes += toWrite;
                m_pointer += toWrite;
                if (writtenBytes == bufferSize) break;
            }
            partStartPos += partSize;

            bufferSizeLeft = bufferSize - writtenBytes;
            partSize = entry->fileSize;
            partWrittenBytes = calc_part_written_bytes(m_pointer, partStartPos, partSize);
            if (partWrittenBytes != -1) {
                if (partWrittenBytes == 0 && m_listener != nullptr) {
                    m_listener->fileStart(const_cast<char *>(entry->name.c_str()), entry->fileSize);
                }
                std::uint64_t toWrite = calc_bytes_to_write(partSize, partWrittenBytes, bufferSizeLeft);
                //fseek(entry->file, partWrittenBytes, SEEK_SET); // this line breaks all, idk why.
                std::uint64_t bytesRead = entry->file->read(buffer + writtenBytes, toWrite);
                writtenBytes += toWrite;
                m_pointer += toWrite;
                if (m_listener != nullptr) {
                    m_listener->fileProgress(const_cast<char *>(entry->name.c_str()), entry->fileSize, partWrittenBytes + toWrite);
                    if (partWrittenBytes + toWrite == partSize) {
                        m_listener->fileEnd(const_cast<char *>(entry->name.c_str()), entry->fileSize);
                    }
                }
                if (writtenBytes == bufferSize) break;
            }
            partStartPos += partSize;
        }
        if (m_listener != nullptr) {
            m_listener->totalProgress(m_pointer);
        }
        return writtenBytes;
    }

    std::uint64_t calcSize() {
        std::uint64_t size = 0;
        for (const VirtualTfaEntry *entry: m_archive->entries) {
            size += headerSize;
            size += entry->nameSize;
            size += entry->fileSize;
        }
        return size;
    }

private:
    VirtualTfaArchive *m_archive;
    IProgressListener *m_listener;

    std::uint64_t m_pointer = 0;
};

VirtualTfaWriter::VirtualTfaWriter(VirtualTfaArchive *archive, IProgressListener *listener) : pImpl(new Impl(archive, listener)) {}

VirtualTfaWriter::~VirtualTfaWriter() = default;

std::uint64_t VirtualTfaWriter::writeTo(char *buffer, std::uint64_t bufferSize) {
    return pImpl->writeTo(buffer, bufferSize);
}

std::uint64_t VirtualTfaWriter::calcSize() {
    return pImpl->calcSize();
}

bool isPathValid(const std::string& path) {
    std::filesystem::path normalizedPath = std::filesystem::weakly_canonical(path);
    return std::all_of(normalizedPath.begin(), normalizedPath.end(), [](const auto& part) {
        return part != "." && part != "..";
    });
}

std::uint64_t deserializeUint64(const char buffer[8]) {
    std::uint64_t value;
    std::memcpy(&value, buffer, sizeof(value));
    return value;
}

class VirtualTfaReader::Impl {
public:
    Impl(std::filesystem::path destDir, IProgressListener *listener) : m_destDir(std::move(destDir)), m_listener(listener) {}
    ~Impl() = default;

    std::uint64_t addReadData(char *buffer, std::uint64_t bufferSize) {
        std::uint64_t readBytes = 0;
        while (readBytes < bufferSize) {
            std::uint64_t bufferSizeLeft;

            bufferSizeLeft = bufferSize - readBytes;
            if (m_cur_remainHeaderSize > 0) {
                std::uint64_t offset = headerSize - m_cur_remainHeaderSize;

                std::uint64_t toRead = std::min(m_cur_remainHeaderSize, bufferSizeLeft);

                std::copy(buffer + readBytes, buffer + readBytes + toRead, m_cur_headerBuf + offset);
                m_cur_remainHeaderSize -= toRead;
                readBytes += toRead;

                if (m_cur_remainHeaderSize == 0) {
                    //std::cout << "Header buffered, reading ..." << std::endl;

                    TfaHeader header = {};
                    std::uint64_t deserializeOffset = 0;

                    std::copy(m_cur_headerBuf + deserializeOffset, m_cur_headerBuf + deserializeOffset + sizeof(header.magic), header.magic);
                    deserializeOffset += sizeof(header.magic);
                    header.version = m_cur_headerBuf[deserializeOffset];
                    deserializeOffset++;
                    header.typeflag = m_cur_headerBuf[deserializeOffset];
                    deserializeOffset++;
                    std::copy(m_cur_headerBuf + deserializeOffset, m_cur_headerBuf + deserializeOffset + sizeof(header.unused), header.unused);
                    deserializeOffset += sizeof(header.unused);
                    std::copy(m_cur_headerBuf + deserializeOffset, m_cur_headerBuf + deserializeOffset + sizeof(header.mode), header.mode);
                    deserializeOffset += sizeof(header.mode);
                    std::copy(m_cur_headerBuf + deserializeOffset, m_cur_headerBuf + deserializeOffset + sizeof(header.ctime), header.ctime);
                    deserializeOffset += sizeof(header.ctime);
                    std::copy(m_cur_headerBuf + deserializeOffset, m_cur_headerBuf + deserializeOffset + sizeof(header.mtime), header.mtime);
                    deserializeOffset += sizeof(header.mtime);
                    std::copy(m_cur_headerBuf + deserializeOffset, m_cur_headerBuf + deserializeOffset + sizeof(header.namesize), header.namesize);
                    deserializeOffset += sizeof(header.namesize);
                    std::copy(m_cur_headerBuf + deserializeOffset, m_cur_headerBuf + deserializeOffset + sizeof(header.filesize), header.filesize);

                    std::bitset<9> bits(header.mode);
                    m_cur_h_mode = static_cast<std::filesystem::perms>(bits.to_ulong());

                    std::uint64_t ctime = deserializeUint64(header.ctime);
                    std::uint64_t mtime = deserializeUint64(header.mtime);

                    m_cur_remainNameSize = m_cur_h_namesize = std::stoull(header.namesize);
                    m_cur_remainFileSize = m_cur_h_filesize = deserializeUint64(header.filesize);

                    m_cur_name = new char[m_cur_h_namesize];

                    //std::cout << "Header readed" << std::endl;
                }
                if (readBytes == bufferSize) break;
            }

            bufferSizeLeft = bufferSize - readBytes;
            if (m_cur_remainNameSize > 0) {
                std::uint64_t offset = m_cur_h_namesize - m_cur_remainNameSize;

                std::uint64_t toRead = std::min(m_cur_remainNameSize, bufferSizeLeft);

                std::copy(buffer + readBytes, buffer + readBytes + toRead, m_cur_name + offset);
                m_cur_remainNameSize -= toRead;
                readBytes += toRead;

                if (m_cur_remainNameSize == 0) {
                    if (!isPathValid(m_cur_name)) {
                        break;
                    }
                    if (m_listener != nullptr) {
                        m_listener->fileStart(m_cur_name, m_cur_h_filesize);
                    }
                    //std::cout << "Reading " << m_cur_name << " ..." << std::endl;
                }
                if (readBytes == bufferSize) break;
            }

            bufferSizeLeft = bufferSize - readBytes;
            if (m_cur_remainFileSize > 0) {
                if (m_cur_ofs != nullptr && m_cur_ofs_name != nullptr && std::strcmp(m_cur_ofs_name, m_cur_name) != 0) {
                    m_cur_ofs->close();
                    m_cur_ofs = nullptr;
                }
                if (m_cur_ofs == nullptr) {
                    std::string filepath = (m_destDir / m_cur_name).string();
                    m_cur_ofs = new std::ofstream(filepath, std::ios::out | std::ios::binary);
                    m_cur_ofs_name = m_cur_name;
                }
                //std::uint64_t offset = m_cur_h_filesize - m_cur_remainFileSize;
                //fseek(m_cur_ofs, offset, SEEK_SET); // causes bugs
                std::uint64_t toRead = std::min(m_cur_remainFileSize, bufferSizeLeft);
                m_cur_ofs->write(buffer + readBytes, static_cast<std::streamsize>(toRead));
                m_cur_remainFileSize -= toRead;
                readBytes += toRead;
                if (m_listener != nullptr) {
                    m_listener->fileProgress(m_cur_name, m_cur_h_filesize, m_cur_h_filesize - m_cur_remainFileSize);
                }
                if (m_cur_remainFileSize == 0) {
                    m_cur_ofs->close();
                    m_cur_ofs_name = nullptr;
                    m_cur_ofs = nullptr;

                    std::string filepath = (m_destDir / m_cur_name).string();
                    setFileMetadata(filepath.c_str(), m_cur_h_mode, m_cur_h_ctime, m_cur_h_mtime);

                    m_cur_remainHeaderSize = headerSize;

                    if (m_listener != nullptr) {
                        m_listener->fileEnd(m_cur_name, m_cur_h_filesize);
                    }
                }
                if (readBytes == bufferSize) break;
            }
        }

        m_totalRead += readBytes;
        if (m_listener != nullptr) {
            m_listener->totalProgress(m_totalRead);
        }

        return readBytes;
    }

private:
    std::filesystem::path m_destDir;
    IProgressListener *m_listener;

    char *m_cur_headerBuf = new char[headerSize];
    std::filesystem::perms m_cur_h_mode{};
    std::uint64_t m_cur_h_ctime = 0;
    std::uint64_t m_cur_h_mtime = 0;
    std::uint64_t m_cur_h_namesize = 0;
    std::uint64_t m_cur_h_filesize = 0;
    char *m_cur_name = nullptr;
    char *m_cur_ofs_name = nullptr;
    std::ofstream *m_cur_ofs = nullptr;
    std::uint64_t m_cur_remainHeaderSize = headerSize;
    std::uint64_t m_cur_remainNameSize = 0;
    std::uint64_t m_cur_remainFileSize = 0;
    std::uint64_t m_totalRead = 0;
};

VirtualTfaReader::VirtualTfaReader(std::filesystem::path destDir, IProgressListener *listener) : pImpl(new Impl(std::move(destDir), listener)) {}

VirtualTfaReader::~VirtualTfaReader() = default;

std::uint64_t VirtualTfaReader::addReadData(char *buffer, std::uint64_t bufferSize) {
    return pImpl->addReadData(buffer, bufferSize);
}

VirtualTfaArchive *virtual_tfa_archive_new() {
    return new VirtualTfaArchive({std::vector<VirtualTfaEntry *>()});
}

VirtualTfaEntry *virtual_tfa_entry_new(VirtualFile &virtualFile) {
    std::filesystem::file_status fileStatus = virtualFile.getStatus();
    if (!exists(fileStatus)) {
        throw std::runtime_error("File not exists");
    }

    TfaHeader header{};

    // magic
    std::string magicNumber = "tfa";
    std::memset(header.magic, 0, sizeof(header.magic));
    std::memcpy(header.magic, magicNumber.c_str(), std::min(sizeof(header.magic) - 1, magicNumber.length()));
    header.magic[sizeof(header.magic) - 1] = '\0';

    header.version = 0;

    // typeflag
    switch (fileStatus.type()) {
        case std::filesystem::file_type::regular:
            header.typeflag = 0;
            break;
        case std::filesystem::file_type::directory:
            header.typeflag = 1;
            break;
        default:
            throw std::runtime_error("Unsupported file type");
    }

    // mode
    std::filesystem::perms permissions = fileStatus.permissions();
    std::string permissionsString = std::bitset<9>(static_cast<unsigned int>(permissions)).to_string();
    std::copy_n(permissionsString.c_str(), sizeof(header.mode) - 1, header.mode);
    //strncpy_s(header.mode, sizeof(header.mode), permissionsString.c_str(), sizeof(header.mode) - 1);
    header.mode[sizeof(header.mode) - 1] = '\0';

    // createdTime & modifiedTime
    std::uint64_t ctime = virtualFile.getCreatedTime();
    std::uint64_t mtime = virtualFile.getModifiedTime();
    std::memcpy(header.ctime, &ctime, sizeof(ctime));
    std::memcpy(header.mtime, &mtime, sizeof(mtime));

    // namesize
    std::string relativePath = virtualFile.getRelativePath();
    std::uint64_t nameSize = relativePath.length() + 1;
    std::string nameSizeString = std::to_string(nameSize);
    if (nameSizeString.length() > std::size(header.namesize)) {
        throw std::runtime_error("File name is too large to fit in header");
    }
    std::memset(header.namesize, ' ', sizeof(header.namesize));
    std::memcpy(header.namesize + sizeof(header.namesize) - nameSizeString.length(), nameSizeString.c_str(),
                nameSizeString.length());

    // filesize
    std::uint64_t fileSize = virtualFile.getSize();
    std::memcpy(header.filesize, &fileSize, sizeof(fileSize));

    return new VirtualTfaEntry({header, nameSize, fileSize, relativePath, &virtualFile});
}

void virtual_tfa_archive_add(VirtualTfaArchive *archive, VirtualTfaEntry *entry) {
    archive->entries.push_back(entry);
}

void virtual_tfa_archive_close(VirtualTfaArchive *archive) {
    for (const VirtualTfaEntry *entry: archive->entries) {
        //fclose(entry->file);
        delete entry;
    }
    delete archive;
}
