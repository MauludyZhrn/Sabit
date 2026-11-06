# Kebijakan Keamanan

Sabit adalah aplikasi desktop Windows untuk rukyat hilal. Aplikasi ini berjalan offline, tidak membuka port jaringan, dan tidak mengirim data keluar. Permukaan serangan utamanya adalah berkas yang dibuka (FITS, TIFF, PNG, JSON, model ONNX), pemuatan pustaka (DLL), dan rantai pengiriman rilis.

Dokumen ini menjelaskan cara melaporkan kerentanan. Rencana keamanan internal ada di `docs/09b-Security-Plan.md`.

> Status: pre-alpha. Nilai bertanda `<...>` harus diisi sebelum rilis pertama.

## 1. Versi yang didukung

| Versi | Status | Menerima perbaikan keamanan |
|---|---|---|
| 1.0.x | Dalam pengembangan (target rilis awal 2027) | Ya, setelah dirilis |
| Pra-rilis (`-rc`, `-alpha`, `-beta`) | Tidak didukung | Tidak; gunakan rilis stabil terbaru |
| Onyx OS (prototipe Python) | Tidak didukung | Tidak |

Kebijakan: hanya dua rilis minor terbaru yang menerima perbaikan keamanan. Perubahan kebijakan ini akan dicatat di tabel di atas.

## 2. Cara melaporkan kerentanan

**Jangan membuka isu publik, diskusi publik, atau pull request untuk kerentanan.**

Gunakan salah satu kanal privat:

1. Fitur pelaporan kerentanan privat pada halaman repositori (tab Security, "Report a vulnerability"), bila tersedia. TODO: aktifkan setelah repositori dibuat.
2. Email ke `security@<domain>` (TODO: tetapkan alamat). Disarankan mengenkripsi dengan kunci PGP/age yang dipublikasikan di `<url-kunci>` (TODO).

Sertakan sebisa mungkin:

- Versi Sabit dan hash build (menu Tentang), versi Windows.
- Deskripsi kerentanan dan dampak yang Anda perkirakan.
- Langkah mereproduksi, atau berkas contoh (misalnya FITS/model yang memicu masalah). Jangan menyertakan data pribadi nyata.
- Hasil crash/log (`app.log`, paket diagnostik) bila ada.
- Bagaimana Anda ingin dikreditkan (atau anonim).

## 3. Target waktu respons & proses penanganan

| Tahap | Target |
|---|---|
| Konfirmasi laporan diterima | ≤ 3 hari kerja |
| Penilaian awal (valid/tidak, perkiraan severity) | ≤ 7 hari kalender |
| Rencana perbaikan dan perkiraan waktu dikomunikasikan | ≤ 14 hari kalender |
| Perbaikan severity Critical/High | Target ≤ 30 hari kalender |
| Perbaikan severity Medium/Low | Target ≤ 90 hari kalender |
| Pengumuman publik | Setelah perbaikan dirilis, dikoordinasikan dengan pelapor |

Target ini adalah niat baik, bukan jaminan. Proyek dikerjakan tim kecil; jika target tidak bisa dipenuhi, kami akan memberi tahu alasannya.

Proses: kami mereproduksi masalah, menilai severity (CVSS sebagai acuan), menyiapkan perbaikan dan uji regresi, merilis versi perbaikan, lalu menerbitkan advisori keamanan yang menjelaskan dampak, versi terdampak, dan versi perbaikan.

Catatan operasional: sehubungan dengan jadwal pengamatan, perbaikan keamanan yang kritis dapat dirilis di luar jadwal rilis biasa, tetapi tetap melalui regresi penuh.

## 4. Responsible disclosure & safe harbor

Kami meminta Anda:

- Memberi kami waktu yang wajar untuk memperbaiki sebelum mengungkap detail secara publik (acuan: 90 hari sejak laporan, atau sampai perbaikan dirilis, mana yang lebih dulu; dapat dibicarakan).
- Tidak mengakses, mengubah, atau menghapus data milik orang lain; hanya menguji pada salinan Sabit dan data milik Anda sendiri.
- Tidak melakukan serangan yang mengganggu pengguna lain, rekayasa sosial terhadap kontributor, atau pengujian fisik terhadap perangkat orang lain.

Selama Anda mengikuti pedoman ini dan bertindak dengan itikad baik, kami tidak akan menempuh atau mendukung tindakan hukum terhadap Anda atas penelitian tersebut, dan kami akan berupaya melindungi identitas Anda bila Anda memintanya. Pernyataan ini tidak mengikat pihak ketiga (mis. vendor SDK) dan bukan nasihat hukum. TODO: tinjau teks safe harbor dengan penasihat hukum organisasi.

## 5. Ruang lingkup

**Dalam lingkup**
- Binari Sabit dan modul yang dikembangkan di repositori ini.
- Pemrosesan berkas masukan: FITS, TIFF, PNG (kalibrasi/impor), JSON (pengaturan, manifest), model ONNX.
- Pemuatan pustaka (DLL search order, pemuatan model/driver).
- Kemasan rilis: integritas ZIP, tanda tangan, checksum, SBOM.
- Pipeline build dan rilis (CI) milik proyek.
- Kesalahan penanganan memori yang dapat dipicu oleh berkas atau perangkat (crash terkontrol, pembacaan di luar batas).

**Di luar lingkup**
- Kerentanan pada SDK atau driver vendor kamera (laporkan ke vendor; beri tahu kami agar dapat mengevaluasi dampak).
- Kerentanan pada pustaka pihak ketiga yang belum terbukti dapat dieksploitasi melalui Sabit (laporkan ke proyek hulu).
- Serangan yang mensyaratkan akses fisik ke perangkat yang sudah tidak terkunci atau hak Administrator pada mesin target.
- Laporan hasil pemindai otomatis tanpa bukti dampak.
- Serangan rekayasa sosial dan phishing.
- Penolakan layanan lokal yang hanya memengaruhi pengguna itu sendiri tanpa dampak keamanan (laporkan sebagai bug biasa).
- Akurasi model deteksi atau perhitungan astronomi (laporkan sebagai bug biasa; bukan kerentanan keamanan).

## 6. Pengakuan kontributor (hall of fame)

Kami mencantumkan nama pelapor yang laporannya menghasilkan perbaikan (kecuali meminta anonim) di advisori keamanan dan di berkas `SECURITY-ACKNOWLEDGMENTS.md`.

| Pelapor | Laporan | Versi perbaikan |
|---|---|---|
| (belum ada) | | |

Proyek ini belum memiliki program hadiah uang (bug bounty). TODO: tinjau bila anggaran tersedia.

## Kontak

- Keamanan: `security@<domain>` (TODO)
- Kunci enkripsi: `<url-kunci>` (TODO)
