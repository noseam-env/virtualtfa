#include "virtualtfa.h"

#include "file_util.h"

#include <stdlib.h>
#include <string.h>

typedef uint32_t tfa_namesize_t;

#define MIN(x, y) ((x) < (y) ? (x) : (y))

/*
 * Header
 */

typedef struct _tfa_header {
        char magic[4];
        char version;
        char typeflag;
        char unused[4]; // reserved
        char mode[4];
        char ctime[8];
        char mtime[8];
        char namesize[4];
        char filesize[8];
} tfa_header;

static tfa_size_t tfa_header_size = sizeof(tfa_header); // 42

/*
 * Input Stream
 */

struct _virtual_tfa_input_stream {
    virtual_tfa_read_function read_function;
    void *read_userdata;
    virtual_tfa_close_function close_function;
    void *close_userdata;
};

virtual_tfa_input_stream *virtual_tfa_input_stream_new(void) {
    virtual_tfa_input_stream *this = (virtual_tfa_input_stream *)malloc(sizeof(virtual_tfa_input_stream));
    if (this) {
        this->read_function = NULL;
        this->read_userdata = NULL;
        this->close_function = NULL;
        this->close_userdata = NULL;
    }
    return this;
}
void virtual_tfa_input_stream_free(virtual_tfa_input_stream *this) {
    if (this) {
        free(this);
    }
}

virtual_tfa_read_function virtual_tfa_input_stream_get_read_function(virtual_tfa_input_stream *this) {
    return this->read_function;
}
void virtual_tfa_input_stream_set_read_function(virtual_tfa_input_stream *this, virtual_tfa_read_function read_function) {
    this->read_function = read_function;
}
void *virtual_tfa_input_stream_get_read_userdata(virtual_tfa_input_stream *this) {
    return this->read_userdata;
}
void virtual_tfa_input_stream_set_read_userdata(virtual_tfa_input_stream *this, void *userdata) {
    this->read_userdata = userdata;
}
virtual_tfa_close_function virtual_tfa_input_stream_get_close_function(virtual_tfa_input_stream *this) {
    return this->close_function;
}
void virtual_tfa_input_stream_set_close_function(virtual_tfa_input_stream *this, virtual_tfa_close_function close_function) {
    this->close_function = close_function;
}
void *virtual_tfa_input_stream_get_close_userdata(virtual_tfa_input_stream *this) {
    return this->close_userdata;
}
void virtual_tfa_input_stream_set_close_userdata(virtual_tfa_input_stream *this, void *userdata) {
    this->close_userdata = userdata;
}
int virtual_tfa_input_stream_read(virtual_tfa_input_stream *this, char *buffer, tfa_size_t buffer_size, tfa_size_t *out_bytes_read) {
    tfa_size_t read_bytes = this->read_function(this->read_userdata, buffer, buffer_size);
    if (out_bytes_read) {
        *out_bytes_read = read_bytes;
    }
    return 0;
}
void virtual_tfa_input_stream_close(virtual_tfa_input_stream *this) {
    if (this->close_function) {
        this->close_function(this->close_userdata);
    }
}

/*
 * Entry
 */

struct _virtual_tfa_entry {
    const char                        *name;
    tfa_size_t                     size;
    virtual_tfa_input_stream_supplier  stream_supplier;
    void                              *stream_supplier_userdata;
    tfa_utime_t                        ctime;
    tfa_utime_t                        mtime;
    tfa_mode_t                         mode;
};

virtual_tfa_entry *virtual_tfa_entry_new(void) {
    virtual_tfa_entry *this = (virtual_tfa_entry *)malloc(sizeof(virtual_tfa_entry));
    if (this) {
        this->name = NULL;
        this->size = 0;
        this->stream_supplier = NULL;
        this->stream_supplier_userdata = NULL;
        this->ctime = 0;
        this->mtime = 0;
        this->mode = 0;
    }
    return this;
}
void virtual_tfa_entry_free(virtual_tfa_entry *this) {
    if (this) {
        free(this);
    }
}

