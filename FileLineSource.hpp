#pragma once
#include "Source.hpp"
#include <string>
#include <fstream>
#include <stdexcept>

class FileLineSource : public Source<std::string> {
private:
    std::string filePath;
public:
    // std::move kaldırıldı, doğrudan atama yapılıyor
    explicit FileLineSource(std::string path) : filePath(path) {}

    void produce(Emitter<std::string>& out) {
        // filePath.c_str() eklenerek eski derleyicilerle uyumlu hale getirildi
        std::ifstream file(filePath.c_str());
        if (!file.is_open()) {
            // VS Code veya IDE'nin binary'yi output/ klasöründen çalıştırması durumunda üst dizini kontrol et
            file.open(("../" + filePath).c_str());
        }
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filePath);
        }

        std::string line;
        while (std::getline(file, line)) {
            out.emit(line);
        }
    }
};
