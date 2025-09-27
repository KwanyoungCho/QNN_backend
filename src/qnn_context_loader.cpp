#include "qnn_context_loader.h"

#include <dlfcn.h>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

// The real QNN headers define a struct full of function pointers. We only forward declare it
// in the header to keep this sample self-contained. When integrating into your build, include
// <QnnInterface.h> (or the appropriate backend interface header) *before* our header so that the
// compiler sees the full definition.
struct QnnInterface {
  // Minimal subset of the entry points we use. If your SDK exposes differently named symbols,
  // update these pointers to match the actual API.
  Qnn_ErrorHandle_t (*backendCreate)(QnnBackend_Handle_t* backend_handle);
  Qnn_ErrorHandle_t (*backendFree)(QnnBackend_Handle_t backend_handle);
  Qnn_ErrorHandle_t (*contextCreateFromBinary)(QnnBackend_Handle_t backend,
                                               const uint8_t* binary,
                                               size_t binary_size,
                                               QnnContext_Handle_t* context_handle);
  Qnn_ErrorHandle_t (*contextFree)(QnnContext_Handle_t context_handle);
  Qnn_ErrorHandle_t (*graphCreateFromContext)(QnnContext_Handle_t context,
                                              const char* graph_name,
                                              QnnGraph_Handle_t* graph_handle);
  Qnn_ErrorHandle_t (*graphFree)(QnnGraph_Handle_t graph_handle);
};

namespace qnn {

BackendLoader::BackendLoader(BackendLoader&& other) noexcept {
  *this = std::move(other);
}

BackendLoader& BackendLoader::operator=(BackendLoader&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  if (interface_ && backend_handle_) {
    interface_->backendFree(backend_handle_);
  }
  if (handle_) {
    dlclose(handle_);
  }

  handle_ = other.handle_;
  interface_ = other.interface_;
  backend_handle_ = other.backend_handle_;

  other.handle_ = nullptr;
  other.interface_ = nullptr;
  other.backend_handle_ = nullptr;
  return *this;
}

BackendLoader::~BackendLoader() {
  if (interface_ && backend_handle_) {
    interface_->backendFree(backend_handle_);
    backend_handle_ = nullptr;
  }

  if (handle_) {
    dlclose(handle_);
    handle_ = nullptr;
  }
}

void BackendLoader::load_backend(std::string_view backend_path, std::string_view interface_symbol) {
  if (backend_handle_) {
    return;  // already loaded
  }

  handle_ = dlopen(std::string(backend_path).c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!handle_) {
    throw std::runtime_error("Failed to dlopen backend: " + std::string(dlerror()));
  }

  using InterfaceGetter = const QnnInterface* (*)();
  auto* getter = reinterpret_cast<InterfaceGetter>(dlsym(handle_, std::string(interface_symbol).c_str()));
  if (!getter) {
    throw std::runtime_error("Unable to resolve interface symbol " + std::string(interface_symbol));
  }

  interface_ = getter();
  if (!interface_ || !interface_->backendCreate) {
    throw std::runtime_error("QNN interface is missing mandatory entry points");
  }

  check_qnn_status(interface_->backendCreate(&backend_handle_), "backendCreate failed");
}

std::vector<uint8_t> read_file(std::string_view path) {
  std::ifstream in(std::string(path), std::ios::binary);
  if (!in) {
    throw std::runtime_error("Unable to open file: " + std::string(path));
  }

  std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (data.empty()) {
    throw std::runtime_error("File is empty: " + std::string(path));
  }
  return data;
}

void check_qnn_status(Qnn_ErrorHandle_t status, std::string_view message) {
  if (status != 0) {
    throw std::runtime_error(std::string(message) + ": status=" + std::to_string(status));
  }
}

ContextAndGraph load_context_and_graph(std::string_view backend_path,
                                       std::string_view context_binary_path,
                                       std::string_view graph_name) {
  ContextAndGraph result;
  result.loader.load_backend(backend_path);
  result.interface = result.loader.interface();
  result.backend = result.loader.backend();

  std::vector<uint8_t> context_binary = read_file(context_binary_path);

  check_qnn_status(result.interface->contextCreateFromBinary(result.backend,
                                                             context_binary.data(),
                                                             context_binary.size(),
                                                             &result.context),
                   "contextCreateFromBinary failed");

  check_qnn_status(result.interface->graphCreateFromContext(result.context,
                                                            std::string(graph_name).c_str(),
                                                            &result.graph),
                   "graphCreateFromContext failed");

  return result;
}

}  // namespace qnn