const char *virtual_tfa_entry_get_name(virtual_tfa_entry *this) {
    return this->name;
}
void virtual_tfa_entry_set_name(virtual_tfa_entry *this, const char *name) {
    this->name = name;
}
tfa_size_t virtual_tfa_entry_get_size(virtual_tfa_entry *this) {
    return this->size;
}
void virtual_tfa_entry_set_size(virtual_tfa_entry *this, tfa_size_t size) {
    this->size = size;
}
virtual_tfa_input_stream_supplier virtual_tfa_entry_get_input_stream_supplier(virtual_tfa_entry *this) {
    return this->stream_supplier;
}
void virtual_tfa_entry_set_input_stream_supplier(virtual_tfa_entry *this, virtual_tfa_input_stream_supplier stream_supplier) {
    this->stream_supplier = stream_supplier;
}
void *virtual_tfa_entry_get_input_stream_supplier_userdata(virtual_tfa_entry *this) {
    return this->stream_supplier_userdata;
}
void virtual_tfa_entry_set_input_stream_supplier_userdata(virtual_tfa_entry *this, void *userdata) {
    this->stream_supplier_userdata = userdata;
}
tfa_utime_t virtual_tfa_entry_get_ctime(virtual_tfa_entry *this) {
    return this->ctime;
}
void virtual_tfa_entry_set_ctime(virtual_tfa_entry *this, tfa_utime_t ctime) {
    this->ctime = ctime;
}
tfa_utime_t virtual_tfa_entry_get_mtime(virtual_tfa_entry *this) {
    return this->mtime;
}
void virtual_tfa_entry_set_mtime(virtual_tfa_entry *this, tfa_utime_t mtime) {
    this->mtime = mtime;
}
tfa_mode_t virtual_tfa_entry_get_mode(virtual_tfa_entry *this) {
    return this->mode;
}
void virtual_tfa_entry_set_mode(virtual_tfa_entry *this, tfa_mode_t mode) {
    this->mode = mode;
}

/*
 * Archive
 */

struct _virtual_tfa_archive {
    virtual_tfa_entry **entries;
    size_t              entries_size;
};

virtual_tfa_archive *virtual_tfa_archive_new(void) {
    virtual_tfa_archive *this = (virtual_tfa_archive *)malloc(sizeof(virtual_tfa_archive));
    if (this) {
        this->entries = NULL;
        this->entries_size = 0;
    }
    return this;
}
void virtual_tfa_archive_free(virtual_tfa_archive *this) {
    if (this) {
        free(this);
    }
}

void virtual_tfa_archive_add(virtual_tfa_archive *this, virtual_tfa_entry *entry) {
    this->entries_size++;
    virtual_tfa_entry **new_entries = (virtual_tfa_entry **)realloc(this->entries, this->entries_size * sizeof(virtual_tfa_entry *));
    if (!new_entries) {
        fprintf(stderr, "virtual_tfa_archive_add: memory allocation failed\n");
        return;
    }
    this->entries = new_entries;
    this->entries[this->entries_size - 1] = entry;
}

/*
 * Writer
 */

int virtual_tfa_util_calc_part_write(tfa_size_t buffer_size_left,
                                     tfa_size_t pointer,
                                     tfa_size_t absolute_part_start_pos,
                                     tfa_size_t current_part_size,
                                     tfa_size_t *out_part_bytes_written,
                                     tfa_size_t *out_part_bytes_to_write) {
    //
    // where is | this is the beginning of the part (the part is considered to be the 'header', 'name', 'file')
    //
    //                                     | ← current_part_size → |
    // total_size:              35 7B 52 5C|E7 CE BF F8 C3 C4 B4 39|2F 9B 90 C7 E5|22 11 13 27
    // pointer:                 35 7B 52 5C|E7 CE BF
    // absolute_part_start_pos: 35 7B 52 5C|
    //
    tfa_size_t absolute_part_end_pos = absolute_part_start_pos + current_part_size;
    if (pointer >= absolute_part_end_pos) { // skip this part
        return 0;
    }
    tfa_size_t part_bytes_written = pointer - absolute_part_start_pos;
    *out_part_bytes_written = part_bytes_written;
    *out_part_bytes_to_write = MIN(current_part_size - part_bytes_written, buffer_size_left);
    return 1;
}

