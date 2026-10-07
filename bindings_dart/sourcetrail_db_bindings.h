#ifndef SOURCETRAIL_DART_DB_BRIDGE_H
#define SOURCETRAIL_DART_DB_BRIDGE_H

#ifdef _WIN32
#define ST_EXPORT __declspec(dllexport)
#else
#define ST_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque C ABI handle owned by the C++ bridge. */
typedef void* StWriterHandle;

/* Create and destroy one SourcetrailDB writer state. */
ST_EXPORT StWriterHandle st_writer_create(void);
ST_EXPORT void st_writer_destroy(StWriterHandle handle);

/* Return the database version and complete SDK version string. */
ST_EXPORT int st_writer_supported_database_version(StWriterHandle handle);
ST_EXPORT const char* st_writer_version(StWriterHandle handle);

/* Open the database and control its transaction lifetime. */
ST_EXPORT int st_writer_open(StWriterHandle handle, const char* path);
ST_EXPORT int st_writer_close(StWriterHandle handle);
ST_EXPORT const char* st_writer_last_error(StWriterHandle handle);
ST_EXPORT int st_writer_begin_transaction(StWriterHandle handle);
ST_EXPORT int st_writer_commit_transaction(StWriterHandle handle);
ST_EXPORT int st_writer_rollback_transaction(StWriterHandle handle);

/* Record a file and its syntax-highlighting language. */
ST_EXPORT int st_writer_record_file(StWriterHandle handle, const char* path);
ST_EXPORT int st_writer_record_file_language(StWriterHandle handle, const char* language);

/* Record a symbol; file may be null for an external reference target. */
ST_EXPORT int st_writer_record_symbol(StWriterHandle handle, const char* stable_id, const char* qualified_name, const char* kind, int definition, const char* file, int start_line, int start_column, int end_line, int end_column);
ST_EXPORT int st_writer_record_symbol_scope_location(StWriterHandle handle, const char* stable_id, const char* file, int start_line, int start_column, int end_line, int end_column);
ST_EXPORT int st_writer_record_symbol_signature_location(StWriterHandle handle, const char* stable_id, const char* file, int start_line, int start_column, int end_line, int end_column);

/* Record a SourcetrailDB local symbol and one of its source locations. */
ST_EXPORT int st_writer_record_local_symbol(StWriterHandle handle, const char* stable_id, const char* name, const char* file, int start_line, int start_column, int end_line, int end_column);

/* Record resolved/unresolved references and clickable diagnostics. */
ST_EXPORT int st_writer_record_reference(StWriterHandle handle, const char* context_id, const char* target_id, const char* kind, const char* file, int start_line, int start_column, int end_line, int end_column, int resolved);
ST_EXPORT int st_writer_record_unresolved_reference(StWriterHandle handle, const char* context_id, const char* kind, const char* file, int start_line, int start_column, int end_line, int end_column);
ST_EXPORT int st_writer_record_error(StWriterHandle handle, const char* message, int fatal, const char* file, int start_line, int start_column, int end_line, int end_column);

#ifdef __cplusplus
}
#endif

#endif
