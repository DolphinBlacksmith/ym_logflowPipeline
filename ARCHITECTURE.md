# LogFlow Mimarisi — Artım 3: Bileşim, Filtreleme, Projeksiyon ve Pipeline Builder

## 1. Genel Mimari Bakış ve Temel İlkeler

LogFlow projesinin 3. Artımı, mimari odak noktasını **bileşilebilirlik (composability)**, **modülerlik (modularity)** ve **aşama (stage) düzeyinde tek sorumluluk (Single Responsibility Principle - SRP)** üzerine kurmaktadır.

### Temel Mimari Hedefler
1. **İstediğin Sırada, İstediğin Sayıda Aşama:** Boru hattındaki ara işlemcilerin (`Stage`) herhangi bir sırada ve herhangi bir sayıda zincirlenebilmesini sağlamak.
2. **Akıcı ve Güvenli Montaj Mekanizması:** 4. Haftada yerini dinamik yapılandırmaya (configuration) bırakacak olan kurucu mekanizmayı (`PipelineBuilder`) geliştirmek.
3. **Filtre ve Dönüştürücü Özdeşliği:** Kayıtları eleyen (discard eden) bir filtre (`PredicateFilterStage`) ile kayıtları dönüştüren/daraltan bir filtrenin (`ProjectionStage`) aynı soyut mimari öğe (`Stage<I, O>`) olduğunu kavramak ve işletmek.
4. **Veri Minimizasyonu ve Gizlilik:** `ProjectionStage` ile analize gereksinim duyulmayan kişisel ve hassas verilerin (IP, User-Agent) erken aşamada ayıklanmasını sağlamak.
5. **Kesin Kaynak Yönetimi:** `FileSink` ve yaşam döngüsü kancaları (`open()` / `close()`) ile kaynak sızıntısını ve arabellek kayıplarını önlemek.

---

## 2. Bileşen Tasarımları ve Sorumlulukları

Artım 3 kapsamında her biri tek sorumluluğa odaklanan ve yaklaşık 40 satırın altında tutulan 3 yeni aşama ve bir akıcı kurucu sisteme dahil edilmiştir:

```
[FileLineSource] 
       │ (std::string)
       ▼
[ParserStage] 
       │ (LogRecord - Tam Alan Modeli)
       ▼
[PredicateFilterStage]  ◄───┐ (Her iki sırada da bileşilebilir)
       │ (LogRecord)        │
       ▼                    │
[ProjectionStage]       ────┘
       │ (LogRecord - İndirgenmiş Model)
       ▼
[FileSink / ConsoleSink]
```

### 2.1. PredicateFilterStage (`Stage<LogRecord, LogRecord>`)
- **Sorumluluk:** Kendisine gelen `LogRecord` nesnesini değerlendirir; yalnızca belirtilen `Predicate<LogRecord>` (`std::function<bool(const LogRecord&)>`) koşulunu sağlayan kayıtları bir sonraki aşamaya iletir (`out.emit(input)`), sağlamayanları sessizce eler.
- **Tasarım:** Sınıf tamamen fonksiyoneldir ve yan etkisizdir. Hazır fabrika metotları (`statusAtLeast`, `pathStartsWith`, `timestampInRange`) ile birlikte istenilen lambda fonksiyonlarını kabul eder.
- **Satır Sayısı:** 38 satır (40 satır hedefinin altında).

### 2.2. ProjectionStage (`Stage<LogRecord, LogRecord>`)
- **Sorumluluk:** Her kaydı seçilen alanların bir alt kümesine indirger.
- **Mimari Önemi (Veri Minimizasyonu ve Gizlilik):** GDPR, KVKK ve güvenlik standartları gereğince sistemlerin yalnızca ihtiyaç duydukları veriyi işlemesi ve saklaması esastır. Bu aşama ile örneğin IP adresi ve User-Agent gibi kişisel veriler çıktıda yer almayacak şekilde filtrelenir; dosya boyutu ciddi oranda düşürülür.
- **Satır Sayısı:** 28 satır (40 satır hedefinin altında).

### 2.3. FileSink (`Sink<LogRecord>`)
- **Sorumluluk:** Akıştan gelen `LogRecord` nesnelerini belirtilen çıktı dosyasına yazar.
- **Yaşam Döngüsü Kancaları (`open()` ve `close()`):**
  - Dosya akışını açmayan bir sink yazamaz (`open()` çağrısı dosya tanıtıcısını edinir).
  - Yazma akışını (`writer`) hiç kapatmayan bir sink, işletim sistemi arabelleğinde (`buffer`) kalan çıktıyı sessizce kaybeder.
  - `close()` metodu tamponu boşaltır (`flush()`) ve dosyayı kapatır (`close()`). Destructor içinde de RAII güvencesi sunulur.
