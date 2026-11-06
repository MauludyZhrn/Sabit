# PRD: Sabit

Versi dokumen: 0.1 (draf) | Tanggal: 23-10-2026 | Status: menunggu review
Dasar dokumen: hasil bedah proyek Onyx OS (Python/PyQt6) dan rekomendasi rebuild (C++/Qt 6).

Sabit adalah nama kerja untuk rebuild Onyx OS. Nama bisa diganti sebelum rilis (lihat Open Questions).

---

## 1. Ringkasan Eksekutif

**Visi.** Satu aplikasi desktop yang bisa dibawa ke lapangan dan dipercaya saat rukyat hilal: kamera tersambung, gambar muncul tanpa lag, hilal terdeteksi secara live, dan semua data tersimpan dengan metadata yang benar.

**Tujuan.**
- Mengganti Onyx OS (prototipe Python) dengan aplikasi native yang lebih cepat, lebih hemat memori, dan tidak butuh Python terpasang di laptop lapangan.
- Memperbaiki cacat fungsional yang ditemukan pada Onyx OS: kontrol exposure/gain yang tidak sampai ke kamera, pipeline 8-bit yang membuang data, stacking yang bukan stacking sungguhan, dan proses berat di thread UI.
- Menyediakan data yang layak dipakai untuk dokumentasi dan verifikasi: FITS 16-bit dengan metadata dari hardware, log deteksi, dan perhitungan astronomi yang jelas dasarnya.

**Nilai bisnis / organisasi.** Mengurangi risiko kegagalan di malam observasi (kesempatan rukyat hanya datang sebulan sekali), meningkatkan kredibilitas data, dan membuat basis kode yang bisa dipelihara tim kecil.

## 2. Latar Belakang & Problem Statement

Onyx OS adalah aplikasi desktop Windows (PyQt6 + OpenCV + YOLO) untuk live view kamera astronomi, deteksi hilal, stacking, kalibrasi flat/dark, dan efemeris bulan. Bedah kode menemukan masalah berikut (ringkas):

| Masalah pada Onyx OS | Dampak di lapangan |
|---|---|
| Slider gain/exposure hanya mengalikan kecerahan gambar secara software; tidak mengubah kamera | Operator mengira sensor diatur, padahal tidak; hilal redup tidak terintegrasi cahaya |
| Pengaturan binning/bit depth/cooler dipanggil sebelum objek kamera ada, jadi tidak berefek | Pengaturan hilang diam-diam saat connect |
| Semua frame dipaksa 8-bit sejak dari worker; RAW16 dinormalisasi per frame | Kecerahan berkedip, rentang dinamis hilang sebelum stacking dan FITS |
| FITS berisi data 8-bit dengan EXPTIME/CCD-TEMP dari nilai UI, bukan hardware | Data tidak valid sebagai bukti ilmiah |
| Proses gambar, histogram, tulis file, dan render berjalan di thread UI | Frame drop saat merekam; UI tersendat |
| Saat AI aktif, layar menampilkan frame lama hasil anotasi | Tampilan tidak benar-benar live |
| "Stacking" adalah rolling window 8-bit; "sum" sama dengan mean; "sigma_clip" tidak membuang outlier | Hasil stacking tidak sesuai nama metodenya |
| Kegagalan koneksi kamera tidak diberi tahu ke UI | UI tampak aktif padahal tidak ada sinyal |
| Perhitungan hilal memakai waktu "sekarang", tanpa waktu Maghrib/moonset | Fitur inti rukyat belum ada |
| 13 dari 15 key pengaturan yang diperiksa tidak dipakai kode | Pengguna mengubah setelan yang tidak berefek |

Masalah yang ingin diselesaikan: operator rukyat butuh alat live view dan deteksi yang responsif, jujur terhadap hardware, dan menghasilkan data yang bisa dipertanggungjawabkan, dijalankan di laptop biasa di lokasi tanpa infrastruktur.

## 3. Tujuan & Success Metrics

