<p align="center">
  <img src="assets/logo.png" alt="Sabit Logo" width="104" />
</p>

<h1 align="center">Sabit</h1>

<p align="center">
  <strong>Presisi Terotomasi untuk Observasi Hilal — Automated Precision for Lunar Sighting</strong>
</p>

<p align="center">
  <a href="https://github.com/MauludyZhrn/Sabit/releases/latest">
    <img src="https://img.shields.io/badge/Release-v1.1.1-8B5CF6?style=for-the-badge" alt="Release v1.1.1" />
  </a>
  <img src="https://img.shields.io/badge/Platform-Windows%2010%2F11%20x64-0EA5E9?style=for-the-badge" alt="Platform Windows 10/11 x64" />
  <img src="https://img.shields.io/badge/Stack-C%2B%2B20%20%7C%20Qt%206.8-22C55E?style=for-the-badge" alt="Stack C++20 / Qt 6.8" />
  <img src="https://img.shields.io/badge/AI-YOLOv8s%20%2B%20YOLOv8n%20ONNX-F59E0B?style=for-the-badge" alt="AI YOLOv8s + YOLOv8n ONNX" />
  <a href="https://sabit-landing-page.vercel.app/">
    <img src="https://img.shields.io/badge/Website-sabit--landing--page.vercel.app-6366F1?style=for-the-badge" alt="Official Website" />
  </a>
</p>

<p align="center">
  <a href="https://sabit-landing-page.vercel.app/"><strong>Website Resmi: Sabit — Presisi Terotomasi untuk Observasi Hilal</strong></a>
  &nbsp;&middot;&nbsp;
  <a href="https://github.com/MauludyZhrn/Sabit/releases/latest"><strong>Download v1.1.1 (Windows x64)</strong></a>
  &nbsp;&middot;&nbsp;
  <a href="docs/HILAL_MODEL_INTEGRATION.md"><strong>Panduan AI Dual-Model</strong></a>
  &nbsp;&middot;&nbsp;
  <a href="docs/User_Manual.pdf"><strong>User Manual (PDF)</strong></a>
</p>

---

