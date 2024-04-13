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

typedef struct _virtual_tfa_archive virtual_tfa_archive;
typedef struct _virtual_tfa_entry virtual_tfa_entry;

typedef tfa_size_t (*virtual_tfa_read_function)(void* userdata, char* buffer, tfa_size_t buffer_size);
typedef void (*virtual_tfa_close_function)(void* userdata);

typedef struct _virtual_tfa_input_stream virtual_tfa_input_stream;

typedef virtual_tfa_input_stream*(*virtual_tfa_input_stream_supplier)(void* userdata);

typedef struct {
    const char*   name;
    tfa_size_t    size;
    tfa_utime_t   ctime;
    tfa_utime_t   mtime;
    tfa_mode_t    mode;
} virtual_tfa_file_info;

typedef struct {
    void (*total_progress)(void* userdata, tfa_size_t);
    void *total_progress_userdata;
    void (*file_start)(void* userdata, const virtual_tfa_file_info*);
    void *file_start_userdata;
    void (*file_progress)(void* userdata, const virtual_tfa_file_info*, tfa_size_t);
    void *file_progress_userdata;
    void (*file_end)(void* userdata, const virtual_tfa_file_info*);
    void *file_end_userdata;
} virtual_tfa_listener;

typedef struct _virtual_tfa_writer virtual_tfa_writer;
typedef struct _virtual_tfa_reader virtual_tfa_reader;

/*
 * Methods
 */

virtual_tfa_input_stream* virtual_tfa_input_stream_new();
void                      virtual_tfa_input_stream_free(virtual_tfa_input_stream*);

virtual_tfa_read_function  virtual_tfa_input_stream_get_read_function(virtual_tfa_input_stream*);
void                       virtual_tfa_input_stream_set_read_function(virtual_tfa_input_stream*, virtual_tfa_read_function);
void*                      virtual_tfa_input_stream_get_read_userdata(virtual_tfa_input_stream*);
void                       virtual_tfa_input_stream_set_read_userdata(virtual_tfa_input_stream*, void*);
virtual_tfa_close_function virtual_tfa_input_stream_get_close_function(virtual_tfa_input_stream*);
void                       virtual_tfa_input_stream_set_close_function(virtual_tfa_input_stream*, virtual_tfa_close_function);
void*                      virtual_tfa_input_stream_get_close_userdata(virtual_tfa_input_stream*);
void                       virtual_tfa_input_stream_set_close_userdata(virtual_tfa_input_stream*, void*);
int                        virtual_tfa_input_stream_read(virtual_tfa_input_stream*, char* buffer, tfa_size_t buffer_size, tfa_size_t* out_bytes_read);
void                       virtual_tfa_input_stream_close(virtual_tfa_input_stream*);

virtual_tfa_entry* virtual_tfa_entry_new();
void			   virtual_tfa_entry_free(virtual_tfa_entry*);

const char*                        virtual_tfa_entry_get_name(virtual_tfa_entry*);
void                               virtual_tfa_entry_set_name(virtual_tfa_entry*, const char*);
tfa_size_t                         virtual_tfa_entry_get_size(virtual_tfa_entry*);
void                               virtual_tfa_entry_set_size(virtual_tfa_entry*, tfa_size_t);
virtual_tfa_input_stream_supplier  virtual_tfa_entry_get_input_stream_supplier(virtual_tfa_entry*);
void                               virtual_tfa_entry_set_input_stream_supplier(virtual_tfa_entry*, virtual_tfa_input_stream_supplier);
void*                              virtual_tfa_entry_get_input_stream_supplier_userdata(virtual_tfa_entry*);
void                               virtual_tfa_entry_set_input_stream_supplier_userdata(virtual_tfa_entry*, void*);
tfa_utime_t                        virtual_tfa_entry_get_ctime(virtual_tfa_entry*);
void                               virtual_tfa_entry_set_ctime(virtual_tfa_entry*, tfa_utime_t);
tfa_utime_t                        virtual_tfa_entry_get_mtime(virtual_tfa_entry*);
void                               virtual_tfa_entry_set_mtime(virtual_tfa_entry*, tfa_utime_t);
tfa_mode_t                         virtual_tfa_entry_get_mode(virtual_tfa_entry*);
void                               virtual_tfa_entry_set_mode(virtual_tfa_entry*, tfa_mode_t);

virtual_tfa_archive* virtual_tfa_archive_new();
void			     virtual_tfa_archive_free(virtual_tfa_archive*);

void virtual_tfa_archive_add(virtual_tfa_archive*, virtual_tfa_entry*);

virtual_tfa_writer* virtual_tfa_writer_new();
void                virtual_tfa_writer_free(virtual_tfa_writer*);

virtual_tfa_archive*  virtual_tfa_writer_get_archive(virtual_tfa_writer*);
void                  virtual_tfa_writer_set_archive(virtual_tfa_writer*, virtual_tfa_archive*);
virtual_tfa_listener* virtual_tfa_writer_get_listener(virtual_tfa_writer*);
void                  virtual_tfa_writer_set_listener(virtual_tfa_writer*, virtual_tfa_listener*);
tfa_size_t            virtual_tfa_writer_calc_size(virtual_tfa_writer*);
int                   virtual_tfa_writer_write(virtual_tfa_writer*, char* buffer, tfa_size_t buffer_size, tfa_size_t* out_bytes_written);

virtual_tfa_reader* virtual_tfa_reader_new();
void                virtual_tfa_reader_free(virtual_tfa_reader*);

const char*           virtual_tfa_reader_get_dest(virtual_tfa_reader*);
void                  virtual_tfa_reader_set_dest(virtual_tfa_reader*, const char* dest);
virtual_tfa_listener* virtual_tfa_reader_get_listener(virtual_tfa_reader*);
void                  virtual_tfa_reader_set_listener(virtual_tfa_reader*, virtual_tfa_listener*);
int                   virtual_tfa_reader_read(virtual_tfa_reader *, char* buffer, tfa_size_t buffer_size, tfa_size_t* out_bytes_read);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //VIRTUALTFA_H
