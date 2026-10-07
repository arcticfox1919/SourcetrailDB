#include "sourcetrail_db_bindings.h"

#include <exception>
#include <sstream>
#include <string>
#include <unordered_map>

#include "SourcetrailDBWriter.h"

// C++ state behind the opaque C ABI handle. STL, exceptions, and object
// layout never cross the boundary; IDs are keyed by Dart stable identifiers.
struct StWriter {
  sourcetrail::SourcetrailDBWriter writer;
  std::unordered_map<std::string, int> symbols;
  std::unordered_map<std::string, int> localSymbols;
  std::unordered_map<std::string, int> files;
  int lastFileId = 0;
  std::string error;
  std::string version;
};

namespace {

StWriter* cast(StWriterHandle handle) {
  return static_cast<StWriter*>(handle);
}

void setError(StWriter* value, const char* message) {
  value->error = message == nullptr ? "unknown error" : message;
}

void setException(StWriter* value, const std::exception& error) {
  value->error = error.what();
}

int fileId(StWriter* value, const char* path) {
  const std::string key(path == nullptr ? "" : path);
  const auto found = value->files.find(key);
  if (found != value->files.end()) {
    return found->second;
  }

  const int id = value->writer.recordFile(key);
  if (id != 0) {
    value->files.emplace(key, id);
  } else {
    value->error = value->writer.getLastError();
  }
  return id;
}

sourcetrail::NameHierarchy nameHierarchy(const char* qualifiedName) {
  sourcetrail::NameHierarchy hierarchy;
  hierarchy.nameDelimiter = "::";

  // Parse each hierarchy component so SourcetrailDB creates parent/child
  // library/class/member nodes and implicit MEMBER edges.
  std::stringstream names(qualifiedName == nullptr ? "" : qualifiedName);
  std::string name;
  while (std::getline(names, name, ':')) {
    if (name.empty()) {
      continue;
    }
    if (names.peek() == ':') {
      names.get();
    }
    hierarchy.nameElements.push_back({"", name, ""});
  }
  if (hierarchy.nameElements.empty()) {
    hierarchy.nameElements.push_back({"", qualifiedName == nullptr ? "" : qualifiedName, ""});
  }
  return hierarchy;
}

sourcetrail::SymbolKind symbolKind(const char* value) {
  const std::string kind(value == nullptr ? "" : value);
  if (kind == "module") return sourcetrail::SymbolKind::MODULE;
  if (kind == "class") return sourcetrail::SymbolKind::CLASS;
  if (kind == "enum") return sourcetrail::SymbolKind::ENUM;
  if (kind == "type") return sourcetrail::SymbolKind::TYPE;
  if (kind == "typedef") return sourcetrail::SymbolKind::TYPEDEF;
  if (kind == "method") return sourcetrail::SymbolKind::METHOD;
  if (kind == "field") return sourcetrail::SymbolKind::FIELD;
  if (kind == "global_variable") {
    return sourcetrail::SymbolKind::GLOBAL_VARIABLE;
  }
  if (kind == "function") return sourcetrail::SymbolKind::FUNCTION;
  return sourcetrail::SymbolKind::TYPE;
}

sourcetrail::ReferenceKind referenceKind(const char* value) {
  const std::string kind(value == nullptr ? "" : value);
  if (kind == "call") return sourcetrail::ReferenceKind::CALL;
  if (kind == "inheritance") return sourcetrail::ReferenceKind::INHERITANCE;
  if (kind == "override") return sourcetrail::ReferenceKind::OVERRIDE;
  if (kind == "import") return sourcetrail::ReferenceKind::IMPORT;
  if (kind == "type_usage") return sourcetrail::ReferenceKind::TYPE_USAGE;
  if (kind == "annotation_usage") {
    return sourcetrail::ReferenceKind::ANNOTATION_USAGE;
  }
  return sourcetrail::ReferenceKind::USAGE;
}

}  // namespace

StWriterHandle st_writer_create(void) {
  try {
    return new StWriter();
  } catch (...) {
    return nullptr;
  }
}

