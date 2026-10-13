# LogFlow - Yazılım Mimarisi Dönem Projesi
## Artım 3: Bileşim — Filtreleme, Projeksiyon ve Pipeline Builder ★★

- **Öğrenci Adı Soyadı:** YunusEmre Demirci  
- **Öğrenci Numarası:** 2404040249  
- **Ders:** Yazılım Tasarımı ve Mimarisi  
- **Sürüm Etiketi:** `v3`  
- **GitHub Deposu:** [DolphinBlacksmith/ym_logflowPipeline](https://github.com/DolphinBlacksmith/ym_logflowPipeline)

---

## 1. Mimari Odak ve Hedefler

LogFlow projesinin 3. Artımında sistem, basit bir 3 parçalı boru hattından **dinamik olarak bileşebilen (composable), modüler ve akıcı bir boru hattı mimarisine** yükseltilmiştir:

1. **Aşamaların Özgürce Bileşimi:** Aşamaların (`Stage`) herhangi bir sırada ve herhangi bir sayıda zincirlenebildiğini kanıtlamak.
2. **Akıcı PipelineBuilder:** 4. Haftada yerini dinamik yapılandırmaya (configuration) bırakacak olan kurucu mekanizmayı (`PipelineBuilder`) geliştirmek.
3. **Filtre ve Dönüştürücü Özdeşliği:** Kayıtları eleyen bir filtre (`PredicateFilterStage`) ile kayıtları dönüştüren bir filtrenin (`ProjectionStage`) aynı soyut mimari öğe (`Stage<I, O>`) olduğunu kavramak.
4. **Veri Minimizasyonu ve Gizlilik:** `ProjectionStage` ile analize gereksinim duyulmayan kişisel verilerin (IP, User-Agent) erken aşamada ayıklanmasını sağlamak.
5. **Kaynak Yönetimi ve Yaşam Döngüsü:** `FileSink` ve yaşam döngüsü kancaları (`open()` / `close()`) ile dosya tanıtıcılarını ve tampon bellekleri (buffer) sızıntısız yönetmek.
6. **Sıralama Deneyinin Belgelenmesi:** Filtreleme ve projeksiyon aşamalarının sırasının değiştirilmesinin sonuçlara ve performansa etkisini `ARCHITECTURE.md` belgesinde deneysel olarak kanıtlamak.

---

## 2. Yeni Geliştirilen Bileşenler (~40 Satır Altında)

Ödev yönergesine uygun olarak geliştirilen üç yeni aşama da temiz, odaklanmış ve 40 satırın altındadır:

| Bileşen | Taban Arayüz | Satır | Sorumluluk ve Mimari Rol |
|---|---|:---:|---|
| [`PredicateFilterStage.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/PredicateFilterStage.hpp) | `Stage<LogRecord, LogRecord>` | **38** | Bir kaydı yalnızca `Predicate<LogRecord>` koşulu sağlanıyorsa iletir. `status >= 400`, `path startsWith /api`, zaman damgası aralığı veya serbest lambda alır. |
| [`ProjectionStage.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/ProjectionStage.hpp) | `Stage<LogRecord, LogRecord>` | **28** | Her kaydı seçilen alanların bir alt kümesine indirger. Çıktı boyutunu azaltır ve kişisel verileri maskeler (veri minimizasyonu). |
| [`FileSink.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/FileSink.hpp) | `Sink<LogRecord>` | **37** | Bir çıktı dosyasına yazar; `open()` ve `close()` metotlarını doğru biçimde uygulayarak tamponda kalan çıktının kaybolmasını önler. |
| [`PipelineBuilder.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/PipelineBuilder.hpp) | Kurucu Sınıf | **95** | Akıcı (fluent) arayüz ile derleme zamanında tip güvenliği sunan ve aşamaları birbirine bağlayan kurucu. |

---

## 3. Akıcı Boru Hattı Kullanımı (Fluent PipelineBuilder)

Aşamalar istenilen sayıda ve sırada birbirine bağlanabilir:

```cpp
Pipeline p = PipelineBuilder::from(new FileLineSource("data/access.log"))
    .via(new ParserStage())
    .via(new PredicateFilterStage([](const LogRecord& r) { return r.status() >= 400; }))
    .via(new ProjectionStage({"timestamp", "clientIp", "status", "path"}))
    .to(new FileSink("output/filtered.log"));

p.run();
```

### Yaşam Döngüsü Kancaları (Lifecycle Hooks)
- `Pipeline::run()` çağrıldığında:
  1. `sink->open()` ve tüm `stage->open()` kancaları çalışır.
  2. `source->produce()` veri akışını tetikler; reaktif olarak aşamalar işletilir.
  3. İşlem tamamlandığında (veya bir hata fırlatıldığında), tüm aşamaların `close()` metotları **ters sırada** çalıştırılır ve arabellekler diske güvenle yazılır.

---

## 4. Komut Satırı Arayüzü (CLI Argument Handling)

`main.cpp` ile derlenen `logflow` ikili dosyası yapılandırılabilir parametrelere sahiptir:

```bash
# 1. Yardım Menüsü
./logflow --help

# 2. Standart Okuma ve Ayrıştırma (ConsoleSink)
./logflow data/access-small.log

# 3. Durum Kodu, Yol Ön-eki, Projeksiyon ve Dosya Çıktısı (FileSink)
./logflow data/access-small.log --min-status 400 --path-prefix /api --fields timestamp,clientIp,status --out output/filtered.log

# 4. Aşama Sıralama Seçeneği (Deney Modu)
./logflow data/access-small.log --min-status 400 --fields timestamp,status --order filter-first --out output/a.log
./logflow data/access-small.log --min-status 400 --fields timestamp,status --order project-first --out output/b.log
```

---

## 5. Sıralama Deneyi ve Mimari Rapor (`ARCHITECTURE.md`)

Ödevin en kritik gereksinimi olan **Bileşilebilirlik ve Sıralama Deneyi**, 25.000 satırlık sentetik erişim günlüğü üzerinde `scripts/run_experiment.py` betiği ile yürütülmüş ve [`ARCHITECTURE.md`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/ARCHITECTURE.md) dosyasında belgelenmiştir.

### Temel Çıkarımlar
1. **Ayrıştırmadan Önce Filtrelemek İmkansızdır:** Ham metin (`std::string`) durum kodu veya yol semantiğini içermez. String araması `False Positive` üretir. Bu yüzden `ParserStage` her zaman ilk adımdır.
2. **Çıktı Özdeşliği (Bileşilebilirlik Kanıtı):**
   - Sıra A: `Parser -> Filter -> Projection -> FileSink`
   - Sıra B: `Parser -> Projection -> Filter -> FileSink`
   - **Sonuç:** Her iki sıra da **birebir aynı 247.389 baytlık çıktıyı ve aynı SHA-256 özetini (`08562754...19a321f`)** üretmiştir.
3. **Performans ve Bellek Ödünleşimi:**
   - Sıra A'da (Filter-First), elenen 20.147 satır için gereksiz `ProjectionStage` nesne kopyalama maliyeti **ödenmemiştir**.
   - Sıra B'de (Project-First), 20.147 nesne boş yere dönüştürülüp anında çöpe atılmıştır.
   - Bu nedenle varsayılan mimari tercih **Filter-First** olmalıdır.

---

## 6. Test Paketi ve Kapsama Oranı

Birim ve entegrasyon test paketimiz (`tests/test_runner.cpp`) **31 adet test senaryosu** barındırır:

- **1-15:** Artım 2 geriye dönük uyumluluk testleri (Geçerli satır, hatalı durum, boşluk, tırnaklı UA, builder vb.)
- **16-20:** `PredicateFilterStage` testleri (status, path-prefix, timestamp range, custom lambda, kayıt bütünlüğü)
- **21-24:** `ProjectionStage` testleri (seçilen alanlar, veri minimizasyonu/gizlilik, toFormattedString, kurucular)
- **25-27:** `FileSink` testleri (open/write/close, açılmadan yazmada hata, tampon boşaltma)
- **28-29:** `PipelineBuilder` testleri (akıcı zincirleme, yaşam döngüsü kancalarının sırası)
- **30:** **3 Aşamalı Entegrasyon Testi:** Test fixture üzerinde okuyan, ayrıştıran, filtreleyen, projeksiyon uygulayan ve dosyaya yazan pipeline doğrulaması.
- **31:** **Bileşilebilirlik Entegrasyon Testi:** Sıra A ve Sıra B çıktılarının disk üzerinde %100 özdeşliğinin doğrulanması.

### Test ve Kapsama Komutları

```bash
# Testleri derle ve çalıştır (31 Test)
make test

# GCOV Satır Kapsama Raporu Üret
make coverage

# 25.000 Satırlık Sıralama Deneyini Çalıştır
make experiment
```

### GCOV Satır Kapsama Raporu (Line Coverage)
```text
File 'PredicateFilterStage.hpp':  Lines executed: 100.00%
File 'ProjectionStage.hpp':       Lines executed: 100.00%
File 'FileSink.hpp':              Lines executed: 100.00%
File 'ParserStage.hpp':           Lines executed:  95.83%
File 'PipelineBuilder.hpp':       Lines executed:  91.51%
File 'Pipeline.hpp':              Lines executed:  88.10%
File 'LogRecord.hpp':             Lines executed:  86.47%
---------------------------------------------------------
GENEL SATIR KAPSAMA:              Lines executed:  94.82% (984 satır)
```

---

## 7. Derleme ve Çalıştırma Kılavuzu

Proje standart C++17 derleyicisi (`g++`) ve `make` ile derlenir:

```bash
# Projeyi derle
make

# Örnek dosyayı çalıştır
make run

# Filtrelenmiş çıktıyı çalıştır ve gör
make run-filtered

# Testleri çalıştır
make test

# Temizle
make clean
```
