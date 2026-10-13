#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <fstream>
#include "../ParserStage.hpp"
#include "../CollectingEmitter.hpp"
#include "../LogRecord.hpp"
#include "../ConsoleSink.hpp"
#include "../Pipeline.hpp"
#include "../PipelineBuilder.hpp"
#include "../PredicateFilterStage.hpp"
#include "../ProjectionStage.hpp"
#include "../FileSink.hpp"
#include "../FileLineSource.hpp"

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

// Yardımcı dosya okuma fonksiyonu
static std::vector<std::string> readFileLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        file.open(("../" + path).c_str());
    }
    if (!file.is_open()) {
        throw std::runtime_error("Dosya okunamadi: " + path);
    }
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

// ============================================================================
// ARTIM 2: ZORUNLU VE MEVCUT TESTLER (1-15)
// ============================================================================

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

TEST_CASE(test_2_eksik_alan) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    std::string line = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /index.html HTTP/1.1\"";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 1);
}

TEST_CASE(test_3_hatali_zaman_damgasi) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    std::string line = "192.168.1.10 - - [29/InvalidMon/2026:99:99:99 +0300] \"GET /index.html HTTP/1.1\" 200 1024";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 1);

    std::string line2 = "192.168.1.10 - - [not-a-timestamp] \"GET /index.html HTTP/1.1\" 200 1024";
    parser.process(line2, emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 2);
}

TEST_CASE(test_4_hatali_durum_kodu) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    std::string line1 = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /index.html HTTP/1.1\" BAD_STATUS 1024";
    parser.process(line1, emitter);

    std::string line2 = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /index.html HTTP/1.1\" 999 1024";
    parser.process(line2, emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 2);
}

TEST_CASE(test_5_bos_satir) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    parser.process("", emitter);
    parser.process("   \t   \r\n", emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 2);
}

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

    LogRecord enrichedRecord = baseRecord.withAttribute("geo_country", "TR")
                                         .withAttribute("authenticated", "true");

    ASSERT_EQ(baseRecord.getAttributes().size(), 0);
    ASSERT_EQ(enrichedRecord.getAttributes().size(), 2);
    ASSERT_STR_EQ(enrichedRecord.getAttributes().at("geo_country"), "TR");
    ASSERT_STR_EQ(enrichedRecord.getAttributes().at("authenticated"), "true");
}

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

TEST_CASE(test_13_protokolsüz_istek_ve_hata_sifirlama) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    parser.open();
    std::string line = "192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /simple-path\" 200 500";
    parser.process(line, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_STR_EQ(emitter[0].getPath(), "/simple-path");

    parser.process("192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"BADREQUEST\" 200 500", emitter);
    ASSERT_EQ(parser.getErrorCount(), 1);

    parser.resetErrorCount();
    ASSERT_EQ(parser.getErrorCount(), 0);
    parser.close();
}

TEST_CASE(test_14_hatali_bayt_degeri) {
    ParserStage parser;
    CollectingEmitter<LogRecord> emitter;

    parser.process("192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /test HTTP/1.1\" 200 NOT_BYTES", emitter);
    parser.process("192.168.1.10 - - [29/Sep/2026:10:00:00 +0300] \"GET /test HTTP/1.1\" 200 -50", emitter);

    ASSERT_EQ(emitter.size(), 0);
    ASSERT_EQ(parser.getErrorCount(), 2);
}

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

// ============================================================================
// ARTIM 3: YENİ AŞAMA VE MİMARİ TESTLERİ (16-31)
// ============================================================================

/**
 * @brief 16. Test: PredicateFilterStage Durum Kodu Filtresi (status >= 400)
 */