Semua metrik diukur pada laptop referensi (ASUMSI: Intel Core i5 generasi 8 atau Ryzen 5 setara, RAM 8 GB, SSD, USB 3.0, Windows 10 22H2 atau Windows 11, tanpa GPU diskrit).

| ID | Tujuan | Metrik (SMART) | Target | Batas waktu |
|---|---|---|---|---|
| G-01 | Live view responsif | Latensi sensor ke layar, p95, 1280x720 | ≤ 100 ms | 22-01-2027 |
| G-02 | Perekaman andal | Frame yang dikirim kamera tapi tidak tercatat, rekam 1920x1080 16-bit, 20 fps ke SSD | ≤ 1% | 22-01-2027 |
| G-03 | AI tidak mengganggu live view | Penurunan FPS preview saat AI aktif | ≤ 5% | 22-01-2027 |
| G-04 | Kontrol hardware nyata | Pengaturan exposure/gain/bin yang terverifikasi terbaca balik dari kamera | 100% pada kamera ZWO uji | 11-12-2026 |
| G-05 | Paritas model AI | Selisih mAP50 antara model .pt asli dan hasil ekspor ONNX pada set uji yang sama | ≤ 1 poin absolut | 08-01-2027 |
| G-06 | Uji lapangan | Sesi uji lapangan tanpa crash | 3 dari 3 (02-11-2026, 05-11-2026, 07-11-2026) | 07-11-2026 |
| G-07 | Kemudahan mulai | Waktu dari ekstrak ZIP sampai gambar pertama (kamera simulator) pada pengguna baru | ≤ 3 menit, tanpa hak Administrator | 23-11-2026 |
| G-08 | Akurasi astronomi | Selisih posisi bulan/matahari dan waktu ijtima terhadap tabel referensi independen | Alt/Az ≤ 0,1 derajat; ijtima ≤ 2 menit | 27-11-2026 |

Catatan: nilai target adalah ASUMSI awal. Setiap target harus dikonfirmasi pemilik produk dan diukur ulang pada perangkat nyata.

## 4. Persona Pengguna

**P1. Pengamat lapangan (Rina, anggota tim rukyat)**
- Kebutuhan: menyambungkan kamera cepat, melihat hilal jelas, merekam tanpa menebak pengaturan.
- Pain point: waktu pengamatan sempit (hitungan menit setelah Maghrib), gelap, tangan sibuk dengan teleskop, laptop bukan spesifikasi tinggi, tidak ada internet.
- Goals: gambar live stabil, deteksi yang membantu tanpa menipu, satu tombol rekam, dan tahu persis kapan data aman tersimpan.

**P2. Koordinator / ahli falak (Pak Hadi, penanggung jawab data)**
- Kebutuhan: data yang bisa diaudit: FITS dengan metadata benar, log deteksi, parameter kamera, lokasi, dan dasar kriteria yang dipakai.
- Pain point: tidak bisa memverifikasi apakah angka di layar berasal dari hardware atau emulasi; kriteria (MABIMS, Odeh, BRIN) tidak jelas rumusnya.
- Goals: laporan sesi yang konsisten, perhitungan astronomi yang bisa dicek silang, dan kriteria yang bisa dikonfigurasi.

**P3. Pengembang / penghobi astrofotografi (Dimas)**
- Kebutuhan: ekspor frame mentah 16-bit, log deteksi untuk melatih ulang model, kamera simulator untuk pengujian.
- Pain point: pipeline lama membuang data; sulit membedakan frame mentah dan frame beranotasi.
- Goals: akses data mentah, format terbuka (FITS/TIFF/CSV), dan struktur sesi yang bisa diproses skrip.

## 5. User Stories

