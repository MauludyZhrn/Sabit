# Test Plan & QA Checklist: Sabit

Versi dokumen: 0.1 (draf) | Tanggal: 23-10-2026
Rujukan: 01-PRD.md (FR, NFR, G), 02-Technical-Specification.md, 04-Project-Timeline-Milestones.md.
Framework otomasi: Catch2 v3 (C++), CTest, pytest (alat bantu Python di `tools/`), Qt Test dengan platform `offscreen` untuk UI.
Nama kelas/fungsi pada contoh kode adalah sketsa API dari Tech Spec; sesuaikan dengan implementasi saat M1.

---

## 1. Tujuan & Ruang Lingkup Testing

**Tujuan:** memastikan Sabit 1.0 aman dipakai pada malam rukyat: kontrol hardware benar, data tidak hilang tanpa diketahui, live view responsif, dan perhitungan astronomi benar.

**In scope**
- Seluruh FR "Must" dan "Should" fase MVP; NFR-01 sampai NFR-15.
- Driver ZWO, UVC, simulator; pipeline kalibrasi, stacking, penulisan berkas; AI ONNX; efemeris dan kriteria; pengaturan dan migrasi Onyx; UI utama; kemasan ZIP.
- Uji lapangan FT1, FT2, FT3.

**Out of scope (MVP)**
- Driver QHY, ASCOM, rekam video, penjelajah sesi (fase 2).
- Pengujian keakuratan model AI di luar uji paritas ONNX dan uji regresi pada set gambar tetap (evaluasi model adalah proses terpisah).
- Pengujian platform selain Windows 10 22H2 / Windows 11 x64.

## 2. Strategi Testing

Piramida (target jumlah relatif, ASUMSI):

| Lapisan | Porsi | Isi | Target coverage |
|---|---|---|---|
| Unit | ±65% | `core`, `imaging`, `astro`, `detect` (tanpa model), parser pengaturan, migrasi | Baris ≥ 80% untuk `core`, `imaging`, `astro` |
| Integrasi | ±20% | Simulator -> pipeline -> penulis FITS -> baca ulang; kontrak `ICamera`; ONNX pada gambar tetap; kill proses saat rekam | Semua jalur data utama |
| UI (offscreen) | ±8% | Pembuatan jendela, ganti tema/bahasa, status kamera, validasi dialog | Semua layar SCR-xx dapat dibuat tanpa crash |
| Sistem/lapangan | ±7% | Kamera nyata, laptop referensi, FT1 sampai FT3 | Semua skenario regresi bagian 7 |

Prinsip:
- Semua logika yang bisa diuji tanpa perangkat harus diuji tanpa perangkat (simulator).
- Tidak ada ambang performa yang dianggap benar sebelum diukur pada laptop referensi.
- Setiap bug yang ditemukan di lapangan menjadi test otomatis atau entri regresi.
- Sanitizer (ASan, UBSan) aktif pada build uji CI; benchmark berjalan pada build Release.

## 3. Environment & Test Data

| Environment | Perangkat | Fungsi |
|---|---|---|
| CI | Windows runner (build dan uji otomatis, simulator, offscreen) | Setiap commit |
| Laptop referensi | ASUMSI: Core i5 gen 8 / Ryzen 5, 8 GB RAM, SSD, USB 3.0, Windows 10 22H2 atau Windows 11, tanpa GPU diskrit | Benchmark performa dan uji manual |
| Laptop terlemah | Perangkat terlemah yang tersedia di tim | Uji degradasi (R-06) |
| VM Windows bersih | Tanpa Visual C++ redistributable, tanpa Python | Uji kemasan (NFR-09) |
| Perangkat keras | 1 kamera ZWO (model dicatat), 1 webcam UVC, kabel USB 3.0 + hub | Uji driver |

**Test data**
- `tests/data/frames/`: frame 16-bit sintetis (gradien penuh 0..65535, vignetting, bintang, outlier piksel, hilal sintetis).
- `tests/data/onyx_settings/`: contoh `settings.json` Onyx OS (valid, kosong, rusak, dengan key tak dikenal).
- `tests/golden/ephemeris/`: tabel referensi independen untuk 10 tanggal x 3 lokasi (posisi bulan/matahari, sunset, moonset, ijtima). Sumber referensi dicatat di berkas README di folder itu (ASUMSI: tabel dibuat dari sumber independen seperti JPL Horizons atau almanak; harus dipilih dan dicatat sebelum M2).
- `tests/data/onnx/`: 20 gambar tetap + keluaran model .pt asli sebagai acuan paritas.
- Rekaman lapangan Onyx lama (jika ada) sebagai bahan uji pemutaran ulang.

