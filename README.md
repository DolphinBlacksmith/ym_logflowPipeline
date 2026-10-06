# LogFlow - Yazılım Mimarisi Dönem Projesi
## Artım 2: Tipli Kayıtlar ve İlk Gerçek Filtre ★★

- **Öğrenci Adı Soyadı:** YunusEmre Demirci  
- **Öğrenci Numarası:** 2404040249  
- **Ders:** Yazılım Tasarımı ve Mimarisi  
- **Sürüm Etiketi:** `v2`  
- **GitHub Deposu:** [DolphinBlacksmith/ym-pipeline-29.09](https://github.com/DolphinBlacksmith/ym-pipeline-29.09)

---

## 1. Mimari Odak ve Hedefler

Bu artımın temel mimari odak noktaları:
1. **Sorumlulukların Ayrılması (Separation of Concerns):** Boru hattındaki her bir bileşen tek bir sorumluluğa sahiptir. Dosya okuyucu (`FileLineSource`) yalnızca dosya okur, ayrıştırıcı (`ParserStage`) yalnızca metin doğrular ve dönüştürür, hedef (`ConsoleSink`) yalnızca ekrana basar.
2. **Veri Soyutlama ve Alan Modeli (Domain Abstraction):** *"İşlem hattı dizgi (string) taşır"* anlayışından, *"işlem hattı zengin ve güvenli bir alan modeli (domain model - `LogRecord`) taşır"* anlayışına geçilmiştir.
3. **Komşu Aşamaların Değişmezliği (Open-Closed Principle):** Mevcut işlem hattının ortasına yeni bir aşama (`ParserStage`) eklenmiş, ancak komşu kaynak aşaması (`FileLineSource`, `Source<I>`) ve arayüzler (`Emitter<T>`, `Stage<I,O>`) hiçbir değişikliğe uğramamıştır.
4. **Test Disiplini ve Test Dublörü:** Boru hattının önümüzdeki 9 hafta boyunca büyüyecek test paketi başlatılmış; dosya sistemine dokunmayan `CollectingEmitter` test dublörü devreye alınmıştır.

---

## 2. Alan Modeli: `LogRecord` (Domain Record)

`LogRecord` sınıfı, boru hattında taşınan değişmez (immutable) veri kaydıdır.

### Alanlar (Fields)
- `Instant timestamp`: `std::chrono::system_clock::time_point` türünde UTC zaman damgası (ISO-8601 formatına dönüştürülebilir).
- `String clientIp`: İstemci IP adresi (`std::string`).
- `String method`: HTTP istek metodu (`GET`, `POST` vb.).
- `String path`: İstek yolu ve sorgu parametreleri (`/search?q=arch` vb.).
- `int status`: HTTP durum kodu (200, 401, 404 vb.).
- `long bytes`: Yanıt boyutu (bayt cinsinden, `-` ise 0).
- `String userAgent`: İstemci tarayıcı/cihaz bilgisi (`Mozilla/5.0...`).
- `Map<String, String> attributes`: Dinamik öznitelik haritası.
- `String raw`: Ayrıştırılan orijinal ham log satırı.

### Değişmezlik (Immutability) ve Builder Deseni
- Sınıfın tüm alanları `const` olarak tanımlanmıştır. Nesne bir kez oluşturulduktan sonra bellekte durumu (state) değiştirilemez. Bu sayede çok iş parçacıklı (multithreaded) veya asenkron işlem hatlarında veri yarışmaları (race conditions) önlenir.
- Karmaşık parametre listelerini yönetmek için `LogRecord::Builder` deseni sunulmuştur.

### `attributes` Haritası Neden Var? (Open-Closed Principle)
> **Mimari Gerekçe:** `attributes` eşlemi (`std::map<std::string, std::string>`), **5–7. haftaların dayanacağı temel genişleme noktasıdır**. Kayıt biçimi değişime kapalı ancak genişletmeye açık olmalıdır (Open-Closed Principle). Çünkü sonraki aşamalar (GeoIP zenginleştirme, güvenlik denetimi, kimlik doğrulama filtreleri vb.), bugünkü `ParserStage`'in hiçbir şey bilmediği yeni metaverileri kayda ekleyecektir.  
> Bu amaçla `withAttribute(key, value)` metodu sunulmuştur: Orijinal kaydın değişmezliğini bozmadan yeni özniteliği içeren yeni bir `LogRecord` kopyası üretir (Functional Immutability).

---

## 3. `ParserStage` ve Hata Yönetimi Stratejisi

`ParserStage`, `Stage<std::string, LogRecord>` arayüzünü uygular. Common Log Format (CLF) ve Combined Log Format satırlarını doğrular ve `LogRecord` nesnelerine dönüştürür.

### Hatalı Satırlar Stratejisi
- **Bu Hafta (Artım 2):** Hatalı veya biçimsiz (malformed) satırlar boru hattında **yalnızca atlanır ve sayılır (`malformedCount`)**. Herhangi bir istisna fırlatılmaz veya geçersiz kayıt sonraki aşamaya iletilmez.
- **Mimari Katmanlaşma:** Hatalı satırların daha gelişmiş yöntemlerle ele alınması (Örn: *Dead Letter Queue*, *Error Channel*, hata kodları içeren *ErrorRecord* ve ikincil hata boru hatları) **5. haftada** gelecektir. Bu yaklaşım, yazılım mimarisinin adım adım ve katman katman olgunlaştığını açıkça göstermektedir.

---

## 4. `ConsoleSink` Çıktı Biçimi

`ConsoleSink`, `Sink<LogRecord>` arayüzünü uygular ve yapılandırılmış log kayıtlarını okunabilir, temiz ve tek satırlık bir formatta konsola yazar:

```text
[2026-09-29T07:00:00Z] 192.168.1.10 "GET /index.html" 200 1024B
[2026-09-29T07:01:05Z] 192.168.1.15 "POST /login" 401 512B
[2026-09-29T07:02:10Z] 192.168.1.20 "GET /dashboard" 200 2048B
[2026-09-29T07:04:20Z] 192.168.1.40 "GET /products?category=books&sort=asc" 200 4096B "Mozilla/5.0 (X11; Ubuntu; Linux x86_64)"
```

Program tamamlandığında en altta ödev şartnamesine uygun olarak:
```text
Toplam hatali/atlanan satir sayisi: <sayi>
```
bilgisi raporlanır.

---

## 5. Test Disiplini ve `CollectingEmitter` Test Dublörü

Ödev şartnamesinde belirtildiği üzere:
> *"Aşama testleri dosya sistemine dokunmamalıdır: aşamayı oluşturur, ona bir dizgi verir ve yaydığı çıktıları bir test Emitter nesnesiyle yakalar. Bu 'toplayıcı emitter' (collecting emitter) test dublörünü (test double) şimdi tanıtın; öğrenciler onu her hafta yeniden kullanacak."*

`CollectingEmitter<T>`, `Emitter<T>` arayüzünü uygulayan bir **Test Spy / Collector** dublörüdür:
- `emit(item)` çağrıldığında nesneyi dahili bir vektörde (`std::vector<T>`) depolar.
- Test senaryolarında dosya I/O işlemlerine gerek kalmaz.
- Testler milisaniyeler içinde, %100 yalıtılmış ve deterministik olarak koşar.

### Zorunlu 8 Test Senaryosu ve Gerçekleştirim

Birim test paketimiz (`tests/test_runner.cpp`) şartnamedeki 8 senaryonun tamamını ve ek mimari senaryoları kapsar:

| No | Test Senaryosu | Test Durumu Açıklaması | Sonuç |
|:---:|---|---|:---:|
| 1 | **Geçerli Satır** (`test_1_gecerli_satir`) | Standart CLF satırı tüm alanlarıyla ayrıştırılır; IP, metod, yol, durum kodu, bayt ve ISO zaman damgası doğrulanır. | **BAŞARILI** |
| 2 | **Eksik Alan** (`test_2_eksik_alan`) | Durum kodu ve bayt boyutu eksik satır reddedilir; emitter boş kalır, hata sayısı 1 artar. | **BAŞARILI** |
| 3 | **Hatalı Zaman Damgası** (`test_3_hatali_zaman_damgasi`) | Geçersiz ay adı (`InvalidMon`), saat aralık aşımı (`99:99:99`) veya bozuk format reddedilir. | **BAŞARILI** |
| 4 | **Hatalı Durum Kodu** (`test_4_hatali_durum_kodu`) | Sayısal olmayan (`BAD_STATUS`) veya HTTP aralığı dışındaki (`999`) kodlar reddedilir. | **BAŞARILI** |
| 5 | **Boş Satır** (`test_5_bos_satir`) | Boş dize (`""`) veya yalnızca boşluk/sekme (`" \t \r\n"`) içeren satırlar atlanır ve sayılır. | **BAŞARILI** |
| 6 | **Fazladan Boşluk** (`test_6_fazladan_bosluk`) | Alanlar arasında birden fazla boşluk veya satır başı/sonu boşlukları tolere edilir; başarıyla ayrıştırılır. | **BAŞARILI** |
| 7 | **Boşluk İçeren Tırnaklı User Agent** (`test_7_bosluk_iceren_tirnakli_user_agent`) | Parantez, boşluk ve özel karakter içeren karmaşık User-Agent eksiksiz olarak ayrıştırılır. | **BAŞARILI** |
| 8 | **Sorgu Dizesi İçeren Satır** (`test_8_sorgu_dizesi_iceren_satir`) | URL yolundaki sorgu parametreleri (`?query=...&page=2`) path alanı içinde korunur. | **BAŞARILI** |
| + | **Tire Boyut Alanı** (`test_9_tire_boyut_alani`) | Boyut alanında içerik olmamasını simgeleyen `-` işareti 0 bayt olarak işlenir. | **BAŞARILI** |
| + | **Değişmezlik ve Builder** (`test_10_logrecord_immutability_and_builder`) | `LogRecord::Builder` deseni ve alanların değişmezliği test edilir. | **BAŞARILI** |
| + | **OCP attributes Genişlemesi** (`test_11_attributes_ocp_extension`) | `withAttribute` ile yeni özniteliklerin orijinal nesneyi bozmadan eklenmesi test edilir. | **BAŞARILI** |
| + | **Combined Log Format** (`test_12_combined_log_format_referer_ve_user_agent`) | Hem Referer hem User-Agent içeren satırların ayrıştırılması doğrulanır. | **BAŞARILI** |
| + | **Protokolsüz İstek ve Sıfırlama** (`test_13_protokolsüz_istek_ve_hata_sifirlama`) | `resetErrorCount`, `open()`, `close()` yaşam döngüsü doğrulanır. | **BAŞARILI** |
| + | **Hatalı Bayt Değeri** (`test_14_hatali_bayt_degeri`) | Sayısal olmayan ve negatif bayt değerleri reddedilir. | **BAŞARILI** |
| + | **CollectingEmitter Fonksiyonları** (`test_15_collecting_emitter_metotlari`) | `empty`, `clear`, `size` metotları doğrulanır. | **BAŞARILI** |

---

## 6. Satır Kapsama (Line Coverage) Raporu

`gcov` aracı ile yürütülen kapsama analizi sonuçları aşağıdadır (`make coverage`):

```text
============================================================
 GCOV SATIR KAPSAMA (LINE COVERAGE) RAPORU
============================================================
File 'LogRecord.hpp'
Lines executed:100.00% of 66

File 'CollectingEmitter.hpp'
Lines executed:93.75% of 16

File 'ParserStage.hpp'
Lines executed:95.00% of 120

File 'tests/test_runner.cpp'
Lines executed:100.00% of 205

Genel Toplam Satır Kapsama Oranı: %97.35 (404 / 415 satır)
============================================================
```

> **Özet:** Çekirdek alan modeli `LogRecord.hpp` **%100**, test dublörü `CollectingEmitter.hpp` **%93.75**, ayrıştırıcı `ParserStage.hpp` **%95.00** satır kapsama oranına ulaşmıştır.

---

## 7. Demo Soruları ve Mimari Cevapları

### Demo Soru 1:
> *"Ayrıştırıcıyı (ParserStage) eklemek için mevcut dosyalardan hangilerini değiştirdiniz?"*

**Cevap:**
1. **Değişmeyen Dosyalar:** `Source.hpp`, `FileLineSource.hpp`, `Emitter.hpp`, `Stage.hpp` ve `Sink.hpp` dosyalarına **hiçbir şekilde dokunulmamıştır**. `FileLineSource` boru hattının ilk aşaması olarak dosyadan satır okuma ve `std::string` yayma (produce) sorumluluğunu aynen sürdürmüştür.
2. **Yeni Eklenen Dosyalar:** 
   - `LogRecord.hpp`: Boru hattının taşıdığı yeni alan modeli (Domain Record).
   - `ParserStage.hpp`: `Stage<std::string, LogRecord>` arayüzünü uygulayan ayrıştırıcı filtre.
   - `CollectingEmitter.hpp`: Bellek içi test dublörü (Test Double).
3. **Değiştirilen Mevcut Dosyalar:**
   - `ConsoleSink.hpp`: Ham dizgi yerine tipli alan modelini tüketecek biçimde `Sink<LogRecord>` olarak güncellendi.
   - `Pipeline.hpp`: Önceki artımda sadece `Source -> Sink` bağlantısı varken, ara filtreleri (`Stage<In, Out>`) bağlayabilecek ve yaşam döngüsünü yönetecek şekilde güncellendi.
   - `main.cpp`: Yeni mimari zinciri (`FileLineSource -> ParserStage -> ConsoleSink`) başlatacak ve hatalı satır sayısını ekrana basacak şekilde güncellendi.
4. **Mimari İlke:** Bu durum, boru hattının bileşenlerinin gevşek bağlı (loosely coupled) olduğunu ve **Separation of Concerns** ile **Open-Closed Principle (OCP)** ilkelerine tam uyum sağlandığını kanıtlar.

---

### Demo Soru 2:
> *"Ayrıştırıcı testiniz bir dosya okumuyor — bu nasıl mümkün oluyor ve neden önemli?"*

**Cevap:**
1. **Nasıl Mümkün Oluyor?**
   - `ParserStage`, `process(const std::string& input, Emitter<LogRecord>& out)` arayüzüne sahiptir.
   - Girdisini dosya sisteminden değil, doğrudan fonksiyona geçirilen bir bellek içi dizgiden (`std::string`) alır.
   - Çıktısını da dosyaya veya konsola yazmak yerine `Emitter<LogRecord>` arayüzüne teslim eder.
   - Test senaryolarında bu arayüzü uygulayan bir **Test Dublörü (Test Double)** olan `CollectingEmitter<LogRecord>` kullanılır.
   - Böylece test sırasında hiçbir disk veya dosya sistemi I/O işlemine ihtiyaç duyulmaz.
2. **Neden Önemli?**
   - **İzolasyon ve Bağımsızlık (Isolation):** Test yalnızca `ParserStage`'in ayrıştırma algoritmasını test eder. Yanlış dosya yolu, izin hataları veya işletim sistemi dosya kilitlerinden bağımsızdır.
   - **Yüksek Hız (Speed):** Disk I/O yapılmadığı için testler milisaniyeler seviyesinde koşar.
   - **Determinizm ve Güvenilirlik:** Test verisi test kodunun içindedir; harici dosyanın silinmesi veya değiştirilmesinden etkilenmez.
   - **Tasarım Kalitesi (Dependency Inversion):** Somut bağımlılıklar (`std::ifstream`) yerine soyut arayüzler (`Emitter`) üzerinden çalışarak test edilebilirliği (testability) maksimize eder.

---

## 8. Derleme ve Çalıştırma Kılavuzu

### Linux Terminali (Make)

```bash
# Projeyi derleme
make

# Örnek veri ile çalıştırma (data/access-small.log)
make run

# Hatalı satırları içeren örnek veri ile çalıştırma
make run-malformed

# Birim testlerini koşma (15 test)
make test

# GCOV Satır Kapsama Raporu alma
make coverage

# Temizleme
make clean
```

### Doğrudan g++ ile Derleme
```bash
# Ana uygulama
g++ -std=c++17 -Wall -Wextra main.cpp -o logflow
./logflow data/access-small.log

# Test paketi
g++ -std=c++17 -Wall -Wextra tests/test_runner.cpp -o test_runner
./test_runner
```

### Windows (Dev-C++ / Code::Blocks)
1. `Project1.dev` proje dosyasını Dev-C++ IDE'si ile açın.
2. Proje ağacında tüm başlık dosyaları (`LogRecord.hpp`, `ParserStage.hpp`, vb.) ve `main.cpp` hazır olarak görüntülenecektir.
3. `F11` (Compile & Run) tuşuna basarak projeyi doğrudan derleyip çalıştırabilirsiniz.