void virtual_tfa_util_setMagicNumber(tfa_header *header) {
    const char *magicNumber = "tfa";
    memset(header->magic, 0, sizeof(header->magic));
    strncpy_s(header->magic, sizeof(header->magic), magicNumber, sizeof(header->magic) - 1);
    header->magic[sizeof(header->magic) - 1] = '\0';
}

void virtual_tfa_util_setMode(tfa_header *header, tfa_mode_t mode) {
    memcpy(header->mode, &mode, sizeof(mode));
}

void virtual_tfa_util_setCTimeAndMTime(tfa_header *header, tfa_utime_t ctime, tfa_utime_t mtime) {
    memcpy(header->ctime, &ctime, sizeof(ctime));
    memcpy(header->mtime, &mtime, sizeof(mtime));
}

void virtual_tfa_util_setNameSize(tfa_header *header, tfa_namesize_t nameSize) {
    memcpy(header->namesize, &nameSize, sizeof(nameSize));
}

void virtual_tfa_util_setFileSize(tfa_header *header, tfa_size_t fileSize) {
    memcpy(header->filesize, &fileSize, sizeof(fileSize));
}

tfa_header *virtual_tfa_util_convert_entry_to_header(virtual_tfa_entry *entry) {
    tfa_header *header = (tfa_header *)malloc(sizeof(tfa_header));
    if (header) {
        virtual_tfa_util_setMagicNumber(header);
        virtual_tfa_util_setMode(header, entry->mode);
        virtual_tfa_util_setCTimeAndMTime(header, entry->ctime, entry->mtime);
        virtual_tfa_util_setNameSize(header, (tfa_namesize_t) strlen(entry->name) + 1); // last \0
        virtual_tfa_util_setFileSize(header, entry->size);
    }
    return header;
}

virtual_tfa_file_info *virtual_tfa_util_convert_entry_to_info(virtual_tfa_entry *entry) {
    virtual_tfa_file_info *fileInfo = (virtual_tfa_file_info *)malloc(sizeof(virtual_tfa_file_info));
    if (fileInfo) {
        fileInfo->name = entry->name;
        fileInfo->size = entry->size;
        fileInfo->ctime = entry->ctime;
        fileInfo->mtime = entry->mtime;
        fileInfo->mode = entry->mode;
    }
    return fileInfo;
}

struct _virtual_tfa_writer {
    virtual_tfa_archive      *archive;
    virtual_tfa_listener     *listener;
    tfa_size_t                pointer;
    tfa_header               *current_header;
    virtual_tfa_input_stream *current_stream;
};

virtual_tfa_writer *virtual_tfa_writer_new(void) {
    virtual_tfa_writer *this = (virtual_tfa_writer *)malloc(sizeof(virtual_tfa_writer));
    if (this) {
        this->archive = NULL;
        this->listener = NULL;
        this->pointer = 0;
        this->current_header = NULL;
        this->current_stream = NULL;
    }
    return this;
}
void virtual_tfa_writer_free(virtual_tfa_writer *this) {
    if (this) {
        free(this);
    }
}