void st_writer_destroy(StWriterHandle handle) {
  delete cast(handle);
}

int st_writer_supported_database_version(StWriterHandle handle) {
  auto* value = cast(handle);
  return value == nullptr ? 0 : value->writer.getSupportedDatabaseVersion();
}

const char* st_writer_version(StWriterHandle handle) {
  auto* value = cast(handle);
  if (value == nullptr) {
    return "";
  }
  value->version = value->writer.getVersionString();
  return value->version.c_str();
}

int st_writer_open(StWriterHandle handle, const char* path) {
  auto* value = cast(handle);
  if (value == nullptr || path == nullptr) {
    return 0;
  }
  try {
    if (value->writer.open(path)) {
      return 1;
    }
    value->error = value->writer.getLastError();
  } catch (const std::exception& error) {
    setException(value, error);
  } catch (...) {
    setError(value, "unknown exception");
  }
  return 0;
}

int st_writer_close(StWriterHandle handle) {
  auto* value = cast(handle);
  if (value == nullptr) {
    return 0;
  }
  try {
    if (value->writer.close()) {
      return 1;
    }
    value->error = value->writer.getLastError();
  } catch (const std::exception& error) {
    setException(value, error);
  } catch (...) {
    setError(value, "unknown exception");
  }
  return 0;
}

const char* st_writer_last_error(StWriterHandle handle) {
  auto* value = cast(handle);
  return value == nullptr ? "invalid handle" : value->error.c_str();
}

int st_writer_begin_transaction(StWriterHandle handle) {
  auto* value = cast(handle);
  if (value == nullptr) return 0;
  if (value->writer.beginTransaction()) return 1;
  value->error = value->writer.getLastError();
  return 0;
}

int st_writer_commit_transaction(StWriterHandle handle) {
  auto* value = cast(handle);
  if (value == nullptr) return 0;
  if (value->writer.commitTransaction()) return 1;
  value->error = value->writer.getLastError();
  return 0;
}

int st_writer_rollback_transaction(StWriterHandle handle) {
  auto* value = cast(handle);
  if (value == nullptr) return 0;
  if (value->writer.rollbackTransaction()) return 1;
  value->error = value->writer.getLastError();
  return 0;
}

int st_writer_record_file(StWriterHandle handle, const char* path) {
  auto* value = cast(handle);
  if (value == nullptr || path == nullptr) return 0;
  value->lastFileId = fileId(value, path);
  return value->lastFileId;
}

int st_writer_record_file_language(StWriterHandle handle, const char* language) {
  auto* value = cast(handle);
  if (value == nullptr || language == nullptr || value->lastFileId == 0) {
    return 0;
  }
  if (value->writer.recordFileLanguage(value->lastFileId, language)) return 1;
  value->error = value->writer.getLastError();
  return 0;
}

int st_writer_record_symbol(
    StWriterHandle handle,
    const char* stable_id,
    const char* qualified_name,
    const char* kind,
    int definition,
    const char* file,
    int start_line,
    int start_column,
    int end_line,
    int end_column) {
  auto* value = cast(handle);
  if (value == nullptr || stable_id == nullptr || qualified_name == nullptr) {
    return 0;
  }

  try {
    const auto found = value->symbols.find(stable_id);
    int id = found == value->symbols.end() ? 0 : found->second;
    if (id == 0) {
      id = value->writer.recordSymbol(nameHierarchy(qualified_name));
      if (id != 0) value->symbols.emplace(stable_id, id);
    }
    if (id == 0) {
      value->error = value->writer.getLastError();
      return 0;
    }
    if (!value->writer.recordSymbolKind(id, symbolKind(kind))) {
      value->error = value->writer.getLastError();
      return 0;
    }
    if (definition &&
        !value->writer.recordSymbolDefinitionKind(
            id, sourcetrail::DefinitionKind::EXPLICIT)) {
      value->error = value->writer.getLastError();
      return 0;
    }
    if (file != nullptr && file[0] != '\0') {
      const int file_id = fileId(value, file);
      if (file_id == 0 ||
          !value->writer.recordSymbolLocation(
              id, {file_id, start_line, start_column, end_line, end_column})) {
        value->error = value->writer.getLastError();
        return 0;
      }
    }
    return id;
  } catch (const std::exception& error) {
    setException(value, error);
  } catch (...) {
    setError(value, "unknown exception");
  }
  return 0;
}

