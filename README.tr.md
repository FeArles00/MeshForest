[English](README.md) · **Türkçe**

# MeshForest — orman yangını erken uyarı mesh'i (uygulama deposu)

Erken yangın göstergelerini izleyen ve kimliği doğrulanmış uyarıları birkaç atlama
üzerinden ekran ve sirenli bir ağ geçidine aktaran, ucuz, güneş paneli/pil ile
beslenen kablosuz algılayıcı düğümler.

**Durum: tezgâh prototipi, test edilmiş veya sertifikalı bir yangın algılama
sistemi DEĞİLDİR.** İki kart bir masada havadan kimliği doğrulanmış paket
alışverişi yaptı (2026-11-06, 18/18 temiz). Menzil, sahadaki paket hata oranı,
çok atlamalı röle davranışı, algılayıcı kalibrasyonu ve güç özerkliği **henüz test
edilmedi**. Bir canın buna bağlı olduğu bir yerde konuşlandırmayın. Her koşuyu ve
sınırlarını kaydeden iç test günlüğü özel olarak tutulur ve bu depo ile
yayımlanmaz.

<a id="what-it-is"></a>

## Nedir bu

Bu depo projenin **uygulama tarafıdır**: ESP8266 firmware sketch'leri, kart
yapılandırması, donanım notları ve proje düzeyinde belgeler. Özel protokol
uygulaması ve referans modeli ayrı bir
[yanki-protocol](https://github.com/erdem-unal0/yanki-protocol) deposunda yaşar:
**YANKI** (*Yerel Ağlarda Noktalararası Katmanlı İletişim*, "Layered Inter-Node
Communication in Local Networks"). Bu depoda
[firmware/lib/yanki-protocol](firmware/lib/yanki-protocol) adresinde bir git alt
modülü olarak tüketilir.

| Bileşen | Açıklama |
|---|---|
| Saha düğümü | NodeMCU (ESP8266) + CC1101 433 MHz radyo + DHT22 + alev algılayıcı (+ MQ-2, henüz bağlanmadı). Kimliği doğrulanmış algılayıcı raporları ve alev uyarıları gönderir, diğer düğümlerin trafiğini röleler. |
| Ağ geçidi (ekran ve sirenli) | Aynı platform + SSD1306 OLED + siren. Uyarıları tekilleştirir, uçtan uca ACK'ler, her benzersiz uyarı için sireni bir kez çalıştırır. |
| Protokol kütüphanesi | **YANKI** — özel, kompakt ikili mesh protokolü (60 baytlık paketler): yönetilen flooding, TTL + yinelenen bastırma, AES-CMAC kimlik doğrulama, yeniden oynatma pencereleri, sınırlı yeniden denemeli uçtan uca ağ geçidi ACK'leri. Şartname: [firmware/lib/yanki-protocol/docs/protocol-spec.md](firmware/lib/yanki-protocol/docs/protocol-spec.md). |
| Kart kütüphanesi | [firmware/lib/MeshForestBoard](firmware/lib/MeshForestBoard) — pinler, radyo parametreleri ve demo zamanlamasını içeren uygulamaya yerel Arduino kütüphanesi. |
| Protokol testleri | Python referans modeli ve ayrık olaylı flood simülatörü burada değil, protokol deposunda yaşar. C++ firmware açılışta altın vektörlere karşı çapraz doğrulama yapar. |

<a id="repository-layout"></a>

## Depo düzeni

```
docs/                       project requirements and hardware notes
  forest_fire_mesh_node_project_spec.md   project requirements (source of truth)
  hardware.md               verified pin map + wiring status
  esp_pinout.md             board silkscreen reference
firmware/
  lib/MeshForestBoard/      app-local board library (pins, radio, timing)
  lib/yanki-protocol/       protocol git submodule (spec, C++ codec, Python model)
  demo_node/ demo_gateway/  the working bench demo (Arduino sketches)
  bringup/                  staged bring-up sketches (blink → SPI probe → ping)
```

<a id="hardware-bench-verified-parts"></a>

## Donanım (tezgâhta doğrulanmış parçalar)

- NodeMCU V3 ESP8266 ESP-12E (**standart dışı klon**: D etiketleri ham GPIO
  numaralarıdır; bkz. [docs/hardware.md](docs/hardware.md) §4)
- CC1101 433 MHz modülü (RadioLib 7.8.1), D14/D13/D12/D15 üzerinde, GDO0 D4 üzerinde
- DHT22 GPIO0 üzerinde, alev algılayıcı GPIO16 üzerinde, MQ-2 GPIO5 üzerinde
  bölücü ile planlanıyor
- Tam doğrulanmış pin tablosu ve bootstrap tuzakları (GPIO0/GPIO2/GPIO15):
  [docs/hardware.md](docs/hardware.md)

<a id="build--flash"></a>

## Derleme ve yükleme

Araç zinciri: arduino-cli 1.5.1, ESP8266 core 3.1.2, FQBN `esp8266:esp8266:nodemcuv2`.

Önce protokol alt modülünü başlatın:

```sh
git submodule update --init --recursive
```

Ardından sketch'leri, kart kütüphanesi ve protokol kütüphanesi de include yolunda
olacak şekilde derleyin:

```sh
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --libraries firmware/lib firmware/demo_node
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --libraries firmware/lib firmware/demo_gateway
arduino-cli upload -p <PORT> --fqbn esp8266:esp8266:nodemcuv2 --libraries firmware/lib firmware/demo_node
```

Sketch'ler protokol kütüphanesinden `<YankiProtocol.h>` ve uygulamaya yerel kart
kütüphanesinden `<MeshForestBoard.h>` dosyalarını içerir. Ayrıntılar için bkz.
[firmware/lib/README.md](firmware/lib/README.md).

<a id="security-model-honest-version"></a>

## Güvenlik modeli (dürüst hali)

- Her paket, tek bir ön paylaşımlı 128 bitlik ağ anahtarı altında AES-128-CMAC
  (RFC 4493) ile kimliği doğrulanır; yeniden oynatma koruması kaynak başına bir
  penceredir.
- **Şifreleme yok**: yükler, alıcısı olan herkes tarafından okunabilir.
- Ele geçirilen bir düğüm ağ anahtarını açığa çıkarır ve ACK'ler dahil her şeyi
  taklit edebilir. ESP8266'da güvenli anahtar saklama yoktur. Anahtar döndürme =
  her şeyi yeniden yüklemek.
- Protokol kütüphanesindeki `YankiConfig.h` içinde yer alan demo anahtarı
  herkese açık bir yer tutucudur. Gerçek her konuşlandırma için, asla commit
  edilmeyen yeni ve rastgele bir anahtar provizyon edin.

Tam tehdit modeli ve sınırlamalar: `yanki-protocol` deposundaki protokol
şartnamesinin güvenlik ve tehdit modeli bölümleri.

<a id="regulatory-note"></a>

## Mevzuat notu

Stok antenle 0 dBm'de, 433 MHz SRD bandı için yapılandırılmıştır (Türkiye, genel
amaçlı kurallar: 10 mW e.r.p., ≤%10 duty cycle). Anten kazancı dahil e.r.p. ve
dolu bant genişliği maskesi hâlâ doğrulanmadı: açık havada iletim yapmadan önce
yerel kurallarınızı kontrol edin.

<a id="license"></a>

## Lisans

MIT: bkz. [LICENSE](LICENSE).
