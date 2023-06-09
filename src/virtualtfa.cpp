#include <fstream>
#include <utility>
#include <vector>
#include <filesystem>
#include <bitset>
#include <string>
#include <sstream>
#include "virtualtfa.hpp"
#include "file_util.hpp"

#if defined(ANDROID)
#include "sys/stat.h"
#endif

struct TfaHeader {
    char magic[4];
    char version;
    char typeflag;
    char unused[4]; // reserved
    char mode[8];
    char ctime[12];
    char mtime[12];
    char namesize[6];
    char filesize[12];
};

struct VirtualTfaArchive {
    std::vector<VirtualTfaEntry *> entries;
};

struct VirtualTfaEntry {
    TfaHeader header;
    std::size_t nameSize;
    std::size_t fileSize;
    std::string name;
    FILE *file;
};

static std::size_t headerSize = sizeof(TfaHeader); // 60

/*struct ParsedTfaHeader {
    char typeflag;
    std::uint16_t mode;
    std::time_t ctime;
    std::time_t mtime;
    std::size_t namesize;
    std::size_t filesize;
};*/

std::ptrdiff_t calc_part_written_bytes(std::size_t pointer, std::size_t partStartPos, std::size_t partSize) {
    //
    // where is | this is the beginning of the part (the part is considered to be the 'header', 'name', 'file')
    // (not binary, just for example)
    //
    //                      ↓   part_size   ↓
    // totalSize:    1 1 0 1|1 1 0 1 0 1 1 1|0 0 1 1 0|1 1 0 1
    // pointer:      1 1 0 1|1 1 0
    // partStartPos: 1 1 0 1|
    //
    std::size_t partEndPos = partStartPos + partSize;
    if (pointer >= partEndPos) { // skip this part
        return -1;
    }
    std::size_t partWrittenBytes = pointer - partStartPos;
    return static_cast<std::ptrdiff_t>(partWrittenBytes);
}

std::size_t calc_bytes_to_write(std::size_t partSize, std::size_t partWrittenBytes, std::size_t bufferSizeLeft) {
    /*std::size_t toWrite = partSize - partWrittenBytes;
    if (toWrite > bufferSizeLeft) {
        toWrite = bufferSizeLeft;
    }
    return toWrite;*/
    return std::min(partSize - partWrittenBytes, bufferSizeLeft);
}

VirtualTfaWriter::VirtualTfaWriter(VirtualTfaArchive *archive, IProgressListener *listener) : m_archive(archive), m_listener(listener) {}

