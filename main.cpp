#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "FileLineSource.hpp"
#include "ParserStage.hpp"
#include "PredicateFilterStage.hpp"
#include "ProjectionStage.hpp"
#include "FileSink.hpp"
#include "ConsoleSink.hpp"
#include "Pipeline.hpp"

static std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

static void printUsage(const char* progName) {
    std::cout << "Kullanim:\n"
              << "  " << progName << " [girdi_dosyasi] [secenekler]\n\n"
              << "Secenekler:\n"
              << "  --min-status <kod>           Belirtilen durum koduna esit veya buyuk kayitlari filtreler (or. 400)\n"
              << "  --path-prefix <on-ek>        Yolu belirtilen on-ek ile baslayan kayitlari filtreler (or. /api)\n"
              << "  --fields <alan1,alan2,...>   Yalnizca secilen alanlari projeksiyonla ciktiya aktarir\n"
              << "  --out, -o, --output <dosya>  Ciktinin yazilacagi dosya yolu (FileSink)\n"
              << "  --order <sira>               Asama siralamasi: 'filter-first' (varsayilan) veya 'project-first'\n"
              << "  --help, -h                   Bu yardim iletisini gosterir\n\n"
              << "Ornek:\n"
              << "  " << progName << " data/access-small.log --min-status 400 --path-prefix /api --fields timestamp,clientIp,status --out output/filtered.log\n";
}

int main(int argc, char* argv[]) {
    std::string inputFile = "";
    std::string outputFile = "";
    int minStatus = -1;
    std::string pathPrefix = "";
    std::string fieldsArg = "";
    std::string order = "filter-first";

    std::vector<std::string> positionalArgs;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--min-status" && i + 1 < argc) {
            minStatus = std::stoi(argv[++i]);
        } else if (arg == "--path-prefix" && i + 1 < argc) {
            pathPrefix = argv[++i];
        } else if (arg == "--fields" && i + 1 < argc) {
            fieldsArg = argv[++i];
        } else if ((arg == "--out" || arg == "-o" || arg == "--output") && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "--order" && i + 1 < argc) {
            order = argv[++i];
        } else if (arg.rfind("--", 0) == 0) {
            std::cerr << "Bilinmeyen secenek: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        } else {
            positionalArgs.push_back(arg);
        }
    }

    if (!positionalArgs.empty()) {
        inputFile = positionalArgs[0];
        if (positionalArgs.size() > 1 && outputFile.empty()) {
            outputFile = positionalArgs[1];
        }
    }

    if (inputFile.empty()) {
        inputFile = "data/access-small.log";
        std::ifstream testFile(inputFile.c_str());
        if (!testFile.is_open()) {
            inputFile = "access-small.log";
        }
    }

    try {
        std::cout << "LogFlow Boru Hatti (Artim 3) Baslatiliyor...\n";
        std::cout << "Girdi Dosyasi: " << inputFile << "\n";
        if (!outputFile.empty()) {
            std::cout << "Cikti Hedefi: " << outputFile << " (FileSink)\n";
        } else {
            std::cout << "Cikti Hedefi: Standart Konsol (ConsoleSink)\n";
        }

        std::cout << "Asama Sirasi: " << order << "\n";
        if (minStatus >= 0) std::cout << "Filtre: min-status >= " << minStatus << "\n";
        if (!pathPrefix.empty()) std::cout << "Filtre: path-prefix = " << pathPrefix << "\n";
        if (!fieldsArg.empty()) std::cout << "Projeksiyon Alanlari: " << fieldsArg << "\n";
        std::cout << "------------------------------------------------------------\n";

        // Filtre koşulu
        bool hasFilter = (minStatus >= 0 || !pathPrefix.empty());
        auto pred = [minStatus, pathPrefix](const LogRecord& r) {
            if (minStatus >= 0 && r.status() < minStatus) return false;
            if (!pathPrefix.empty() && r.path().rfind(pathPrefix, 0) != 0) return false;
            return true;
        };

        std::vector<std::string> fields = split(fieldsArg, ',');
        bool hasProjection = !fields.empty();

        // Ayrıştırıcı (Hata sayısını almak için referansla eklenir)
        ParserStage parser;

        // Akıcı Boru Hattı İnşası (PipelineBuilder)
        auto step = PipelineBuilder::from(new FileLineSource(inputFile))
            .via(&parser, false);

        if (order == "project-first") {
            if (hasProjection) {
                step = step.via(new ProjectionStage(fields));
            }
            if (hasFilter) {
                step = step.via(new PredicateFilterStage(pred));
            }
        } else {
            // Varsayılan: filter-first
            if (hasFilter) {
                step = step.via(new PredicateFilterStage(pred));
            }
            if (hasProjection) {
                step = step.via(new ProjectionStage(fields));
            }
        }

        // Hedef aşama bağlama ve boru hattı çalıştırma
        if (!outputFile.empty()) {
            Pipeline p = step.to(new FileSink(outputFile));
            p.run();
        } else {
            Pipeline p = step.to(new ConsoleSink());
            p.run();
        }

        std::cout << "------------------------------------------------------------\n";
        std::cout << "Boru Hatti Basariyla Tamamlandi.\n";
        std::cout << "Toplam hatali/atlanan satir sayisi: " << parser.getErrorCount() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Hata olustu: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
