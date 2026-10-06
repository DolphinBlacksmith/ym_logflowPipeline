#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "../ParserStage.hpp"
#include "../CollectingEmitter.hpp"
#include "../LogRecord.hpp"
#include "../ConsoleSink.hpp"
#include "../Pipeline.hpp"

// Basit ve Şık Birim Test Makroları (Zero-dependency xUnit / JUnit stili)
static int g_passCount = 0;
static int g_failCount = 0;

#define TEST_CASE(name) void name()
#define RUN_TEST(name) do { \
    std::cout << "[TEST] " << #name << " kosuluyor... "; \
    try { \
        name(); \
        std::cout << "BASARILI (PASSED)\n"; \
        g_passCount++; \
    } catch (const std::exception& e) { \
        std::cout << "BASARISIZ (FAILED): " << e.what() << "\n"; \
        g_failCount++; \
    } catch (...) { \
        std::cout << "BASARISIZ (FAILED): Bilinmeyen hata\n"; \
        g_failCount++; \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) throw std::runtime_error("Dogrulama basarisiz: " #cond); \
} while(0)

#define ASSERT_EQ(actual, expected) do { \
    if (!((actual) == (expected))) { \
        throw std::runtime_error("Esitlik basarisiz: Beklenen '" + std::to_string(expected) + "' fakat alinan '" + std::to_string(actual) + "'"); \
    } \
} while(0)

#define ASSERT_STR_EQ(actual, expected) do { \
    if ((actual) != (expected)) { \
        throw std::runtime_error("Dizgi esitligi basarisiz: Beklenen '" + std::string(expected) + "' fakat alinan '" + std::string(actual) + "'"); \
    } \
} while(0)

// ============================================================================
// ÖDEVDE İSTENEN ZORUNLU 8 TEST SENARYOSU (Dosya sistemine dokunmadan Emitter ile)
// ============================================================================

/**
 * @brief 1. Test: Geçerli Satır (Valid Line)
 * Standart bir CLF satırının tüm alanlarıyla hatasız ayrıştırıldığını doğrular.
 */
TEST_CASE(test_1_gecerli_satir) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    std::string line = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /index.html HTTP/1.1\" 200 1024";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_EQ(parser.getErrorCount(), 0);

    const LogRecord& rec = emitter[0];
    ASSERT_STR_EQ(rec.getClientIp(), "192.168.1.10");
    ASSERT_STR_EQ(rec.getMethod(), "GET");
    ASSERT_STR_EQ(rec.getPath(), "/index.html");
    ASSERT_EQ(rec.getStatus(), 200);
    ASSERT_EQ(rec.getBytes(), 1024);
    ASSERT_STR_EQ(rec.getRaw(), line);
    ASSERT_STR_EQ(rec.getTimestampIso(), "2026-09-29T07:00:00Z");
}

/**
 * @brief 2. Test: Eksik Alan (Missing Field)
 * Satırda zorunlu alanlar (durum kodu, boyut vb.) eksik olduğunda atlandığını ve hata sayıldığını doğrular.
 */
TEST_CASE(test_2_eksik_alan) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    // Durum kodu ve bayt boyutu eksik
    std::string line = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /index.html HTTP/1.1\"";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 1);
}

/**
 * @brief 3. Test: Hatalı Zaman Damgası (Invalid Timestamp)
 * Geçersiz format, geçersiz ay ismi veya sınır dışı tarih/saat değerlerinin reddedildiğini doğrular.
 */
TEST_CASE(test_3_hatali_zaman_damgasi) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    // Ay ismi 'InvalidMon' ve saat '99:99:99'
    std::string line = "192.168.1.10 - - [29/InvalidMon/2026:99:99:99 +0300] \"GET /index.html HTTP/1.1\" 200 1024";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 1);

    // Bozuk format örneği
    std::string line2 = "192.168.1.10 - - [not-a-timestamp] \"GET /index.html HTTP/1.1\" 200 1024";
    parser.process(line2, emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 2);
}

/**
 * @brief 4. Test: Hatalı Durum Kodu (Invalid Status Code)
 * Sayısal olmayan veya HTTP aralığı (100-599) dışındaki durum kodlarının reddedildiğini doğrular.
 */
TEST_CASE(test_4_hatali_durum_kodu) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    // Sayısal olmayan durum kodu
    std::string line1 = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /index.html HTTP/1.1\" BAD_STATUS 1024";
    parser.process(line1, emitter);

    // Aralık dışı (örneğin 999)
    std::string line2 = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /index.html HTTP/1.1\" 999 1024";
    parser.process(line2, emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 2);
}

/**
 * @brief 5. Test: Boş Satır (Empty Line)
 * Tamamen boş veya yalnızca sekme/boşluk karakteri içeren satırların atlandığını ve sayıldığını doğrular.
 */