virtual_tfa_archive *virtual_tfa_writer_get_archive(virtual_tfa_writer *this) {
    return this->archive;
}
void virtual_tfa_writer_set_archive(virtual_tfa_writer *this, virtual_tfa_archive *archive) {
    this->archive = archive;
}
virtual_tfa_listener *virtual_tfa_writer_get_listener(virtual_tfa_writer *this) {
    return this->listener;
}
void virtual_tfa_writer_set_listener(virtual_tfa_writer *this, virtual_tfa_listener *listener) {
    this->listener = listener;
}
tfa_size_t virtual_tfa_writer_calc_size(virtual_tfa_writer *this) {
    tfa_size_t size = 0;
    for (int i = 0; i < this->archive->entries_size; ++i) {
        virtual_tfa_entry *entry = this->archive->entries[i];
        if (!entry) continue;
        size += tfa_header_size + strlen(entry->name) + 1 + entry->size; // name will have \0 at end
    }
    return size;
}
int virtual_tfa_writer_write(virtual_tfa_writer *this, char *buffer, tfa_size_t buffer_size, tfa_size_t *out_bytes_written) {
    tfa_size_t bytes_written = 0;
    tfa_size_t buffer_size_left = buffer_size;
    tfa_size_t absolute_part_start_pos = 0; // absolute part start position
    tfa_size_t current_part_size;
    tfa_size_t total_part_bytes_written;
    tfa_size_t part_bytes_to_write;

    for (int i = 0; i < this->archive->entries_size; ++i) {
        virtual_tfa_entry *entry = this->archive->entries[i];
        if (!entry) continue;

        // Header
        current_part_size = tfa_header_size;
        if (virtual_tfa_util_calc_part_write(buffer_size_left,
                                             this->pointer,
                                             absolute_part_start_pos,
                                             current_part_size,
                                             &total_part_bytes_written,
                                             &part_bytes_to_write)) {
            if (!this->current_header) {
                this->current_header = virtual_tfa_util_convert_entry_to_header(entry);
                if (!this->current_header) {
                    fprintf(stderr, "virtual_tfa_writer_write: unable to create header\n");
                    return 1;
                }
            }
            char *headerBufferPtr = (char *)(this->current_header) + total_part_bytes_written; // offset
            memcpy(buffer + bytes_written, headerBufferPtr, part_bytes_to_write);
            buffer_size_left -= part_bytes_to_write;
            bytes_written += part_bytes_to_write;
            this->pointer += part_bytes_to_write;
            if (total_part_bytes_written + part_bytes_to_write == current_part_size) {
                free(this->current_header);
                this->current_header = NULL;
            }
            if (bytes_written == buffer_size) break;
        }
        absolute_part_start_pos += current_part_size;

        // Name
        current_part_size = strlen(entry->name) + 1; // last \0
        if (virtual_tfa_util_calc_part_write(buffer_size_left,
                                             this->pointer,
                                             absolute_part_start_pos,
                                             current_part_size,
                                             &total_part_bytes_written,
                                             &part_bytes_to_write)) {
            const char *nameBufferPtr = entry->name + total_part_bytes_written; // offset
            memcpy(buffer + bytes_written, nameBufferPtr, part_bytes_to_write);
            if (total_part_bytes_written + part_bytes_to_write == current_part_size) {
                buffer[bytes_written + part_bytes_to_write - 1] = '\0';
            }
            buffer_size_left -= part_bytes_to_write;
            bytes_written += part_bytes_to_write;
            this->pointer += part_bytes_to_write;
            if (bytes_written == buffer_size) break;
        }
        absolute_part_start_pos += current_part_size;

        // File Data
        current_part_size = entry->size;
        if (virtual_tfa_util_calc_part_write(buffer_size_left,
                                             this->pointer,
                                             absolute_part_start_pos,
                                             current_part_size,
                                             &total_part_bytes_written,
                                             &part_bytes_to_write)) {
            if (!this->current_stream) {
                this->current_stream = entry->stream_supplier(entry->stream_supplier_userdata);
                if (!this->current_stream) {
                    fprintf(stderr, "virtual_tfa_writer_write: unable to create input stream\n");
                    return 1;
                }
            }
            if (this->listener && total_part_bytes_written == 0) {
                virtual_tfa_file_info *fileInfo = virtual_tfa_util_convert_entry_to_info(entry);
                if (fileInfo) {
                    this->listener->file_start(this->listener->file_start_userdata, fileInfo);
                    free(fileInfo);
                }
            }
            //fseek(entry->file, partWrittenBytes, SEEK_SET); // currently removed: current_stream->seek
            int read_result = virtual_tfa_input_stream_read(this->current_stream, buffer + bytes_written, part_bytes_to_write, NULL);
            if (read_result != 0) {
                fprintf(stderr, "virtual_tfa_writer_write: read error\n");
                return 1;
            }
            buffer_size_left -= part_bytes_to_write;
            bytes_written += part_bytes_to_write;
            this->pointer += part_bytes_to_write;
            if (this->listener) {
                virtual_tfa_file_info *fileInfo = virtual_tfa_util_convert_entry_to_info(entry);
                if (fileInfo) {
                    this->listener->file_progress(this->listener->file_progress_userdata, fileInfo, total_part_bytes_written + part_bytes_to_write);
                    if (total_part_bytes_written + part_bytes_to_write == current_part_size) {
                        this->listener->file_end(this->listener->file_end_userdata, fileInfo);
                    }
                    free(fileInfo);
                }
            }
            if (total_part_bytes_written + part_bytes_to_write == current_part_size) {
                virtual_tfa_input_stream_close(this->current_stream);
                virtual_tfa_input_stream_free(this->current_stream);
                this->current_stream = NULL;
            }
            if (bytes_written == buffer_size) break;
        }
        absolute_part_start_pos += current_part_size;
    }

    if (this->listener) {
        this->listener->total_progress(this->listener->total_progress_userdata, this->pointer);
    }

    if (out_bytes_written) {
        *out_bytes_written = bytes_written;
    }

    return 0;
}

