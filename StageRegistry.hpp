#pragma once

#include "StageFactory.hpp"
#include <map>
#include <string>
#include <vector>
#include <sstream>

/**
 * @brief Veri Odaklı Aşama Kayıt Defteri (StageRegistry).
 *
 * Artım 4 Mimari Odak:
 * - Genişletilebilirlik (Extensibility / Open-Closed Principle)
 * - Eklenti / Kayıt Defteri (Plugin / Registry) Tasarım Kalıbı
 * - Sabit kodlanmış `switch` veya `if-else` blokları İÇERMEZ; tamamen veri odaklı
 *   (data-driven) tek bir kayıt tablosu (`std::map<std::string, StageFactory>`) ve
 *   otomatik eklenti kayıt mekanizması (`REGISTER_LOGFLOW_STAGE`) kullanır.
 */
class StageRegistry {
private:
    std::map<std::string, StageFactory> stageTable_;
    std::map<std::string, StageFactory::SourceCreator> sourceTable_;
    std::map<std::string, StageFactory::SinkCreator> sinkTable_;

    static std::string joinKeys(const std::vector<std::string>& keys) {
        std::ostringstream oss;
        for (size_t i = 0; i < keys.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << keys[i];
        }
        return oss.str();
    }

public:
    /**
     * @brief Varsayılan yerleşik bileşenleri tek bir veri odaklı kayıt tablosundan yükler.
     */
    StageRegistry()
        : stageTable_{
              {"parse",   StageFactory::forParserStage("parse",   &StageFactory::createParseStage)},
              {"filter",  StageFactory::forRecordStage("filter",  &StageFactory::createFilterStage)},
              {"project", StageFactory::forRecordStage("project", &StageFactory::createProjectStage)}
          },
          sourceTable_{
              {"file",    &StageFactory::createFileSource}
          },
          sinkTable_{
              {"file",    &StageFactory::createFileSink},
              {"console", &StageFactory::createConsoleSink}
          } {}

    /**
     * @brief Küresel (Singleton) Kayıt Defteri Erişimcisi.
     * Statik eklenti makroları (`REGISTER_LOGFLOW_STAGE`) bu örnek üzerinden otomatik kayıt yapar.
     */
    static StageRegistry& instance() {
        static StageRegistry globalRegistry;
        return globalRegistry;
    }

    // ========================================================================
    // EKLENTİ KAYIT METOTLARI (YENİ AŞAMALAR İÇİN AÇIK-KAPALI İLKESİ - OCP)
    // ========================================================================

    void registerStage(const std::string& typeName, const StageFactory& factory) {
        if (typeName.empty()) {
            throw ConfigValidationException("Kayit edilecek asama turu adi bos olamaz.");
        }
        stageTable_[typeName] = factory;
    }

    void registerStage(const std::string& typeName, StageFactory::RecordStageCreator creator) {
        registerStage(typeName, StageFactory::forRecordStage(typeName, std::move(creator)));
    }

    void registerParserStage(const std::string& typeName, StageFactory::ParserStageCreator creator) {
        registerStage(typeName, StageFactory::forParserStage(typeName, std::move(creator)));
    }

    void registerSource(const std::string& typeName, StageFactory::SourceCreator creator) {
        if (typeName.empty()) {
            throw ConfigValidationException("Kayit edilecek kaynak (source) turu adi bos olamaz.");
        }
        sourceTable_[typeName] = std::move(creator);
    }

    void registerSink(const std::string& typeName, StageFactory::SinkCreator creator) {
        if (typeName.empty()) {
            throw ConfigValidationException("Kayit edilecek hedef (sink) turu adi bos olamaz.");
        }
        sinkTable_[typeName] = std::move(creator);
    }

    // ========================================================================
    // SORGULAMA VE FABRİKA ÇÖZÜMLEME METOTLARI (SWITCH-FREE LOOKUP)
    // ========================================================================

