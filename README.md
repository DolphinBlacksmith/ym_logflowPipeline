# LogFlow - Yazılım Mimarisi Dönem Projesi
## Artım 4: Yapılandırma Odaklı Birleştirme ve Aşama Kaydı ★★★

- **Öğrenci Adı Soyadı:** YunusEmre Demirci  
- **Öğrenci Numarası:** 2404040249  
- **Ders:** Yazılım Tasarımı ve Mimarisi  
- **Sürüm Etiketi:** `v4`  
- **GitHub Deposu:** [DolphinBlacksmith/ym_logflowPipeline](https://github.com/DolphinBlacksmith/ym_logflowPipeline)

---

## 1. Mimari Odak ve Hedefler

LogFlow projesinin 4. Artımında, boru hattı mimarisinin tanımı (topoloji) ile gerçekleştirimi (C++ sınıfları) birbirinden tamamen ayrılmıştır:

1. **Bildirimsel Yapılandırma (Declarative Configuration):** İşlem hattı topolojisi (Source, Stages, Sink ve parametreleri) YAML yapılandırma dosyalarına taşınmıştır.
2. **Veri Odaklı Eklenti / Kayıt Defteri (`StageRegistry` & `StageFactory`):** Sabit kodlanmış `switch` veya `if-else` blokları kullanılmadan, aşama türü adlarını ilgili fabrika fonksiyonlarına eşleyen veri odaklı bir kayıt sistemi geliştirilmiştir.
3. **Yeniden Derlemeden Yeni Boru Hattı (Zero Recompilation Milestone):** Değiştirilmemiş aynı `./logflow` çalıştırılabilir dosyası, üç farklı YAML yapılandırma dosyası kullanılarak üç farklı çıktı üretir.
4. **Yapılandırma Zamanı Tür Denetimi (Fail Fast, Fail Loudly):** Eksik parametre, yanlış türde parametre, bilinmeyen aşama türü veya boş işlem hattı durumlarında herhangi bir veri işlenmeden önce açık ve uygulanabilir hata mesajları üretilir.

---

## 2. Artım 4 Bileşenleri

| Bileşen / Dosya | Sorumluluk ve Mimari Rol |
|---|---|
| [`StageFactory.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/StageFactory.hpp) | Tür güvenli `ConfigValue`, parametre doğrulayıcı `StageParams`, `ConfigValidationException` ve bileşen fabrika metotlarını (`createFileSource`, `createParseStage`, `createFilterStage`, `createProjectStage`, `createFileSink`, `createConsoleSink`) içerir. |
| [`StageRegistry.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/StageRegistry.hpp) | Aşama, kaynak ve hedef türlerini `std::map` tabanlı tek bir kayıt tablosunda tutar (`switch` içermez). Çalışma zamanında dinamik eklenti kaydını (`registerStage`) ve `REGISTER_LOGFLOW_STAGE` makrosunu destekler. |
| [`PipelineLoader.hpp`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/PipelineLoader.hpp) | YAML yapılandırma dosyasını okur (`YamlConfigParser`), topolojiyi ve parametreleri doğrular, `StageRegistry` üzerinden aşamaları bağlayarak çalıştırılabilir bir `Pipeline` nesnesi döndürür. |
| [`configs/error-report.yaml`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/configs/error-report.yaml) | **1. Yapılandırma (Hata Raporu):** `status >= 400` olan hata kayıtlarını filtreler ve `[timestamp, clientIp, status, path]` alanlarını `out/errors.txt` dosyasına yazar. |
| [`configs/api-traffic.yaml`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/configs/api-traffic.yaml) | **2. Yapılandırma (API Trafiği Veri Çıkarımı):** `path startsWith "/api"` olan istekleri filtreler ve `[timestamp, method, path, status, bytes]` alanlarını `out/api-traffic.txt` dosyasına yazar. |
| [`configs/bandwidth-by-client.yaml`](file:///home/yunus/Desktop/yazılım_mimarisi/ym_logflowPipeline/configs/bandwidth-by-client.yaml) | **3. Yapılandırma (İstemci Bazında Bant Genişliği):** `bytes >= 1024` olan yüksek boyutlu yanıtları filtreler ve `[clientIp, bytes, path, status]` alanlarını `out/bandwidth-by-client.txt` dosyasına yazar. |

---

## 3. Çalıştırma Kılavuzu (`--config`)

Aynı çalıştırılabilir `./logflow` ikili dosyası, yeniden derlenmeden üç farklı yapılandırma dosyasıyla üç farklı çıktı üretir:

```bash
# Projeyi derle
make

# 1. Hata Raporu (out/errors.txt üretir)
./logflow --config configs/error-report.yaml
cat out/errors.txt

# 2. API Trafiği Veri Çıkarımı (out/api-traffic.txt üretir)
./logflow --config configs/api-traffic.yaml
cat out/api-traffic.txt

# 3. İstemci Bazında Bant Genişliği Çıkarımı (out/bandwidth-by-client.txt üretir)
./logflow --config configs/bandwidth-by-client.yaml
cat out/bandwidth-by-client.txt

# Üç yapılandırmayı tek komutla peş peşe çalıştır
make run-configs

# İnteraktif Yapılandırma & Hata Denetimi Menüsü
./logflow
```

### Üretilen Üç Farklı Çıktı

- **`out/errors.txt` (`configs/error-report.yaml`):**
  ```text
  2026-09-29T07:01:05Z 192.168.1.15 401 /api/login
  2026-09-29T07:03:15Z 192.168.1.25 404 /api/orders
  2026-09-29T07:05:25Z 192.168.1.35 500 /api/pay
  ```

- **`out/api-traffic.txt` (`configs/api-traffic.yaml`):**
  ```text
  2026-09-29T07:01:05Z POST /api/login 401 512
  2026-09-29T07:02:10Z GET /api/users 200 2048
  2026-09-29T07:03:15Z GET /api/orders 404 256
  2026-09-29T07:05:25Z POST /api/pay 500 128
  2026-09-29T07:06:30Z DELETE /api/session 204 0
  ```

- **`out/bandwidth-by-client.txt` (`configs/bandwidth-by-client.yaml`):**
  ```text
  192.168.1.10 1024 /index.html 200
  192.168.1.20 2048 /api/users 200
  192.168.1.30 4096 /dashboard 200
  192.168.1.45 8192 /static/bundle.js 200
  ```

---

## 4. Yapılandırma Zamanı Hata Mesajı Örnekleri (Fail Fast, Fail Loudly)

Sistem, herhangi bir veri satırı işlenmeden önce yapılandırma hatalarını yakalar ve açık, uygulanabilir (actionable) hata mesajları üretir:

### Örnek 1: Bilinmeyen Bir Aşama Türü (`Unknown Stage Type`)
- **Hatalı Yapılandırma:**
  ```yaml
  stages:
    - type: parse
      params: { format: clf }
    - type: geoip_lookup
      params: { db: city.mmdb }
  ```
- **Üretilen Hata Mesajı:**
  ```text
  [Yapilandirma Hatasi] Bilinmeyen asama turu (Unknown stage type): 'geoip_lookup'. Kayitli gecerli asama turleri: [filter, parse, project].
  ```

### Örnek 2: Eksik Zorunlu Parametre (`Missing Required Parameter`)
- **Hatalı Yapılandırma (`value` parametresi unutulmuş):**
  ```yaml
  stages:
    - type: filter
      params: { field: status, op: ">=" }
  ```
- **Üretilen Hata Mesajı:**
  ```text
  [Yapilandirma Hatasi] Eksik parametre (Missing parameter): 'stage (type: filter)' icinde zorunlu 'value' parametresi bulunamadi. Cozum: params bloguna 'value: <deger>' ekleyin.
  ```

### Örnek 3: Yanlış Türde Parametre (`Wrong Parameter Type`)
- **Hatalı Yapılandırma (Sayısal `status` alanı için metin verilmiş):**
  ```yaml
  stages:
    - type: filter
      params: { field: status, op: ">=", value: "dort_yuz" }
  ```
- **Üretilen Hata Mesajı:**
  ```text
  [Yapilandirma Hatasi] Yanlis turde parametre (Wrong parameter type): 'stage (type: filter) [field=status]' icindeki 'value' parametresi 'integer (tamsayi)' turunde olmalidir, ancak 'string (metin)' ('dort_yuz') verildi.
  ```

### Örnek 4: Boş Bir İşlem Hattı (`Empty Pipeline`)
- **Hatalı Yapılandırma (`stages: []`):**
  ```yaml
  pipeline:
    name: empty-pipeline-demo
    source: { type: file, params: { path: data/access.log } }
    stages: []
    sink: { type: file, params: { path: out/errors.txt } }
  ```
- **Üretilen Hata Mesajı:**
  ```text
  [Yapilandirma Hatasi] Bos islem hatti (Empty pipeline): 'pipeline.stages' listesi bos olamaz; en az bir asama (or. parse) icermelidir.
  ```

### Örnek 5: Aşama Akış Türü Uyumsuzluğu (`parse` Öncesinde `filter` Kullanımı)
- **Hatalı Yapılandırma:**
  ```yaml
  stages:
    - type: filter
      params: { field: status, op: ">=", value: 400 }
    - type: parse
      params: { format: clf }
  ```
- **Üretilen Hata Mesajı:**
  ```text
  [Yapilandirma Hatasi] Asama siralama/tur uyumsuzlugu: 'filter' asamasi girdi olarak 'LogRecord' bekliyor, ancak mevcut akis turu 'string'. Cozum: 'filter' asamasindan once 'parse' asamasini ekleyin.
  ```

---

## 5. Test Paketi (42 Birim ve Entegrasyon Testi)

Test paketimiz (`tests/test_runner.cpp`) toplam **42 adet test senaryosu** içerir:

- **1–15:** Artım 2 testleri (`ParserStage`, `LogRecord`, `CollectingEmitter`)
- **16–32:** Artım 3 testleri (`PredicateFilterStage`, `ProjectionStage`, `FileSink`, `PipelineBuilder`, sıralama bağımsızlığı)
- **33–42:** Artım 4 yükleyici ve kayıt defteri testleri:
  - `test_33_loader_valid_configuration` (Geçerli bir yapılandırma)
  - `test_34_loader_unknown_stage_type` (Bilinmeyen bir aşama türü)
  - `test_35_loader_missing_parameter` (Eksik bir parametre)
  - `test_36_loader_wrong_parameter_type` (Yanlış türde bir parametre)
  - `test_37_loader_empty_pipeline` (Boş bir işlem hattı)
  - `test_38_three_config_files_produce_different_outputs` (3 ayrı yapılandırma dosyasının 3 farklı çıktı üretmesi)
  - `test_39_stage_registry_dynamic_plugin_registration` (`StageRegistry` dinamik eklenti kaydı)
  - `test_40_loader_stage_stream_type_mismatch` (Akış türü uyumsuzluğu denetimi)
  - `test_41_loader_unknown_source_sink_and_extra_params` (Bilinmeyen source/sink ve tanımsız parametre denetimi)
  - `test_42_filter_factory_all_operators_and_fields` (`StageFactory` tüm operatör ve alan testleri)

```bash
# 42 testi derle ve çalıştır
make test

# GCOV Satır Kapsama Raporu Üret
make coverage
```