/*
 * Reader
 */

virtual_tfa_file_info *virtual_tfa_util_file_info_constructor(const char *name, tfa_size_t filesize, tfa_utime_t ctime, tfa_utime_t mtime) {
    virtual_tfa_file_info *fileInfo = (virtual_tfa_file_info *)malloc(sizeof(virtual_tfa_file_info));
    if (fileInfo) {
        fileInfo->name = name;
        fileInfo->size = filesize;
        fileInfo->ctime = ctime;
        fileInfo->mtime = mtime;
        fileInfo->mode = 0;
    }
    return fileInfo;
}

bool virtual_tfa_util_is_path_valid(const char *path) {
    const char* delimiter = "/";
    char* pathCopy = _strdup(path);
    char* nextToken;
    char* token = strtok_s(pathCopy, delimiter, &nextToken);

    while (token) {
        if (strcmp(token, ".") == 0 || strcmp(token, "..") == 0) {
            free(pathCopy);
            return false;
        }
        token = strtok_s(NULL, delimiter, &nextToken);
    }

    free(pathCopy);
    return true;
}

uint32_t virtual_tfa_util_deserialize_uint32(const char buffer[4]) {
    uint32_t value = 0;
    memcpy(&value, buffer, sizeof(value));
    return value;
}

uint64_t virtual_tfa_util_deserialize_uint64(const char buffer[8]) {
    uint64_t value = 0;
    memcpy(&value, buffer, sizeof(value));
    return value;
}

struct _virtual_tfa_reader {
    const char           *dest;
    virtual_tfa_listener *listener;

    char             *_cur_headerBuf;
    tfa_mode_t        _cur_h_mode;
    tfa_utime_t       _cur_h_ctime;
    tfa_utime_t       _cur_h_mtime;
    tfa_namesize_t    _cur_h_namesize;
    tfa_size_t    _cur_h_filesize;
    char             *_cur_name;
    FILE             *_cur_ofs;
    tfa_size_t  _cur_remainHeaderSize;
    tfa_namesize_t    _cur_remainNameSize;
    tfa_size_t    _cur_remainFileSize;
    tfa_size_t    _totalRead;
};