| ID | Cerita | Persona | Terkait FR |
|---|---|---|---|
| US-01 | Sebagai pengamat, saya ingin menyambungkan kamera ZWO dengan satu klik, agar saya bisa mulai melihat gambar sebelum Maghrib. | P1 | FR-01, FR-05 |
| US-02 | Sebagai pengamat, saya ingin mengubah exposure dan gain dan melihat hasilnya langsung dari sensor, agar saya tahu pengaturan benar-benar diterapkan. | P1 | FR-04 |
| US-03 | Sebagai pengamat, saya ingin tampilan merah (night vision), agar mata saya tetap beradaptasi dengan gelap. | P1 | FR-28 |
| US-04 | Sebagai pengamat, saya ingin deteksi hilal tampil sebagai overlay tanpa menunda gambar live, agar saya tetap melihat langit sebenarnya. | P1 | FR-19, FR-22 |
| US-05 | Sebagai pengamat, saya ingin tahu waktu Maghrib, moonset, dan kondisi hilal saat itu, agar saya tahu kapan harus siap. | P1, P2 | FR-23, FR-24 |
| US-06 | Sebagai pengamat, saya ingin perekaman berlanjut aman dan memberi peringatan jika disk penuh atau ada frame hilang, agar saya tidak kehilangan data tanpa tahu. | P1 | FR-14, FR-18 |
| US-07 | Sebagai ahli falak, saya ingin FITS berisi exposure, gain, suhu sensor, dan lokasi yang berasal dari hardware dan pengaturan sesi, agar data bisa diaudit. | P2 | FR-15, FR-17 |
| US-08 | Sebagai ahli falak, saya ingin memilih kriteria visibilitas dan melihat ambang yang dipakai, agar hasil bisa dibandingkan dengan keputusan otoritas. | P2 | FR-25 |
| US-09 | Sebagai ahli falak, saya ingin mengekspor paket diagnostik sesi, agar masalah bisa dianalisis tanpa saya hadir di lokasi. | P2 | FR-33 |
| US-10 | Sebagai pengembang, saya ingin kamera simulator yang menghasilkan frame 16-bit realistis, agar saya bisa menguji tanpa perangkat. | P3 | FR-03 |
| US-11 | Sebagai pengembang, saya ingin log deteksi dengan parameter kamera tiap baris, agar saya bisa membuat dataset latih ulang. | P3 | FR-20 |
| US-12 | Sebagai pengamat, saya ingin menjalankan aplikasi tanpa hak Administrator, agar saya bisa memakai laptop pinjaman. | P1 | FR-36 |
| US-13 | Sebagai pengamat, saya ingin laptop tidak tidur saat sesi berjalan, agar perekaman tidak terputus. | P1 | FR-32 |
| US-14 | Sebagai pengamat, saya ingin kalibrasi dark/flat dari file atau dari capture langsung, agar gambar hilal lebih bersih. | P1, P3 | FR-11 |
| US-15 | Sebagai pengguna Onyx OS, saya ingin pengaturan lama saya dibawa ke Sabit, agar tidak mengisi ulang dari nol. | P1, P2 | FR-31 |

## 6. Functional Requirements

Prioritas MoSCoW. Kolom Fase: **MVP** = rilis 1.0 (target 23-11-2026), **F2** = fase 2.

### 6.1 Kamera & capture

| ID | Fitur | Deskripsi | Prioritas | Fase | Acceptance Criteria |
|---|---|---|---|---|---|
| FR-01 | Driver ZWO ASI native | Deteksi, koneksi, dan streaming kamera ZWO lewat SDK native. | Must | MVP | Kamera ZWO uji terdeteksi dan menampilkan frame dalam ≤ 5 detik setelah klik Connect; Cabut kabel memicu status Disconnected ≤ 3 detik. |
| FR-02 | Driver UVC | Webcam dan capture card lewat DirectShow/Media Foundation. | Must | MVP | Webcam uji menampilkan frame; daftar kamera diperbarui saat perangkat dicolok. |
| FR-03 | Kamera simulator | Menghasilkan frame 16-bit sintetis (mode hilal senja, deep sky, high speed) dengan noise dan bintang. | Must | MVP | Ketiga mode menghasilkan frame berbeda (diuji otomatis); metadata exposure/gain mengikuti pengaturan. |
| FR-04 | Kontrol hardware | Exposure, gain, binning, bit depth, ROI, dan cooler diterapkan ke sensor; rentang nilai berasal dari capabilities kamera. | Must | MVP | Setiap perubahan dibaca balik dari kamera dan ditampilkan; kontrol yang tidak didukung kamera tampak nonaktif dengan alasan. |
| FR-05 | Status koneksi | UI mendapat event Connected/Error/Disconnected dengan pesan jelas; ada reconnect otomatis opsional. | Must | MVP | Simulasi kamera putus menampilkan status error ≤ 3 detik dan tidak membuat aplikasi hang; reconnect sukses mengembalikan pengaturan terakhir. |
| FR-06 | Driver QHYCCD | Driver native QHY. | Should | F2 | Kamera QHY uji lolos set acceptance FR-01 dan FR-04. |
| FR-07 | Driver ASCOM | Driver COM ASCOM dan daftar driver dari registry. | Should | F2 | Simulator ASCOM (OmniSim) terdeteksi dan menampilkan frame; tidak ada daftar driver palsu bila registry kosong. |