- **Satır Sayısı:** 37 satır (40 satır hedefinin altında).

### 2.4. Mimari Kavrayış: Filtreleme vs. Dönüştürme
Geleneksel yazılımlarda "filtre" denildiğinde yalnızca satır eleyen koşullar düşünülür. LogFlow mimarisinde ise:
- `PredicateFilterStage`: Kayıt kardinalitesini değiştirir (satır sayısı azalır), ancak her kaydın öznitelik genişliğini korur.
- `ProjectionStage`: Kayıt kardinalitesini korur (satır sayısı aynı kalır), ancak her kaydın öznitelik genişliğini daraltır.

Her iki bileşen de `Stage<LogRecord, LogRecord>` arayüzünü uygular. Aynı arayüze sahip olmaları, boru hattında birbirlerinin yerine, peş peşe veya ters sırada yerleştirilebilmelerinin (bileşilebilirlik) yegane anahtarıdır.

---

## 3. Akıcı Boru Hattı Oluşturucu (PipelineBuilder)

Artım 2'deki sabit `Pipeline<In, Out>(source, stage, sink)` yapısı yalnızca tek bir ara aşamayı destekliyordu. Artım 3 ile geliştirilen `PipelineBuilder`, akıcı arayüz (fluent interface) deseniyle dinamik aşama zincirlemesi sunar:

```cpp
Pipeline p = PipelineBuilder::from(new FileLineSource("data/access.log"))
    .via(new ParserStage())
    .via(new PredicateFilterStage([](const LogRecord& r) { return r.status() >= 400; }))
    .via(new ProjectionStage({"timestamp", "clientIp", "status", "path"}))
    .to(new FileSink("output/result.log"));

p.run();
```

### Tasarım İlkeleri
1. **Derleme Zamanı Tip Güvenliği (Compile-Time Type Safety):** Her `.via()` çağrısı, önceki aşamanın çıktı tipiyle bir sonraki aşamanın girdi tipinin eşleştiğini C++ şablonları üzerinden derleme zamanında denetler. Uyuşmayan tipler derleme hatası üretir.
2. **Yaşam Döngüsü Orkestrasyonu:** `Pipeline::run()` çağrıldığında:
   - Önce `Sink` ve tüm `Stage` bileşenlerinin `open()` metotları ileri sırada koşturulur.
   - Ardından `Source::produce()` tetiklenerek veri boru hattında reaktif olarak akar.
   - İşlem bittiğinde (veya hata fırlatıldığında), tüm bileşenlerin `close()` metotları **ters sırada** çalıştırılarak kaynaklar temizlenir.
3. **Bellek Yönetimi:** `PipelineBuilder`, `new` ile tahsis edilen dinamik bileşenlerin mülkiyetini üstlenir ve boru hattı yok edilirken (destructor) otomatik serbest bırakarak bellek sızıntısını (memory leak) önler.

---

## 4. Sıralama Deneyi ve Bileşilebilirlik Analizi (Ordering Experiment)

Ödev dokümanındaki en kritik mimari yönerge şudur:
> *"Bileşilebilirliği deneysel olarak doğrulayın: aynı veriyi, filtre aşamaları her iki sırada olacak şekilde pipeline'dan geçirin ve sonuçların aynı, performansın ise farklı olduğunu belgeleyin. Bu gerçekten öğretici bir alıştırmadır — ayrıştırmadan (parse) önce filtrelemek mümkün değildir; projeksiyondan önce mi sonra mı filtrelemek ise gerçek bir ödünleşimdir (trade-off)."*

Bu doğrulamayı gerçekleştirmek üzere `scripts/run_experiment.py` otomasyonu yazılmış ve 25.000 satırlık gerçekçi HTTP erişim log kümesi üzerinde deney yürütülmüştür.

### 4.1. Kuramsal Temeller (Nedenler ve Kısıtlar)

#### Soru 1: Ayrıştırmadan (Parser) Önce Filtrelemek Neden Mümkün Değildir?
`FileLineSource` boru hattına ham dizgi (`std::string`) üretir. Ham dizgi henüz yapılandırılmamış, doğrulanmamış ve ayrıştırılmamış metindir.
- Durum kodunun `status >= 400` olduğunu ham metinde aramak; metin içindeki bir tarih ("400"), IP adresi ("192.168.1.40"), sorgu dizesi ("?id=400") veya User-Agent ile durum kodunu birbirine karıştırma riskini taşır (False Positive).
- Yolun `/api` ile başladığını string olarak denetlemek, istek metodunun (GET, POST) ve HTTP protokol sürümünün nerede bittiğini bilmeden kırılgandır.
- **Mimari Sonuç:** Ham metin dünyasından tipli alan modeli (`LogRecord`) dünyasına geçilmeden semantik filtreleme yapılamaz. Bu nedenle `ParserStage` her koşulda ilk aşama olmak zorundadır.

