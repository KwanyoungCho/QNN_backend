#include "qnn_context_loader.h"

#include <iostream>

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <path/to/libQnnHtp.so> <path/to/context.bin> [graph_name]\n";
    return 1;
  }

  try {
    const std::string backend_path = argv[1];
    const std::string context_binary = argv[2];
    const std::string graph_name = argc > 3 ? argv[3] : "graph_0";

    auto result = qnn::load_context_and_graph(backend_path, context_binary, graph_name);

    std::cout << "Successfully instantiated QNN context and graph." << std::endl;
    std::cout << "  Backend handle: " << result.backend << std::endl;
    std::cout << "  Context handle: " << result.context << std::endl;
    std::cout << "  Graph handle:   " << result.graph << std::endl;
  } catch (const std::exception& ex) {
    std::cerr << "QNN initialization failed: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}