### 6.2 Live view & pemrosesan

| ID | Fitur | Deskripsi | Prioritas | Fase | Acceptance Criteria |
|---|---|---|---|---|---|
| FR-08 | Live view | Preview dengan latensi rendah; frame lama dibuang, bukan antre. | Must | MVP | NFR-01 terpenuhi; saat pemrosesan lambat, preview tetap menampilkan frame terbaru. |
| FR-09 | Stretch & histogram | Black/white level manual dan auto (persentil), histogram live per kanal. | Must | MVP | Auto stretch tidak berkedip pada scene statis (variasi kecerahan antar-frame ≤ 2%); histogram diperbarui ≥ 5 Hz. |
| FR-10 | Peningkatan kontras | CLAHE, sharpen, invert pada pipeline float. | Should | MVP | Mengaktifkan/menonaktifkan tidak mengubah data frame yang direkam kecuali pengguna memilih "rekam hasil proses". |
| FR-11 | Kalibrasi | Master dark dan master flat dari file atau dari capture; kalibrasi pada data 16-bit/float. | Must | MVP | Flat dibuat dari frame mentah tanpa gain/WB/flat lama; frame uji sintetis dengan vignetting menjadi rata (deviasi ≤ 2% setelah koreksi). |
| FR-12 | Live stacking | Alignment (translasi), akumulasi float32 dengan penghitung frame, metode mean dan sigma-clip sungguhan; median dan sum sebagai tambahan. | Must (mean, sigma-clip), Should (median, sum) | MVP | Sigma-clip membuang piksel di luar k sigma (uji dengan outlier sintetis); sum tidak dibagi N; batas RAM mengikuti pengaturan dan tidak terlampaui. |
| FR-13 | Pipeline 16-bit | Data tetap 16-bit atau float sampai tahap tampilan. | Must | MVP | Uji gradien 16-bit: histogram data mentah tidak kehilangan level; konversi 8-bit hanya di tahap render. |

### 6.3 Perekaman & data

| ID | Fitur | Deskripsi | Prioritas | Fase | Acceptance Criteria |
|---|---|---|---|---|---|
| FR-14 | Quick capture & sequence | Ambil satu frame atau urutan (jumlah frame, durasi, tanpa batas); format FITS, TIFF 16-bit, PNG 16-bit. | Must | MVP | Sequence 100 frame menghasilkan 100 berkas bernomor berurutan; berhenti tepat pada batas. |
| FR-15 | Metadata FITS | Header berisi DATE-OBS (UTC), EXPTIME, GAIN, CCD-TEMP (terukur), binning, bit depth, INSTRUME, lokasi, objek, versi perangkat lunak. | Must | MVP | Nilai header sama dengan nilai yang dibaca dari kamera pada saat frame; diuji dengan simulator dan ZWO. |
| FR-16 | Rekam video | Rekam AVI/MP4 dengan FPS aktual. | Could | F2 | Durasi video sesuai waktu rekam ±2%. |
| FR-17 | Sesi | Setiap rekaman berada di folder sesi dengan manifest JSON (lokasi, kamera, pengaturan, versi) dan log sesi. | Must | MVP | Manifest valid terhadap skema; folder sesi tetap terbaca bila aplikasi dimatikan paksa (manifest ditulis berkala). |
| FR-18 | Indikator rekam | Tampilkan frame tersimpan, frame hilang, kecepatan tulis, sisa disk dan perkiraan waktu sisa. | Must | MVP | Peringatan muncul saat sisa disk < 2 GB; frame hilang tercatat di log sesi dan manifest, tidak pernah diam-diam. |

