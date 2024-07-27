#include "virtualtfa.h"

#include "file_util.h"

#include <stdlib.h>
#include <string.h>

#if (defined(_WIN32) || defined(_WIN64))
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h> // endian swap
#endif

typedef uint32_t tfa_namesize_t;

#define MIN(x, y) ((x) < (y) ? (x) : (y))

/*
 * Header
 */

// See README.md
typedef struct _tfa_header {
    char magic[6];
    char version;
    char typeflag;
    char reserved[8];
    char mode[4];
    char ctime[8];
    char mtime[8];
    char namesize[4];
    char filesize[8];
} tfa_header;

static tfa_size_t tfa_header_size = sizeof(tfa_header); // 48

/*
 * Utility
 */

// Write big-endian 32-bit signed integer
void virtualtfa_util_write_i32(char* buf, int32_t value) {
    uint32_t data = htonl((uint32_t) value);
    memcpy(buf, &data, 4);
}

// Read big-endian 32-bit signed integer
int32_t virtualtfa_util_read_i32(const char* buf) {
    uint32_t data;
    memcpy(&data, buf, 4);
    data = ntohl(data);
    return (int32_t) data;
}

// Write big-endian 32-bit unsigned integer
void virtualtfa_util_write_u32(char* buf, uint32_t value) {
    value = htonl(value);
    memcpy(buf, &value, 4);
}

// Read big-endian 32-bit unsigned integer
uint32_t virtualtfa_util_read_u32(const char* buf) {
    uint32_t data;
    memcpy(&data, buf, 4);
    data = ntohl(data);
    return data;
}

// Write big-endian 64-bit unsigned integer
void virtualtfa_util_write_u64(char* buf, uint64_t value) {
    value = htonll(value);
    memcpy(buf, &value, 8);
}

// Read big-endian 64-bit unsigned integer
uint64_t virtualtfa_util_read_u64(const char* buf) {
    uint64_t data;
    memcpy(&data, buf, 8);
    data = ntohll(data);
    return data;
}

/*
 * Input Stream
 */

struct _virtualtfa_input_stream {
    virtualtfa_read_function read_function;
    void* read_userdata;
    virtualtfa_close_function close_function;
    void* close_userdata;
};

virtualtfa_input_stream* virtualtfa_input_stream_new() {
    virtualtfa_input_stream* this = (virtualtfa_input_stream*) malloc(sizeof(virtualtfa_input_stream));
    if (this) {
        this->read_function = NULL;
        this->read_userdata = NULL;
        this->close_function = NULL;
        this->close_userdata = NULL;
    }
    return this;
}

void virtualtfa_input_stream_free(virtualtfa_input_stream* this) {
    if (this) {
        free(this);
    }
}

virtualtfa_read_function virtualtfa_input_stream_get_read_function(virtualtfa_input_stream* this) {
    return this->read_function;
}

void
virtualtfa_input_stream_set_read_function(virtualtfa_input_stream* this, virtualtfa_read_function read_function) {
    this->read_function = read_function;
}

void* virtualtfa_input_stream_get_read_userdata(virtualtfa_input_stream* this) {
    return this->read_userdata;
}

void virtualtfa_input_stream_set_read_userdata(virtualtfa_input_stream* this, void* userdata) {
    this->read_userdata = userdata;
}

virtualtfa_close_function virtualtfa_input_stream_get_close_function(virtualtfa_input_stream* this) {
    return this->close_function;
}

void
virtualtfa_input_stream_set_close_function(virtualtfa_input_stream* this, virtualtfa_close_function close_function) {
    this->close_function = close_function;
}

void* virtualtfa_input_stream_get_close_userdata(virtualtfa_input_stream* this) {
    return this->close_userdata;
}

void virtualtfa_input_stream_set_close_userdata(virtualtfa_input_stream* this, void* userdata) {
    this->close_userdata = userdata;
}

int virtualtfa_input_stream_read(virtualtfa_input_stream* this,
                                  char* buffer,
                                  tfa_size_t buffer_size,
                                  tfa_size_t* out_bytes_read) {
    tfa_size_t read_bytes = this->read_function(this->read_userdata, buffer, buffer_size);
    if (out_bytes_read) {
        *out_bytes_read = read_bytes;
    }
    return 0;
}

void virtualtfa_input_stream_close(virtualtfa_input_stream* this) {
    if (this->close_function) {
        this->close_function(this->close_userdata);
    }
}

/*
 * Entry
 */

struct _virtualtfa_entry {
    const char* name;
    tfa_size_t size;
    virtualtfa_input_stream_supplier stream_supplier;
    void* stream_supplier_userdata;
    tfa_utime_t ctime;
    tfa_utime_t mtime;
    tfa_mode_t mode;
};