## 4. Checklist Fungsional

Centang saat lolos pada build RC. Kode di kurung merujuk FR/TC.

### 4.1 Kamera dan koneksi
- [ ] Kamera ZWO terdeteksi dan tersambung ≤ 5 detik (FR-01, TC-001)
- [ ] Webcam UVC terdeteksi dan menampilkan frame (FR-02, TC-002)
- [ ] Simulator menampilkan tiga mode yang berbeda (FR-03, TC-003)
- [ ] Setiap perubahan exposure/gain/bin/bit depth terbaca balik dari kamera (FR-04, TC-004)
- [ ] Nilai di luar rentang ditolak dengan pesan rentang yang benar (FR-04, TC-005)
- [ ] Kontrol yang tidak didukung kamera redup dengan penjelasan (FR-04, TC-006)
- [ ] Kamera dicabut: status Disconnected ≤ 3 detik, aplikasi tidak hang (FR-05, TC-007)
- [ ] Sambung ulang otomatis menerapkan pengaturan terakhir (FR-05, TC-008)
- [ ] Daftar kamera tidak memuat entri palsu (TC-009)

### 4.2 Live view dan pemrosesan
- [ ] Latensi p95 ≤ 100 ms pada 1280x720 (NFR-01, TC-020)
- [ ] Preview selalu frame terbaru saat pemrosesan diperlambat (FR-08, TC-021)
- [ ] Auto stretch tidak berkedip pada scene statis (FR-09, TC-022)
- [ ] Histogram diperbarui ≥ 5 Hz (FR-09, TC-023)
- [ ] Data 16-bit tidak kehilangan level sampai tahap render (FR-13, TC-024)
- [ ] Kalibrasi flat meratakan vignetting sintetis (deviasi ≤ 2%) (FR-11, TC-025)
- [ ] Dark dikurangkan dengan benar dan tidak menghasilkan nilai negatif/wrap (FR-11, TC-026)
- [ ] Flat dibuat dari frame mentah tanpa gain/WB/flat lama (FR-11, TC-027)
- [ ] Sigma-clip membuang outlier; mean dan sum berbeda sesuai definisinya (FR-12, TC-028)
- [ ] Stacking menghormati batas RAM (FR-12, TC-029)
- [ ] CLAHE/sharpen/invert tidak mengubah data rekaman (FR-10, TC-030)

### 4.3 Perekaman dan data
- [ ] Quick capture menghasilkan satu berkas dengan nama benar (FR-14, TC-040)
- [ ] Sequence N frame menghasilkan tepat N berkas berurutan (FR-14, TC-041)
- [ ] Header FITS sama dengan nilai hardware pada frame yang sama (FR-15, TC-042)
- [ ] Manifest sesi valid terhadap skema; ditulis berkala (FR-17, TC-043)
- [ ] Kill proses saat rekam: sesi sebelumnya tidak rusak, berkas terakhir terdeteksi tidak lengkap (NFR-07, TC-044)
- [ ] Peringatan disk < 2 GB muncul; rekam berhenti aman saat disk penuh (FR-18, TC-045)
- [ ] Frame hilang selalu dihitung dan tercatat di log dan manifest (NFR-06, TC-046)

### 4.4 Deteksi AI
- [ ] Model ONNX dimuat; tombol AI menampilkan tiga keadaan (FR-19, TC-050)
- [ ] Overlay muncul/hilang sesuai tombol AI; overlay basi (> 500 ms) tidak digambar (FR-19, TC-051)
- [ ] FPS preview turun ≤ 5% saat AI aktif (NFR-03, TC-052)
- [ ] Setiap deteksi di overlay memiliki satu baris log lengkap (FR-20, TC-053)
- [ ] Perubahan threshold berlaku tanpa restart (FR-21, TC-054)
- [ ] Frame rekaman identik piksel dengan frame mentah saat AI aktif (FR-22, TC-055)
- [ ] Model hilang/rusak: AI nonaktif dengan pesan jelas, live view tetap jalan (TC-056)
- [ ] Paritas ONNX terhadap .pt: selisih mAP50 ≤ 1 poin (G-05, TC-057)

