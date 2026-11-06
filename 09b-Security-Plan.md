# Security Plan (Internal): Sabit

Versi dokumen: 0.1 (draf) | Tanggal: 23-10-2026 | Klasifikasi dokumen: internal
Rujukan: 02-Technical-Specification.md (bagian 8), 06-Deployment-Maintenance-Plan.md, 09a-SECURITY.md.

Jenis aplikasi: desktop Windows, offline, tanpa akun. Stack: C++20, Qt 6, OpenCV, ONNX Runtime, CFITSIO, SDK kamera vendor.
Hal yang perlu diverifikasi manual ditandai "ASUMSI:" atau "VERIFIKASI:".

---

## 1. Aset & Klasifikasi Data

| Aset | Klasifikasi | Alasan | Lokasi |
|---|---|---|---|
| Binari Sabit dan DLL pendamping | Integritas tinggi | Dijalankan di laptop lapangan; kompromi berarti eksekusi kode | ZIP rilis, folder aplikasi |
| Model ONNX (`models/*.onnx`) | Integritas tinggi | Model yang dimanipulasi dapat menghasilkan deteksi palsu atau memicu bug parser | Folder aplikasi |
| Data pengamatan (FITS, CSV, manifest) | Integritas tinggi, kerahasiaan rendah sampai sedang | Dipakai sebagai bukti pengamatan; dapat memuat lokasi dan nama pengamat | Folder sesi |
| Nama pengamat dan koordinat lokasi | Data pribadi (ASUMSI: dapat tergolong data pribadi menurut UU PDP; VERIFIKASI dengan penasihat hukum) | Mengidentifikasi orang dan tempat | `session.json`, `settings.json`, paket diagnostik |
| Pengaturan (`settings.json`, profil) | Rendah | Konfigurasi pengguna | `%LOCALAPPDATA%\Sabit\` |
| Kunci/sertifikat tanda tangan kode | Rahasia, kritis | Memalsukan rilis jika bocor | Rahasia CI atau token perangkat keras |
| Akses repositori dan CI | Rahasia | Mengubah kode dan rilis | Platform hosting |
| Log (`app.log`, `session.log`) | Rendah sampai sedang | Dapat memuat path dan lokasi | Folder log dan sesi |

Aturan klasifikasi:
- Integritas lebih penting daripada kerahasiaan: kesalahan data pengamatan lebih merugikan daripada kebocorannya.
- Aplikasi tidak menyimpan kredensial, token, atau rahasia pengguna.
- Tidak ada data yang dikirim keluar; pengiriman paket diagnostik adalah tindakan manual oleh pengguna.

## 2. Threat Model (STRIDE)

Batas kepercayaan: (1) sistem berkas (berkas masukan yang tidak tepercaya), (2) driver dan SDK vendor (kode pihak ketiga dalam proses), (3) rantai build dan distribusi.

| ID | Kategori | Ancaman | Komponen | Dampak | Kontrol |
|---|---|---|---|---|---|
| TH-01 | Tampering | DLL hijacking: DLL berbahaya di CWD/PATH dimuat | Pemuatan pustaka | Eksekusi kode dengan hak pengguna | `SetDefaultDllDirectories`, hapus CWD dari pencarian DLL, tidak membundel DLL sistem, tanda tangan DLL vendor diverifikasi (VERIFIKASI jika vendor menandatangani) |
| TH-02 | Tampering | Berkas FITS/TIFF/PNG berbahaya memicu overflow/pembacaan di luar batas | Parser/impor kalibrasi | Crash atau eksekusi kode | Batas dimensi dan ukuran, validasi header, fuzzing, ASan di CI, pustaka diperbarui |
| TH-03 | Tampering | Model ONNX dimanipulasi (deteksi palsu atau bug parser) | `detect` | Hasil menyesatkan, crash | Hash SHA-256 diverifikasi terhadap `models/manifest.json`; model di luar manifest butuh konfirmasi eksplisit; pustaka ONNX Runtime diperbarui |
| TH-04 | Spoofing | Rilis palsu (ZIP dimodifikasi) | Distribusi | Malware di laptop lapangan | Tanda tangan kode, `SHA256SUMS.txt`, instruksi verifikasi di README, halaman rilis resmi |
| TH-05 | Tampering | Kompromi rantai pasok (dependensi vcpkg, aksi CI) | Build | Binari berbahaya | Pin versi/baseline vcpkg, pin aksi CI ke SHA, SBOM, SCA, review perubahan dependensi |
| TH-06 | Elevation of privilege | Aplikasi meminta Administrator (seperti Onyx OS) lalu dieksploitasi | Aplikasi | Kontrol penuh mesin | Target FR-36: tidak butuh Administrator; manifest tidak meminta elevasi |
| TH-07 | Repudiation | Data pengamatan diubah setelah sesi tanpa jejak | Data sesi | Bukti tidak dapat dipercaya | Manifest dengan counters; checksum arsip (SHA256) per sesi; versi aplikasi dan metode tercatat |
| TH-08 | Information disclosure | Paket diagnostik atau log membocorkan lokasi/nama | Diagnostik, log | Privasi pengamat | Opsi menyamarkan koordinat, dialog memperlihatkan isi sebelum ekspor, tidak menyertakan frame secara default |
| TH-09 | Information disclosure | Rahasia ikut ke repositori/binari/log CI | Repositori, CI | Kompromi sertifikat | Secret scanning, rahasia hanya pada job rilis, tidak ada rahasia dalam aplikasi |
| TH-10 | Denial of service | Berkas sangat besar atau frame rusak membuat aplikasi hang | Impor, pipeline | Pengamatan terganggu | Batas ukuran, timeout operasi hardware, thread tidak menunggu tanpa batas |
| TH-11 | Tampering | Driver/SDK vendor berisi kerentanan | Driver | Kompromi proses | Pembaruan SDK terjadwal, isolasi driver di thread kamera, kebijakan kesalahan yang aman (tidak menjalankan input tidak tepercaya melalui SDK) |
| TH-12 | Tampering | Pengaturan atau profil JSON berbahaya (nilai ekstrem) | Settings | Crash, perilaku tidak aman | Validasi skema dan rentang; pemulihan default |
| TH-13 | Spoofing | Kamera simulator dipakai sebagai kamera nyata (data palsu) | Kamera | Data sintetis dianggap pengamatan | Manifest mencatat driver; sesi simulator diberi tanda eksplisit di manifest dan nama folder |
| TH-14 | Repudiation | Jam sistem salah sehingga timestamp tidak valid | Clock | Bukti waktu tidak andal | Panel waktu, status sinkron di manifest, peringatan > 24 jam |

## 3. Autentikasi & Otorisasi

Sabit tidak memiliki akun, kata sandi, sesi, atau peran; tidak berlaku: hashing kata sandi, MFA, RBAC, rate limiting. Kontrol yang berlaku:

- **Otorisasi bersandar pada OS:** Sabit berjalan sebagai pengguna Windows biasa. Berkas pengaturan di `%LOCALAPPDATA%` mewarisi ACL pengguna.
- **Tanpa elevasi:** manifest aplikasi memakai `asInvoker`; aplikasi tidak meminta Administrator.
- **Integritas rilis (padanan autentikasi):** tanda tangan kode Authenticode pada `Sabit.exe` (dan DLL yang dibangun sendiri) serta `SHA256SUMS.txt`.
- **Akses ke repositori dan CI:** MFA wajib untuk semua anggota; hak minimum; perlindungan branch `main` (review wajib, status CI wajib); kunci akses pribadi dengan masa berlaku.
- **Sertifikat tanda tangan:** hanya tersedia untuk job rilis pada tag oleh pemelihara tertentu; rotasi dan pengingat kedaluwarsa (lihat 06-Deployment bagian 13).

## 4. Perlindungan Data

| Aspek | Kontrol |
|---|---|
| Enkripsi in transit | Tidak ada transmisi data; aplikasi tidak membuka koneksi. Jika fitur jaringan ditambahkan nanti (misalnya pemeriksaan versi), wajib TLS dengan validasi sertifikat dan persetujuan eksplisit pengguna |
| Enkripsi at rest | Data sesi tidak dienkripsi secara default (kebutuhan: dibuka alat lain). Saran dokumentasi: aktifkan BitLocker pada laptop lapangan |
| Manajemen secret | Tidak ada secret dalam aplikasi; secret CI dalam penyimpanan rahasia platform; tidak dicetak di log |
| Retensi | Log aplikasi dirotasi (10 x 5 MB); data sesi dikelola pengguna; paket diagnostik tidak disimpan oleh aplikasi |
| Masking log | Tidak menulis nama pengamat penuh di `app.log`; koordinat tidak dicatat di `app.log`; opsi penyamaran koordinat saat ekspor diagnostik (dibulatkan ke 1 desimal atau dihapus) |
| Privasi | Aplikasi tidak melakukan telemetri. Nama pengamat opsional. Daftar data pribadi dan lokasi penyimpanannya dicantumkan di dokumentasi pengguna. ASUMSI: perlu tinjauan hukum terhadap UU PDP |

## 5. Secure Coding Checklist

Disesuaikan untuk aplikasi desktop C++ (CSRF, CORS, dan header keamanan web tidak berlaku).

- [ ] Semua masukan tidak tepercaya (berkas, JSON, model, nama sesi) divalidasi: panjang, rentang, tipe, dimensi.
- [ ] Batas ukuran berkas impor (ASUMSI: 512 MB) dan batas dimensi gambar sebelum alokasi memori.
- [ ] Aritmetika ukuran (lebar x tinggi x kanal x byte) diperiksa terhadap overflow sebelum alokasi.
- [ ] Tidak ada `strcpy`, `sprintf`, `gets`; gunakan `std::string`, `std::span`, `fmt`/`std::format`.
- [ ] RAII untuk semua handle (SDK, file, thread); tidak ada `new`/`delete` mentah di jalur panas.
- [ ] Tidak ada format string dari masukan pengguna.
- [ ] Nama berkas dari pengguna dinormalisasi; path traversal dicegah (`..`, drive/UNC tidak diharapkan); penulisan hanya di folder sesi.
- [ ] Penulisan berkas atomik (berkas sementara lalu rename); izin default folder pengguna.
- [ ] JSON diparse dengan batas kedalaman/ukuran; key tidak dikenal dipertahankan, bukan dieksekusi.
- [ ] Tidak ada eksekusi perintah shell dengan masukan pengguna (`system`, `ShellExecute` dengan argumen dari pengguna dilarang; membuka folder memakai API yang menerima path tervalidasi).
- [ ] Pustaka dimuat hanya dari folder aplikasi (lihat contoh di bawah).
- [ ] Model ONNX hanya dari `models/` dengan hash cocok.
- [ ] Output ke UI di-escape sesuai konteks (teks biasa, bukan HTML dari masukan).
- [ ] Tidak ada koneksi jaringan; pemeriksaan di CI (uji lalu lintas pada simulator).
- [ ] Unggah/upload file: tidak berlaku (tidak ada server); impor lokal mengikuti poin validasi di atas.
- [ ] Kesalahan dilaporkan dengan aman: pesan galat tidak membocorkan path sensitif di luar kebutuhan; crash dump lokal saja.

Contoh konfigurasi/kode aman untuk poin berisiko tinggi:

**a. Pengerasan pencarian DLL (panggil di awal `main`, sebelum memuat pustaka apa pun)**

```cpp
#include <windows.h>

