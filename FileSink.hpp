#pragma once

#include "Sink.hpp"
#include "LogRecord.hpp"
#include <fstream>
#include <stdexcept>
#include <filesystem>

/**
 * @brief FileSink — cikti dosyasina yazar; open()/close() yasam dongusunu uygular.
 */
class FileSink : public Sink<LogRecord> {
private:
    std::string filePath;
    std::ofstream outFile;

public:
    explicit FileSink(std::string path) : filePath(std::move(path)) {}
    ~FileSink() override { close(); }

    void open() override {
        auto p = std::filesystem::u8path(filePath);
        if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
        outFile.open(filePath.c_str());
        if (!outFile.is_open()) throw std::runtime_error("Could not open output file: " + filePath);
    }

    void consume(const LogRecord& record) override {
        if (!outFile.is_open()) throw std::runtime_error("FileSink is not open: " + filePath);
        outFile << record.toFormattedString() << "\n";
    }

    void close() override {
        if (outFile.is_open()) {
            outFile.flush();
            outFile.close();
        }
    }
};