### 4.5 Astronomi
- [ ] Posisi bulan/matahari sesuai referensi (Alt/Az ≤ 0,1 derajat) (FR-23, TC-060)
- [ ] Ijtima sesuai referensi (≤ 2 menit) (FR-23, TC-061)
- [ ] Sunset dan moonset sesuai referensi (≤ 1 menit) (FR-24, TC-062)
- [ ] Kriteria MABIMS: nilai batas menghasilkan hasil benar (3,0 / 6,4) (FR-25, TC-063)
- [ ] Kriteria Odeh sesuai contoh dalam publikasi sumber, diverifikasi ahli falak (FR-25, TC-064)
- [ ] Zona waktu tampil di UI dan dicatat di manifest (FR-27, TC-065)
- [ ] Lokasi tidak valid ditolak dengan pesan rentang (FR-27, TC-066)
- [ ] Metode kalender Hijriyah tampil dan tercatat (FR-26, TC-067)

### 4.6 Aplikasi dan pengaturan
- [ ] Tiga tema berfungsi; Night Red tidak memiliki elemen putih terang (FR-28, TC-070)
- [ ] Ganti bahasa ID/EN tanpa string hardcode tersisa (FR-29, TC-071)
- [ ] Profil capture mengubah nilai terbaca dari kamera (FR-30, TC-072)
- [ ] Impor Onyx menghasilkan settings v2 valid; laporan key yang dibuang (FR-31, TC-073)
- [ ] Laptop tidak tidur selama sesi 60 menit (FR-32, TC-074)
- [ ] Paket diagnostik dibuat ≤ 10 detik dan tidak berisi frame bila tidak dipilih (FR-33, TC-075)
- [ ] Peringatan sinkron jam muncul bila jam digeser (FR-35, TC-076)
- [ ] Semua skenario inti berjalan dengan akun standar (FR-36, TC-077)

## 5. Test Case Detail

Tipe: P = positif, N = negatif, E = edge.

