# Panduan Pengujian Lapangan SABIT (Testing Guide)

Dokumen ini merupakan panduan teknis bagi praktisi dan ahli falak yang berpartisipasi dalam pengujian perangkat lunak observasi SABIT.

---

## 1. Langkah Instalasi Perangkat Lunak

1. Dapatkan berkas penginstal resmi: `Sabit-Setup-v1.0.0-x64.exe`.
2. Klik ganda berkas tersebut untuk menjalankan penginstal.
3. Lokasi instalasi standar berada pada direktori pengguna:
   `%LOCALAPPDATA%\Programs\Sabit`
   Instalasi ini **tidak membutuhkan hak akses Administrator** dan tidak akan mengubah berkas sistem Windows Anda.
4. Pilih opsi pembuatan ikon pintasan di Desktop (Desktop Shortcut) dan Menu Mulai (Start Menu) sesuai preferensi Anda.
5. Klik **Install**, lalu tunggu hingga proses penyalinan selesai.
6. Tandai opsi **Launch SABIT** dan klik **Finish** untuk memulai aplikasi.

---

## 2. Pengaturan Awal Pengamatan (First-Time Setup)

1. **Memeriksa Posisi dan Lokasi Stasiun**:
   - Buka menu **File > Pengaturan...** (atau tekan pintasan `Ctrl+,`).
   - Pada tab **Lokasi**, pilih preset stasiun Anda (tersedia 38 ibu kota provinsi di Indonesia) atau masukkan koordinat lintang, bujur, dan elevasi GPS secara manual.
   - Klik **Simpan Pengaturan**.
2. **Menghubungkan Perangkat Kamera**:
   - Buka menu **Kamera > Hubungkan Kamera...** (atau tekan `Ctrl+K`).
   - Pilih jenis driver kamera Anda:
     - **Simulator**: Untuk demonstrasi dan simulasi posisi hilal berbasis data toposentrik.
     - **DirectShow / UVC**: Untuk webcam, HDMI Capture card (menghubungkan DSLR/mirrorless), atau virtual webcam.
     - **ASCOM Camera**: Untuk kamera astro yang terpasang melalui ASCOM Platform.
     - **ZWO ASI / QHYCCD**: Untuk kamera cooled astro CMOS khusus.
   - Klik **Hubungkan**.
3. **Memeriksa Efemeris & Kriteria Visibilitas**:
   - Perhatikan panel **Astronomi & Efemeris** di sebelah kanan.
   - Pastikan informasi waktu toposentrik Matahari dan Bulan terhitung secara dinamis.
   - Tekan tombol **Detail Efemeris** (atau `Ctrl+D`) untuk melihat perbandingan kriteria MABIMS baru, Danjon, dan Odeh.

---

## 3. Alur Pengujian Observasi dan Perekaman Data

1. **Pemrosesan Citra & Peningkatan Kontras**:
   - Pada panel **Pemrosesan & Stacking**, aktifkan fitur **Peregangan Kontras Otomatis** (Auto Contrast Stretch).
   - Uji fitur **Peningkatan CLAHE** dan **Penajaman (Unsharp Mask)** untuk meningkatkan rasio kontras sabit terhadap latar langit senja/siang.
   - Jalankan **Live Stacking** (Metode Mean / Sigma-Clip) untuk mengamati peredaman derau (noise reduction) dan keselarasan frame (phase correlation).
2. **Deteksi Cerdas Berbasis AI**:
   - Aktifkan tombol **AI Sensor** pada bilah alat atas.
   - Amati apakah kotak pembatas (bounding box) dan tingkat keyakinan (confidence score) muncul saat sabit terdeteksi pada preview video.
3. **Perekaman Sesi dan Ekspor Laporan Resmi**:
   - Buka menu **Capture > Mulai Sesi Observasi...** (atau tekan `Ctrl+Shift+S`).
   - Masukkan nama lokasi stasiun, nama target, dan nama observer.
   - Tekan tombol **Rekam** (`F6`) untuk mengambil rangkaian frame ilmiah 16-bit FITS.
   - Setelah observasi selesai, pilih menu **File > Ekspor Laporan Observasi Resmi...** (`Ctrl+E`) untuk menghasilkan dokumen laporan berformat PDF standar yang siap diajukan ke instansi terkait (misalnya sidang isbat Kementerian Agama RI).

---

## 4. Pelaporan Kendala dan Umpan Balik

- Catat seluruh temuan teknis atau ketidaksesuaian hasil pada lembar `TEST_SCENARIOS.csv` dan `OBSERVATION_LOG.csv`.
- Apabila menemukan kesalahan program (bug/crash), gunakan templat pada berkas `BUG_REPORT_TEMPLATE.md`.
- Berikan penilaian ergonomi dan kemudahan guna pada lembar `FEEDBACK_FORM.md`.
