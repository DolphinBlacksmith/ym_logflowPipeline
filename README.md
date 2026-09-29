# Yazılım Mimarisi - Pipe & Filter Pipeline Mimarisi

Bu proje, Yazılım Mimarisi dersinde uygulanan **Pipe and Filter** (Boru Hattı ve Filtre) mimari deseninin C++ ile nesne yönelimli olarak gerçeklenmiş basit bir örneğidir.

## Proje Mimarisi

- **Source (`Source.hpp`, `FileLineSource.hpp`):** Veri kaynağını temsil eder. `access-small.log` dosyasından satır satır okuma yaparak boru hattına veri iletir (`produce`).
- **Emitter (`Emitter.hpp`):** Verilerin bir sonraki aşamaya iletilmesini sağlayan soyut arayüzdür (`emit`).
- **Stage (`Stage.hpp`):** Boru hattı üzerindeki ara filtre ve işlem adımlarını tanımlar (`process`).
- **Sink (`Sink.hpp`, `ConsoleSink.hpp`):** Boru hattının son hedefidir. Gelen verileri tüketir/ekrana yazdırır (`consume`).
- **Pipeline (`Pipeline.hpp`):** Kaynak (`Source`) ile hedefi (`Sink`) birbirine bağlayan ve akışı yöneten ana sınıftır (`run`).

## Derleme ve Çalıştırma

### Linux (Terminal / Make)
```bash
make
make run
```
Veya doğrudan derleyici ile:
```bash
g++ -std=c++11 -Wall -Wextra main.cpp -o pipeline
./pipeline
```

İsteğe bağlı olarak farklı bir log dosyası argüman olarak verilebilir:
```bash
./pipeline baska_bir_dosya.log
```

### Dev-C++ / Code::Blocks (Windows)
- `Project1.dev` proje dosyasını açıp `F11` (Compile & Run) ile çalıştırabilirsiniz.
