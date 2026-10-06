#pragma once

#include "Stage.hpp"
#include "LogRecord.hpp"
#include <string>
#include <regex>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iostream>

/**
 * @brief Common Log Format (CLF) ve Combined Log Format için Ayrıştırıcı Aşama.
 * 
 * Mimari Görev ve Sorumluluk:
 * 1. Ham metin satırını (`std::string`) alır, doğrular ve alan modeline (`LogRecord`) dönüştürür.
 * 2. Boru hattı içinde "dizgi" dünyasından "tipli alan modeli" dünyasına geçiş köprüsüdür.
 * 3. Hatalı satırlar: Bu hafta (Artım 2) mimarinin ilk adımı olarak hatalı satırlar
 *    yalnızca ATLANIR ve SAYILIR.
 * 
 * Gelecek Artımlar İçin Mimari Not:
 * Hatalı satırların daha kapsamlı ele alınması (Örn: Dead Letter Queue, Error Channel,
 * ayrıştırma hatası nedenini içeren ErrorRecord ve ayrı hata boru hattı) Artım 5'te
 * gelecektir. Mimari bu şekilde katman katman ve artımlı olarak inşa edilmektedir.
 */
class ParserStage : public Stage<std::string, LogRecord> {
private:
    size_t malformedCount = 0;
    std::regex logRegex;

    /**
     * @brief Gün, ay, yıl değerlerinden sivil takvime göre Unix gün sayısını hesaplar.
     * Howard Hinnant'ın C++20 standardına temel oluşturan taşınabilir ve bağımsız algoritmasıdır.
     */
    static time_t daysFromCivil(int y, int m, int d) {
        y -= (m <= 2);
        const int era = (y >= 0 ? y : y - 399) / 400;
        const unsigned yoe = static_cast<unsigned>(y - era * 400);
        const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
        const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097 + static_cast<int>(doe) - 719468;
    }