TEST_CASE(test_5_bos_satir) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    parser.process("", emitter);
    parser.process("   \t   \r\n", emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 2);
}

/**
 * @brief 6. Test: Fazladan Boşluk (Extra Whitespace)
 * Alanlar arasında birden fazla boşluk veya satır başında/sonunda boşluk olduğunda ayrıştırıcının tolerans gösterip başarıyla çalıştığını doğrular.
 */
TEST_CASE(test_6_fazladan_bosluk) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    std::string line = "   192.168.1.10    -    -    [29/Sep/2026:10:00:00 +0300]    \"GET /index.html HTTP/1.1\"    200    1024   ";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_EQ(parser.getErrorCount(), 0);
    ASSERT_STR_EQ(emitter[0].getClientIp(), "192.168.1.10");
    ASSERT_EQ(emitter[0].getStatus(), 200);
    ASSERT_EQ(emitter[0].getBytes(), 1024);
}

/**
 * @brief 7. Test: Boşluk İçeren Tırnaklı User Agent (Quoted User Agent with Spaces)
 * İçinde boşluk, parantez ve özel karakterler bulunan tırnak içindeki User-Agent başlığının eksiksiz ayrıştırıldığını doğrular.
 */
TEST_CASE(test_7_bosluk_iceren_tirnakli_user_agent) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    std::string ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36";
    std::string line = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /index.html HTTP/1.1\" 200 1024 \"" + ua + "\"";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_EQ(parser.getErrorCount(), 0);
    ASSERT_STR_EQ(emitter[0].getUserAgent(), ua);
}

/**
 * @brief 8. Test: Sorgu Dizesi İçeren Bir Satır (Line with Query String)
 * İstek URL'sindeki '?id=42&name=test&filter=true' gibi sorgu parametrelerinin path içinde korunduğunu doğrular.
 */
TEST_CASE(test_8_sorgu_dizesi_iceren_satir) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    std::string line = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /search?query=software+architecture&page=2&sort=desc HTTP/1.1\" 200 1024";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_EQ(parser.getErrorCount(), 0);
    ASSERT_STR_EQ(emitter[0].getMethod(), "GET");
    ASSERT_STR_EQ(emitter[0].getPath(), "/search?query=software+architecture&page=2&sort=desc");
}

// ============================================================================
// EK MİMARİ VE GÜVENİLİRLİK TESTLERİ (BONUS)
// ============================================================================

/**
 * @brief 9. Test: Boyut Alanında Tire ('-') Olması Durumu
 * HTTP cevabında içerik olmadığında CLF standartlarına göre '-' yazılır; bunun 0 bayt olarak işlendiğini doğrular.
 */
TEST_CASE(test_9_tire_boyut_alani) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    std::string line = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /no-content HTTP/1.1\" 204 -";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_EQ(parser.getErrorCount(), 0);
    ASSERT_EQ(emitter[0].getStatus(), 204);
    ASSERT_EQ(emitter[0].getBytes(), 0);
}

/**
 * @brief 10. Test: LogRecord Değişmezlik (Immutability) ve Builder Deseni
 * LogRecord'un Builder ile güvenle oluşturulduğunu ve değişmez kaldığını doğrular.
 */
TEST_CASE(test_10_logrecord_immutability_and_builder) {
    auto now = std::chrono::system_clock::now();
    LogRecord record = LogRecord::Builder()
        .setTimestamp(now)
        .setClientIp("10.0.0.1")
        .setMethod("POST")
        .setPath("/api/v1/data")
        .setStatus(201)
        .setBytes(512)
        .setUserAgent("TestAgent/1.0")
        .addAttribute("env", "production")
        .setRaw("raw log content")
        .build();

    ASSERT_STR_EQ(record.getClientIp(), "10.0.0.1");
    ASSERT_STR_EQ(record.getMethod(), "POST");
    ASSERT_STR_EQ(record.getPath(), "/api/v1/data");
    ASSERT_EQ(record.getStatus(), 201);
    ASSERT_EQ(record.getBytes(), 512);
    ASSERT_STR_EQ(record.getUserAgent(), "TestAgent/1.0");
    ASSERT_STR_EQ(record.getRaw(), "raw log content");
    ASSERT_STR_EQ(record.getAttributes().at("env"), "production");
}

/**
 * @brief 11. Test: LogRecord attributes Haritası ve OCP Genişlemesi
 * 5-7. haftalarda yeni filtrelerin kayıt üzerine öznitelik ekleyebilme kabiliyetini doğrular.
 */
