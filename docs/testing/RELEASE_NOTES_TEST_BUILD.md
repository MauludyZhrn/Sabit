# Catatan Rilis Versi Uji Coba: SABIT v1.0.0 (Research Test Build)

Aplikasi: SABIT â€” Scientific Astronomical Imaging & Observation Toolkit
Versi: 1.0.0 (Release Build x64)
Arsitektur: Native C++20 / Qt 6.8 (Windows 64-bit)
Tujuan: Pengujian Lapangan dan Pengumpulan Data Riset Tesis Magister Ilmu Falak
Pengembang: Mauludy Zahron

---

## 1. Lingkup Pengujian (Testing Scope)

Versi uji coba ini ditujukan khusus bagi praktisi astronomi, akademisi falak, pengamat rukyatul hilal, dan anggota tim hisab rukyat untuk mengevaluasi fungsionalitas workstation observasi astronomi terintegrasi pada sistem operasi Windows 10/11.

Fokus evaluasi meliputi:
1. **Pengujian Antarmuka & Ergonomi Pengamatan**: Keterbacaan tampilan saat observasi senja, efektivitas tema gelap (Dark) dan penglihatan malam (Night Vision Red).
2. **Kalkulasi Efemeris & Kriteria Visibilitas**: Perhitungan toposentrik posisi Bulan dan Matahari, waktu terbenam, serta evaluasi kriteria MABIMS baru (ketinggian >= 3 derajat, elongasi >= 6.4 derajat).
3. **Pemrosesan Citra & Live Stacking**: Responsivitas peregangan kontras MTF otomatis, CLAHE, unsharp mask, dan perataan pergeseran fase (phase correlation).
4. **Deteksi Hilal Cerdas (ONNX Runtime)**: Pengenalan citra sabit muda berbantuan kecerdasan buatan dan visualisasi telemetry.
5. **Perekaman Data Ilmiah**: Penyimpanan citra 16-bit berformat FITS lengkap dengan WCS header, video AVI/MP4, dan ekspor lembar verifikasi laporan PDF resmi.

---

## 2. Batasan Aplikasi yang Diketahui (Known Limitations)

1. **Driver Kamera Khusus (Dedicated Astro Cameras)**:
   - Driver perangkat keras pihak ketiga seperti ZWO ASI (`ASICamera2.dll`) dan QHYCCD (`qhyccd.dll`) tunduk pada lisensi eksklusif manufaktur dan tidak dibundel secara langsung dalam paket instalasi ini.
   - Pengguna kamera ZWO/QHY diwajibkan menginstal driver resmi vendor sebelum menghubungkan perangkat keras.
   - Kamera simulator sintetis (`Sabit Synthetic Simulator Camera`) dan seluruh kamera UVC/DirectShow (Webcam, DSLR HDMI Capture Card, Canon EOS Webcam Utility) dapat langsung digunakan tanpa dependensi eksternal.
2. **Kebutuhan Akselerasi Grafis**:
   - Preview video 60 fps dan perataan live stacking memanfaatkan akselerasi CPU multi-threading. Pada perangkat keras berspesifikasi rendah (< 4 core), performa AI dibatasi maksimal 3 frame per detik secara otomatis guna menjaga kelancaran preview.
3. **Konektivitas Internet**:
   - SABIT dirancang untuk dapat beroperasi 100% luring (offline) di pos observasi terpencil tanpa memerlukan koneksi internet.

---

## 3. Persyaratan Sistem Minimum

- Sistem Operasi: Microsoft Windows 10 (versi 21H2 ke atas) atau Windows 11 64-bit.
- Prosesor: Intel Core i3 generasi ke-8 / AMD Ryzen 3 atau lebih tinggi (disarankan 4 core ke atas).
- Memori RAM: 4 GB minimum (disarankan 8 GB untuk stacking citra resolusi tinggi).
- Ruang Penyimpanan: 250 MB untuk instalasi aplikasi, ditambah ruang kosong untuk penyimpanan hasil observasi FITS/video.