### 6.4 Deteksi AI

| ID | Fitur | Deskripsi | Prioritas | Fase | Acceptance Criteria |
|---|---|---|---|---|---|
| FR-19 | Deteksi live | Inferensi ONNX di thread terpisah; hasil digambar sebagai overlay di atas live view. | Must | MVP | NFR-03 terpenuhi; overlay hilang saat AI dimatikan. |
| FR-20 | Log deteksi | CSV per sesi: waktu UTC, kelas, confidence, pusat, bbox, exposure, gain, suhu, nomor frame. | Must | MVP | Setiap deteksi di overlay punya satu baris log; penulisan dikelompokkan (batch) dan tidak dilakukan per frame di thread UI. |
| FR-21 | Threshold | Confidence threshold dapat diubah saat berjalan. | Must | MVP | Perubahan berlaku pada inferensi berikutnya tanpa restart. |
| FR-22 | Rekaman bersih | Overlay dan anotasi tidak pernah tertulis ke frame yang direkam. | Must | MVP | Selisih piksel antara frame rekaman dan frame mentah kamera = 0 saat AI aktif. |

### 6.5 Astronomi

| ID | Fitur | Deskripsi | Prioritas | Fase | Acceptance Criteria |
|---|---|---|---|---|---|
| FR-23 | Efemeris realtime | Alt/Az bulan dan matahari (topocentric), elongasi, iluminasi, umur bulan, ijtima sebelum dan sesudah. | Must | MVP | G-08 terpenuhi terhadap tabel referensi. |
| FR-24 | Waktu Maghrib & kondisi hilal | Hitung sunset, moonset, selisih waktu, dan kondisi hilal (alt, elongasi, umur) pada saat sunset serta saat terbaik pengamatan. | Must | MVP | Hasil sunset/moonset sama dengan referensi dalam ±1 menit untuk 10 tanggal uji. |
| FR-25 | Kriteria visibilitas | Kriteria sebagai tabel data: MABIMS baru (alt ≥ 3°, elongasi ≥ 6,4°), BRIN, Odeh; tampil nilai ambang dan hasil. | Must (MABIMS), Should (BRIN, Odeh) | MVP | Kriteria Odeh memakai rumus dari publikasi aslinya, diverifikasi ahli falak sebelum dirilis (lihat Open Questions). |
| FR-26 | Kalender Hijriyah | Tanggal Hijriyah dengan metode yang dapat dipilih dan dicatat dalam manifest. | Should | MVP | Metode kalender tertulis di panel dan manifest; hari berganti pada Maghrib bila opsi diaktifkan. |
| FR-27 | Lokasi & zona waktu | Preset dan lokasi kustom (lintang, bujur, elevasi), toggle refraksi, zona waktu yang dipilih pengguna. | Must | MVP | Zona waktu tidak diambil diam-diam dari sistem tanpa tampil di UI; perubahan lokasi memperbarui seluruh panel ≤ 1 detik. |

### 6.6 Aplikasi & pengaturan