| ID | Skenario | Prasyarat | Langkah | Hasil Diharapkan | Prioritas | Tipe |
|---|---|---|---|---|---|---|
| TC-001 | Sambung kamera ZWO | Kamera terpasang USB 3.0, driver terpasang | 1. Buka SCR-04. 2. Pilih ZWO. 3. Klik Sambungkan. | Status Terhubung ≤ 5 detik; frame tampil; kontrol aktif sesuai kemampuan | P1 | P |
| TC-004 | Baca-balik pengaturan | TC-001 lulus | 1. Set exposure 50 ms. 2. Baca nilai di UI. 3. Baca nilai dari SDK/log. | Nilai UI = nilai SDK; log mencatat selisih bila ada pembulatan | P1 | P |
| TC-005 | Gain di luar rentang | Kamera tersambung | 1. Masukkan gain 99999. | Ditolak dengan pesan rentang; nilai tetap nilai sebelumnya | P1 | N |
| TC-007 | Kabel dicabut saat live | Live view berjalan | 1. Cabut kabel USB. | Status merah ≤ 3 detik; tidak ada hang; tombol Rekam nonaktif | P1 | E |
| TC-008 | Sambung ulang | TC-007 | 1. Colok kembali. 2. Tunggu. | Kamera tersambung kembali ≤ 15 detik; pengaturan terakhir diterapkan dan ditampilkan | P1 | E |
| TC-009 | Tanpa kamera | Tidak ada perangkat | 1. Buka SCR-04. | Empty state dengan panduan; simulator tersedia; tidak ada entri palsu | P2 | N |
| TC-021 | Preview tidak antre | Simulator, penundaan buatan 200 ms di tahap proses | 1. Jalankan 30 detik. 2. Bandingkan nomor frame di layar dengan frame terbaru kamera. | Selisih ≤ 2 frame; ring buffer tidak membesar | P1 | E |
| TC-024 | Jalur 16-bit | Simulator gradien 0..65535 | 1. Rekam 1 frame FITS. 2. Baca ulang. | 65.536 level tidak berkurang (histogram data mentah utuh) | P1 | P |
| TC-026 | Dark tidak wrap | Master dark lebih terang dari beberapa piksel frame | 1. Aktifkan dark. | Nilai negatif dijepit ke 0 (bukan wrap-around) | P1 | E |
| TC-028 | Sigma-clip | 20 frame sintetis, 1 frame berisi piksel 65535 di satu posisi | 1. Stack sigma-clip k=2. | Piksel outlier tidak mempengaruhi hasil; hasil = nilai latar ± toleransi | P1 | P |
| TC-029 | Batas RAM | stack_ram_mb = 64, frame 1920x1080 | 1. Jalankan stacking 100 frame. | Memori stacking ≤ 64 MB; UI memberi tahu jendela dikurangi | P2 | E |
| TC-041 | Sequence N frame | Simulator | 1. Sequence 100 frame FITS. | 100 berkas berurutan; tidak lebih; manifest counters = 100/0 | P1 | P |
| TC-042 | Header FITS | Kamera ZWO | 1. Set exposure 100 ms, gain 200. 2. Rekam 1 frame. 3. Baca header. | EXPTIME=0.1, GAIN=200, CCD-TEMP = suhu terukur, DATE-OBS UTC; sama dengan log driver | P1 | P |
| TC-044 | Kill saat rekam | Rekam 1080p aktif | 1. Matikan proses paksa. 2. Buka folder. | Berkas lama utuh; berkas sementara terdeteksi tidak lengkap; manifest terakhir ≤ 5 detik sebelum kill | P1 | E |
| TC-045 | Disk penuh | Folder pada partisi kecil | 1. Rekam sampai penuh. | Rekam berhenti aman; manifest final; UI menjelaskan sebab; tidak crash | P1 | N |
| TC-046 | Beban berlebih | Simulator 60 fps, disk diperlambat | 1. Rekam 60 detik. | frames_written + frames_dropped = frames_sent; tidak ada drop tanpa catatan | P1 | E |
| TC-051 | Overlay basi | AI dilambatkan 800 ms | 1. Aktifkan AI. | Overlay yang usianya > 500 ms tidak digambar | P2 | E |
| TC-055 | Rekaman bersih | AI aktif, deteksi muncul | 1. Rekam 10 frame. 2. Bandingkan dengan frame mentah simulator (seed sama). | Selisih piksel = 0 | P1 | P |
| TC-056 | Model rusak | File model dipotong | 1. Aktifkan AI. | AI nonaktif + pesan; live view normal | P2 | N |
| TC-057 | Paritas ONNX | 20 gambar tetap | 1. Jalankan ONNX. 2. Bandingkan dengan acuan .pt. | Kotak dan confidence dalam toleransi; mAP50 selisih ≤ 1 poin | P1 | P |
| TC-061 | Ijtima | Tabel golden | 1. Hitung ijtima 10 tanggal. | Selisih ≤ 2 menit terhadap referensi | P1 | P |
| TC-062 | Sunset/moonset | Tabel golden, 3 lokasi (termasuk lintang tinggi dan dekat ekuator) | 1. Hitung. | Selisih ≤ 1 menit | P1 | P |
| TC-063 | Batas MABIMS | Nilai uji alt=2,99/3,00/3,01 dan elongasi 6,39/6,40/6,41 | 1. Evaluasi kriteria. | Hanya pasangan yang memenuhi kedua batas (≥) yang lolos | P1 | E |
| TC-066 | Koordinat tidak valid | SCR-07 | 1. Isi lintang 95. | Kolom merah dengan pesan rentang; tidak tersimpan | P2 | N |
| TC-070 | Tema Night Red | Tema aktif | 1. Buka semua layar SCR-xx. 2. Tangkap layar. | Tidak ada piksel dengan luminans di atas ambang (ASUMSI: komponen hijau dan biru ≤ batas palet) | P2 | P |
| TC-073 | Impor Onyx | `settings.json` Onyx contoh | 1. Berkas > Impor. | Settings v2 valid; laporan memuat `usb_limit`, `gpu_acceleration`, dst. sebagai dibuang | P2 | P |
| TC-074 | Cegah tidur | Setelan daya: tidur 5 menit | 1. Mulai sesi. 2. Biarkan 60 menit. | Laptop tidak tidur; setelah sesi berakhir, perilaku normal kembali | P2 | P |
| TC-077 | Akun standar | Akun tanpa hak Administrator | 1. Jalankan semua skenario TC-001..TC-046. | Semua lulus; bila ada yang gagal, dicatat penyebab (driver/izin) | P1 | P |