static void harden_dll_search() {
    // Hapus direktori kerja saat ini dari pencarian DLL.
    SetDllDirectoryW(L"");
    // Hanya aplikasi dan System32.
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                             LOAD_LIBRARY_SEARCH_SYSTEM32);
}
```

Untuk DLL vendor yang dimuat dinamis, gunakan path absolut ke folder aplikasi dan `LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR)`. VERIFIKASI: perilaku `SetDefaultDllDirectories` terhadap DLL yang dimuat statis (import table) pada versi Windows yang didukung; uji dengan DLL palsu di CWD.

**b. Verifikasi hash model sebelum dimuat**

```cpp
bool load_model_verified(const std::filesystem::path& model,
                         const ModelManifest& manifest) {
    const auto actual = sha256_file(model);               // fungsi util yang diuji
    const auto it = manifest.find(model.filename().string());
    if (it == manifest.end() || !constant_time_equal(it->second.sha256, actual))
        return false;                                     // tolak; UI meminta konfirmasi eksplisit
    return onnx_session_.load(model);
}
```

**c. Pemeriksaan ukuran sebelum alokasi (anti-overflow)**

```cpp
std::optional<size_t> frame_bytes(uint32_t w, uint32_t h, uint32_t ch, uint32_t bytes_per_px) {
    constexpr uint64_t kMax = 1ull << 31;                 // 2 GiB (ASUMSI)
    const uint64_t n = uint64_t(w) * h * ch * bytes_per_px;
    if (w == 0 || h == 0 || w > 32768 || h > 32768 || n > kMax) return std::nullopt;
    return static_cast<size_t>(n);
}
```

**d. Pengerasan kompilasi (MSVC, melalui CMake)**

```cmake
if (MSVC)
  target_compile_options(sabit PRIVATE /sdl /GS /guard:cf /W4 /permissive-)
  target_link_options(sabit PRIVATE /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT /guard:cf)