| ID | Fitur | Deskripsi | Prioritas | Fase | Acceptance Criteria |
|---|---|---|---|---|---|
| FR-28 | Tema | Dark, Night Red, Light; seluruh elemen mengikuti token tema. | Must | MVP | Tidak ada elemen putih terang pada tema Night Red; semua tema lolos pemeriksaan kontras NFR-12. |
| FR-29 | Bahasa | Indonesia dan Inggris lengkap (menu, dialog, log, pesan error). | Must | MVP | Tidak ada string hardcode di luar sistem terjemahan (diperiksa otomatis). |
| FR-30 | Profil capture | Profil Hilal Siang, Hilal Senja, dan kustom yang menerapkan exposure/gain/stretch ke hardware. | Must | MVP | Menerapkan profil mengubah nilai yang terbaca dari kamera, bukan hanya slider. |
| FR-31 | Migrasi pengaturan | Impor settings.json Onyx OS ke format versi 2; key yang tidak dipakai dibuang dan dilaporkan. | Should | MVP | File Onyx uji menghasilkan settings v2 valid; laporan migrasi mencantumkan key yang dibuang. |
| FR-32 | Cegah tidur | Cegah sleep/layar mati selama sesi aktif. | Must | MVP | Laptop tidak tidur selama sesi 60 menit pada setelan daya default (uji manual); dilepas saat sesi selesai. |
| FR-33 | Paket diagnostik | Ekspor ZIP berisi log, manifest, pengaturan (lokasi dapat disamarkan), daftar perangkat, dan info sistem. | Should | MVP | ZIP dibuat ≤ 10 detik; tidak berisi frame gambar kecuali dipilih. |
| FR-34 | Penjelajah sesi | Buka sesi lama: daftar frame, deteksi, dan ekspor ringkasan. Laporan PDF sebagai tambahan. | Should (penjelajah), Could (PDF) | F2 | Sesi uji 1.000 frame terbuka ≤ 3 detik. |
| FR-35 | Panel waktu | Tampilkan UTC dan waktu lokal; status sinkronisasi jam dari `w32tm`; peringatan bila sinkron terakhir > 24 jam. | Should | MVP | Peringatan muncul pada jam yang sengaja digeser (uji manual). |
| FR-36 | Tanpa Administrator | Operasi normal (kamera ZWO, UVC, simulator, rekam, AI) tidak memerlukan hak Administrator. | Must | MVP | Seluruh skenario uji lapangan dijalankan dengan akun standar. Jika ada driver yang memerlukannya, tercatat dan dijelaskan di dokumentasi. |

## 7. Non-Functional Requirements

| ID | Kategori | Requirement | Cara uji |
|---|---|---|---|
| NFR-01 | Performa | Latensi sensor ke layar p95 ≤ 100 ms pada 1280x720; ≤ 150 ms pada 1920x1080 (ASUMSI). | Benchmark dengan timestamp frame dan penanda layar. |
| NFR-02 | Performa | Waktu frame UI p95 ≤ 16 ms saat live view + overlay + histogram. | Profil bawaan (mode `--perf`). |
| NFR-03 | Performa | AI berjalan ≥ 3 inferensi/detik pada input 1280x720 di CPU laptop referensi; penurunan FPS preview ≤ 5%. | Benchmark dengan AI on/off. |
| NFR-04 | Performa | Startup dingin ≤ 5 detik sampai jendela interaktif. | Stopwatch otomatis, 10 kali, ambil p95. |
| NFR-05 | Sumber daya | Ring buffer dan stacking dibatasi pengaturan (default: ring 512 MB, stacking 2048 MB). | Uji dengan batas rendah; aplikasi tidak melewati batas. |
| NFR-06 | Keandalan | Frame yang dibuang karena beban selalu dihitung dan dilaporkan. | Uji beban buatan; hitungan = selisih kirim dan tulis. |
| NFR-07 | Keandalan | Mati daya atau kill proses tidak merusak berkas sesi sebelumnya; berkas FITS terakhir mungkin tidak lengkap tetapi terdeteksi. | Uji kill -9 saat rekam. |
| NFR-08 | Operasional | Berfungsi penuh tanpa internet; tidak ada koneksi jaringan keluar secara default. | Pemantauan jaringan saat uji; firewall blok total. |
| NFR-09 | Distribusi | Tersedia sebagai ZIP portabel; tidak memerlukan runtime Python atau Visual C++ redistributable terpisah (dibundel). | Uji di Windows bersih (VM). |
| NFR-10 | Kompatibilitas | Windows 10 22H2 dan Windows 11, x64. | Matriks uji di QA. |
| NFR-11 | Tampilan | Skala DPI 100% sampai 200%; jendela minimum 1120x720. | Uji pada tiga resolusi dan skala. |
| NFR-12 | Aksesibilitas | Kontras teks minimal 4,5:1 pada tema Dark dan Light; pada Night Red minimal 4,5:1 dalam palet merah; semua fungsi inti dapat dijangkau keyboard; target klik ≥ 40x40 px. | Pemeriksaan kontras otomatis dari token; uji keyboard manual. |
| NFR-13 | Keamanan | Binari dan ZIP rilis ditandatangani; checksum dipublikasikan; tidak ada pembaruan otomatis tanpa persetujuan pengguna. Rincian di 09b-Security-Plan.md. | Verifikasi tanda tangan di pipeline. |
| NFR-14 | Kemudahan pelihara | Cakupan baris kode modul `core`, `imaging`, `astro` ≥ 80% (ASUMSI); semua pesan terjemahan lewat sistem i18n. | Laporan coverage CI. |
| NFR-15 | Daya | CPU idle (tanpa kamera) < 2%. | Pengukuran 5 menit. |