    /**
     * @brief CLF zaman damgasını (örn: "29/Sep/2026:10:00:00 +0300") UTC time_point'e çevirir.
     */
    static bool parseClfTimestamp(const std::string& tsStr, std::chrono::system_clock::time_point& outTp) {
        // Biçim: dd/Mon/yyyy:HH:MM:SS +ZZZZ veya -ZZZZ
        std::regex tsRegex(R"(^(\d{1,2})/([A-Za-z]{3})/(\d{4}):(\d{2}):(\d{2}):(\d{2})\s+([+-]\d{4})$)");
        std::smatch m;
        if (!std::regex_match(tsStr, m, tsRegex)) {
            return false;
        }

        int day = std::stoi(m[1].str());
        std::string monStr = m[2].str();
        int year = std::stoi(m[3].str());
        int hour = std::stoi(m[4].str());
        int min = std::stoi(m[5].str());
        int sec = std::stoi(m[6].str());
        std::string tz = m[7].str();

        // Temel aralık doğrulamaları
        if (day < 1 || day > 31 || hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 60) {
            return false;
        }

        const char* months[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
        int month = -1;
        for (int i = 0; i < 12; ++i) {
            if (monStr == months[i]) {
                month = i + 1; // 1-12
                break;
            }
        }
        if (month == -1) {
            return false;
        }

        time_t days = daysFromCivil(year, month, day);
        time_t localSecs = days * 86400 + hour * 3600 + min * 60 + sec;

        // Zaman dilimi sapması (+0300 -> +3 saat, 0 dakika)
        int tzSign = (tz[0] == '-') ? -1 : 1;
        int tzHours = std::stoi(tz.substr(1, 2));
        int tzMins = std::stoi(tz.substr(3, 2));
        int totalOffsetSec = tzSign * (tzHours * 3600 + tzMins * 60);

        // UTC zamanı = yerel zaman - ofset
        time_t utcSecs = localSecs - totalOffsetSec;
        outTp = std::chrono::system_clock::from_time_t(utcSecs);
        return true;
    }

public:
    ParserStage()
        // Regex: IP, Ident, User, [Timestamp], "Request", Status, Bytes, [Referer], [User-Agent]
        // Fazladan boşluklara (\s+) ve isteğe bağlı User-Agent/Referer alanlarına toleranslıdır.
        : logRegex(R"DELIM(^\s*(\S+)\s+(\S+)\s+(\S+)\s+\[([^\]]+)\]\s+"([^"]*)"\s+(\S+)\s+(\S+)(?:\s+"([^"]*)")?(?:\s+"([^"]*)")?\s*$)DELIM") {}

    ~ParserStage() override = default;

    void open() override {
        // Aşama başladığında sayacı sıfırlayabilir veya hazırlık yapabilir
    }

    void close() override {
        // Aşama tamamlandığında kaynakları temizleme
    }

    /**
     * @brief Gelen ham metin satırını ayrıştırır. Geçerliyse out.emit(LogRecord) çağrılır,
     * hatalıysa atlanır ve malformedCount artırılır.
     */
    void process(const std::string& input, Emitter<LogRecord>& out) override {
        // 1. Boş satır veya yalnızca boşluk içeren satır kontrolü
        if (input.find_first_not_of(" \t\r\n") == std::string::npos) {
            malformedCount++;
            return;
        }

        // 2. Regex ile desene uygunluk kontrolü
        std::smatch match;
        if (!std::regex_match(input, match, logRegex)) {
            malformedCount++;
            return;
        }

        std::string ip = match[1].str();
        // ident = match[2].str();
        // user = match[3].str();
        std::string timestampStr = match[4].str();
        std::string requestStr = match[5].str();
        std::string statusStr = match[6].str();
        std::string bytesStr = match[7].str();
        std::string opt1 = match[8].matched ? match[8].str() : "";
        std::string opt2 = match[9].matched ? match[9].str() : "";

        // 3. HTTP Durum Kodu Doğrulaması (100 - 599 arası tamsayı)
        int status = 0;
        try {
            size_t idx = 0;
            status = std::stoi(statusStr, &idx);
            if (idx != statusStr.length() || status < 100 || status > 599) {
                malformedCount++;
                return;
            }
        } catch (...) {
            malformedCount++;
            return;
        }

        // 4. Gönderilen Bayt Doğrulaması (negatif olamaz, '-' ise 0 kabul edilir)
        long long bytes = 0;
        if (bytesStr == "-") {
            bytes = 0;
        } else {
            try {
                size_t idx = 0;
                bytes = std::stoll(bytesStr, &idx);
                if (idx != bytesStr.length() || bytes < 0) {
                    malformedCount++;
                    return;
                }
            } catch (...) {
                malformedCount++;
                return;
            }
        }

        // 5. Zaman Damgası Ayrıştırma ve Doğrulama
        std::chrono::system_clock::time_point timestamp;
        if (!parseClfTimestamp(timestampStr, timestamp)) {
            malformedCount++;
            return;
        }

        // 6. İstek Satırını Ayrıştırma (Method ve Path - sorgu dizesi dahil)
        size_t firstSpace = requestStr.find(' ');
        if (firstSpace == std::string::npos || firstSpace == 0) {
            malformedCount++;
            return;
        }

        std::string method = requestStr.substr(0, firstSpace);
        std::string rest = requestStr.substr(firstSpace + 1);
        while (!rest.empty() && rest.front() == ' ') {
            rest.erase(0, 1);
        }

        std::string path;
        size_t lastSpace = rest.rfind(' ');
        if (lastSpace != std::string::npos && rest.substr(lastSpace + 1).rfind("HTTP/", 0) == 0) {
            path = rest.substr(0, lastSpace);
        } else {
            path = rest;
        }

        if (path.empty()) {
            malformedCount++;
            return;
        }

        // 7. User-Agent ve İsteğe Bağlı Öznitelikler (Attributes)
        std::string userAgent = "";
        std::map<std::string, std::string> attributes;

        if (!opt2.empty()) {
            // Hem Referer hem User-Agent mevcut (Combined Log Format)
            attributes["referer"] = opt1;
            userAgent = opt2;
        } else if (!opt1.empty()) {
            // Yalnızca tek tırnaklı ek alan mevcut (User-Agent)
            userAgent = opt1;
        }

        // 8. Alan Modeli Nesnesini Oluştur ve Sonraki Aşamaya Yay (Emit)
        LogRecord record(timestamp, ip, method, path, status, bytes, userAgent, attributes, input);
        out.emit(record);
    }

    /**
     * @brief Atlanan ve hatalı olan toplam satır sayısını döndürür.
     */
    size_t getErrorCount() const {
        return malformedCount;
    }

    /**
     * @brief Hata sayacını sıfırlar.
     */
    void resetErrorCount() {
        malformedCount = 0;
    }
};