TEST_CASE(test_16_predicate_filter_status) {
    PredicateFilterStage filter(PredicateFilterStage::statusAtLeast(400));
    CollectingEmitter<LogRecord> emitter;

    LogRecord r200 = LogRecord::Builder().setStatus(200).setPath("/ok").build();
    LogRecord r401 = LogRecord::Builder().setStatus(401).setPath("/unauth").build();
    LogRecord r404 = LogRecord::Builder().setStatus(404).setPath("/notfound").build();
    LogRecord r500 = LogRecord::Builder().setStatus(500).setPath("/error").build();

    filter.process(r200, emitter);
    filter.process(r401, emitter);
    filter.process(r404, emitter);
    filter.process(r500, emitter);

    ASSERT_EQ(emitter.size(), 3);
    ASSERT_EQ(emitter[0].status(), 401);
    ASSERT_EQ(emitter[1].status(), 404);
    ASSERT_EQ(emitter[2].status(), 500);
}

/**
 * @brief 17. Test: PredicateFilterStage Yol Ön-eki Filtresi (path startsWith /api)
 */
TEST_CASE(test_17_predicate_filter_path_prefix) {
    PredicateFilterStage filter(PredicateFilterStage::pathStartsWith("/api"));
    CollectingEmitter<LogRecord> emitter;

    LogRecord r1 = LogRecord::Builder().setPath("/api/v1/users").build();
    LogRecord r2 = LogRecord::Builder().setPath("/api/login").build();
    LogRecord r3 = LogRecord::Builder().setPath("/index.html").build();
    LogRecord r4 = LogRecord::Builder().setPath("/static/style.css").build();

    filter.process(r1, emitter);
    filter.process(r2, emitter);
    filter.process(r3, emitter);
    filter.process(r4, emitter);

    ASSERT_EQ(emitter.size(), 2);
    ASSERT_STR_EQ(emitter[0].path(), "/api/v1/users");
    ASSERT_STR_EQ(emitter[1].path(), "/api/login");
}

/**
 * @brief 18. Test: PredicateFilterStage Zaman Damgası Aralığı Filtresi
 */