virtualtfa_entry* virtualtfa_entry_new() {
    virtualtfa_entry* this = (virtualtfa_entry*) malloc(sizeof(virtualtfa_entry));
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

void virtualtfa_entry_free(virtualtfa_entry* this) {
    if (this) {
        free(this);
    }
}

const char* virtualtfa_entry_get_name(virtualtfa_entry* this) {
    return this->name;
}

void virtualtfa_entry_set_name(virtualtfa_entry* this, const char* name) {
    this->name = name;
}

tfa_size_t virtualtfa_entry_get_size(virtualtfa_entry* this) {
    return this->size;
}

void virtualtfa_entry_set_size(virtualtfa_entry* this, tfa_size_t size) {
    this->size = size;
}

virtualtfa_input_stream_supplier virtualtfa_entry_get_input_stream_supplier(virtualtfa_entry* this) {
    return this->stream_supplier;
}

void virtualtfa_entry_set_input_stream_supplier(virtualtfa_entry* this,
                                                 virtualtfa_input_stream_supplier stream_supplier) {
    this->stream_supplier = stream_supplier;
}

void* virtualtfa_entry_get_input_stream_supplier_userdata(virtualtfa_entry* this) {
    return this->stream_supplier_userdata;
}

void virtualtfa_entry_set_input_stream_supplier_userdata(virtualtfa_entry* this, void* userdata) {
    this->stream_supplier_userdata = userdata;
}

tfa_utime_t virtualtfa_entry_get_ctime(virtualtfa_entry* this) {
    return this->ctime;
}

void virtualtfa_entry_set_ctime(virtualtfa_entry* this, tfa_utime_t ctime) {
    this->ctime = ctime;
}

tfa_utime_t virtualtfa_entry_get_mtime(virtualtfa_entry* this) {
    return this->mtime;
}

void virtualtfa_entry_set_mtime(virtualtfa_entry* this, tfa_utime_t mtime) {
    this->mtime = mtime;
}

tfa_mode_t virtualtfa_entry_get_mode(virtualtfa_entry* this) {
    return this->mode;
}

void virtualtfa_entry_set_mode(virtualtfa_entry* this, tfa_mode_t mode) {
    this->mode = mode;
}

/*
 * Archive
 */

struct _virtualtfa_archive {
    virtualtfa_entry** entries;
    size_t entries_size;
};

virtualtfa_archive* virtualtfa_archive_new() {
    virtualtfa_archive* this = (virtualtfa_archive*) malloc(sizeof(virtualtfa_archive));
    if (this) {
        this->entries = NULL;
        this->entries_size = 0;
    }
    return this;
}

void virtualtfa_archive_free(virtualtfa_archive* this) {
    if (this) {
        free(this);
    }
}

void virtualtfa_archive_add(virtualtfa_archive* this, virtualtfa_entry* entry) {
    this->entries_size++;
    virtualtfa_entry** new_entries = (virtualtfa_entry**) realloc(this->entries,
                                                                    this->entries_size * sizeof(virtualtfa_entry*));
    if (!new_entries) {
        fprintf(stderr, "virtualtfa_archive_add: memory allocation failed\n");
        return;
    }
    this->entries = new_entries;
    this->entries[this->entries_size - 1] = entry;
}

/*
 * Writer
 */

int virtualtfa_util_calc_part_write(tfa_size_t buffer_size_left,
                                     tfa_size_t pointer,
                                     tfa_size_t absolute_part_start_pos,
                                     tfa_size_t current_part_size,
                                     tfa_size_t* out_part_bytes_written,
                                     tfa_size_t* out_part_bytes_to_write) {
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

const char virtualtfa_magic[4] = {'t', 'f', 'a', '1'};

void virtualtfa_util_set_magic(tfa_header* header) {
    memcpy(header->magic, virtualtfa_magic, sizeof(virtualtfa_magic));
}

void virtualtfa_util_set_mode(tfa_header* header, tfa_mode_t mode) {
    virtualtfa_util_write_i32(header->mode, mode);
}

void virtualtfa_util_set_ctime_mtime(tfa_header* header, tfa_utime_t ctime, tfa_utime_t mtime) {
    virtualtfa_util_write_u64(header->ctime, ctime);
    virtualtfa_util_write_u64(header->mtime, mtime);
}

void virtualtfa_util_set_namesize(tfa_header* header, tfa_namesize_t namesize) {
    virtualtfa_util_write_u32(header->namesize, namesize);
}

void virtualtfa_util_set_filesize(tfa_header* header, tfa_size_t filesize) {
    virtualtfa_util_write_u32(header->filesize, filesize);
}

tfa_header* virtualtfa_util_convert_entry_to_header(virtualtfa_entry* entry) {
    tfa_header* header = (tfa_header*) malloc(sizeof(tfa_header));
    if (header) {
        virtualtfa_util_set_magic(header);
        virtualtfa_util_set_mode(header, entry->mode);
        virtualtfa_util_set_ctime_mtime(header, entry->ctime, entry->mtime);
        virtualtfa_util_set_namesize(header, (tfa_namesize_t) strlen(entry->name)); // without null terminator
        virtualtfa_util_set_filesize(header, entry->size);
    }
    return header;
}

virtualtfa_file_info* virtualtfa_util_convert_entry_to_info(virtualtfa_entry* entry) {
    virtualtfa_file_info* fileInfo = (virtualtfa_file_info*) malloc(sizeof(virtualtfa_file_info));
    if (fileInfo) {
        fileInfo->name = entry->name;
        fileInfo->size = entry->size;
        fileInfo->ctime = entry->ctime;
        fileInfo->mtime = entry->mtime;
        fileInfo->mode = entry->mode;
    }
    return fileInfo;
}

struct _virtualtfa_writer {
    virtualtfa_archive* archive;
    virtualtfa_listener* listener;
    tfa_size_t pointer;
    tfa_header* current_header;
    virtualtfa_input_stream* current_stream;
};

virtualtfa_writer* virtualtfa_writer_new() {
    virtualtfa_writer* this = (virtualtfa_writer*) malloc(sizeof(virtualtfa_writer));
    if (this) {
        this->archive = NULL;
        this->listener = NULL;
        this->pointer = 0;
        this->current_header = NULL;
        this->current_stream = NULL;
    }
    return this;
}

void virtualtfa_writer_free(virtualtfa_writer* this) {
    if (this) {
        free(this);
    }
}

virtualtfa_archive* virtualtfa_writer_get_archive(virtualtfa_writer* this) {
    return this->archive;
}

void virtualtfa_writer_set_archive(virtualtfa_writer* this, virtualtfa_archive* archive) {
    this->archive = archive;
}

virtualtfa_listener* virtualtfa_writer_get_listener(virtualtfa_writer* this) {
    return this->listener;
}

void virtualtfa_writer_set_listener(virtualtfa_writer* this, virtualtfa_listener* listener) {
    this->listener = listener;
}

tfa_size_t virtualtfa_writer_calc_size(virtualtfa_writer* this) {
    tfa_size_t size = 0;
    for (int i = 0; i < this->archive->entries_size; ++i) {
        virtualtfa_entry* entry = this->archive->entries[i];
        if (!entry) continue;
        size += tfa_header_size + strlen(entry->name) + 1 + entry->size; // name will have \0 at end
    }
    return size;
}

int virtualtfa_writer_write(virtualtfa_writer* this,
                             char* buffer,
                             tfa_size_t buffer_size,
                             tfa_size_t* out_bytes_written) {
    tfa_size_t bytes_written = 0;
    tfa_size_t buffer_size_left = buffer_size;
    tfa_size_t absolute_part_start_pos = 0; // absolute part start position
    tfa_size_t current_part_size;
    tfa_size_t total_part_bytes_written;
    tfa_size_t part_bytes_to_write;

    for (int i = 0; i < this->archive->entries_size; ++i) {
        virtualtfa_entry* entry = this->archive->entries[i];
        if (!entry) continue;

        // Header
        current_part_size = tfa_header_size;
        if (virtualtfa_util_calc_part_write(buffer_size_left,
                                             this->pointer,
                                             absolute_part_start_pos,
                                             current_part_size,
                                             &total_part_bytes_written,
                                             &part_bytes_to_write)) {
            if (!this->current_header) {
                this->current_header = virtualtfa_util_convert_entry_to_header(entry);
                if (!this->current_header) {
                    fprintf(stderr, "virtualtfa_writer_write: unable to create header\n");
                    return 1;
                }
            }
            char* headerBufferPtr = (char*) (this->current_header) + total_part_bytes_written; // offset
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
        current_part_size = strlen(entry->name); // without null terminator
        if (virtualtfa_util_calc_part_write(buffer_size_left,
                                             this->pointer,
                                             absolute_part_start_pos,
                                             current_part_size,
                                             &total_part_bytes_written,
                                             &part_bytes_to_write)) {
            const char* nameBufferPtr = entry->name + total_part_bytes_written; // offset
            memcpy(buffer + bytes_written, nameBufferPtr, part_bytes_to_write);
            buffer_size_left -= part_bytes_to_write;
            bytes_written += part_bytes_to_write;
            this->pointer += part_bytes_to_write;
            if (bytes_written == buffer_size) break;
        }
        absolute_part_start_pos += current_part_size;

        // File Data
        current_part_size = entry->size;
        if (virtualtfa_util_calc_part_write(buffer_size_left,
                                             this->pointer,
                                             absolute_part_start_pos,
                                             current_part_size,
                                             &total_part_bytes_written,
                                             &part_bytes_to_write)) {
            if (!this->current_stream) {
                this->current_stream = entry->stream_supplier(entry->stream_supplier_userdata);
                if (!this->current_stream) {
                    fprintf(stderr, "virtualtfa_writer_write: unable to create input stream\n");
                    return 1;
                }
            }
            if (this->listener && total_part_bytes_written == 0) {
                virtualtfa_file_info* fileInfo = virtualtfa_util_convert_entry_to_info(entry);
                if (fileInfo) {
                    this->listener->file_start(this->listener->file_start_userdata, fileInfo);
                    free(fileInfo);
                }
            }
            //fseek(entry->file, partWrittenBytes, SEEK_SET); // currently removed: current_stream->seek
            int read_result = virtualtfa_input_stream_read(this->current_stream, buffer + bytes_written,
                                                            part_bytes_to_write, NULL);
            if (read_result != 0) {
                fprintf(stderr, "virtualtfa_writer_write: read error\n");
                return 1;
            }
            buffer_size_left -= part_bytes_to_write;
            bytes_written += part_bytes_to_write;
            this->pointer += part_bytes_to_write;
            if (this->listener) {
                virtualtfa_file_info* fileInfo = virtualtfa_util_convert_entry_to_info(entry);
                if (fileInfo) {
                    this->listener->file_progress(this->listener->file_progress_userdata, fileInfo,
                                                  total_part_bytes_written + part_bytes_to_write);
                    if (total_part_bytes_written + part_bytes_to_write == current_part_size) {
                        this->listener->file_end(this->listener->file_end_userdata, fileInfo);
                    }
                    free(fileInfo);
                }
            }
            if (total_part_bytes_written + part_bytes_to_write == current_part_size) {
                virtualtfa_input_stream_close(this->current_stream);
                virtualtfa_input_stream_free(this->current_stream);
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

virtualtfa_file_info virtualtfa_util_file_info_constructor(const char* name,
                                                             tfa_size_t filesize,
                                                             tfa_utime_t ctime,
                                                             tfa_utime_t mtime) {
    virtualtfa_file_info fileinfo;
    fileinfo.name = name;
    fileinfo.size = filesize;
    fileinfo.ctime = ctime;
    fileinfo.mtime = mtime;
    fileinfo.mode = 0;
    return fileinfo;
}

const char* virtualtfa_util_delimiter = "/";

bool virtualtfa_util_is_path_valid(const char* path) {
    char* str = strdup(path);
    char* token = strtok(str, virtualtfa_util_delimiter);

    while (token != NULL) {
        if (strcmp(token, ".") == 0 || strcmp(token, "..") == 0) {
            free(str);
            return false;
        }
        token = strtok(NULL, virtualtfa_util_delimiter);
    }

    free(str);
    return true;
}

struct _virtualtfa_reader {
    const char* dest;
    virtualtfa_listener* listener;

    char* _cur_headerBuf;
    tfa_mode_t _cur_h_mode;
    tfa_utime_t _cur_h_ctime;
    tfa_utime_t _cur_h_mtime;
    tfa_namesize_t _cur_h_namesize;
    tfa_size_t _cur_h_filesize;
    char* _cur_name;
    FILE* _cur_ofs;
    tfa_size_t _cur_remainHeaderSize;
    tfa_namesize_t _cur_remainNameSize;
    tfa_size_t _cur_remainFileSize;
    tfa_size_t _totalRead;
};

virtualtfa_reader* virtualtfa_reader_new() {
    virtualtfa_reader* this = (virtualtfa_reader*) malloc(sizeof(virtualtfa_reader));
    if (this) {
        this->dest = NULL;
        this->listener = NULL;
        this->_cur_headerBuf = (char*) malloc(tfa_header_size);
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

void virtualtfa_reader_free(virtualtfa_reader* this) {
    if (this) {
        free(this);
    }
}

const char* virtualtfa_reader_get_dest(virtualtfa_reader* this) {
    return this->dest;
}

void virtualtfa_reader_set_dest(virtualtfa_reader* this, const char* dest) {
    this->dest = dest;
}

virtualtfa_listener* virtualtfa_reader_get_listener(virtualtfa_reader* this) {
    return this->listener;
}

void virtualtfa_reader_set_listener(virtualtfa_reader* this, virtualtfa_listener* listener) {
    this->listener = listener;
}

int virtualtfa_reader_read(virtualtfa_reader* this, char* buffer, tfa_size_t buffer_size, tfa_size_t* out_bytes_read) {
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
                memcpy(header.reserved, this->_cur_headerBuf + deserialize_pointer, sizeof(header.reserved));
                deserialize_pointer += sizeof(header.reserved);
                memcpy(header.mode, this->_cur_headerBuf + deserialize_pointer, sizeof(header.mode));
                deserialize_pointer += sizeof(header.mode);
                memcpy(header.ctime, this->_cur_headerBuf + deserialize_pointer, sizeof(header.ctime));
                deserialize_pointer += sizeof(header.ctime);
                memcpy(header.mtime, this->_cur_headerBuf + deserialize_pointer, sizeof(header.mtime));
                deserialize_pointer += sizeof(header.mtime);
                memcpy(header.namesize, this->_cur_headerBuf + deserialize_pointer, sizeof(header.namesize));
                deserialize_pointer += sizeof(header.namesize);
                memcpy(header.filesize, this->_cur_headerBuf + deserialize_pointer, sizeof(header.filesize));

                // TODO: rethink about permissions
                this->_cur_h_mode = (tfa_mode_t) virtualtfa_util_read_i32(header.mode);

                this->_cur_h_ctime = virtualtfa_util_read_u64(header.ctime);
                this->_cur_h_mtime = virtualtfa_util_read_u64(header.mtime);

                this->_cur_remainNameSize = this->_cur_h_namesize = virtualtfa_util_read_u32(header.namesize);
                this->_cur_remainFileSize = this->_cur_h_filesize = virtualtfa_util_read_u32(header.filesize);

                this->_cur_name = (char*) malloc(this->_cur_h_namesize + 1);
                this->_cur_name[this->_cur_h_namesize] = '\0';

                //printf("Header readed\n");
            }
            if (bytes_read == buffer_size) break;
        }

        // Name
        if (this->_cur_remainNameSize > 0) {
            tfa_size_t buffer_offset = this->_cur_h_namesize - this->_cur_remainNameSize;

            tfa_size_t to_read = MIN(this->_cur_remainNameSize, buffer_size_left);

            memcpy(this->_cur_name + buffer_offset, buffer + bytes_read, to_read);

            this->_cur_remainNameSize -= (tfa_namesize_t) to_read;

            buffer_size_left -= to_read;
            bytes_read += to_read;

            if (this->_cur_remainNameSize == 0) {
                if (!virtualtfa_util_is_path_valid(this->_cur_name)) {
                    fprintf(stderr, "virtualtfa_reader_read: invalid file name\n");
                    break;
                }
                if (this->listener) {
                    virtualtfa_file_info fileinfo = virtualtfa_util_file_info_constructor(this->_cur_name,
                                                                                            this->_cur_h_filesize,
                                                                                            this->_cur_h_ctime,
                                                                                            this->_cur_h_mtime);
                    this->listener->file_start(this->listener->file_start_userdata, &fileinfo);
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
                if ((this->_cur_ofs = fopen(filepath, "wb")) != 0) {
                    fprintf(stderr, "virtualtfa_reader_read: failed to open the file %s\n", filepath);
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
                virtualtfa_file_info fileinfo = virtualtfa_util_file_info_constructor(this->_cur_name,
                                                                                        this->_cur_h_filesize,
                                                                                        this->_cur_h_ctime,
                                                                                        this->_cur_h_mtime);
                this->listener->file_progress(this->listener->file_progress_userdata, &fileinfo,
                                              this->_cur_h_filesize - this->_cur_remainFileSize);
            }
            if (this->_cur_remainFileSize == 0) {
                fclose(this->_cur_ofs);
                this->_cur_ofs = NULL;

                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s/%s", this->dest, this->_cur_name);

                virtualtfa_util_set_file_metadata(filepath, this->_cur_h_mode, this->_cur_h_ctime, this->_cur_h_mtime);

                this->_cur_remainHeaderSize = tfa_header_size;

                if (this->listener) {
                    virtualtfa_file_info fileinfo = virtualtfa_util_file_info_constructor(this->_cur_name,
                                                                                            this->_cur_h_filesize,
                                                                                            this->_cur_h_ctime,
                                                                                            this->_cur_h_mtime);
                    this->listener->file_end(this->listener->file_end_userdata, &fileinfo);
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

