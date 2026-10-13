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
              << "  --filter <1|2|3|4|5>         Hazir filtre seceneklerinden birini dogrudan calistirir\n"
              << "  --min-status <kod>           Belirtilen durum koduna esit veya buyuk kayitlari filtreler (or. 400)\n"
              << "  --path-prefix <on-ek>        Yolu belirtilen on-ek ile baslayan kayitlari filtreler (or. /api)\n"
              << "  --fields <alan1,alan2,...>   Yalnizca secilen alanlari projeksiyonla ciktiya aktarir\n"
              << "  --out, -o, --output <dosya>  Ciktinin yazilacagi dosya yolu (FileSink)\n"
              << "  --order <sira>               Asama siralamasi: 'filter-first' (varsayilan) veya 'project-first'\n"
              << "  --help, -h                   Bu yardim iletisini gosterir\n\n"
              << "Ornekler:\n"
              << "  " << progName << " --filter 1\n"
              << "  " << progName << " data/access-small.log --min-status 400 --path-prefix /api --fields timestamp,clientIp,status\n";
}

// Belirtilen konfigürasyona göre boru hattını çalıştıran fonksiyon
static void executePipeline(const std::string& inputFile,
                            const std::string& outputFile,
                            int filterChoice,
                            int minStatus,
                            const std::string& pathPrefix,
                            const std::string& fieldsArg,
                            const std::string& order) {
    std::cout << "\n======================================================================\n";
    std::cout << " LogFlow Boru Hatti (Artim 3) Calistiriliyor...\n";
    std::cout << " Girdi Dosyasi : " << inputFile << "\n";
    if (!outputFile.empty()) {
        std::cout << " Cikti Hedefi  : " << outputFile << " (FileSink)\n";
    } else {
        std::cout << " Cikti Hedefi  : Standart Terminal (ConsoleSink)\n";
    }

    std::vector<std::string> fields = split(fieldsArg, ',');
    bool hasProjection = !fields.empty();

    ParserStage parser;
    auto step = PipelineBuilder::from(new FileLineSource(inputFile))
        .via(&parser, false);

    // Filtreleri hazırla
    std::vector<Stage<LogRecord, LogRecord>*> filterStages;

    if (filterChoice == 1) {
        std::cout << " Secilen Filtre: [1] Durum Kodu Filtresi (status >= 400)\n";
        filterStages.push_back(new PredicateFilterStage(PredicateFilterStage::statusAtLeast(400)));
    } else if (filterChoice == 2) {
        std::cout << " Secilen Filtre: [2] Yol On-Eki Filtresi (path startsWith /api)\n";
        filterStages.push_back(new PredicateFilterStage(PredicateFilterStage::pathStartsWith("/api")));
    } else if (filterChoice == 3) {
        std::cout << " Secilen Filtre: [3] Zaman Damgasi Araligi (10:01:00 - 10:04:00)\n";
        filterStages.push_back(new PredicateFilterStage([](const LogRecord& r) {
            std::string iso = r.getTimestampIso();
            return (iso >= "2026-09-29T07:01:00Z" && iso <= "2026-09-29T07:04:00Z");
        }));
    } else if (filterChoice == 4) {
        std::cout << " Secilen Filtre: [4] Coklu Filtre Zinciri (status >= 400 VE path startsWith /api)\n";
        filterStages.push_back(new PredicateFilterStage(PredicateFilterStage::statusAtLeast(400)));
        filterStages.push_back(new PredicateFilterStage(PredicateFilterStage::pathStartsWith("/api")));
    } else {
        // Özel CLI argümanları (varsa)
        if (minStatus >= 0) {
            std::cout << " Filtre Kurali : min-status >= " << minStatus << "\n";
            filterStages.push_back(new PredicateFilterStage(PredicateFilterStage::statusAtLeast(minStatus)));
        }
        if (!pathPrefix.empty()) {
            std::cout << " Filtre Kurali : path-prefix = " << pathPrefix << "\n";
            filterStages.push_back(new PredicateFilterStage(PredicateFilterStage::pathStartsWith(pathPrefix)));
        }
        if (filterStages.empty()) {
            std::cout << " Filtre Kurali : Filtresiz (Tum Gecerli Kayitlar)\n";
        }
    }

    if (!fieldsArg.empty()) {
        std::cout << " Projeksiyon   : " << fieldsArg << "\n";
    }
    std::cout << "----------------------------------------------------------------------\n";

    // Aşama sıralaması
    if (order == "project-first") {
        if (hasProjection) {
            step = step.via(new ProjectionStage(fields));
        }
        for (auto* filter : filterStages) {
            step = step.via(filter);
        }
    } else {
        // Filter-First (Varsayılan ve Tavsiye Edilen)
        for (auto* filter : filterStages) {
            step = step.via(filter);
        }
        if (hasProjection) {
            step = step.via(new ProjectionStage(fields));
        }
    }

    // Hedefe bağla ve çalıştır
    if (!outputFile.empty()) {
        Pipeline p = step.to(new FileSink(outputFile));
        p.run();
        std::cout << "-> Cikti basariyla '" << outputFile << "' dosyasina yazildi.\n";
    } else {
        Pipeline p = step.to(new ConsoleSink());
        p.run();
    }

    std::cout << "----------------------------------------------------------------------\n";
    std::cout << " Boru Hatti Tamamlandi | Atlanan/Hatali Satir Sayisi: " << parser.getErrorCount() << "\n";
    std::cout << "======================================================================\n\n";
}