Kasus negatif, boundary, dan error handling tambahan (dijalankan otomatis bila memungkinkan):

| ID | Skenario | Hasil Diharapkan | Tipe |
|---|---|---|---|
| TC-080 | Berkas pengaturan rusak | Muat default, simpan `.corrupt-<tanggal>`, beri tahu pengguna | N |
| TC-081 | FITS impor dimensi tidak cocok | Ditolak dengan pesan dimensi | N |
| TC-082 | Berkas impor > batas ukuran | Ditolak sebelum dimuat penuh | N |
| TC-083 | Exposure di batas minimum dan maksimum | Diterapkan dan terbaca balik; di luar batas ditolak | E |
| TC-084 | Resolusi 1x1 / ROI ekstrem | Ditolak atau dijepit dengan pesan; tidak crash | E |
| TC-085 | Folder simpan tidak bisa ditulis | SCR-05 menolak dengan pesan | N |
| TC-086 | Nama sesi mengandung karakter terlarang | Dinormalisasi; manifest mencatat nama asli dan nama folder | E |
| TC-087 | Hari tanpa sunset (lintang ekstrem) | Pesan "tidak ada sunset" bukan crash | E |
| TC-088 | Jam sistem mundur selama rekam | Timestamp tetap monoton atau ditandai; log memberi peringatan | E |
| TC-089 | Dua kali klik Sambungkan cepat | Hanya satu koneksi; tidak ada thread ganda | E |

## 6. Checklist Non-Fungsional

### 6.1 Performa dan beban
- [ ] Latensi sensor ke layar p95 ≤ 100 ms (1280x720) dan ≤ 150 ms (1920x1080) (NFR-01)
- [ ] Waktu frame UI p95 ≤ 16 ms (NFR-02)
- [ ] AI ≥ 3 inferensi/detik di laptop referensi; FPS preview turun ≤ 5% (NFR-03)
- [ ] Startup dingin ≤ 5 detik, p95 dari 10 kali (NFR-04)
- [ ] Ring buffer dan stacking tidak melewati batas pengaturan (NFR-05)
- [ ] Rekam 1920x1080 16-bit, 20 fps, 30 menit ke SSD: frame hilang ≤ 1% (G-02)
- [ ] Uji rendam 2 jam (live view + AI + rekam): tidak ada kebocoran memori (selisih memori setelah menit ke-10 dan menit ke-120 ≤ 5%)
- [ ] CPU idle < 2% tanpa kamera (NFR-15)
- [ ] Laptop terlemah: degradasi teratur (AI berkurang dulu, preview tidak)

### 6.2 Keamanan
Mengikuti OWASP Top 10 yang relevan untuk aplikasi desktop offline. Rinci di 09b-Security-Plan.md.
- [ ] Tidak ada koneksi jaringan keluar (pemantauan lalu lintas selama sesi penuh)
- [ ] Berkas FITS/TIFF/PNG hasil fuzz sederhana (header rusak, dimensi besar) tidak menyebabkan crash atau pembacaan di luar batas (injeksi/berkas berbahaya)
- [ ] Model ONNX dengan hash tidak cocok ditolak tanpa konfirmasi
- [ ] Hanya DLL dari folder aplikasi yang dimuat (uji penempatan DLL palsu di CWD dan PATH)
- [ ] ZIP rilis bertanda tangan valid; checksum cocok
- [ ] Paket diagnostik menyamarkan koordinat bila dipilih; tidak berisi data lain
- [ ] Tidak ada rahasia/kunci di binari atau repo (pemindaian)
- [ ] Autentikasi dan XSS: tidak berlaku (tidak ada akun/web); dicatat sebagai tidak dapat diterapkan

### 6.3 Kompatibilitas perangkat dan OS
- [ ] Windows 10 22H2 x64, Windows 11 x64
- [ ] Skala DPI 100%, 125%, 150%, 200%
- [ ] Resolusi 1366x768, 1920x1080, 2560x1440; jendela minimum 1120x720
- [ ] Kamera ZWO (model uji), webcam UVC, simulator
- [ ] USB 3.0 langsung dan lewat hub
- [ ] VM Windows bersih tanpa runtime tambahan

