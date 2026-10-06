#include <iostream>
#include <string>
#include "FileLineSource.hpp"
#include "ParserStage.hpp"
#include "ConsoleSink.hpp"
#include "Pipeline.hpp"

int main(int argc, char* argv[]) {
    // Varsayılan log dosyası: data/access-small.log (veya yerel access-small.log)
    std::string filePath = "data/access-small.log";

    if (argc > 1) {
        filePath = argv[1];
    } else {
        // Dev-C++ veya proje dizininden doğrudan çalıştırma durumunda dosya kontrolü
        std::ifstream testFile(filePath.c_str());
        if (!testFile.is_open()) {
            filePath = "access-small.log";
        }
    }

    try {
        std::cout << "LogFlow Boru Hatti Baslatiliyor...\n";
        std::cout << "Girdi Dosyasi: " << filePath << "\n";
        std::cout << "------------------------------------------------------------\n";

        // 1. Kaynak: Metin dosyasından satır satır okur (Source<std::string>)
        FileLineSource source(filePath);

        // 2. Ara Aşama: CLF satırlarını LogRecord nesnesine ayrıştırır (Stage<std::string, LogRecord>)
        ParserStage parser;

        // 3. Hedef: Ayrıştırılan LogRecord nesnelerini tek satırda biçimlendirip konsola yazar (Sink<LogRecord>)
        ConsoleSink sink;

        // 4. Boru Hattı: Kaynak -> Ayrıştırıcı -> Hedef zincirini bağlar ve yürütür
        Pipeline<std::string, LogRecord> pipeline(&source, &parser, &sink);
        pipeline.run();

        // 5. Ödev Gereksinimi: Hatalı satırlar sayılır ve toplam sayı en sonda yazdırılır
        std::cout << "------------------------------------------------------------\n";
        std::cout << "Toplam hatali/atlanan satir sayisi: " << parser.getErrorCount() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Hata olustu: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