int main(int argc, char* argv[]) {
    std::string inputFile = "";
    std::string outputFile = "";
    int minStatus = -1;
    std::string pathPrefix = "";
    std::string fieldsArg = "";
    std::string order = "filter-first";
    int filterChoice = -1;

    std::vector<std::string> positionalArgs;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--filter" && i + 1 < argc) {
            filterChoice = std::stoi(argv[++i]);
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

    // Komut satırından özel bir filtre veya parametre verilmişse doğrudan çalıştır
    bool hasCliFlags = (filterChoice > 0 || minStatus >= 0 || !pathPrefix.empty() || !fieldsArg.empty() || !outputFile.empty());

    if (hasCliFlags) {
        executePipeline(inputFile, outputFile, filterChoice, minStatus, pathPrefix, fieldsArg, order);
        return 0;
    }

    // Parametresiz çalıştırma (IDE / F9): Hocaya gösterilecek interaktif seçim menüsü
    while (true) {
        std::cout << "\n======================================================================\n";
        std::cout << "       LogFlow Boru Hatti (Artim 3) - Filtre Secim Menusu\n";
        std::cout << "======================================================================\n";
        std::cout << "Girdi Log Dosyasi: " << inputFile << "\n\n";
        std::cout << "Hocanin istedigi 3 adet filtre hazirdir. Lutfen secim yapin:\n\n";
        std::cout << "  [1] 1. Filtre: Durum Kodu Filtresi (status >= 400)\n";
        std::cout << "      -> 400 ve uzeri hata kodlarina (401, 404, 500) sahip kayitlari getirir.\n\n";
        std::cout << "  [2] 2. Filtre: Yol On-Eki Filtresi (path startsWith /api)\n";
        std::cout << "      -> Yalnizca /api ile baslayan endpoint isteklerini getirir.\n\n";
        std::cout << "  [3] 3. Filtre: Zaman Damgasi Araligi (10:01:00 - 10:04:00)\n";
        std::cout << "      -> Belirtilen zaman diliminde gerceklesen kayitlari getirir.\n\n";
        std::cout << "  [4] Ekstra: Coklu Filtre Zinciri (status >= 400 VE path startsWith /api)\n";
        std::cout << "      -> Birden fazla filtre asamasinin arka arkaya baglandigi pipeline.\n\n";
        std::cout << "  [5] Filtresiz: Tum Gecerli Kayitlari Listele\n";
        std::cout << "  [0] Cikis\n";
        std::cout << "----------------------------------------------------------------------\n";
        std::cout << "Seciminiz [1-5 veya 0]: " << std::flush;

        std::string inputStr;
        if (!std::getline(std::cin, inputStr)) {
            // EOF veya terminal kapandı
            break;
        }

        size_t first = inputStr.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            std::cout << "-> Herhangi bir secim yapmadiniz. Lutfen bir sayi (1-5) tuslayip Enter'a basin.\n";
            continue;
        }
        std::string trimmed = inputStr.substr(first);

        int choice = -1;
        try {
            choice = std::stoi(trimmed);
        } catch (...) {
            std::cout << "\n[!] Gecersiz karakter! Lutfen sadece 0-5 arasi bir rakam girin.\n";
            continue;
        }

        if (choice == 0) {
            std::cout << "\nLogFlow'dan cikis yapildi. Iyi calismalar!\n";
            break;
        }

        if (choice >= 1 && choice <= 5) {
            executePipeline(inputFile, outputFile, choice, minStatus, pathPrefix, fieldsArg, order);
            std::cout << "(Farkli bir filtre denemek icin asagidan yeni secim yapabilir veya 0 ile cikabilirsiniz)\n";
        } else {
            std::cout << "\n[!] Gecersiz secim! Lutfen 1 ile 5 arasinda bir secenek belirleyin.\n";
        }
    }

    return 0;
}