### 6.4 Aksesibilitas
- [ ] Kontras tema Dark dan Light ≥ 4,5:1 (pemeriksaan otomatis dari token)
- [ ] Night Red: kontras dalam palet merah dicatat dan ≥ 4,5:1
- [ ] Semua fungsi inti dapat dicapai dengan keyboard; urutan Tab logis
- [ ] Nama aksesibel pada semua kontrol; slider membacakan nilai dan satuan
- [ ] Target klik ≥ 40x40 px pada toolbar dan tombol utama

### 6.5 Usability
- [ ] Pengguna baru mencapai gambar pertama (simulator) ≤ 3 menit tanpa bantuan (G-07), diuji pada 3 orang
- [ ] Tester lapangan menemukan Rekam, Quick Capture, dan AI tanpa petunjuk di bawah 1 menit
- [ ] Setiap pesan galat memuat penyebab dan tindakan berikutnya
- [ ] Tidak ada elemen UI yang tidak berfungsi (kontrol tanpa efek, entri menu palsu)

### 6.6 Kompatibilitas data dan migrasi
- [ ] Settings v1 hipotetis dan Onyx dimigrasikan ke v2 tanpa kehilangan nilai yang dipakai
- [ ] Berkas settings versi lebih baru dari aplikasi dibuka dengan peringatan, tidak ditimpa
- [ ] FITS yang dihasilkan terbuka di setidaknya dua alat pihak ketiga (ASUMSI: dipilih tim; contoh: pustaka astropy dan satu penampil FITS) dengan header lengkap
- [ ] Manifest sesi versi 1 tetap terbaca di versi berikutnya

## 7. Regression Suite

Wajib dijalankan tiap rilis (otomatis bila bisa; kolom Sumber menunjukkan jenisnya).

| No | Skenario | Sumber |
|---|---|---|
| 1 | Build CI dari nol, semua unit dan integrasi lulus | Otomatis |
| 2 | Kontrak `ICamera` terhadap simulator | Otomatis |
| 3 | TC-024 jalur 16-bit | Otomatis |
| 4 | TC-028 sigma-clip dan stacking | Otomatis |
| 5 | TC-041/TC-042 sequence dan header FITS | Otomatis |
| 6 | TC-046 hitungan frame hilang | Otomatis |
| 7 | TC-055 rekaman bersih | Otomatis |
| 8 | TC-057 paritas ONNX | Otomatis |
| 9 | TC-060 sampai TC-063 efemeris dan kriteria | Otomatis |
| 10 | TC-073 migrasi Onyx dan TC-080 pengaturan rusak | Otomatis |
| 11 | Pembuatan semua layar offscreen, pergantian tema dan bahasa | Otomatis |
| 12 | Sambung kamera ZWO, ubah exposure/gain, baca-balik (TC-001, TC-004) | Manual (perangkat) |
| 13 | Kabel dicabut dan sambung ulang (TC-007, TC-008) | Manual (perangkat) |
| 14 | Rekam 30 menit 1080p 16-bit, hitung frame hilang | Manual (laptop referensi) |
| 15 | Benchmark latensi p95 dan FPS dengan AI on/off | Manual (laptop referensi) |
| 16 | Smoke test pasca-build di VM bersih (bagian 10) | Manual |

## 8. Exit/Entry Criteria

**Entry (mulai tahap uji sistem/RC)**
- Feature freeze (M6) tercapai; build RC tersedia dari CI.
- Seluruh unit dan integrasi lulus; tidak ada bug severity 1 terbuka dari tahap sebelumnya.
- Kamera uji, laptop referensi, VM bersih siap.

**Exit (RC diterima, M7)**
- Seluruh regresi bagian 7 lulus.
- Tidak ada bug severity 1 atau 2 terbuka; bug severity 3 memiliki keputusan tertulis (perbaiki atau tunda).
- Target G-01, G-02, G-03, G-05, G-07, G-08 terpenuhi pada laptop referensi.
- Coverage `core`/`imaging`/`astro` ≥ 80%.
- FT1 dan FT2 selesai tanpa crash; temuan dikelola.
- ZIP bertanda tangan dan checksum diverifikasi.

**Exit uji lapangan (setiap FT):** laporan tertulis (temuan, rekaman, paket diagnostik) dalam 1 hari.

## 9. Manajemen Bug

**Severity**