> [!TIP]
> **Kunjungi Website Resmi Sabit!**
> Lihat pratinjau antarmuka, sorotan fitur observasi hilal, dokumentasi ringkas, dan unduhan installer terbaru secara praktis melalui halaman **[Sabit — Presisi Terotomasi untuk Observasi Hilal](https://sabit-landing-page.vercel.app/)**.

---

## Tentang Aplikasi (Overview & Scientific Purpose)

**Sabit** (*Automated Precision for Lunar Sighting*) adalah workstation desktop Windows native (`C++20 / Qt 6.8`) yang dirancang khusus untuk observasi hilal astronomis (*rukyatul hilal*), pemrosesan citra langit real-time, kalkulasi efemeris toposentrik, dan deteksi sabit muda berbantuan visi komputer (*ONNX Runtime*).

Dikembangkan sebagai bagian dari riset Tesis Magister Ilmu Falak, Sabit menyatukan akuisisi kamera astronomi, pemrosesan citra 16-bit tanpa latensi, perhitungan posisi Bulan/Matahari, serta deteksi AI ke dalam satu aplikasi terpadu yang siap bekerja **100% offline** di pos observasi lapangan.

Mengamati hilal muda memiliki tantangan fisika dan komputasi yang ketat:
1. **Rasio Kontras Sangat Rendah**: Luminansi sabit tipis sering kali hanya berada sepersekian persen di atas latar langit senja (*civil twilight*) atau siang hari.
2. **Jendela Pengamatan Singkat**: Jeda antara matahari terbenam (*maghrib*) dan terbenamnya bulan (*lag time*) umumnya hanya berlangsung sekitar 15–40 menit dalam kondisi atmosfer yang cepat berubah.
3. **Verifikasi Kriteria Astronomi & Fikih**: Evaluasi visibilitas menuntut data koordinat toposentrik presisi terhadap kriteria standar seperti **MABIMS Baru** (`tinggi >= 3.0 deg`, `elongasi >= 6.4 deg`), **Limit Danjon**, dan **Kriteria Odeh**, disertai dokumentasi bukti citra FITS yang utuh.

> **Catatan Status Riset**: Algoritma deteksi AI dan model efemeris di dalam perangkat lunak ini merupakan instrumen bantu riset akademik untuk membantu observer menemukan kandidat sabit serta mencatat dokumentasi teknis. Hasil deteksi AI tidak dimaksudkan sebagai penentu tunggal keabsahan rukyat secara astronomis maupun syar'i.

---

## Cara Download & Instalasi

### Opsi 1 — Installer Windows `.exe` (Direkomendasikan untuk Pengguna Umum)

1. Kunjungi website resmi **[Sabit — Presisi Terotomasi untuk Observasi Hilal](https://sabit-landing-page.vercel.app/)** atau buka halaman **[GitHub Releases](https://github.com/MauludyZhrn/Sabit/releases/latest)**.
2. Unduh file **`Sabit-Setup-v1.1.1-x64.exe`** (Windows 10/11 64-bit).
3. *(Opsional)* Verifikasi keaslian installer terhadap berkas `SHA256SUMS.txt` dari halaman release menggunakan PowerShell:
   ```powershell
   Get-FileHash .\Sabit-Setup-v1.1.1-x64.exe -Algorithm SHA256
   ```
4. Jalankan installer `Sabit-Setup-v1.1.1-x64.exe`. Instalasi bersifat *per-user* ke `%LOCALAPPDATA%\Programs\Sabit` sehingga **tidak memerlukan hak Administrator**. Paket installer sudah membundel runtime Qt6, ONNX Runtime, model AI YOLOv8s & YOLOv8n, serta panduan pengguna.
5. Jalankan **Sabit** dari Start Menu atau ikon Desktop. Untuk pembaruan versi mendatang, gunakan menu **`Help -> Check for Updates`** langsung dari dalam aplikasi.

### Opsi 2 — Build dari Source Code (Developer)

#### 1. Clone Repositori
```bash
git clone https://github.com/MauludyZhrn/Sabit.git
cd Sabit
```

#### 2. Siapkan Toolchain (`MinGW-w64 GCC 13+` & `Qt 6.8.0`)
```powershell
$env:PATH = "D:\mingw64\bin;D:\Qt\6.8.0\mingw_64\bin;" + $env:PATH
```

#### 3. Konfigurasi & Kompilasi CMake
```powershell
cmake -B build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel 2
```
Atau menggunakan MSVC (Visual Studio 2022):
```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

#### 4. Jalankan Suite Pengujian (`CTest`)
```powershell
ctest --test-dir build-release -C Release --output-on-failure
```

---

## Fitur Utama

| Pilar Sistem | Kemampuan Teknis |
| :--- | :--- |
| **Multi-Driver Kamera Astronomi** | Dukungan native **ZWO ASI**, **QHYCCD**, jembatan **ASCOM Platform**, **DirectShow / UVC** (webcam, DSLR HDMI capture card, Canon EOS Webcam Utility dengan auto-deteksi mode Mono/RGB), serta **Synthetic Simulator** berbasis efemeris. |
| **Pemrosesan Citra & Live Stacking** | Pipeline multi-thread bebas freeze: auto-stretch **MTF**, **CLAHE**, *unsharp mask*, pengurangan *banding*, koreksi **Master Dark / Master Flat**, serta **Live Stacking** (Mean & Sigma-Clipped Average) dengan perataan pergeseran sub-piksel FFT Phase Correlation. |
| **Efemeris & Kriteria Visibilitas** | Perhitungan toposentrik real-time (elevasi, azimut, elongasi, lebar sabit *W*, iluminasi, waktu maghrib & moonset), kalender ganda Gregorian & Hijriah, serta status otomatis **MABIMS Baru**, **Odeh**, dan **Danjon**. |
| **Deteksi AI Dual-Model (HILAL-DETECT V2)** | Inferensi real-time **ONNX Runtime** (`v1.17.1` C API) dengan pergantian model **YOLOv8s** / **YOLOv8n** tanpa restart melalui `File -> Settings -> Hardware & AI`, log setiap bounding box ke `detections.csv`, dan dukungan GPU CUDA terverifikasi saat runtime tersedia. |
| **Perekaman Ilmiah, Pelaporan & Update** | Ekspor lossless **FITS 16-bit** dengan header WCS/efemeris, video **AVI/MP4**, foto **PNG/TIFF**, struktur sesi terorganisir (`logs/` & `captures/`), ekspor **Laporan PDF Sidang Isbat**, **Live Log terminal berwarna**, pelaporan masalah **tanpa login**, serta **Check for Updates**. |

---

## Deteksi AI Dual-Model (`HILAL-DETECT V2`)

Mulai **Sabit v1.1.0+** (termasuk layout **v1.1.1** terbaru), pengguna dapat berpindah melalui **`File -> Settings -> Hardware & AI`** antara model akurasi tinggi dan model ringan sesuai spesifikasi perangkat keras:

| Properti | YOLOv8s (`Primary - Recommended`) | YOLOv8n (`Lightweight Alternative`) | Legacy `hilal_v1` (Dipertahankan) |
| :--- | :--- | :--- | :--- |
| **Profile ID** | `yolov8s` (Default) | `yolov8n` | `hilal_v1` |
| **Lokasi Berkas** | `models/hilal/yolov8s/hilal_yolov8s_640.onnx` | `models/hilal/yolov8n/best.onnx` | `models/hilal_v1.onnx` |
| **Kontrak Input / Output** | `images` `[1, 3, 640, 640]` / `output0` `[1, 5, 8400]` | `images` `[1, 3, 640, 640]` / `output0` `[1, 5, 8400]` | `images` `[1, 3, 640, 640]` / `output0` `[1, 5, 8400]` |
| **Confidence Default** | `0.30` | `0.75` | `0.40` |
| **NMS IoU Host C++** | `0.45` | `0.45` | `0.45` |
| **Latensi CPU (`sample_input.jpg`)** | `138.664 ms` | `54.0181 ms` | `47.8785 ms` (sim 1280x720) |
| **Paritas Skor vs Referensi Training** | `0.585283` (`diff 1.19e-7`) | `0.8134` (`diff 5.96e-8`) | - |

Detail lengkap kontrak tensor letterbox (`RGB`, `NCHW`, `/ 255.0`, `pad=114`), pemulihan koordinat, dan hasil validasi 10 butir tersedia di [docs/HILAL_MODEL_INTEGRATION.md](docs/HILAL_MODEL_INTEGRATION.md).

---

## Arsitektur Sistem

Sabit menggunakan arsitektur asinkron multi-thread untuk menjamin preview tetap responsif saat perekaman maupun inferensi AI berjalan:

```
+-----------------------------------------------------------------------+
|                              Sabit UI                                 |
|  (Qt 6 GUI Thread: Main Window, Panels, Video Viewport, Live Logs)    |
+-----------------------------------------------------------------------+
             ^                                      ^
             | (Signals / Slots)                    | (LatestSlot)
+-----------------------------------------------------------------------+
|                           AppController                               |
|        (Central Coordinator, Session Manager, Pipeline Dispatcher)    |
+-----------------------------------------------------------------------+
        |                     |                       |
        v                     v                       v
+------------------+  +--------------------+  +-------------------------+
| Camera Worker    |  | Processing Worker  |  | AI Inference Worker     |
| (ICamera Drivers)|  | (Live Stacker,     |  | (ONNX Runtime v1.17.1,  |
| - UVC / DShow    |  |  CLAHE, Stretch,   |  |  YOLOv8s / YOLOv8n,     |
| - ZWO ASI        |  |  Dark/Flat Calib.) |  |  Host NMS & Restore)    |
| - QHYCCD         |  +--------------------+  +-------------------------+
| - ASCOM / Sim    |          |
+------------------+          v
        |             +--------------------+
        v             | Recording Worker   |
+------------------+  | (FITS 16-bit, AVI, |
| RingBuffer       |  |  MP4, PNG, TIFF)   |
| (Zero-Allocation)|  +--------------------+
+------------------+
```

---

## Persyaratan Sistem

- **Sistem Operasi**: Microsoft Windows 10 (versi 21H2 atau lebih baru) / Windows 11 64-bit.
- **Prosesor**: Intel Core i5 (Gen-8+) / AMD Ryzen 5 atau setara sangat disarankan untuk *live stacking* & inferensi AI.
- **Memori RAM**: Minimum 8 GB (16 GB disarankan untuk stacking resolusi tinggi).
- **Penyimpanan**: SSD cepat disarankan untuk perekaman kontinu FITS 16-bit dan video.
- **Antarmuka Hardware**: Port USB 3.0 / USB 3.1 untuk kamera astronomi (ZWO/QHY driver vendor perlu diinstal jika menggunakan hardware kamera khusus tersebut).

---

## Struktur Direktori

```
Sabit/
├── assets/                         # Ikon aplikasi (logo.png/.ico) dan sumber SVG antarmuka
├── cmake/                          # Helper CMake dan konfigurasi toolchain
├── docs/                           # Dokumentasi teknis dan panduan pengguna
│   ├── HILAL_MODEL_INTEGRATION.md  # Dokumentasi integrasi dual-model HILAL-DETECT V2
│   ├── User_Manual.pdf             # Panduan lengkap penggunaan aplikasi
│   └── testing/                    # Catatan rilis uji coba & panduan evaluasi riset
├── include_fix/                    # Header kompatibilitas lintas-kompiler
├── installer/                      # Skrip pembangun installer Inno Setup 7 (Sabit.iss)
├── models/                         # Model ONNX AI & verifikasi integritas SHA-256
│   ├── manifest.json               # Manifes skema v2 + hash integritas seluruh model
│   ├── hilal/                      # Paket dual-model HILAL-DETECT V2
│   │   ├── model_profiles.json     # Profil & threshold YOLOv8s dan YOLOv8n
│   │   ├── yolov8s/                # Model utama YOLOv8s, metadata, sample, & kontrak
│   │   └── yolov8n/                # Model ringan YOLOv8n, metadata, & sample
│   └── hilal_v1.onnx               # Model legacy v1 (tetap dipertahankan)
├── src/                            # Kode sumber C++20 / Qt 6.8
│   ├── app/                        # AppController, manajemen sesi, logika pipeline
│   ├── astro/                      # Efemeris bulan/matahari & kriteria visibilitas
│   ├── camera/                     # Abstraksi driver kamera (ASCOM, QHY, UVC, ZWO, Sim)
│   ├── core/                       # RingBuffer, LatestSlot, Logger, Result/Error
│   ├── detect/                     # Mesin inferensi OnnxDetector & logger deteksi CSV
│   ├── imaging/                    # Stacker, ImageStretcher, kalibrasi, penulis FITS/AVI/MP4
│   ├── ui/                         # Antarmuka Qt6 (MainWindow, panel, dialog update/issue)
│   └── main.cpp                    # Entry point aplikasi & proteksi DLL search
├── tests/                          # Unit test & integration test suite (4 target CTest)
├── tools/                          # Skrip validasi (termasuk check_no_emoji.py)
├── vendor/                         # SDK & pustaka pihak ketiga (ONNX Runtime, Qlementine, dll.)
├── CMakeLists.txt                  # Konfigurasi utama CMake
├── settings.example.json           # Templat konfigurasi runtime pengguna
└── version.json                    # Manifes versi & catatan rilis untuk Check for Updates
```

---

## Konfigurasi (`settings.json`)

Sabit membaca dan menyimpan preferensi pada berkas `settings.json` di direktori aplikasi (templat tersedia di [settings.example.json](settings.example.json)):

```json
{
  "schema": 2,
  "general": {
    "language": "en",
    "theme": "dark",
    "prevent_sleep": true
  },
  "site": {
    "preset": "Jakarta",
    "latitude": -6.2088,
    "longitude": 106.8456,
    "elevation_m": 10.0,
    "timezone": "Asia/Jakarta",
    "refraction": false
  },
  "astro": {
    "criteria": "MABIMS-baru",
    "calendar": "rukyat_hilal_indonesia"
  },
  "capture": {
    "output_dir": "Captures",
    "format": "fits",
    "filename_pattern": "{DATE}_{TIME}_{TARGET}_{SEQ}"
  },
  "ai": {
    "active_model": "yolov8s",
    "model": "models/hilal/yolov8s/hilal_yolov8s_640.onnx",
    "confidence": 0.30,
    "nms_iou": 0.45,
    "provider": "CPUExecutionProvider"
  }
}
```

---

## Dokumentasi Terkait

- **[Website Resmi Sabit — Presisi Terotomasi untuk Observasi Hilal](https://sabit-landing-page.vercel.app/)** — Halaman showcase interaktif dan pusat unduhan aplikasi.
- **[HILAL-DETECT V2 Dual-Model Integration Guide](docs/HILAL_MODEL_INTEGRATION.md)** — Spesifikasi lengkap YOLOv8s & YOLOv8n, kontrak preprocessing/postprocessing ONNX, dan hasil validasi.
- **[Panduan Pengguna / User Manual (PDF)](docs/User_Manual.pdf)** — Panduan pengoperasian workstation Sabit di lapangan.

---

## Status Lisensi

**License Status: To be determined.**

Seluruh hak cipta dilindungi. Repositori ini merupakan bagian dari riset akademik dan pengembangan perangkat lunak untuk Tesis Magister Ilmu Falak. Header SDK perangkat keras milik pihak ketiga (ZWO ASI, QHYCCD) tetap tunduk pada lisensi dan hak cipta pemegang merek masing-masing.