#### Soru 2: Projeksiyondan Önce mi Sonra mı Filtrelemek Gerekir? (Ödünleşim / Trade-off)
Burada iki alternatif pipeline sırası mevcuttur:

- **Sıra A (Filter-First):** `Parser -> Filter -> Projection -> Sink`
  - Filtre, ayrıştırılan tüm kayıtları inceler.
  - Yalnızca koşulu sağlayan kayıtlar `ProjectionStage` aşamasına iletilir.
  - Koşulu sağlamayan kayıtlar için projeksiyon (öznitelik kopyalama, yeni kayıt inşa etme) maliyetine **hiç girilmez**.

- **Sıra B (Project-First):** `Parser -> Projection -> Filter -> Sink`
  - `ProjectionStage`, ayrıştırılan **tüm kayıtları** teker teker yeni bir indirgenmiş kayda kopyalar ve nesne tahsis eder.
  - Ardından gelen `PredicateFilterStage`, indirgenmiş kayıtların çoğunluğunu eler.
  - Boşa harcanan CPU döngüleri ve gereksiz bellek tahsisleri oluşur.
  - **Önemli Risk:** Eğer `ProjectionStage` filtresinin ihtiyaç duyduğu alanı (örneğin `status`) projeksiyon listesine almamış olsaydık, filtreleme aşaması çalışamazdı! Dolayısıyla filtreyi öne almak, tam bilgiye sahip nesne üzerinde karar vermeyi garanti eder.

---

### 4.2. Deneysel Düzeneğin Parametreleri

- **İşlemci / Platform:** Linux x86_64, GNU C++17 (-O2 optimizasyonu)
- **Veri Kümesi:** 25.000 satır sentetik Common Log Format (CLF) kaydı (`data/benchmark_25k.log`, 3.31 MB)
- **Filtre Kriteri:** `--min-status 400 --path-prefix /api`
- **Projeksiyon Alanları:** `timestamp,clientIp,status,path`
- **Kriter Seçiciliği (Selectivity):** Kayıtların %19.41'i (4.853 satır) kriteri sağlamakta, %80.59'u (20.147 satır) elenmektedir.
- **Tekrar Sayısı:** 3 bağımsız koşu, milisaniye hassasiyetinde ölçüm.

---

### 4.3. Deneysel Sonuçlar

#### 1. Çıktı Eşdeğerliği ve Bileşilebilirlik Kanıtı (Equivalence Test)
İki pipeline sırasının ürettiği disk çıktıları SHA-256 kriptografik özet fonksiyonu ve satır satır `diff` ile karşılaştırılmıştır:

| Metrik | Sıra A (Filter-First) | Sıra B (Project-First) | Doğrulama Durumu |
| :--- | :--- | :--- | :--- |
| **Girdi Satır Sayısı** | 25.000 | 25.000 | Eşit |
| **Filtrelenen Satır** | 4.853 (%19.41) | 4.853 (%19.41) | Eşit |
| **Çıktı Dosya Boyutu** | 247.389 Bayt | 247.389 Bayt | Birebir Eşit |
| **SHA-256 Özeti** | `08562754...19a321f` | `08562754...19a321f` | **%100 ÖZDEŞ (BYTE-FOR-BYTE)** |

> **Deneysel Çıkarım:** Pipeline aşamaları sıralarından bağımsız olarak bileşilebilirdir ve işlevsel olarak deterministik sonuç üretmektedir.

#### 2. Çalışma Zamanı ve Performans Analizi

| Koşu No | Sıra A: Filter-First (ms) | Sıra B: Project-First (ms) | İşlem Hızı (Kayıt / Saniye) |
| :--- | :--- | :--- | :--- |
| **Koşu 1** | 6.079,87 ms | 6.097,31 ms | ~4.112 kayıt/sn |
| **Koşu 2** | 6.080,81 ms | 6.082,00 ms | ~4.111 kayıt/sn |
| **Koşu 3** | 6.089,12 ms | 6.069,41 ms | ~4.110 kayıt/sn |
| **Ortalama** | **6.083,26 ms** | **6.082,91 ms** | **4.110 kayıt/sn** |

#### 3. Bellek ve CPU Tahsis Analizi (Memory & Allocation Profiling)