| Severity | Definisi | Contoh |
|---|---|---|
| S1 | Menghalangi tugas inti atau menyebabkan kehilangan data diam-diam | Crash saat rekam; frame hilang tidak dilaporkan; pengaturan tidak sampai ke kamera |
| S2 | Fungsi penting rusak, ada jalan memutar | Reconnect gagal tetapi sambung manual berhasil; kriteria Odeh salah |
| S3 | Fungsi minor atau tampilan | Tooltip salah; teks terpotong |
| S4 | Kosmetik/peningkatan | Penyelarasan ikon |

**Prioritas:** P1 diperbaiki sebelum RC; P2 sebelum rilis berikutnya; P3 dijadwalkan.

**Template laporan bug**

```
Judul:
ID / tanggal / pelapor:
Versi build dan hash:
Lingkungan (OS, laptop, kamera, driver, versi SDK):
Severity / prioritas usulan:
Langkah mereproduksi:
 1.
 2.
Hasil aktual:
Hasil diharapkan:
Frekuensi (selalu / kadang / sekali):
Lampiran: paket diagnostik (Alat > Ekspor diagnostik), tangkapan layar, session.log
Catatan:
```

## 10. Smoke Test Pasca-Deploy

Dijalankan pada VM bersih setelah ZIP rilis diunduh (≤ 15 menit):

- [ ] Verifikasi checksum SHA-256 ZIP cocok dengan yang dipublikasikan
- [ ] Verifikasi tanda tangan digital `Sabit.exe` valid
- [ ] Ekstrak ZIP ke folder tanpa hak khusus; jalankan `Sabit.exe` dengan akun standar
- [ ] Jendela muncul ≤ 5 detik; wizard pertama berjalan
- [ ] Pilih Simulator - Hilal senja; gambar tampil
- [ ] Ubah exposure/gain; nilai berubah di UI
- [ ] Quick capture menghasilkan FITS yang dapat dibaca; header terisi
- [ ] Nyalakan AI; overlay muncul pada simulator
- [ ] Efemeris menampilkan nilai untuk lokasi preset
- [ ] Ganti tema dan bahasa
- [ ] Tutup aplikasi; tidak ada proses tersisa; `app.log` tanpa error

## 11. Laporan & Metrik QA

| Metrik | Definisi | Target |
|---|---|---|
| Pass rate | Test case lulus / dijalankan, per rilis | ≥ 98% pada RC; 100% untuk regresi nomor 1 sampai 11 |
| Defect density | Bug per 1.000 baris kode baru | Dipantau tren; tidak ada target mutlak (ASUMSI) |
| Coverage | Baris pada `core`, `imaging`, `astro` | ≥ 80% |
| Escaped defects | Bug S1/S2 yang ditemukan di lapangan, bukan di QA | 0 pada FT3 |
| Latensi/FPS | Hasil benchmark per rilis | Sesuai NFR-01 sampai NFR-03 |
| Waktu perbaikan S1 | Dari laporan sampai build perbaikan | ≤ 2 hari kerja |

Laporan QA per sprint: ringkasan hasil, tren metrik, bug terbuka per severity, risiko. Setelah FT: laporan lapangan terpisah.

## 12. Risiko Testing

| Risiko | Dampak | Mitigasi |
|---|---|---|
| Hanya satu kamera ZWO tersedia | Variasi perangkat rendah | Dokumentasikan model; kontrak driver diuji ketat di simulator |
| Sedikit kesempatan uji lapangan | Bug muncul di FT3 | FT2b, rekaman lama, pemutaran ulang, simulator |
| Tabel referensi efemeris tidak independen | Kesalahan sistematik lolos | Gunakan sumber berbeda dari pustaka yang dipakai; ahli falak memeriksa sampel |
| Benchmark tidak stabil karena beban latar belakang | Angka tidak dapat dipercaya | Jalankan 10 kali, laporkan p50/p95, matikan proses non-esensial |
| Set data AI terlalu kecil | Paritas ONNX tidak mewakili | Tambah gambar tetap dari rekaman lapangan setelah FT |
| Pengujian UI otomatis terbatas | Regresi visual lolos | Tangkapan layar acuan untuk layar utama, tinjauan manual per rilis |

## 13. Contoh Kode Otomasi (Catch2 v3)