## 8. Di Luar Cakupan (Out of Scope)

- Kontrol mount dan goto/tracking otomatis (kandidat fase 3).
- Plate solving dan identifikasi bintang.
- Penetapan atau pengesahan resmi awal bulan; Sabit hanya alat bantu pengamatan dan pencatatan.
- Platform selain Windows x64 (macOS, Linux, mobile).
- Sinkronisasi cloud, multi-pengguna, dan streaming jarak jauh.
- Pelatihan ulang model di dalam aplikasi (pelatihan tetap di luar, memakai log dan frame mentah dari Sabit).
- SDK kamera DSLR Canon/Nikon. Onyx OS hanya mendukung capture card UVC yang dinamai "DSLR"; Sabit memperlakukannya sebagai UVC.

## 9. Asumsi, Dependensi & Risiko

**Asumsi**
- A-01: Tim pengembang 2 orang (1 platform/kamera/UI, 1 imaging/AI/astro), tersedia 4 person-day per minggu masing-masing; 1 ahli falak sebagai penasihat paruh waktu.
- A-02: Kamera uji ZWO tersedia sejak awal November 2026; model kamera dan resolusi belum diketahui.
- A-03: SDK ZWO dan QHY boleh disertakan dalam distribusi sesuai lisensi vendor (belum diverifikasi).
- A-04: Model hilal Onyx (`onyx_hilal_v1.pt`) boleh dipakai dan diekspor ke ONNX; sumber dataset dan lisensinya belum diketahui.
- A-05: Laptop lapangan memenuhi spesifikasi referensi pada bagian 3.

**Dependensi**
- SDK kamera ZWO (QHY di fase 2), Qt 6, OpenCV, ONNX Runtime, CFITSIO, pustaka efemeris (kandidat: Astronomy Engine; lisensi dan akurasi diverifikasi di M2).
- Penasihat falak untuk rumus kriteria dan verifikasi hasil.
- Jadwal ijtima: kesempatan uji lapangan nyata hanya sebulan sekali.

**Risiko**

| ID | Risiko | Probabilitas | Dampak | Mitigasi |
|---|---|---|---|---|
| R-01 | Perilaku SDK ZWO berbeda dari dugaan (timeout, bandwidth USB) | Sedang | Tinggi | Spike driver di sprint 2; uji pada laptop target; fallback ke mode bandwidth rendah |
| R-02 | Model ONNX berbeda hasil dari .pt | Sedang | Tinggi | Uji paritas (G-05) di sprint 5; tetap sediakan jalur ekspor dengan operator yang diketahui didukung |
| R-03 | Lisensi model/pelatihan membatasi distribusi (pustaka pelatihan berlisensi AGPL-3.0) | Sedang | Tinggi | Tinjauan hukum sebelum rilis; pelatihan tetap terpisah dari aplikasi; hanya runtime ONNX yang didistribusikan |
| R-04 | Jadwal MVP mepet terhadap malam rukyat 23-11-2026 | Tinggi | Tinggi | Scope MVP dikunci (04-Timeline); QHY/ASCOM/video ditunda ke F2; sprint 7 sebagai buffer |
| R-05 | Rumus kriteria (terutama Odeh) salah tafsir | Sedang | Tinggi | Verifikasi tertulis penasihat falak; golden test dari contoh dalam publikasi |
| R-06 | Laptop lapangan lebih lemah dari referensi | Sedang | Sedang | Mode ringan (turunkan frekuensi AI, resolusi preview); uji pada laptop paling lemah yang tersedia |
| R-07 | Kebutuhan hak Administrator ternyata disebabkan driver USB | Sedang | Sedang | Selidiki penyebab di sprint 2; dokumentasikan driver yang benar |
| R-08 | Sedikit kesempatan uji lapangan | Tinggi | Sedang | Gunakan bulan sabit muda biasa dan rekaman Onyx lama sebagai data uji tambahan |