virtual_tfa_reader *virtual_tfa_reader_new(void) {
    virtual_tfa_reader *this = (virtual_tfa_reader *)malloc(sizeof(virtual_tfa_reader));
    if (this) {
        this->dest = NULL;
        this->listener = NULL;
        this->_cur_headerBuf = (char*)malloc(tfa_header_size * sizeof(char));
        this->_cur_h_mode = 0;
        this->_cur_h_ctime = 0;
        this->_cur_h_mtime = 0;
        this->_cur_h_namesize = 0;
        this->_cur_h_filesize = 0;
        this->_cur_name = NULL;
        this->_cur_ofs = NULL;
        this->_cur_remainHeaderSize = tfa_header_size;
        this->_cur_remainNameSize = 0;
        this->_cur_remainFileSize = 0;
        this->_cur_remainFileSize = 0;
        this->_totalRead = 0;
    }
    return this;
}
void virtual_tfa_reader_free(virtual_tfa_reader *this) {
    if (this) {
        free(this);
    }
}

const char *virtual_tfa_reader_get_dest(virtual_tfa_reader *this) {
    return this->dest;
}
void virtual_tfa_reader_set_dest(virtual_tfa_reader *this, const char *dest) {
    this->dest = dest;
}
virtual_tfa_listener *virtual_tfa_reader_get_listener(virtual_tfa_reader *this) {
    return this->listener;
}
void virtual_tfa_reader_set_listener(virtual_tfa_reader *this, virtual_tfa_listener *listener) {
    this->listener = listener;
}
int virtual_tfa_reader_read(virtual_tfa_reader *this, char *buffer, tfa_size_t buffer_size, tfa_size_t *out_bytes_read) {
    tfa_size_t bytes_read = 0;
    tfa_size_t buffer_size_left = buffer_size;

    while (buffer_size_left > 0) {
        // Header
        if (this->_cur_remainHeaderSize > 0) {
            tfa_size_t buffer_offset = tfa_header_size - this->_cur_remainHeaderSize;

            tfa_size_t to_read = MIN(this->_cur_remainHeaderSize, buffer_size_left);

            memcpy(this->_cur_headerBuf + buffer_offset, buffer + bytes_read, to_read);

            this->_cur_remainHeaderSize -= to_read;

            buffer_size_left -= to_read;
            bytes_read += to_read;

            if (this->_cur_remainHeaderSize == 0) {
                //printf("Header buffered, reading ...\n");

                tfa_header header = {0};
                tfa_size_t deserialize_pointer = 0;

                memcpy(header.magic, this->_cur_headerBuf + deserialize_pointer, sizeof(header.magic));
                deserialize_pointer += sizeof(header.magic);
                header.version = this->_cur_headerBuf[deserialize_pointer];
                deserialize_pointer++;
                header.typeflag = this->_cur_headerBuf[deserialize_pointer];
                deserialize_pointer++;
                memcpy(header.unused, this->_cur_headerBuf + deserialize_pointer, sizeof(header.unused));
                deserialize_pointer += sizeof(header.unused);
                memcpy(header.mode, this->_cur_headerBuf + deserialize_pointer, sizeof(header.mode));
                deserialize_pointer += sizeof(header.mode);
                memcpy(header.ctime, this->_cur_headerBuf + deserialize_pointer, sizeof(header.ctime));
                deserialize_pointer += sizeof(header.ctime);
                memcpy(header.mtime, this->_cur_headerBuf + deserialize_pointer, sizeof(header.mtime));
                deserialize_pointer += sizeof(header.mtime);
                memcpy(header.namesize, this->_cur_headerBuf + deserialize_pointer, sizeof(header.namesize));
                deserialize_pointer += sizeof(header.namesize);
                memcpy(header.filesize, this->_cur_headerBuf + deserialize_pointer, sizeof(header.filesize));

                this->_cur_h_mode = (tfa_mode_t) virtual_tfa_util_deserialize_uint32(header.mode); // TODO: rethink about permissions

                this->_cur_h_ctime = virtual_tfa_util_deserialize_uint64(header.ctime);
                this->_cur_h_mtime = virtual_tfa_util_deserialize_uint64(header.mtime);

                this->_cur_remainNameSize = this->_cur_h_namesize = virtual_tfa_util_deserialize_uint32(header.namesize);
                this->_cur_remainFileSize = this->_cur_h_filesize = virtual_tfa_util_deserialize_uint64(header.filesize);

                this->_cur_name = (char*)malloc(this->_cur_h_namesize * sizeof(char));

                //printf("Header readed\n");
            }
            if (bytes_read == buffer_size) break;
        }

        // Name
        if (this->_cur_remainNameSize > 0) {
            tfa_size_t buffer_offset = this->_cur_h_namesize - this->_cur_remainNameSize;

            tfa_size_t to_read = MIN(this->_cur_remainNameSize, buffer_size_left);

            memcpy_s(this->_cur_name + buffer_offset, this->_cur_remainNameSize, buffer + bytes_read, to_read);

            this->_cur_remainNameSize -= (tfa_namesize_t) to_read;

            buffer_size_left -= to_read;
            bytes_read += to_read;

            if (this->_cur_remainNameSize == 0) {
                if (!virtual_tfa_util_is_path_valid(this->_cur_name)) {
                    fprintf(stderr, "virtual_tfa_reader_read: invalid file name\n");
                    break;
                }
                if (this->listener) {
                    virtual_tfa_file_info *fileInfo = virtual_tfa_util_file_info_constructor(this->_cur_name,
                                                                                             this->_cur_h_filesize,
                                                                                             this->_cur_h_ctime,
                                                                                             this->_cur_h_mtime);
                    if (fileInfo) {
                        this->listener->file_start(this->listener->file_start_userdata, fileInfo);
                        free(fileInfo);
                    }
                }
                //printf("Reading %s ...\n", this->_cur_name);
            }
            if (bytes_read == buffer_size) break;
        }

        // File Data
        if (this->_cur_remainFileSize > 0) {
            if (!this->_cur_ofs) {
                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s/%s", this->dest, this->_cur_name);
                //printf("%s\n", filepath);
                if (fopen_s(&this->_cur_ofs, filepath, "wb") != 0) {
                    fprintf(stderr, "virtual_tfa_reader_read: failed to open the file %s\n", filepath);
                    return 0;
                }
            }

            //tfa_size_t offset = this->_cur_h_filesize - this->_cur_remainFileSize;

            tfa_size_t to_read = MIN(this->_cur_remainFileSize, buffer_size_left);

            //fseek(this->_cur_ofs, offset, SEEK_SET); // causes bugs
            fwrite(buffer + bytes_read, 1, to_read, this->_cur_ofs);

            this->_cur_remainFileSize -= to_read;

            buffer_size_left -= to_read;
            bytes_read += to_read;

            if (this->listener) {
                virtual_tfa_file_info *fileInfo = virtual_tfa_util_file_info_constructor(this->_cur_name,
                                                                                         this->_cur_h_filesize,
                                                                                         this->_cur_h_ctime,
                                                                                         this->_cur_h_mtime);
                if (fileInfo) {
                    this->listener->file_progress(this->listener->file_progress_userdata, fileInfo, this->_cur_h_filesize - this->_cur_remainFileSize);
                    free(fileInfo);
                }
            }
            if (this->_cur_remainFileSize == 0) {
                fclose(this->_cur_ofs);
                this->_cur_ofs = NULL;

                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s/%s", this->dest, this->_cur_name);

                virtual_tfa_util_set_file_metadata(filepath, this->_cur_h_mode, this->_cur_h_ctime, this->_cur_h_mtime);

                this->_cur_remainHeaderSize = tfa_header_size;

                if (this->listener) {
                    virtual_tfa_file_info *fileInfo = virtual_tfa_util_file_info_constructor(this->_cur_name,
                                                                                             this->_cur_h_filesize,
                                                                                             this->_cur_h_ctime,
                                                                                             this->_cur_h_mtime);
                    if (fileInfo) {
                        this->listener->file_end(this->listener->file_end_userdata, fileInfo);
                        free(fileInfo);
                    }
                }
            }
            if (bytes_read == buffer_size) break;
        }
    }

    this->_totalRead += bytes_read;
    if (this->listener) {
        this->listener->total_progress(this->listener->total_progress_userdata, this->_totalRead);
    }

    if (out_bytes_read) {
        *out_bytes_read = bytes_read;
    }

    return 0;
}