std::size_t VirtualTfaWriter::writeTo(char *buffer, std::size_t bufferSize) {
    std::size_t partStartPos = 0;
    std::size_t writtenBytes = 0;
    for (const VirtualTfaEntry *entry: m_archive->entries) {
        std::size_t bufferSizeLeft;
        std::size_t partSize;
        std::ptrdiff_t partWrittenBytes;

        bufferSizeLeft = bufferSize - writtenBytes;
        partSize = headerSize;
        partWrittenBytes = calc_part_written_bytes(m_pointer, partStartPos, partSize);
        if (partWrittenBytes != -1) {
            std::size_t toWrite = calc_bytes_to_write(partSize, partWrittenBytes, bufferSizeLeft);
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
            std::size_t toWrite = calc_bytes_to_write(partSize, partWrittenBytes, bufferSizeLeft);
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
            std::size_t toWrite = calc_bytes_to_write(partSize, partWrittenBytes, bufferSizeLeft);
            //fseek(entry->file, partWrittenBytes, SEEK_SET); // this line breaks all, idk why.
            std::size_t bytesRead = fread(buffer + writtenBytes, 1, toWrite, entry->file);
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

std::size_t VirtualTfaWriter::calcSize() {
    std::size_t size = 0;
    for (const VirtualTfaEntry *entry: m_archive->entries) {
        size += headerSize;
        size += entry->nameSize;
        size += entry->fileSize;
    }
    return size;
}

bool isPathValid(const std::string& path) {
    std::filesystem::path normalizedPath = std::filesystem::weakly_canonical(path);
    return std::all_of(normalizedPath.begin(), normalizedPath.end(), [](const auto& part) {
        return part != "." && part != "..";
    });
}

VirtualTfaReader::VirtualTfaReader(std::filesystem::path destDir, IProgressListener *listener) : m_destDir(std::move(destDir)), m_listener(listener), m_cur_headerBuf(new char[headerSize]), m_cur_remainHeaderSize(headerSize) {}

std::size_t VirtualTfaReader::addReadData(char *buffer, std::size_t bufferSize) {
    std::size_t readBytes = 0;
    while (readBytes < bufferSize) {
        std::size_t bufferSizeLeft;

        bufferSizeLeft = bufferSize - readBytes;
        if (m_cur_remainHeaderSize > 0) {
            std::size_t offset = headerSize - m_cur_remainHeaderSize;

            std::size_t toRead = std::min(m_cur_remainHeaderSize, bufferSizeLeft);

            std::copy(buffer + readBytes, buffer + readBytes + toRead, m_cur_headerBuf + offset);
            m_cur_remainHeaderSize -= toRead;
            readBytes += toRead;

            if (m_cur_remainHeaderSize == 0) {
                //std::cout << "Header buffered, reading ..." << std::endl;

                TfaHeader header = {};
                std::size_t deserializeOffset = 0;

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

                unsigned long ctime_value = std::strtoul(header.ctime, nullptr, 8);
                m_cur_h_ctime = static_cast<time_t>(ctime_value);

                unsigned long mtime_value = std::strtoul(header.mtime, nullptr, 8);
                m_cur_h_mtime = static_cast<time_t>(mtime_value);

                m_cur_remainNameSize = m_cur_h_namesize = std::stoull(header.namesize);
                m_cur_remainFileSize = m_cur_h_filesize = std::stoull(header.filesize);

                m_cur_name = new char[m_cur_h_namesize];

                //std::cout << "Header readed" << std::endl;
            }
            if (readBytes == bufferSize) break;
        }

        bufferSizeLeft = bufferSize - readBytes;
        if (m_cur_remainNameSize > 0) {
            std::size_t offset = m_cur_h_namesize - m_cur_remainNameSize;

            std::size_t toRead = std::min(m_cur_remainNameSize, bufferSizeLeft);

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
            if (m_cur_FILE != nullptr && m_cur_FILEname != nullptr && std::strcmp(m_cur_FILEname, m_cur_name) != 0) {
                fclose(m_cur_FILE);
                m_cur_FILE = nullptr;
            }
            if (m_cur_FILE == nullptr) {
                std::string filepath = (m_destDir / m_cur_name).string();
                m_cur_FILE = fopen(filepath.c_str(), "wb");
                m_cur_FILEname = m_cur_name;
            }
            //std::size_t offset = m_cur_h_filesize - m_cur_remainFileSize;
            //fseek(m_cur_FILE, offset, SEEK_SET); // not required, but safe
            std::size_t toRead = std::min(m_cur_remainFileSize, bufferSizeLeft);
            fwrite(buffer + readBytes, 1, toRead, m_cur_FILE);
            m_cur_remainFileSize -= toRead;
            readBytes += toRead;
            if (m_listener != nullptr) {
                m_listener->fileProgress(m_cur_name, m_cur_h_filesize, m_cur_h_filesize - m_cur_remainFileSize);
            }
            if (m_cur_remainFileSize == 0) {
                fclose(m_cur_FILE);
                m_cur_FILEname = nullptr;
                m_cur_FILE = nullptr;

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

VirtualTfaArchive *virtual_tfa_archive_new() {
    return new VirtualTfaArchive({std::vector<VirtualTfaEntry *>()});
}

VirtualTfaEntry *virtual_tfa_entry_new(const std::filesystem::path &filePath) {
    std::filesystem::file_status fileStatus = std::filesystem::status(filePath);
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

    // ctime & mtime
    time_t *ctime = nullptr;
    time_t *mtime = nullptr;
#ifdef _WIN32
    struct _stat64 fileStat{};
    if (_stat64(filePath.string().c_str(), &fileStat) == 0) {
        ctime = &fileStat.st_ctime;
        mtime = &fileStat.st_mtime;
    }
#else
    struct stat fileStat{};
    if (stat(filePath.string().c_str(), &fileStat) == 0) {
        ctime = &fileStat.st_ctime;
        mtime = &fileStat.st_mtime;
    }
#endif

    if (ctime != nullptr) {
        std::sprintf(header.ctime, "%011o", static_cast<unsigned int>(*ctime));
    }
    if (mtime != nullptr) {
        std::sprintf(header.mtime, "%011o", static_cast<unsigned int>(*mtime));
    }

    // namesize
    std::string filename = filePath.filename().string();
    std::size_t nameSize = filename.length() + 1;
    std::string nameSizeString = std::to_string(nameSize);
    if (nameSizeString.length() > std::size(header.namesize)) {
        throw std::runtime_error("File name is too large to fit in header");
    }
    std::memset(header.namesize, ' ', sizeof(header.namesize));
    std::memcpy(header.namesize + sizeof(header.namesize) - nameSizeString.length(), nameSizeString.c_str(),
                nameSizeString.length());

    // filesize
    std::size_t fileSize = std::filesystem::file_size(filePath);
    std::string fileSizeString = std::to_string(fileSize);
    if (fileSizeString.length() > std::size(header.filesize)) {
        throw std::runtime_error("File size is too large to fit in header");
    }
    std::memset(header.filesize, ' ', sizeof(header.filesize));
    std::memcpy(header.filesize + sizeof(header.filesize) - fileSizeString.length(), fileSizeString.c_str(),
                fileSizeString.length());

    FILE *file = fopen(filePath.string().c_str(), "rb");
    return new VirtualTfaEntry({header, nameSize, fileSize, filename, file});
}

void virtual_tfa_archive_add(VirtualTfaArchive *archive, VirtualTfaEntry *entry) {
    archive->entries.push_back(entry);
}

void virtual_tfa_archive_close(VirtualTfaArchive *archive) {
    for (const VirtualTfaEntry *entry: archive->entries) {
        fclose(entry->file);
        delete entry;
    }
    delete archive;
}

/*int main(int argc, char *argv[]) {
    try {
        VirtualTfaArchive *archive = virtual_tfa_archive_new();

        for (int i = 1; i < argc; ++i) {
            char* argument = argv[i];
            VirtualTfaEntry *entry = virtual_tfa_entry_new(argument);
            virtual_tfa_archive_add(archive, entry);
        }

        VirtualTfaWriter tfaWriter = *new VirtualTfaWriter(archive, nullptr);
        std::cout << "size: " << std::to_string(tfaWriter.calcSize()) << std::endl;

        std::ofstream fileOut("output.tfa", std::ios::binary);
        if (fileOut.is_open()) {
            char buffer[8192 * 4];
            std::size_t bytesWritten = tfaWriter.writeTo(buffer, sizeof(buffer));
            while (bytesWritten > 0) {
                fileOut.write(buffer, bytesWritten);
                bytesWritten = tfaWriter.writeTo(buffer, sizeof(buffer));
            }

            fileOut.close();

            std::cout << "Wrote output.tfa" << std::endl;

            FILE *fileIn = fopen("output.tfa", "rb");
            if (fileIn) {
                VirtualTfaReader tfaReader = *new VirtualTfaReader("./output", nullptr);

                char buffer[8192 * 4];
                std::size_t bytesRead = fread(buffer, 1, sizeof(buffer), fileIn);
                while (bytesRead > 0) {
                    tfaReader.addReadData(buffer, bytesRead);
                    bytesRead = fread(buffer, 1, sizeof(buffer), fileIn);
                }
            } else {
                std::cout << "File is closed" << std::endl;
            }
            fclose(fileIn);
        } else {
            std::cout << "File is closed" << std::endl;
        }
    } catch (std::exception &ex) {
        std::cout << "Err: " << ex.what() << std::endl;
    }

    return 0;
}*/