Tiga test kritis. Nama tipe mengikuti sketsa di Tech Spec (`RingBuffer`, `Stacker`, `Ephemeris`); sesuaikan setelah API final.

### 13.1 Ring buffer: drop-oldest dan penghitung (mendukung TC-046, NFR-06)

```cpp
#include <catch2/catch_test_macros.hpp>
#include "core/ring_buffer.h"

using sabit::core::RingBuffer;

TEST_CASE("RingBuffer membuang elemen terlama dan menghitung drop", "[core][ringbuffer]") {
    RingBuffer<int> rb(/*capacity=*/3);

    for (int i = 1; i <= 5; ++i) rb.push(i);   // 1,2 terbuang

    CHECK(rb.size() == 3);
    CHECK(rb.dropped() == 2);

    int v = 0;
    REQUIRE(rb.try_pop(v));  CHECK(v == 3);
    REQUIRE(rb.try_pop(v));  CHECK(v == 4);
    REQUIRE(rb.try_pop(v));  CHECK(v == 5);
    CHECK_FALSE(rb.try_pop(v));

    // Invarian: terkirim = tertulis + terbuang
    CHECK(rb.pushed() == rb.popped() + rb.dropped());
}
```

### 13.2 Stacker: sigma-clip membuang outlier, sum tidak dibagi N (TC-028)

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "imaging/stacker.h"
#include "testing/synthetic_frames.h"   // helper: frame konstan 16-bit

using namespace sabit::imaging;
using Catch::Matchers::WithinAbs;

TEST_CASE("Sigma-clip membuang piksel outlier", "[imaging][stack]") {
    const int W = 64, H = 64, N = 20;
    Stacker stacker({.method = StackMethod::SigmaClip, .k_sigma = 2.0, .max_frames = N});

    for (int i = 0; i < N; ++i) {
        auto f = make_constant_frame(W, H, /*value=*/1000);
        if (i == 7) f.at(10, 10) = 65535;          // satu piksel outlier
        stacker.add(f);
    }

    const auto out = stacker.result();             // float32
    CHECK_THAT(out.at(10, 10), WithinAbs(1000.0, 1.0));  // outlier tidak ikut
    CHECK_THAT(out.at(0, 0),   WithinAbs(1000.0, 0.5));
}

TEST_CASE("Metode Sum tidak dibagi jumlah frame", "[imaging][stack]") {
    Stacker stacker({.method = StackMethod::Sum, .max_frames = 4});
    for (int i = 0; i < 4; ++i) stacker.add(make_constant_frame(8, 8, 100));

    CHECK_THAT(stacker.result().at(0, 0), WithinAbs(400.0, 0.001));
}
```

### 13.3 Efemeris: ijtima terhadap tabel referensi (TC-061, G-08)

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <chrono>
#include "astro/ephemeris.h"
#include "testing/golden_loader.h"   // membaca tests/golden/ephemeris/*.csv

using namespace std::chrono;

TEST_CASE("Ijtima sesuai tabel referensi (<= 2 menit)", "[astro][golden]") {
    const auto rows = load_golden_ijtima("tests/golden/ephemeris/ijtima.csv");
    REQUIRE_FALSE(rows.empty());

    sabit::astro::Ephemeris eph;
    for (const auto& r : rows) {
        DYNAMIC_SECTION("Ijtima dekat " << r.approx_utc_iso) {
            const auto got = eph.next_new_moon(r.search_from_utc);
            const auto diff = duration_cast<seconds>(got - r.expected_utc);
            CHECK(std::abs(diff.count()) <= 120);
        }
    }
}
```

Tambahan yang disarankan di CI: build uji dengan `-fsanitize=address,undefined` (Clang/MSVC ASan), `ctest --output-on-failure`, laporan coverage (OpenCppCoverage atau alat setara), dan job benchmark `--perf` dengan simulator.

## 14. Open Questions

1. Sumber tabel referensi efemeris yang disetujui ahli falak; dibuat oleh siapa?
2. Berapa banyak perangkat kamera yang bisa dipinjam untuk uji kompatibilitas?
3. Apakah ada standar internal penerimaan data untuk lembaga (misalnya format laporan sesi) yang harus diuji?
4. Alat pihak ketiga mana yang menjadi acuan "FITS terbaca benar"?
5. Siapa yang menjadi tester lapangan dan apakah mereka bagian dari target persona P1?