endif()
```

**e. Manifest aplikasi (tanpa elevasi)**

```xml
<requestedExecutionLevel level="asInvoker" uiAccess="false"/>
```

## 6. Checklist OWASP Top 10

Mengacu pada OWASP Top 10 (2021), dipetakan ke konteks aplikasi desktop offline. Status: **Berlaku/Sebagian/Tidak berlaku**, dan kondisi saat ini (draf; belum ada kode).

| Item | Relevansi ke Sabit | Kontrol | Status |
|---|---|---|---|
| A01 Broken Access Control | Sebagian: akses berkas/ACL, hak Administrator | `asInvoker`, folder pengguna, validasi path | Rencana |
| A02 Cryptographic Failures | Sebagian: integritas (SHA-256, tanda tangan); tidak ada enkripsi data bawaan | SHA-256 untuk model dan rilis, Authenticode, saran BitLocker | Rencana |
| A03 Injection | Sebagian: path/argumen, JSON, parser berkas | Validasi, tanpa shell, parser dengan batas, fuzzing | Rencana |
| A04 Insecure Design | Berlaku | Threat model (bagian 2), review desain | Dalam penyusunan |
| A05 Security Misconfiguration | Berlaku: pencarian DLL, flag kompilasi | Bagian 5 a, d, e | Rencana |
| A06 Vulnerable and Outdated Components | Berlaku: Qt, OpenCV, ONNX Runtime, CFITSIO, SDK | SCA, jadwal pembaruan, SBOM | Rencana |
| A07 Identification and Authentication Failures | Tidak berlaku (tanpa akun); padanannya akses repo/CI dengan MFA | Bagian 3 | Rencana |
| A08 Software and Data Integrity Failures | Berlaku: rilis, model, CI | Tanda tangan, hash model, pin aksi CI, review PR | Rencana |
| A09 Security Logging and Monitoring Failures | Sebagian: log lokal | Bagian 10 | Rencana |
| A10 Server-Side Request Forgery | Tidak berlaku (tanpa jaringan) | Uji bahwa tidak ada koneksi keluar | Rencana |

## 7. Keamanan Dependensi & Supply Chain

- **Pin versi:** vcpkg dengan baseline commit tetap (`vcpkg.json` + `vcpkg-configuration.json`); aksi CI di-pin ke commit SHA; model dan SDK dengan hash.
- **SBOM:** dihasilkan tiap rilis (ASUMSI: SPDX dari vcpkg atau CycloneDX dari alat lain; VERIFIKASI kemampuan alat pada versi yang dipakai). Disertakan di halaman rilis.
- **SCA/audit:** pemeriksaan bulanan terhadap advisori untuk Qt, OpenCV, ONNX Runtime, CFITSIO, dan pustaka efemeris; alat otomatis (ASUMSI: OSV-Scanner atau setara; VERIFIKASI cakupan untuk paket C/C++).
- **Pembaruan:** patch keamanan diprioritaskan; kenaikan versi besar melalui regresi penuh.
- **Peninjauan dependensi baru:** setiap dependensi baru butuh persetujuan pemelihara (lisensi, aktivitas pemeliharaan, riwayat kerentanan, ukuran).
- **SDK vendor:** versi dicatat; hash DLL dicatat di manifest rilis; pembaruan SDK diuji sebelum dibundel.
- **Alat ekspor model (Python):** hanya di mesin pengembang; `tools/requirements.txt` dengan versi dan hash terkunci; model hasil ekspor di-commit dengan model card (sumber data, versi, hash).
- **Lisensi:** pelacakan lisensi dependensi (Qt LGPL, pustaka pelatihan AGPL hanya di luar aplikasi) agar kewajiban terpenuhi.

## 8. Keamanan Infrastruktur

Tidak ada server. Infrastruktur yang ada: laptop lapangan, mesin developer, platform repositori/CI.

- **Laptop lapangan:** akun standar (bukan Administrator), BitLocker (saran), pembaruan Windows terjadwal (di luar jendela beku rilis), antivirus aktif dengan pengecualian terbatas pada folder aplikasi bila perlu, USB untuk rilis hanya dari sumber terverifikasi (checksum).
- **Jaringan:** Sabit tidak butuh jaringan; pada lokasi lapangan, jaringan dapat dimatikan. Firewall Windows: tidak perlu aturan masuk.
- **Mesin developer:** disk terenkripsi, MFA pada akun, tidak menyimpan sertifikat tanda tangan di laptop pribadi.
- **Least privilege:** akses repositori sesuai peran; hak merge dibatasi; akses sertifikat hanya pemelihara rilis.
- **Hardening:** minimalkan DLL terbundel; nonaktifkan fitur Qt yang tidak dipakai (jaringan, QML, WebEngine) pada build.

## 9. Keamanan CI/CD

| Kontrol | Detail |
|---|---|
| Secret scanning | `gitleaks` atau fitur bawaan platform pada setiap PR dan push |
| SAST | CodeQL untuk C++ (ASUMSI: tersedia untuk repositori) atau alat setara; analisis statis kompilator (`/analyze` MSVC) |
| Sanitizer | ASan/UBSan pada build uji |
| DAST | Tidak berlaku (tidak ada layanan jaringan); diganti fuzzing parser |
| Hak minimum workflow | `permissions: contents: read` default; `contents: write` hanya pada job rilis |
| PR dari fork | Tidak mendapat rahasia; tidak menjalankan job rilis |
| Pin aksi | Semua aksi di-pin ke commit SHA; pembaruan melalui PR terjadwal |
| Perlindungan branch | Review wajib minimal 1 orang, status CI wajib, tanpa force-push ke `main`, tag rilis dilindungi |
| Artifact bertanda tangan | `Sabit.exe` Authenticode; ZIP + `SHA256SUMS.txt`; opsional attestation provenance build (VERIFIKASI dukungan platform) |
| Isolasi | Runner efemeral; penandatanganan hanya setelah semua uji lulus; sertifikat dihapus dari disk setelah dipakai |
| Audit | Log CI disimpan; perubahan konfigurasi workflow ditinjau seperti kode |

## 10. Logging, Monitoring & Deteksi

- **Yang dicatat:** koneksi/putus kamera, perubahan pengaturan perangkat keras (nilai diminta dan terbaca), penolakan model karena hash tidak cocok, kegagalan validasi berkas, kegagalan pemuatan pustaka, drop frame, crash.
- **Yang tidak dicatat:** nama pengamat penuh di `app.log`, koordinat di `app.log`, rahasia apa pun.
- **Deteksi lokal:** peringatan di UI untuk model tak dikenal, berkas ditolak, jam tidak sinkron; log dapat ditinjau pasca-sesi.
- **Deteksi eksternal:** pemantauan advisori keamanan untuk dependensi (bulanan); pelaporan kerentanan lewat SECURITY.md; pemantauan kedaluwarsa sertifikat.
- **Crash dump:** lokal saja; pengiriman manual oleh pengguna; dihapus setelah dianalisis.
- **Integritas rilis:** pengguna diajarkan memverifikasi checksum dan tanda tangan (README).

## 11. Incident Response Plan

Jenis insiden keamanan yang diantisipasi: (a) rilis atau sertifikat tanda tangan terkompromi, (b) kerentanan kritis dilaporkan atau dieksploitasi, (c) kebocoran data pribadi pengamat (misalnya paket diagnostik dikirim ke pihak yang salah), (d) kompromi akun repositori/CI.

| Fase | Tindakan | Penanggung jawab |
|---|---|---|
| Deteksi | Laporan via SECURITY.md, advisori hulu, anomali CI, laporan pengguna. Catat waktu, sumber, dugaan dampak | Penerima laporan |
| Triase | Tentukan severity (Critical/High/Medium/Low), versi terdampak, apakah aktif dieksploitasi. Bentuk tim respons (pemelihara rilis, pemilik produk, penasihat hukum bila data pribadi) | Pemilik produk |
| Containment | Tarik atau tandai rilis bermasalah di halaman rilis; cabut akses/rahasia yang terkompromi; bekukan rilis; kasus sertifikat: ajukan pencabutan ke penerbit dan hentikan penggunaan | Pemelihara rilis |
| Eradikasi | Perbaiki akar masalah; bersihkan dependensi/akun terkompromi; bangun ulang dari sumber bersih di runner baru; tambah uji regresi | Developer |
| Recovery | Rilis versi perbaikan bertanda tangan (sertifikat baru bila perlu); publikasikan checksum baru; panduan pengguna untuk mengganti versi dan memverifikasi; uji pada laptop lapangan | Pemelihara rilis |
| Komunikasi | Advisori keamanan di repositori (dampak, versi, perbaikan, langkah pengguna); beri tahu pelapor; jika data pribadi terdampak, pemberitahuan sesuai kewajiban hukum yang berlaku (VERIFIKASI ketentuan UU PDP dengan penasihat hukum) | Pemilik produk |
| Post-mortem | Dalam 5 hari kerja; tanpa menyalahkan individu; tindakan korektif dengan owner dan tenggat | Seluruh tim |

Catatan jendela pengamatan: bila insiden terjadi dalam 7 hari sebelum malam rukyat, prioritas pertama adalah memastikan jalur cadangan (versi sebelumnya yang terverifikasi atau Onyx OS) tersedia, baru memperbaiki.

Template ringkas post-mortem keamanan: lihat 06-Deployment bagian 14 (tambahkan bagian "Data apa yang terdampak" dan "Siapa yang harus diberi tahu").

## 12. Pengujian Keamanan

| Aktivitas | Cakupan | Jadwal | Kriteria lulus |
|---|---|---|---|
| Fuzzing parser | FITS, TIFF, PNG, JSON pengaturan/manifest, loader model (libFuzzer/AFL++; ASUMSI pilihan alat) | CI malam (durasi tetap) sejak M5; sesi panjang sebelum RC | Tidak ada crash/ASan error pada korpus selama durasi yang ditentukan; temuan menjadi tes regresi |
| Uji DLL hijacking | DLL palsu di CWD dan PATH untuk setiap DLL yang dimuat | Sebelum RC (M7) | Tidak ada DLL palsu yang dimuat |
| Verifikasi rilis | Tanda tangan, checksum, SBOM | Setiap rilis | Semua valid; SBOM sesuai isi ZIP |
| Uji jaringan | Pemantauan lalu lintas saat sesi penuh pada simulator | Setiap RC | Tidak ada koneksi keluar |
| Peninjauan kode keamanan | Modul impor berkas, pemuatan model/pustaka, penulis berkas | Sebelum RC | Temuan High/Critical ditutup |
| SCA dan pembaruan dependensi | Seluruh `vcpkg.json` + SDK | Bulanan dan sebelum RC | Tidak ada kerentanan Critical/High yang diketahui dan dapat dieksploitasi tanpa mitigasi |
| Uji privasi paket diagnostik | Isi paket dengan dan tanpa penyamaran | Sebelum RC | Koordinat tersamarkan sesuai opsi; tidak ada frame tanpa pilihan |
| Pentest eksternal (opsional) | Aplikasi dan rantai rilis | Setelah v1.0 stabil, tahunan bila anggaran tersedia | Tidak ada temuan High/Critical terbuka |

Kriteria lulus rilis: tidak ada temuan keamanan High/Critical terbuka pada RC; temuan Medium memiliki rencana dan owner.

## 13. Daftar Risiko & Rencana Remediasi

| ID | Risiko | Level | Remediasi | Prioritas | Owner | Target |
|---|---|---|---|---|---|---|
| SR-01 | Pencarian DLL memuat DLL dari lokasi tidak tepercaya (Onyx membawa `winusb.dll`, `msvcr90.dll`, `msvcp90.dll` di folder aplikasi) | Tinggi | Tidak membundel DLL sistem; `SetDefaultDllDirectories`; uji DLL palsu | P1 | Dev A | M2 (27-11-2026), uji M7 |
| SR-02 | Parser berkas tidak tahan masukan berbahaya | Tinggi | Batas ukuran/dimensi, validasi, fuzzing, ASan | P1 | Dev B | M5 (08-01-2027) |
| SR-03 | Model ONNX dimanipulasi | Sedang | Verifikasi hash, manifest, konfirmasi untuk model luar | P1 | Dev B | M5 |
| SR-04 | Sertifikat tanda tangan bocor | Tinggi | Penyimpanan aman (rahasia CI terbatas atau token perangkat keras), akses terbatas, rencana pencabutan | P1 | Pemilik produk | Sebelum M7 |
| SR-05 | Dependensi/aksi CI berbahaya atau berkerentanan | Sedang | Pin versi, SBOM, SCA bulanan, review dependensi baru | P2 | Dev A | Berkelanjutan, awal di M1 |
| SR-06 | Aplikasi diminta berjalan sebagai Administrator | Sedang | Selidiki penyebab (driver USB); target tanpa elevasi; manifest `asInvoker` | P2 | Dev A | S2 (27-11-2026) |
| SR-07 | Data pribadi pengamat bocor lewat paket diagnostik | Sedang | Opsi penyamaran, dialog pratinjau isi, default tanpa frame, tinjauan UU PDP | P2 | Pemilik produk | Sebelum M7 |
| SR-08 | Data pengamatan dimodifikasi tanpa jejak | Sedang | Checksum arsip per sesi, manifest counters, versi aplikasi tercatat | P2 | Dev B | M5 |
| SR-09 | Pengguna memakai rilis palsu/versi lama | Sedang | Instruksi verifikasi checksum, nomor versi di UI, catatan rilis | P3 | Dev A | M7 |
| SR-10 | SDK vendor berkerentanan | Sedang | Pantau advisori vendor; uji pembaruan sebelum bundling | P3 | Dev A | Berkelanjutan |
| SR-11 | Lisensi pustaka pelatihan model (AGPL) melanggar distribusi | Sedang | Hanya runtime ONNX yang didistribusikan; tinjauan hukum; model card mencatat asal | P1 | Pemilik produk | Sebelum 28-12-2026 |
| SR-12 | Repositori/CI akun terkompromi | Tinggi | MFA wajib, perlindungan branch, hak minimum, audit akses kuartalan | P1 | Pemilik produk | M1 |

## Open Questions dan hal yang perlu diverifikasi manual

1. Apakah data pengamat (nama, lokasi) tergolong data pribadi menurut UU PDP untuk lembaga pengguna? Konsultasi hukum.
2. Penerbit dan mekanisme penyimpanan sertifikat tanda tangan kode.
3. Apakah vendor SDK menandatangani DLL mereka? Ini menentukan apakah verifikasi tanda tangan DLL dapat diterapkan.
4. Perilaku `SetDefaultDllDirectories` terhadap import statis pada Windows 10 22H2 dan Windows 11; uji praktis diperlukan.
5. Alat SBOM dan SCA yang benar-benar mencakup paket C/C++ dari vcpkg pada versi yang dipakai.
6. Apakah repositori publik atau privat (memengaruhi fitur keamanan platform yang tersedia)?
7. Asal data latih model dan lisensi bobot model.
8. Apakah lembaga pengguna mewajibkan enkripsi data pengamatan (BitLocker atau lainnya)?
