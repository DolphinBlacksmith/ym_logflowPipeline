# Değişiklik Günlüğü (Change Log)

**Dönem Projesi:** LogFlow İşlem Hattı (Pipe and Filter)  
**Öğrenci:** YunusEmre Demirci (2404040249)  

---

## [v3] Sürüm 3.0 (Artım 3 — Bileşim: Filtreleme, Projeksiyon ve Pipeline Builder)
**Tarih:** Ekim 2026  
**Etiket:** `v3`  
**Mimari Odak:** Modülerlik, bileşilebilirlik (composability), aşama düzeyinde tek sorumluluk (SRP), veri minimizasyonu ve yaşam döngüsü yönetimi.

### 1. Yeni Eklenen Dosyalar
- [`PredicateFilterStage.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/PredicateFilterStage.hpp) (38 satır): Bir kaydı yalnızca `Predicate<LogRecord>` koşulu sağlanıyorsa ileten filtre aşaması (`status >= 400`, `path startsWith /api`, `timestampInRange`, özel lambda).
- [`ProjectionStage.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/ProjectionStage.hpp) (28 satır): Her kaydı seçilen alanların bir alt kümesine indirgeyen projeksiyon aşaması (veri minimizasyonu ve gizlilik odaklı).
- [`FileSink.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/FileSink.hpp) (37 satır): Bir çıktı dosyasına yazan, `open()` ve `close()` yaşam döngüsü kancalarını uygulayan ve arabellek kayıplarını önleyen hedef aşama.
- [`PipelineBuilder.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/PipelineBuilder.hpp): Aşamaların derleme zamanında tip güvenliğiyle, istenen sayıda ve sırada birbirine bağlanmasını sağlayan akıcı (fluent) kurucu sınıfı.
- [`ARCHITECTURE.md`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/ARCHITECTURE.md): Mimari ilkeleri, aşama tasarımlarını ve 25.000 satırlık log kümesi üzerinde yürütülen sıralama deneyini (Ordering Experiment & Equivalence Test) belgeleyen kapsamlı teknik rapor.
- [`scripts/run_experiment.py`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/scripts/run_experiment.py): Sıralama deneyini (Sıra A: Filter-First vs. Sıra B: Project-First) otomatize eden, SHA-256 doğrulamasını yapan ve çalışma sürelerini ölçen betik.
- [`tests/fixtures/test_access.log`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/tests/fixtures/test_access.log): 3 aşamalı entegrasyon testi için küçük ve kontrollü test verisi.

### 2. Değiştirilen Dosyalar
- [`Sink.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/Sink.hpp): Yaşam döngüsü yönetimi için `virtual void open() {}` ve `virtual void close() {}` kancaları eklendi (geriye dönük tam uyumlu).
- [`LogRecord.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/LogRecord.hpp): Projeksiyon desteği (`projectedFields_`, `isProjected()`), tek tip biçimlendirme (`toFormattedString()`), Builder'da `project(...)` statik metodu ve lambda dostu kısa getter takma adları (`status()`, `path()`, `timestamp()` vb.) eklendi.
- [`ConsoleSink.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/ConsoleSink.hpp): Biçimlendirme işlemi `record.toFormattedString()` metoduna devredilerek projeksiyonlu çıktılarla tam uyumlu hale getirildi.
- [`Pipeline.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/Pipeline.hpp): Çok aşamalı akıcı boru hatlarını yürütecek ve RAII tabanlı otomatik kaynak temizliği yapacak şekilde güncellendi; Artım 2'deki 3 parametreli kurucu da korundu.
- [`main.cpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/main.cpp): `--min-status`, `--path-prefix`, `--fields`, `--out`, `--order` komut satırı argümanlarını ayrıştıran ve boru hattını `PipelineBuilder` ile dinamik inşa eden CLI filtre arayüzüne dönüştürüldü.
- [`tests/test_runner.cpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/tests/test_runner.cpp): 15 olan test sayısı 31'e çıkarıldı; PredicateFilterStage, ProjectionStage, FileSink, PipelineBuilder, 3 aşamalı entegrasyon ve sıralama bağımsızlığı testleri eklendi.
- [`Makefile`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/Makefile): Yeni başlık dosyaları, `run-filtered` ve `experiment` hedefleri eklendi.
- [`README.md`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/README.md): Artım 3 yenilikleri, yeni CLI kullanımı ve kapsama oranları eklendi.

### 3. Test ve Satır Kapsama Sonuçları
- **Testler:** 31 / 31 Başarılı (%100 Başarı)
- **Satır Kapsama (Line Coverage):** **%94.82** (984 satır üzerinden)
- **Sıralama Deneyi:** Sıra A ve Sıra B çıktılarının SHA-256 kriptografik özeti birebir aynı çıktı (`08562754...19a321f`).

---

## [v2] Sürüm 2.0 (Artım 2 — Tipli Kayıtlar ve İlk Gerçek Filtre)
**Tarih:** Ekim 2026  
**Etiket:** `v2`  
- `LogRecord` alan modeli ve Builder deseni eklendi.
- `ParserStage` Common Log Format ayrıştırıcısı yazıldı, hatalı satırlar sayıldı.
- `ConsoleSink` güncellendi.
- `CollectingEmitter` test dublörü ve 15 test senaryosu geliştirildi.
- GCOV kapsama oranı %97+ olarak raporlandı.

---

## [v1.0] Sürüm 1.0 (Artım 1 — Boru Hattı Başlangıcı)
**Tarih:** Eylül 2026  
**Etiket:** `v1.0`  
- `Source`, `Sink`, `Stage`, `Emitter` temel arayüzleri kuruldu.
- `FileLineSource` ve `ConsoleSink` ile doğrudan `Source -> Sink` boru hattı işletildi.
