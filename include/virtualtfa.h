#ifndef VIRTUALTFA_H
#define VIRTUALTFA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

typedef uint64_t tfa_size_t;
typedef uint64_t tfa_utime_t;
typedef int32_t tfa_mode_t;

typedef struct _virtualtfa_archive virtualtfa_archive;
typedef struct _virtualtfa_entry virtualtfa_entry;

typedef tfa_size_t (*virtualtfa_read_function)(void* userdata, char* buffer, tfa_size_t buffer_size);
typedef void (*virtualtfa_close_function)(void* userdata);

typedef struct _virtualtfa_input_stream virtualtfa_input_stream;

typedef virtualtfa_input_stream*(*virtualtfa_input_stream_supplier)(void* userdata);

typedef struct {
    const char*   name;
    tfa_size_t    size;
    tfa_utime_t   ctime;
    tfa_utime_t   mtime;
    tfa_mode_t    mode;
} virtualtfa_file_info;

typedef struct {
    void (*total_progress)(void* userdata, tfa_size_t);
    void *total_progress_userdata;
    void (*file_start)(void* userdata, const virtualtfa_file_info*);
    void *file_start_userdata;
    void (*file_progress)(void* userdata, const virtualtfa_file_info*, tfa_size_t);
    void *file_progress_userdata;
    void (*file_end)(void* userdata, const virtualtfa_file_info*);
    void *file_end_userdata;
} virtualtfa_listener;

typedef struct _virtualtfa_writer virtualtfa_writer;
typedef struct _virtualtfa_reader virtualtfa_reader;

/*
 * Methods
 */

virtualtfa_input_stream* virtualtfa_input_stream_new(void);
void                      virtualtfa_input_stream_free(virtualtfa_input_stream*);

virtualtfa_read_function  virtualtfa_input_stream_get_read_function(virtualtfa_input_stream*);
void                       virtualtfa_input_stream_set_read_function(virtualtfa_input_stream*, virtualtfa_read_function);
void*                      virtualtfa_input_stream_get_read_userdata(virtualtfa_input_stream*);
void                       virtualtfa_input_stream_set_read_userdata(virtualtfa_input_stream*, void*);
virtualtfa_close_function virtualtfa_input_stream_get_close_function(virtualtfa_input_stream*);
void                       virtualtfa_input_stream_set_close_function(virtualtfa_input_stream*, virtualtfa_close_function);
void*                      virtualtfa_input_stream_get_close_userdata(virtualtfa_input_stream*);
void                       virtualtfa_input_stream_set_close_userdata(virtualtfa_input_stream*, void*);
int                        virtualtfa_input_stream_read(virtualtfa_input_stream*, char* buffer, tfa_size_t buffer_size, tfa_size_t* out_bytes_read);
void                       virtualtfa_input_stream_close(virtualtfa_input_stream*);

virtualtfa_entry* virtualtfa_entry_new(void);
void			   virtualtfa_entry_free(virtualtfa_entry*);

const char*                        virtualtfa_entry_get_name(virtualtfa_entry*);
void                               virtualtfa_entry_set_name(virtualtfa_entry*, const char*);
tfa_size_t                         virtualtfa_entry_get_size(virtualtfa_entry*);
void                               virtualtfa_entry_set_size(virtualtfa_entry*, tfa_size_t);
virtualtfa_input_stream_supplier  virtualtfa_entry_get_input_stream_supplier(virtualtfa_entry*);
void                               virtualtfa_entry_set_input_stream_supplier(virtualtfa_entry*, virtualtfa_input_stream_supplier);
void*                              virtualtfa_entry_get_input_stream_supplier_userdata(virtualtfa_entry*);
void                               virtualtfa_entry_set_input_stream_supplier_userdata(virtualtfa_entry*, void*);
tfa_utime_t                        virtualtfa_entry_get_ctime(virtualtfa_entry*);
void                               virtualtfa_entry_set_ctime(virtualtfa_entry*, tfa_utime_t);
tfa_utime_t                        virtualtfa_entry_get_mtime(virtualtfa_entry*);
void                               virtualtfa_entry_set_mtime(virtualtfa_entry*, tfa_utime_t);
tfa_mode_t                         virtualtfa_entry_get_mode(virtualtfa_entry*);
void                               virtualtfa_entry_set_mode(virtualtfa_entry*, tfa_mode_t);

virtualtfa_archive* virtualtfa_archive_new(void);
void			           virtualtfa_archive_free(virtualtfa_archive*);

void virtualtfa_archive_add(virtualtfa_archive*, virtualtfa_entry*);

virtualtfa_writer* virtualtfa_writer_new(void);
void                virtualtfa_writer_free(virtualtfa_writer*);

virtualtfa_archive*  virtualtfa_writer_get_archive(virtualtfa_writer*);
void                  virtualtfa_writer_set_archive(virtualtfa_writer*, virtualtfa_archive*);
virtualtfa_listener* virtualtfa_writer_get_listener(virtualtfa_writer*);
void                  virtualtfa_writer_set_listener(virtualtfa_writer*, virtualtfa_listener*);
tfa_size_t            virtualtfa_writer_calc_size(virtualtfa_writer*);
int                   virtualtfa_writer_write(virtualtfa_writer*, char* buffer, tfa_size_t buffer_size, tfa_size_t* out_bytes_written);

virtualtfa_reader* virtualtfa_reader_new(void);
void                virtualtfa_reader_free(virtualtfa_reader*);

const char*           virtualtfa_reader_get_dest(virtualtfa_reader*);
void                  virtualtfa_reader_set_dest(virtualtfa_reader*, const char* dest);
virtualtfa_listener* virtualtfa_reader_get_listener(virtualtfa_reader*);
void                  virtualtfa_reader_set_listener(virtualtfa_reader*, virtualtfa_listener*);
int                   virtualtfa_reader_read(virtualtfa_reader *, char* buffer, tfa_size_t buffer_size, tfa_size_t* out_bytes_read);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //VIRTUALTFA_H
