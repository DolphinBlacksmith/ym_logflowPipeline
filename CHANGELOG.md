# Değişiklik Günlüğü (Change Log)

**Dönem Projesi:** LogFlow İşlem Hattı (Pipe and Filter)  
**Öğrenci:** YunusEmre Demirci (2404040249)  

---

## [v4] Sürüm 4.0 (Artım 4 — Yapılandırma Odaklı Birleştirme ve Aşama Kaydı)
**Tarih:** Ekim 2026  
**Etiket:** `v4`  
**Mimari Odak:** Genişletilebilirlik, eklenti/kayıt defteri (plugin/registry) tasarım kalıbı, mimari tanımının mimari gerçekleştiriminden ayrılması ve yapılandırma zamanı tür doğrulaması (fail fast, fail loudly).

### 1. Yeni Eklenen Dosyalar
- [`StageFactory.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/StageFactory.hpp): Tür güvenli `ConfigValue`, parametre doğrulayıcı `StageParams`, `ConfigValidationException`, `PipelineWiringState` ve bileşen fabrika metotları (`createFileSource`, `createParseStage`, `createFilterStage`, `createProjectStage`, `createFileSink`, `createConsoleSink`).
- [`StageRegistry.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/StageRegistry.hpp): Sabit kodlanmış `switch` içermeyen, veri odaklı (`std::map<std::string, StageFactory>`) aşama/kaynak/hedef kayıt defteri ve `REGISTER_LOGFLOW_STAGE` otomatik eklenti kayıt makrosu.
- [`PipelineLoader.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/PipelineLoader.hpp): Sıfır bağımlılıklı YAML ayrıştırıcısı (`YamlConfigParser`) ve yapılandırmayı doğrulayıp `Pipeline` nesnesi inşa eden yükleyici sınıf.
- [`configs/error-report.yaml`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/configs/error-report.yaml): `status >= 400` hata raporu üreten bildirimsel yapılandırma (`out/errors.txt`).
- [`configs/api-traffic.yaml`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/configs/api-traffic.yaml): `path startsWith "/api"` API trafiği veri çıkarımı yapan yapılandırma (`out/api-traffic.txt`).
- [`configs/bandwidth-by-client.yaml`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/configs/bandwidth-by-client.yaml): `bytes >= 1024` istemci bazlı bant genişliği veri çıkarımı yapan yapılandırma (`out/bandwidth-by-client.txt`).
- [`data/access.log`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/data/access.log): Yapılandırma dosyalarının kullandığı örnek CLF erişim günlüğü.

### 2. Değiştirilen Dosyalar
- [`Pipeline.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/Pipeline.hpp): Yapılandırma dosyasındaki boru hattı adını (`name`) saklamak için `setName()` ve `getName()` erişimcileri eklendi.
- [`main.cpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/main.cpp): `--config <dosya.yaml>` (ve `-c`) komut satırı desteği ve Artım 4 interaktif yapılandırma/hata denetimi menüsü eklendi.
- [`tests/test_runner.cpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/tests/test_runner.cpp): Test sayısı 32'den 42'ye çıkarıldı; geçerli yapılandırma, bilinmeyen aşama türü, eksik parametre, yanlış türde parametre, boş işlem hattı, 3 ayrı yapılandırma çıktısı ve dinamik eklenti kaydı testleri eklendi.
- [`ARCHITECTURE.md`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/ARCHITECTURE.md): Bildirimsel YAML yapılandırma şeması, `StageRegistry`/`StageFactory`/`PipelineLoader` mimarisi ve yapılandırma zamanı vs. çalışma zamanı veri hataları ayrımı belgelendi.
- [`README.md`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/README.md): Artım 4 `--config` kullanımı, üç yapılandırmanın çıktıları ve yapılandırma zamanı hata mesajı örnekleri eklendi.

---

## [v3] Sürüm 3.0 (Artım 3 — Bileşim: Filtreleme, Projeksiyon ve Pipeline Builder)
**Tarih:** Ekim 2026  
**Etiket:** `v3`  
- `PredicateFilterStage`, `ProjectionStage`, `FileSink` ve `PipelineBuilder` eklendi.
- 25.000 satırlık sıralama deneyi (Filter-First vs. Project-First) gerçekleştirildi ve belgelendi.

---

## [v2] Sürüm 2.0 (Artım 2 — Tipli Kayıtlar ve İlk Gerçek Filtre)
**Tarih:** Ekim 2026  
**Etiket:** `v2`  
- `LogRecord` alan modeli, Builder deseni ve `ParserStage` CLF ayrıştırıcısı eklendi.

---

## [v1.0] Sürüm 1.0 (Artım 1 — Boru Hattı Başlangıcı)
**Tarih:** Eylül 2026  
**Etiket:** `v1.0`  
- `Source`, `Sink`, `Stage`, `Emitter` temel arayüzleri kuruldu.