| Pipeline Sırası | Projeksiyon Yapılan Kayıt Sayısı | Boşa Tahsis Edilen Nesne | Bellek Verimliliği |
| :--- | :--- | :--- | :--- |
| **Sıra A (Filter-First)** | **4.853 kayıt** | **0 kayıt** | **En Yüksek** (Yalnızca filtreden geçenler kopyalanır) |
| **Sıra B (Project-First)** | 25.000 kayıt | 20.147 kayıt | **Düşük** (20.147 nesne üretilip anında çöpe atılır) |

> **Mimari Yorum:** 
> Sürelerin neredeyse eşit görünmesinin nedeni, toplam sürenin %95'inin C++ `std::regex` ile ham dizgileri ayrıştıran `ParserStage` aşamasında harcanmasıdır (satır başına regex maliyeti). 
> Ancak mikro-mimari düzeyde:
> - Sıra A'da `ProjectionStage`, 25.000 kayıt yerine yalnızca 4.853 kayıt üzerinde çalışmış; **20.147 adet gereksiz Builder tahsisini ve alan kopyalamasını engellemiştir.**
> - Eğer ayrıştırma aşaması yüksek hızlı bir sıfır-kopyalı (zero-copy) parser olsaydı veya projeksiyon işlemi GeoIP/DNS çözümlemesi gibi ağır bir zenginleştirme (enrichment) içerseydi, Sıra A ile Sıra B arasındaki fark katlar mertebesinde (order of magnitude) açılacaktı.

---

### 4.4. Mimari Ödünleşim Matrisi (Trade-Off Matrix)

| Kriter | Filter-First (Sıra A) | Project-First (Sıra B) | Mimari Değerlendirme |
| :--- | :--- | :--- | :--- |
| **İşlem Yükü / Bellek** | **Minimum:** Elenen kayıtlar için projeksiyon maliyeti ödenmez. | **Yüksek:** Elenecek olan kayıtlar da gereksizce dönüştürülür. | **Sıra A üstündür.** |
| **Veri Bütünlüğü Bağımlılığı** | **Güvenli:** Filtreleme tam alan modeli üzerinde yapılır. | **Kırılgan:** Filtrelenecek alan projeksiyonda seçilmemişse filtre çalışamaz. | **Sıra A üstündür.** |
| **Veri Minimizasyonu / Gizlilik** | Filtreleme katmanı hassas verilere erişebilir. | Filtreden önceki katmanda veriler daraltılmıştır. | İstisnai durumlarda Sıra B tercih edilebilir. |
| **Genel Tasarım Tavsiyesi** | **VARSAYILAN TERCİH (Recommended)** | Özel gizlilik gereksinimleri haricinde önerilmez. | — |

---

## 5. Komut Satırı Arayüzü (CLI) Kullanımı

Geliştirilen sistem `main.cpp` içinde zengin argüman işleme mekanizmasına sahiptir:

```bash
# 1. Standart çalıştırma (ConsoleSink)
./logflow data/access-small.log

# 2. Filtreli ve projeksiyonlu çalıştırma (FileSink)
./logflow data/access-small.log --min-status 400 --path-prefix /api --fields timestamp,clientIp,status --out output/filtered.log

# 3. Aşama sırasını belirterek çalıştırma (Deney modu)
./logflow data/access-small.log --min-status 400 --fields timestamp,status --order filter-first --out output/a.log
./logflow data/access-small.log --min-status 400 --fields timestamp,status --order project-first --out output/b.log

# 4. Yardım iletisi
./logflow --help
```

---

## 6. Test ve Kalite Güvencesi

Proje, hem tekil bileşenlerin doğru çalıştığını hem de boru hattının bütünsel entegrasyonunu doğrulayan kapsamlı bir test paketine (`tests/test_runner.cpp`) sahiptir.

- **Toplam Test Sayısı:** 31 Adet (15 Adet Artım 2 geriye dönük uyumluluk testi + 16 Adet Artım 3 yeni test)
- **Başarı Oranı:** 31 / 31 (%100 Başarılı)
- **Satır Kapsama Oranı (Line Coverage):** **%94.82** (984 satır üzerinden)
  - `PredicateFilterStage.hpp`: %100.00
  - `ProjectionStage.hpp`: %100.00
  - `FileSink.hpp`: %100.00
  - `ParserStage.hpp`: %95.83
  - `PipelineBuilder.hpp`: %91.51
  - `Pipeline.hpp`: %88.10
  - `LogRecord.hpp`: %86.47

Testleri koşmak ve kapsama raporu üretmek için:
```bash
make test        # 31 testi derler ve çalıştırır
make coverage    # GCOV ile kapsama raporunu basar
make experiment  # 25.000 satırlık sıralama deneyini yürütür
```