TEST_CASE(test_18_predicate_filter_timestamp_range) {
    using namespace std::chrono;
    auto t1 = system_clock::from_time_t(1000);
    auto t2 = system_clock::from_time_t(2000);
    auto t3 = system_clock::from_time_t(3000);

    PredicateFilterStage filter(PredicateFilterStage::timestampInRange(
        system_clock::from_time_t(1500),
        system_clock::from_time_t(2500)
    ));
    CollectingEmitter<LogRecord> emitter;

    filter.process(LogRecord::Builder().setTimestamp(t1).setPath("/t1").build(), emitter);
    filter.process(LogRecord::Builder().setTimestamp(t2).setPath("/t2").build(), emitter);
    filter.process(LogRecord::Builder().setTimestamp(t3).setPath("/t3").build(), emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_STR_EQ(emitter[0].path(), "/t2");
}

/**
 * @brief 19. Test: PredicateFilterStage Özel Lambda Koşulu (r -> r.status() >= 400 && r.bytes() > 1000)
 */
TEST_CASE(test_19_predicate_filter_custom_lambda) {
    PredicateFilterStage filter([](const LogRecord& r) {
        return r.status() >= 400 && r.bytes() > 1000;
    });
    CollectingEmitter<LogRecord> emitter;

    LogRecord r1 = LogRecord::Builder().setStatus(404).setBytes(500).build();   // status uyuyor, bytes uymuyor
    LogRecord r2 = LogRecord::Builder().setStatus(200).setBytes(2000).build();  // bytes uyuyor, status uymuyor
    LogRecord r3 = LogRecord::Builder().setStatus(500).setBytes(4096).build();  // her ikisi de uyuyor

    filter.process(r1, emitter);
    filter.process(r2, emitter);
    filter.process(r3, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_EQ(emitter[0].status(), 500);
    ASSERT_EQ(emitter[0].bytes(), 4096);
}

/**
 * @brief 20. Test: PredicateFilterStage Kayıt Bütünlüğünü Korur (Değiştirmeden iletir)
 */
TEST_CASE(test_20_predicate_filter_kayit_butunlugu) {
    PredicateFilterStage filter([](const LogRecord&) { return true; });
    CollectingEmitter<LogRecord> emitter;

    LogRecord orig = LogRecord::Builder()
        .setClientIp("192.168.1.50")
        .setMethod("DELETE")
        .setPath("/api/item/42")
        .setStatus(204)
        .setBytes(0)
        .setUserAgent("CustomClient/2.0")
        .build();

    filter.process(orig, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_STR_EQ(emitter[0].clientIp(), "192.168.1.50");
    ASSERT_STR_EQ(emitter[0].method(), "DELETE");
    ASSERT_STR_EQ(emitter[0].path(), "/api/item/42");
    ASSERT_EQ(emitter[0].status(), 204);
}

/**
 * @brief 21. Test: ProjectionStage Seçilen Alanların Alt Kümesine İndirgeme
 */
TEST_CASE(test_21_projection_stage_fields_subset) {
    ProjectionStage projection({"timestamp", "clientIp", "status", "path"});
    CollectingEmitter<LogRecord> emitter;

    auto now = std::chrono::system_clock::now();
    LogRecord fullRecord = LogRecord::Builder()
        .setTimestamp(now)
        .setClientIp("10.0.0.5")
        .setMethod("POST")
        .setPath("/submit")
        .setStatus(200)
        .setBytes(1500)
        .setUserAgent("Mozilla/5.0")
        .build();

    projection.process(fullRecord, emitter);

    ASSERT_EQ(emitter.size(), 1);
    const LogRecord& proj = emitter[0];
    ASSERT_TRUE(proj.isProjected());
    ASSERT_STR_EQ(proj.clientIp(), "10.0.0.5");
    ASSERT_STR_EQ(proj.path(), "/submit");
    ASSERT_EQ(proj.status(), 200);

    // Seçilmeyen alanların projeksiyonda sıfırlandığını/varsayılan olduğunu doğrula
    ASSERT_STR_EQ(proj.method(), "");
    ASSERT_EQ(proj.bytes(), 0);
    ASSERT_STR_EQ(proj.userAgent(), "");
}

/**
 * @brief 22. Test: ProjectionStage Veri Minimizasyonu ve Gizlilik (Privacy)
 */
TEST_CASE(test_22_projection_stage_data_minimization) {
    // Yalnızca durum ve yol seçilir; IP ve User-Agent gibi kişisel veriler ayıklanır
    ProjectionStage projection({"status", "path"});
    CollectingEmitter<LogRecord> emitter;

    LogRecord sensitive = LogRecord::Builder()
        .setClientIp("203.0.113.195")
        .setUserAgent("SecretBrowser/99")
        .setPath("/dashboard")
        .setStatus(200)
        .build();

    projection.process(sensitive, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_STR_EQ(emitter[0].clientIp(), "");
    ASSERT_STR_EQ(emitter[0].userAgent(), "");
    ASSERT_STR_EQ(emitter[0].path(), "/dashboard");
    ASSERT_EQ(emitter[0].status(), 200);
}

/**
 * @brief 23. Test: ProjectionStage toFormattedString Çıktısı
 */
TEST_CASE(test_23_projection_stage_to_formatted_string) {
    ProjectionStage projection({"clientIp", "status", "path"});
    CollectingEmitter<LogRecord> emitter;

    LogRecord rec = LogRecord::Builder()
        .setClientIp("192.168.1.1")
        .setStatus(404)
        .setPath("/notfound")
        .build();

    projection.process(rec, emitter);

    ASSERT_EQ(emitter.size(), 1);
    ASSERT_STR_EQ(emitter[0].toFormattedString(), "192.168.1.1 404 /notfound");
}

/**
 * @brief 24. Test: ProjectionStage Çoklu Kurucu Yöntemleri (vector, init_list, variadic)
 */
TEST_CASE(test_24_projection_stage_constructors) {
    ProjectionStage p1(std::vector<std::string>{"status", "path"});
    ProjectionStage p2{"status", "path"};
    ProjectionStage p3("status", "path");

    ASSERT_EQ(p1.getFields().size(), 2);
    ASSERT_EQ(p2.getFields().size(), 2);
    ASSERT_EQ(p3.getFields().size(), 2);
    ASSERT_STR_EQ(p3.getFields()[0], "status");
    ASSERT_STR_EQ(p3.getFields()[1], "path");
}

/**
 * @brief 25. Test: FileSink Yazma ve Kapatma Döngüsü
 */
TEST_CASE(test_25_filesink_write_and_close) {
    std::string testPath = "tests/output/filesink_test.log";
    FileSink sink(testPath);

    sink.open();
    LogRecord r1 = LogRecord::Builder().setClientIp("1.1.1.1").setMethod("GET").setPath("/").setStatus(200).setBytes(100).build();
    LogRecord r2 = LogRecord::Builder().setClientIp("2.2.2.2").setMethod("POST").setPath("/data").setStatus(201).setBytes(200).build();

    sink.consume(r1);
    sink.consume(r2);
    sink.close();

    auto lines = readFileLines(testPath);
    ASSERT_EQ(lines.size(), 2);
    ASSERT_TRUE(lines[0].find("1.1.1.1") != std::string::npos);
    ASSERT_TRUE(lines[1].find("2.2.2.2") != std::string::npos);
}

/**
 * @brief 26. Test: FileSink Açılmadan Tüketim Yapılırsa Hata Fırlatır
 */
TEST_CASE(test_26_filesink_lifecycle_unopened_throws) {
    FileSink sink("tests/output/unopened.log");
    LogRecord r = LogRecord::Builder().setStatus(200).build();

    bool threw = false;
    try {
        sink.consume(r);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    ASSERT_TRUE(threw);
}

/**
 * @brief 27. Test: FileSink close() Arabelleği Temizler ve Dosyayı Kapatır
 */
TEST_CASE(test_27_filesink_close_flushes_buffer) {
    std::string path = "tests/output/flush_test.log";
    {
        FileSink sink(path);
        sink.open();
        sink.consume(LogRecord::Builder().setClientIp("10.0.0.1").setMethod("GET").setPath("/flush").setStatus(200).setBytes(50).build());
        sink.close();
    }

    auto lines = readFileLines(path);
    ASSERT_EQ(lines.size(), 1);
    ASSERT_TRUE(lines[0].find("/flush") != std::string::npos);
}

/**
 * @brief 28. Test: PipelineBuilder Akıcı Çok Aşamalı Boru Hattı
 */
TEST_CASE(test_28_pipeline_builder_fluent_chain) {
    class DummyTestSource : public Source<std::string> {
    public:
        void produce(Emitter<std::string>& out) override {
            out.emit("192.168.1.1 - - [29/Sep/2026:10:00:00 +0300] \"GET /api/v1 HTTP/1.1\" 200 100");
            out.emit("192.168.1.2 - - [29/Sep/2026:10:01:00 +0300] \"GET /api/v2 HTTP/1.1\" 500 500");
        }
    };

    class DummyMemorySink : public Sink<LogRecord> {
    public:
        std::vector<LogRecord> items;
        void consume(const LogRecord& item) override {
            items.push_back(item);
        }
    };

    DummyMemorySink* memSink = new DummyMemorySink();
    Pipeline p = PipelineBuilder::from(new DummyTestSource())
        .via(new ParserStage())
        .via(new PredicateFilterStage(PredicateFilterStage::statusAtLeast(400)))
        .via(new ProjectionStage({"status", "path"}))
        .to(memSink, false);

    p.run();

    ASSERT_EQ(memSink->items.size(), 1);
    ASSERT_EQ(memSink->items[0].status(), 500);
    ASSERT_STR_EQ(memSink->items[0].path(), "/api/v2");
    delete memSink;
}

/**
 * @brief 29. Test: PipelineBuilder Yaşam Döngüsü Kancalarını (open/close) Sırayla Çalıştırır
 */
TEST_CASE(test_29_pipeline_builder_lifecycle_invocations) {
    static std::vector<std::string> lifecycleEvents;
    lifecycleEvents.clear();

    class LifecycleStage : public Stage<LogRecord, LogRecord> {
    private:
        std::string name;
    public:
        LifecycleStage(std::string n) : name(n) {}
        void open() override { lifecycleEvents.push_back(name + "_OPEN"); }
        void process(const LogRecord& in, Emitter<LogRecord>& out) override { out.emit(in); }
        void close() override { lifecycleEvents.push_back(name + "_CLOSE"); }
    };

    class LifecycleSink : public Sink<LogRecord> {
    public:
        void open() override { lifecycleEvents.push_back("SINK_OPEN"); }
        void consume(const LogRecord&) override {}
        void close() override { lifecycleEvents.push_back("SINK_CLOSE"); }
    };

    class DummySingleSource : public Source<LogRecord> {
    public:
        void produce(Emitter<LogRecord>& out) override {
            out.emit(LogRecord::Builder().setStatus(200).build());
        }
    };

    Pipeline p = PipelineBuilder::from(new DummySingleSource())
        .via(new LifecycleStage("STAGE1"))
        .via(new LifecycleStage("STAGE2"))
        .to(new LifecycleSink());

    p.run();

    // open() çağrıları: STAGE1 -> STAGE2 -> SINK
    ASSERT_TRUE(lifecycleEvents.size() == 6);
    ASSERT_STR_EQ(lifecycleEvents[0], "STAGE1_OPEN");
    ASSERT_STR_EQ(lifecycleEvents[1], "STAGE2_OPEN");
    ASSERT_STR_EQ(lifecycleEvents[2], "SINK_OPEN");

    // close() çağrıları: SINK -> STAGE2 -> STAGE1 (ters sıra kaynak yönetimi)
    ASSERT_STR_EQ(lifecycleEvents[3], "SINK_CLOSE");
    ASSERT_STR_EQ(lifecycleEvents[4], "STAGE2_CLOSE");
    ASSERT_STR_EQ(lifecycleEvents[5], "STAGE1_CLOSE");
}

/**
 * @brief 30. Test: Entegrasyon Testi - Test Fixture Üzerinde 3 Aşamalı Pipeline Çıktı Doğrulaması
 * 
 * Gereksinim:
 * "küçük bir test verisi (fixture) üzerinde 3 aşamalı bir pipeline çalıştırıp
 * çıktı dosyasının içeriğini doğrulayan (assert eden) bir entegrasyon testi."
 */
TEST_CASE(test_30_integration_3_stage_pipeline_fixture) {
    std::string fixturePath = "tests/fixtures/test_access.log";
    std::string outputPath = "tests/output/result_3stage.log";

    // 3 Aşamalı Pipeline: FileLineSource -> ParserStage -> PredicateFilterStage -> ProjectionStage -> FileSink
    Pipeline p = PipelineBuilder::from(new FileLineSource(fixturePath))
        .via(new ParserStage())
        .via(new PredicateFilterStage([](const LogRecord& r) { return r.status() >= 400; }))
        .via(new ProjectionStage({"timestamp", "clientIp", "status", "path"}))
        .to(new FileSink(outputPath));

    p.run();

    auto lines = readFileLines(outputPath);

    // Fixture içinde status >= 400 olan 4 geçerli satır vardır:
    // 192.168.1.15 (401), 192.168.1.25 (403), 192.168.1.30 (404), 192.168.1.35 (500)
    // Hatalı satır (MALFORMED_...) atlanmıştır.
    ASSERT_EQ(lines.size(), 4);

    ASSERT_STR_EQ(lines[0], "2026-09-29T07:01:05Z 192.168.1.15 401 /api/login");
    ASSERT_STR_EQ(lines[1], "2026-09-29T07:03:15Z 192.168.1.25 403 /api/secrets");
    ASSERT_STR_EQ(lines[2], "2026-09-29T07:04:20Z 192.168.1.30 404 /api/missing");
    ASSERT_STR_EQ(lines[3], "2026-09-29T07:05:25Z 192.168.1.35 500 /api/crash");
}

/**
 * @brief 31. Test: Entegrasyon Testi - Bileşilebilirlik ve Sıralama Bağımsızlığı (Order Invariance)
 * 
 * Gereksinim:
 * "Bileşilebilirliği deneysel olarak doğrulayın: aynı veriyi, filtre aşamaları
 * her iki sırada olacak şekilde pipeline'dan geçirin ve sonuçların aynı olduğunu belgeleyin."
 */
TEST_CASE(test_31_integration_composability_and_order_invariance) {
    std::string fixturePath = "tests/fixtures/test_access.log";
    std::string outOrderA = "tests/output/out_order_filter_first.log";
    std::string outOrderB = "tests/output/out_order_project_first.log";

    // Sıra A: Parser -> Filter -> Projection -> FileSink
    Pipeline pA = PipelineBuilder::from(new FileLineSource(fixturePath))
        .via(new ParserStage())
        .via(new PredicateFilterStage([](const LogRecord& r) { return r.status() >= 400; }))
        .via(new ProjectionStage({"timestamp", "clientIp", "status", "path"}))
        .to(new FileSink(outOrderA));
    pA.run();

    // Sıra B: Parser -> Projection -> Filter -> FileSink
    Pipeline pB = PipelineBuilder::from(new FileLineSource(fixturePath))
        .via(new ParserStage())
        .via(new ProjectionStage({"timestamp", "clientIp", "status", "path"}))
        .via(new PredicateFilterStage([](const LogRecord& r) { return r.status() >= 400; }))
        .to(new FileSink(outOrderB));
    pB.run();

    auto linesA = readFileLines(outOrderA);
    auto linesB = readFileLines(outOrderB);

    ASSERT_EQ(linesA.size(), linesB.size());
    for (size_t i = 0; i < linesA.size(); ++i) {
        ASSERT_STR_EQ(linesA[i], linesB[i]);
    }
}

/**
 * @brief 32. Test: Coklu Filtre Asamalarinin Arka Arkaya Zincirlenmesi (Multiple Filters in Series)
 * 
 * Gereksinim:
 * "Aşamaların (stage) herhangi bir sırada ve herhangi bir sayıda bir araya getirilebildiğini göstermek."
 * 
 * Bu test, boru hattında BİRDEN FAZLA PredicateFilterStage aşamasının (status >= 400 VE path startsWith /api VE bytes > 300)
 * arka arkaya eklenebildiğini ve her birinin tek sorumlulukla çalıştığını doğrular.
 */
TEST_CASE(test_32_multiple_predicate_filters_chained) {
    std::string fixturePath = "tests/fixtures/test_access.log";
    std::string outMulti = "tests/output/out_multi_filters.log";

    // 1. ParserStage
    // 2. Filtre 1: status >= 400
    // 3. Filtre 2: path startsWith /api
    // 4. Filtre 3: bytes > 300
    // 5. ProjectionStage: status, path, bytes
    // 6. FileSink
    Pipeline p = PipelineBuilder::from(new FileLineSource(fixturePath))
        .via(new ParserStage())
        .via(new PredicateFilterStage(PredicateFilterStage::statusAtLeast(400)))
        .via(new PredicateFilterStage(PredicateFilterStage::pathStartsWith("/api")))
        .via(new PredicateFilterStage([](const LogRecord& r) { return r.bytes() > 300; }))
        .via(new ProjectionStage({"status", "path", "bytes"}))
        .to(new FileSink(outMulti));
    p.run();

    auto lines = readFileLines(outMulti);
    // Fixture içinde status >= 400 ve /api olanlar:
    // 192.168.1.15 -> 401, /api/login, 512B (>300 -> KALIR)
    // 192.168.1.25 -> 403, /api/secrets, 256B (<=300 -> ELENIR)
    // 192.168.1.30 -> 404, /api/missing, 128B (<=300 -> ELENIR)
    // 192.168.1.35 -> 500, /api/crash, 4096B (>300 -> KALIR)
    ASSERT_EQ(lines.size(), 2);
    ASSERT_STR_EQ(lines[0], "401 /api/login 512");
    ASSERT_STR_EQ(lines[1], "500 /api/crash 4096");
}

// ============================================================================
// ANA TEST KOŞUCU (MAIN)
// ============================================================================

int main() {
    std::cout << "============================================================\n";
    std::cout << " LogFlow Artim 3 - Birim ve Entegrasyon Test Paketi\n";
    std::cout << "============================================================\n";

    // Artım 2 Mevcut Testleri (1-15)
    RUN_TEST(test_1_gecerli_satir);
    RUN_TEST(test_2_eksik_alan);
    RUN_TEST(test_3_hatali_zaman_damgasi);
    RUN_TEST(test_4_hatali_durum_kodu);
    RUN_TEST(test_5_bos_satir);
    RUN_TEST(test_6_fazladan_bosluk);
    RUN_TEST(test_7_bosluk_iceren_tirnakli_user_agent);
    RUN_TEST(test_8_sorgu_dizesi_iceren_satir);
    RUN_TEST(test_9_tire_boyut_alani);
    RUN_TEST(test_10_logrecord_immutability_and_builder);
    RUN_TEST(test_11_attributes_ocp_extension);
    RUN_TEST(test_12_combined_log_format_referer_ve_user_agent);
    RUN_TEST(test_13_protokolsüz_istek_ve_hata_sifirlama);
    RUN_TEST(test_14_hatali_bayt_degeri);
    RUN_TEST(test_15_collecting_emitter_metotlari);

    // Artım 3 Yeni Testleri (16-32)
    std::cout << "------------------------------------------------------------\n";
    std::cout << " Artim 3: Yeni Asama ve Entegrasyon Testleri\n";
    std::cout << "------------------------------------------------------------\n";
    RUN_TEST(test_16_predicate_filter_status);
    RUN_TEST(test_17_predicate_filter_path_prefix);
    RUN_TEST(test_18_predicate_filter_timestamp_range);
    RUN_TEST(test_19_predicate_filter_custom_lambda);
    RUN_TEST(test_20_predicate_filter_kayit_butunlugu);

    RUN_TEST(test_21_projection_stage_fields_subset);
    RUN_TEST(test_22_projection_stage_data_minimization);
    RUN_TEST(test_23_projection_stage_to_formatted_string);
    RUN_TEST(test_24_projection_stage_constructors);

    RUN_TEST(test_25_filesink_write_and_close);
    RUN_TEST(test_26_filesink_lifecycle_unopened_throws);
    RUN_TEST(test_27_filesink_close_flushes_buffer);

    RUN_TEST(test_28_pipeline_builder_fluent_chain);
    RUN_TEST(test_29_pipeline_builder_lifecycle_invocations);

    RUN_TEST(test_30_integration_3_stage_pipeline_fixture);
    RUN_TEST(test_31_integration_composability_and_order_invariance);
    RUN_TEST(test_32_multiple_predicate_filters_chained);

    std::cout << "============================================================\n";
    std::cout << "Test Sonuclari: " << g_passCount << " Basarili, " << g_failCount << " Basarisiz\n";
    std::cout << "============================================================\n";

    return (g_failCount == 0) ? 0 : 1;
}