int st_writer_record_reference(
    StWriterHandle handle,
    const char* context_id,
    const char* target_id,
    const char* kind,
    const char* file,
    int start_line,
    int start_column,
    int end_line,
    int end_column,
    int resolved) {
  (void)resolved;
  auto* value = cast(handle);
  if (value == nullptr || context_id == nullptr || target_id == nullptr ||
      file == nullptr) {
    return 0;
  }

  const auto source = value->symbols.find(context_id);
  const auto target = value->symbols.find(target_id);
  if (source == value->symbols.end() || target == value->symbols.end()) {
    value->error = "reference refers to a symbol that was not recorded";
    return 0;
  }

  const int id = value->writer.recordReference(
      source->second, target->second, referenceKind(kind));
  if (id == 0) {
    value->error = value->writer.getLastError();
    return 0;
  }
  const int file_id = fileId(value, file);
  if (file_id == 0 ||
      !value->writer.recordReferenceLocation(
          id, {file_id, start_line, start_column, end_line, end_column})) {
    value->error = value->writer.getLastError();
    return 0;
  }
  return id;
}

int st_writer_record_local_symbol(
    StWriterHandle handle,
    const char* stable_id,
    const char* name,
    const char* file,
    int start_line,
    int start_column,
    int end_line,
    int end_column) {
  auto* value = cast(handle);
  if (value == nullptr || stable_id == nullptr || name == nullptr ||
      file == nullptr) {
    return 0;
  }

  try {
    const auto found = value->localSymbols.find(stable_id);
    int id = found == value->localSymbols.end() ? 0 : found->second;
    if (id == 0) {
      id = value->writer.recordLocalSymbol(name);
      if (id != 0) value->localSymbols.emplace(stable_id, id);
    }
    if (id == 0) {
      value->error = value->writer.getLastError();
      return 0;
    }

    const int file_id = fileId(value, file);
    if (file_id == 0 ||
        !value->writer.recordLocalSymbolLocation(
            id, {file_id, start_line, start_column, end_line, end_column})) {
      value->error = value->writer.getLastError();
      return 0;
    }
    return id;
  } catch (const std::exception& error) {
    setException(value, error);
  } catch (...) {
    setError(value, "unknown exception");
  }
  return 0;
}

int st_writer_record_unresolved_reference(
    StWriterHandle handle,
    const char* context_id,
    const char* kind,
    const char* file,
    int start_line,
    int start_column,
    int end_line,
    int end_column) {
  auto* value = cast(handle);
  if (value == nullptr || context_id == nullptr || file == nullptr) return 0;
  const auto source = value->symbols.find(context_id);
  if (source == value->symbols.end()) return 0;

  const int file_id = fileId(value, file);
  if (file_id == 0) return 0;
  const int id = value->writer.recordReferenceToUnsolvedSymhol(
      source->second,
      referenceKind(kind),
      {file_id, start_line, start_column, end_line, end_column});
  if (id == 0) value->error = value->writer.getLastError();
  return id;
}

int st_writer_record_error(
    StWriterHandle handle,
    const char* message,
    int fatal,
    const char* file,
    int start_line,
    int start_column,
    int end_line,
    int end_column) {
  auto* value = cast(handle);
  if (value == nullptr || message == nullptr || file == nullptr) return 0;
  const int file_id = fileId(value, file);
  if (file_id == 0) return 0;
  if (value->writer.recordError(
          message,
          fatal != 0,
          {file_id, start_line, start_column, end_line, end_column})) {
    return 1;
  }
  value->error = value->writer.getLastError();
  return 0;
}