TEST_CASE(test_11_attributes_ocp_extension) {
    auto now = std::chrono::system_clock::now();
    LogRecord baseRecord = LogRecord::Builder()
        .setTimestamp(now)
        .setClientIp("10.0.0.1")
        .setMethod("GET")
        .setPath("/")
        .setStatus(200)
        .setBytes(100)
        .build();

    ASSERT_EQ(baseRecord.getAttributes().size(), 0);

    // Yeni öznitelik eklenmiş yeni değişmez kopya oluşturulur
    LogRecord enrichedRecord = baseRecord.withAttribute("geo_country", "TR")
                                         .withAttribute("authenticated", "true");

    // Orijinal nesne değişmez kalır
    ASSERT_EQ(baseRecord.getAttributes().size(), 0);

    // Zenginleştirilmiş nesne yeni özniteliklere sahiptir
    ASSERT_EQ(enrichedRecord.getAttributes().size(), 2);
    ASSERT_STR_EQ(enrichedRecord.getAttributes().at("geo_country"), "TR");
    ASSERT_STR_EQ(enrichedRecord.getAttributes().at("authenticated"), "true");
}

/**
 * @brief 12. Test: Combined Log Format (Hem Referer Hem User-Agent) Testi
 */
TEST_CASE(test_12_combined_log_format_referer_ve_user_agent) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    std::string line = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /index.html HTTP/1.1\" 200 1024 \"http://example.com/home\" \"Mozilla/5.0\"";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_EQ(parser.getErrorCount(), 0);
    ASSERT_STR_EQ(emitter[0].getUserAgent(), "Mozilla/5.0");
    ASSERT_STR_EQ(emitter[0].getAttributes().at("referer"), "http://example.com/home");
}

/**
 * @brief 13. Test: HTTP Protokolü Olmayan Basit İstek Yolu ve Hata Sayacı Sıfırlama
 */
TEST_CASE(test_13_protokolsüz_istek_ve_hata_sifirlama) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    parser.open();
    std::string line = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /simple-path\" 200 500";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_STR_EQ(emitter[0].getPath(), "/simple-path");

    // Hatalı istek satırı (boşluksuz metod)
    parser.process("192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"BADREQUEST\" 200 500", emitter);
    ASSERT_EQ(parser.getErrorCount(), 1);

    // Hata sayacını sıfırlama
    parser.resetErrorCount();
    ASSERT_EQ(parser.getErrorCount(), 0);
    parser.close();
}

/**
 * @brief 14. Test: Hatalı Bayt Değeri Testi
 */
TEST_CASE(test_14_hatali_bayt_degeri) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    // Sayısal olmayan bayt değeri
    parser.process("192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /test HTTP/1.1\" 200 NOT_BYTES", emitter);
    // Negatif bayt değeri
    parser.process("192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /test HTTP/1.1\" 200 -50", emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 2);
}

/**
 * @brief 15. Test: CollectingEmitter Metotları (clear, empty)
 */
TEST_CASE(test_15_collecting_emitter_metotlari) {
    CollectingEmitter<LogRecord> emitter;
    ASSERT_TRUE(emitter.empty());

    LogRecord rec = LogRecord::Builder()
        .setClientIp("127.0.0.1")
        .setMethod("GET")
        .setPath("/")
        .build();

    emitter.emit(rec);
    ASSERT_EQ(emitter.size(), 1);
    ASSERT_TRUE(!emitter.empty());

    emitter.clear();
    ASSERT_EQ(emitter.size(), 0);
    ASSERT_TRUE(emitter.empty());
}

int main() {
    std::cout << "============================================================\n";
    std::cout << " LogFlow Artim 2 - Birim Test Paketi (Unit Test Suite)\n";
    std::cout << "============================================================\n";

    // Zorunlu 8 Test Senaryosu
    RUN_TEST(test_1_gecerli_satir);
    RUN_TEST(test_2_eksik_alan);
    RUN_TEST(test_3_hatali_zaman_damgasi);
    RUN_TEST(test_4_hatali_durum_kodu);
    RUN_TEST(test_5_bos_satir);
    RUN_TEST(test_6_fazladan_bosluk);
    RUN_TEST(test_7_bosluk_iceren_tirnakli_user_agent);
    RUN_TEST(test_8_sorgu_dizesi_iceren_satir);

    // Ek Test Senaryoları
    RUN_TEST(test_9_tire_boyut_alani);
    RUN_TEST(test_10_logrecord_immutability_and_builder);
    RUN_TEST(test_11_attributes_ocp_extension);
    RUN_TEST(test_12_combined_log_format_referer_ve_user_agent);
    RUN_TEST(test_13_protokolsüz_istek_ve_hata_sifirlama);
    RUN_TEST(test_14_hatali_bayt_degeri);
    RUN_TEST(test_15_collecting_emitter_metotlari);

    std::cout << "============================================================\n";
    std::cout << "Test Sonuclari: " << g_passCount << " Basarili, " << g_failCount << " Basarisiz\n";
    std::cout << "============================================================\n";

    return (g_failCount == 0) ? 0 : 1;
}
