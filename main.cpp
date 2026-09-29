#include <iostream>
#include "FileLineSource.hpp"
#include "ConsoleSink.hpp"
#include "Pipeline.hpp"

int main(int argc, char* argv[]) {
    // Dev-C++ üzerinden F11 ile çalıştırırken argüman verilmediğinde hata almamak için:
    std::string filePath = "access-small.log"; 
    
    if (argc > 1) {
        filePath = argv[1];
    } else {
        std::cout << "Arguman verilmedi, varsayilan 'access-small.log' kullaniliyor...\n";
    }

    try {
        FileLineSource source(filePath);
        ConsoleSink sink;

        Pipeline<std::string, std::string> pipeline(&source, &sink);
        pipeline.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
