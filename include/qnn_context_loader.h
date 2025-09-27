#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Forward declarations for QNN types to avoid needing the actual headers during host-side
// development. When building against the Qualcomm Neural Network SDK, include
// the appropriate QNN headers *before* this file or define the full types here.
struct QnnInterface;
typedef void* QnnBackend_Handle_t;
typedef void* QnnContext_Handle_t;
typedef void* QnnGraph_Handle_t;
typedef int Qnn_ErrorHandle_t;

namespace qnn {

//! Simple RAII wrapper around dlopen/dlsym for loading the QNN backend shared library.
class BackendLoader {
 public:
  BackendLoader() = default;
  BackendLoader(const BackendLoader&) = delete;
  BackendLoader& operator=(const BackendLoader&) = delete;
  BackendLoader(BackendLoader&&) noexcept;
  BackendLoader& operator=(BackendLoader&&) noexcept;
  ~BackendLoader();

  //! Loads the backend shared object and resolves the interface entry point.
  void load_backend(std::string_view backend_path, std::string_view interface_symbol = "QnnInterface");

  //! Returns the resolved QNN interface pointer.
  const QnnInterface* interface() const { return interface_; }

  //! Returns the backend handle created through the interface.
  QnnBackend_Handle_t backend() const { return backend_handle_; }

 private:
  void* handle_ = nullptr;
  const QnnInterface* interface_ = nullptr;
  QnnBackend_Handle_t backend_handle_ = nullptr;
};

//! Helper that materializes a QNN context and a graph from a serialized context binary.
struct ContextAndGraph {
  BackendLoader loader;
  const QnnInterface* interface = nullptr;
  QnnBackend_Handle_t backend = nullptr;
  QnnContext_Handle_t context = nullptr;
  QnnGraph_Handle_t graph = nullptr;
};

//! Loads a serialized QNN context binary from disk and instantiates a context and graph.
ContextAndGraph load_context_and_graph(std::string_view backend_path,
                                       std::string_view context_binary_path,
                                       std::string_view graph_name = "graph_0");

//! Utility to read a binary file into memory.
std::vector<uint8_t> read_file(std::string_view path);

//! Convenience wrapper that throws on non-success QNN status values.
void check_qnn_status(Qnn_ErrorHandle_t status, std::string_view message);

}  // namespace qnn