## 10. Rilis & Fase

| Fase | Isi | Target |
|---|---|---|
| **MVP 1.0** | FR-01 sampai FR-05, FR-08 sampai FR-15, FR-17 sampai FR-33, FR-35, FR-36 (ZWO, UVC, simulator; AI ONNX; efemeris; kalibrasi; stacking; FITS) | RC 23-11-2026; pemakaian lapangan 23-11-2026 |
| **Fase 2 (v1.1)** | FR-06 QHY, FR-07 ASCOM, FR-16 video, FR-34 penjelajah sesi dan laporan PDF | Maret sampai April 2027 |
| **Fase 3** | Kontrol mount (ASCOM/Alpaca), perbaikan model, profil kamera tambahan | Ditentukan setelah evaluasi lapangan |

Catatan: tanggal malam rukyat 23-11-2026 adalah ASUMSI dari perhitungan perangkat sendiri untuk lokasi Jakarta; penetapan resmi oleh otoritas.

## 11. Open Questions

1. Nama final aplikasi dan identitas visual: tetap "Sabit"?
2. Model kamera ZWO yang akan dipakai, dan apakah ada kamera QHY yang wajib didukung di MVP?
3. Asal dataset dan lisensi `onyx_hilal_v1.pt`; metrik (mAP, recall) pada set uji; daftar kelas.
4. Spesifikasi laptop lapangan yang sebenarnya.
5. Kriteria mana yang wajib ada selain MABIMS baru? Rujukan resmi rumus Odeh dan BRIN yang dipakai.
6. Metode kalender Hijriyah yang diharapkan (Umm al-Qura, hisab Indonesia, atau pilihan)?
7. Siapa pemilik sertifikat tanda tangan kode, dan apakah dibeli atau memakai skema gratis untuk proyek terbuka?
8. Lisensi proyek (proprietary, MIT, GPL)? Ini memengaruhi pilihan Qt (LGPL) dan pustaka lain.
9. Boleh atau tidak SDK ZWO/QHY disertakan di ZIP rilis (redistribusi)?
10. Penyebab sebenarnya Onyx OS meminta Run as Administrator.
11. Apakah teleskop/mount yang dipakai perlu dikontrol dari aplikasi (menentukan prioritas fase 3)?

## 12. Lampiran

**Glosarium**
- **Hilal**: bulan sabit muda pertama setelah ijtima.
- **Ijtima**: konjungsi bulan dan matahari (bulan baru astronomis).
- **Rukyat**: pengamatan hilal.
- **Elongasi**: jarak sudut bulan-matahari.
- **Alt/Az**: tinggi dan azimut.
- **MABIMS**: kriteria visibilitas hilal bersama Brunei, Indonesia, Malaysia, Singapura.
- **FITS**: format berkas standar data astronomi.
- **Stacking**: penggabungan banyak frame untuk meningkatkan rasio sinyal-derau.
- **Dark/Flat**: frame kalibrasi derau sensor dan ketidakrataan medan.
- **ONNX**: format model yang dijalankan oleh ONNX Runtime.
- **Ring buffer**: antrean melingkar berukuran tetap; saat penuh, frame terlama dibuang.

**Referensi internal**
- Hasil bedah Onyx OS (sesi analisis ini).
- 02-Technical-Specification.md, 03-User-Flow-Wireframe.md, 04-Project-Timeline-Milestones.md, 05-Testing-QA-Checklist.md, 09b-Security-Plan.md.
