# Değişiklik Günlüğü (Change Log) - Sürüm v2.0 (Artım 2)

**Dönem Projesi:** LogFlow İşlem Hattı (Pipe and Filter)  
**Öğrenci:** YunusEmre Demirci (2404040249)  
**Sürüm:** v2.0 (Etiket: `v2`)  
**Tarih:** Ekim 2026  

---

## 1. Genel Bakış ve Mimari Odak
Artım 2 kapsamında sistem:
- "İşlem hattı ham dizgi (`std::string`) taşır" anlayışından, "işlem hattı zengin bir alan modeli (`LogRecord`) taşır" anlayışına geçirilmiştir.
- Mevcut boru hattının ortasına yeni bir ara filtre aşaması (`ParserStage`) yerleştirilmiş ve komşu aşamaların (`FileLineSource`, `Source`, `Emitter`) hiçbirinin değişmediği gözlemlenmiştir.
- Disk ve dosya sistemine dokunmayan bir test dublörü (`CollectingEmitter`) ile kapsamlı bir birim test paketi ve satır kapsama (line coverage) analizi devreye alınmıştır.

---

## 2. Değiştirilen Dosyalar (Modified Files)

| Dosya | Değişiklik Nedeni ve Yapılan İşlem |
|---|---|
| [`ConsoleSink.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/ConsoleSink.hpp) | Önceki artımda `Sink<std::string>` olarak ham satırları yazdırırken, Artım 2'de `Sink<LogRecord>` arayüzünü uygulayacak şekilde güncellendi. Ayrıştırılmış `LogRecord` nesnesini tek satırlık okunabilir ve yapılandırılmış biçimde konsola basar. |
| [`Pipeline.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/Pipeline.hpp) | Önceki artımda yalnızca `Source -> Sink` doğrudan bağlantısını desteklerken, ara filtre/aşama (`Stage<In, Out>`) bağlamayı destekleyecek şekilde güncellendi. Yaşam döngüsü metodları (`open()` ve `close()`) işletildi. |
| [`main.cpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/main.cpp) | `FileLineSource` -> `ParserStage` -> `ConsoleSink` boru hattını birbirine bağlayacak ve işlem tamamlandığında toplam hatalı/atlanan satır sayısını ekrana yazdıracak şekilde güncellendi. |
| [`Makefile`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/Makefile) | `logflow` ikili dosyasını üretecek, birim testleri (`make test`) koşacak ve gcov kapsama analizini (`make coverage`) çalıştıracak hedeflerle zenginleştirildi. |
| [`Project1.dev`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/Project1.dev) | Yeni başlık dosyalarını (`LogRecord.hpp`, `ParserStage.hpp`, `CollectingEmitter.hpp`) içerecek biçimde güncellendi. |
| [`README.md`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/README.md) | Artım 2 gereksinimleri, demo soruları ve cevapları, kapsama oranları ve mimari detaylarla baştan sona güncellendi. |

---

## 3. Yeni Eklenen Dosyalar (Added Files)

| Dosya | Amaç ve Sorumluluk |
|---|---|
| [`LogRecord.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/LogRecord.hpp) | Değişmez (Immutable) alan modeli sınıfı. `Instant timestamp`, `clientIp`, `method`, `path`, `status`, `bytes`, `userAgent`, `attributes` ve `raw` alanlarını içerir. Builder tasarım deseni ile nesne inşası ve 5-7. haftalar için OCP genişleme noktası olan `attributes` haritasını barındırır. |
| [`ParserStage.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/ParserStage.hpp) | `Stage<std::string, LogRecord>` arayüzünü uygulayan Common Log Format (CLF) ve Combined Log Format ayrıştırıcı aşaması. Hatalı satırları sayar ve atlar (Artım 5'te Dead Letter Queue mimarisine zemin hazırlar). |
| [`CollectingEmitter.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/CollectingEmitter.hpp) | `Emitter<T>` arayüzünü uygulayan bellek içi Test Dublörü (Test Double / Spy). Testlerin dosya sistemine dokunmadan hızlı, yalıtılmış ve deterministik çalışmasını sağlar. |
| [`tests/test_runner.cpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/tests/test_runner.cpp) | Ödevde zorunlu kılınan 8 temel test senaryosu ve ek mimari doğrulama testlerini içeren sıfır bağımlılıklı (zero-dependency) birim test paketi. |
| [`data/access-small.log`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/data/access-small.log) | Ödev şartnamesinde belirtilen standart örnek log verisi. |
| [`data/access-malformed.log`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/data/access-malformed.log) | Hatalı satırların başarıyla atlanıp sayıldığını doğrulamak için hazırlanan karma test verisi. |
| [`CHANGELOG.md`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/CHANGELOG.md) | Bu değişiklik günlüğü belgesi. |

---

## 4. Değiştirilmeyen Dosyalar (Unchanged Files)

| Dosya | Durum ve Mimari Önemi |
|---|---|
| [`Source.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/Source.hpp) | **Değişmedi.** Soyut veri kaynağı arayüzü mimarinin temel yapı taşıdır. |
| [`FileLineSource.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/FileLineSource.hpp) | **Değişmedi.** Dosyadan satır satır metin okuma sorumluluğu aynen korundu. Araya yeni bir ayrıştırıcı aşama eklenmesi veri kaynağını hiçbir şekilde etkilemedi (Separation of Concerns). |
| [`Emitter.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/Emitter.hpp) | **Değişmedi.** Aşamalar arası veri iletim kontratı sabit kaldı. |
| [`Stage.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/Stage.hpp) | **Değişmedi.** Boru hattı filtre arayüzü polimorfik olarak korundu. |
| [`Sink.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/Sink.hpp) | **Değişmedi.** Boru hattı hedef arayüzü korundu. |

---

## 5. Mimari Değerlendirme
Bu artım ile Pipe & Filter deseninin en büyük gücü olan:
1. **Separation of Concerns:** Dosya okuma (`FileLineSource`), veri ayrıştırma (`ParserStage`) ve ekrana yazdırma (`ConsoleSink`) birbirini tanımaz.
2. **Open-Closed Principle:** Araya yeni aşama eklendiğinde mevcut aşamalar değiştirilmemiş, yeni aşamalar sisteme kolayca entegre edilmiştir.
3. **Testability (Test Edilebilirlik):** `CollectingEmitter` sayesinde aşamalar I/O katmanından bağımsız olarak %97+ satır kapsama oranı ile test edilmiştir.