    bool hasStage(const std::string& typeName) const {
        return stageTable_.find(typeName) != stageTable_.end();
    }

    bool hasSource(const std::string& typeName) const {
        return sourceTable_.find(typeName) != sourceTable_.end();
    }

    bool hasSink(const std::string& typeName) const {
        return sinkTable_.find(typeName) != sinkTable_.end();
    }

    std::vector<std::string> registeredStageTypes() const {
        std::vector<std::string> keys;
        keys.reserve(stageTable_.size());
        for (const auto& kv : stageTable_) keys.push_back(kv.first);
        return keys;
    }

    std::vector<std::string> registeredSourceTypes() const {
        std::vector<std::string> keys;
        keys.reserve(sourceTable_.size());
        for (const auto& kv : sourceTable_) keys.push_back(kv.first);
        return keys;
    }

    std::vector<std::string> registeredSinkTypes() const {
        std::vector<std::string> keys;
        keys.reserve(sinkTable_.size());
        for (const auto& kv : sinkTable_) keys.push_back(kv.first);
        return keys;
    }

    /**
     * @brief Aşama türü adına karşılık gelen StageFactory nesnesini bulur.
     * Tür kayıtlı değilse, kayıtlı tüm geçerli türleri içeren açık bir hata fırlatır.
     */
    const StageFactory& getStageFactory(const std::string& typeName) const {
        auto it = stageTable_.find(typeName);
        if (it == stageTable_.end()) {
            throw ConfigValidationException(
                "Bilinmeyen asama turu (Unknown stage type): '" + typeName +
                "'. Kayitli gecerli asama turleri: [" + joinKeys(registeredStageTypes()) + "].");
        }
        return it->second;
    }

    /**
     * @brief Kaynak (Source) türü adına karşılık gelen fabrikayı çalıştırır.
     */
    Source<std::string>* createSource(const std::string& typeName, const StageParams& params) const {
        auto it = sourceTable_.find(typeName);
        if (it == sourceTable_.end()) {
            throw ConfigValidationException(
                "Bilinmeyen kaynak turu (Unknown source type): '" + typeName +
                "'. Kayitli gecerli kaynak turleri: [" + joinKeys(registeredSourceTypes()) + "].");
        }
        return it->second(params);
    }

    /**
     * @brief Hedef (Sink) türü adına karşılık gelen fabrikayı çalıştırır.
     */
    Sink<LogRecord>* createSink(const std::string& typeName, const StageParams& params) const {
        auto it = sinkTable_.find(typeName);
        if (it == sinkTable_.end()) {
            throw ConfigValidationException(
                "Bilinmeyen hedef turu (Unknown sink type): '" + typeName +
                "'. Kayitli gecerli hedef turleri: [" + joinKeys(registeredSinkTypes()) + "].");
        }
        return it->second(params);
    }
};

/**
 * @brief Bildirimsel Otomatik Aşama Kayıt Yardımcısı (Self-Registering Plugin Helper).
 *
 * C#'taki `[Stage("name")]` özniteliği ve Java'daki `ServiceLoader` mekanizmasının
 * C++ karşılığıdır. Yeni bir aşama başlık dosyasında `REGISTER_LOGFLOW_STAGE("isim", fabrika)`
 * çağrıldığında program başlarken otomatik olarak `StageRegistry::instance()` tablosuna eklenir.
 */
struct StageAutoRegistrar {
    StageAutoRegistrar(const std::string& typeName, StageFactory::RecordStageCreator creator) {
        StageRegistry::instance().registerStage(typeName, std::move(creator));
    }
};

#define LOGFLOW_CONCAT_INNER(a, b) a##b
#define LOGFLOW_CONCAT(a, b) LOGFLOW_CONCAT_INNER(a, b)
#define REGISTER_LOGFLOW_STAGE(typeName, creatorFn) \
    static StageAutoRegistrar LOGFLOW_CONCAT(g_logflow_stage_reg_, __LINE__)(typeName, creatorFn)
