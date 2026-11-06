---
name: sabit-add-camera-driver
description: Menambah atau memperbaiki driver kamera di repositori Sabit (C++20/Qt 6) agar memenuhi kontrak ICamera. Gunakan saat diminta membuat driver baru (QHY, ASCOM, UVC, kamera vendor lain), memperbaiki driver yang pengaturannya tidak sampai ke hardware, atau menulis uji kontrak driver. Jangan dipakai untuk logika UI, pemrosesan citra, atau kode Python Onyx OS lama.
---

# Skill: Menambah driver kamera Sabit

## Ringkasan

Skill ini memandu pembuatan driver kamera yang mengikuti kontrak `ICamera` pada Sabit: satu thread kamera, pengaturan masuk lewat antrean perintah, nilai dibaca balik dari hardware, frame 16-bit dengan metadata hardware, dan semua kegagalan dilaporkan sebagai event. Tujuannya menghindari cacat yang ada pada Onyx OS (pengaturan tidak sampai ke kamera, kegagalan diam-diam, data 8-bit).

## Kapan digunakan / tidak digunakan

Gunakan bila:
- Diminta "tambah driver untuk kamera X", "dukung QHY/ASCOM/UVC", atau "driver ini tidak menerapkan exposure/gain".
- Diminta menulis atau memperbaiki uji kontrak untuk driver.

Jangan gunakan bila:
- Tugas hanya mengubah tampilan, panel UI, atau tema.
- Tugas pemrosesan citra (kalibrasi, stacking, stretch) atau AI.
- Tugas melibatkan port langsung kode Python Onyx OS tanpa meninjau kontrak (Onyx OS bukan acuan perilaku yang benar).

## Prasyarat

Periksa sebelum mulai:
1. Repositori memuat `docs/02-Technical-Specification.md`, bagian 5.1 (kontrak `ICamera`) dan 5.2 (`Frame`). Baca keduanya.
2. Struktur: `src/camera/icamera.h`, `src/camera/registry.cpp`, `src/camera/sim/` (referensi implementasi), `tests/integration/camera_contract_test.cpp`.
3. SDK vendor: header dan dokumentasi resmi tersedia di `vendor/<nama>/`. Jika tidak tersedia, **berhenti dan minta** dokumentasi; jangan menebak nama fungsi atau konstanta SDK.
4. Lisensi redistribusi SDK sudah dicatat di `vendor/<nama>/LICENSE-NOTES.md`. Jika belum, tandai TODO dan beri tahu pemilik produk.

## Alur kerja

1. **Baca kontrak dan referensi.** Buka `icamera.h` dan `sim/` untuk pola yang dipakai (kolam frame, antrean perintah, event).
2. **Petakan kemampuan SDK ke `CameraCaps`.** Untuk tiap kemampuan (exposure, gain, binning, bit depth, ROI, cooler, warna/mono, Bayer), baca dari SDK, bukan nilai tetap.
   - Jika SDK tidak menyediakan rentang, isi `caps()` dengan "tidak didukung" untuk kontrol itu. Jangan mengarang rentang.
3. **Buat kelas driver** `src/camera/<nama>/<nama>_camera.{h,cpp}` yang mengimplementasikan `ICamera`.
   - Satu thread milik driver; semua panggilan SDK terjadi di thread itu.
   - `apply()` hanya memasukkan perintah ke antrean dan mengembalikan `std::future` yang diselesaikan setelah nilai dibaca balik.
4. **Implementasi `open()`.**
   - Kembalikan kode galat spesifik: `NotFound`, `Busy`, `DriverMissing`, `Permission`, `HardwareError`.
   - Muat DLL vendor hanya dari folder aplikasi (lihat Aturan).
5. **Implementasi `start()` dan loop frame.**
   - Ambil frame dengan timeout; timeout dihitung, bukan error fatal.
   - Isi `Frame`: lebar/tinggi, kanal, `bit_depth`, `bayer`, piksel 16-bit, `exposure_us`, `gain`, `sensor_temp_c` (NaN bila tidak ada sensor), `binning`, `timestamp_utc`, `clock_sync`.
   - Data 8-bit di-upcast tanpa normalisasi per frame (geser bit tetap).
6. **Implementasi `apply()`.**
   - Validasi terhadap `caps()`; di luar rentang → `OutOfRange` dengan rentang yang benar.
   - Setelah memanggil SDK, **baca balik** nilai dan kembalikan nilai yang dibaca. Jika driver membulatkan, kembalikan nilai pembulatan dan catat selisih di log.
   - Perubahan yang butuh restart streaming (mis. binning/ROI) ditangani di dalam thread kamera; jangan memaksa UI mengurusnya.
7. **Implementasi `status()`** dengan suhu terukur dan daya pendingin bila SDK menyediakan.
8. **Daftarkan driver** di `registry.cpp` (enumerasi `CameraId` dengan nama yang akurat; tidak ada entri palsu).
9. **Tangani putus koneksi.** Deteksi kegagalan SDK/USB, hentikan thread dengan bersih, kirim `onDisconnected` dalam ≤ 3 detik. Tidak ada `wait()` tanpa batas waktu.
10. **Tulis uji.**
    - Tambahkan driver ke suite kontrak (`camera_contract_test.cpp`) dengan fixture yang melewati perangkat nyata bila tersedia dan dilewati (`SKIP`) bila tidak.
    - Tambahkan uji unit untuk pemetaan caps, validasi rentang, dan penerjemahan kode galat memakai SDK palsu (mock) bila memungkinkan.
11. **Verifikasi** dengan checklist di bawah dan jalankan `ctest --preset windows-release`.
12. **Dokumentasikan.** Perbarui `docs/` (daftar kamera didukung, catatan lisensi SDK, batasan yang diketahui).

Titik keputusan:
- SDK tidak memberi tahu rentang nilai → tandai kontrol "tidak didukung", jangan menebak.
- SDK terikat thread atau COM (mis. ASCOM) → pastikan seluruh interaksi di thread kamera dengan model apartemen yang sesuai (STA untuk COM).
- Kamera tidak punya sensor suhu → `sensor_temp_c = NaN`; metadata FITS jangan menulis `CCD-TEMP`.

## Aturan & konvensi

Lakukan:
- Semua I/O hardware di thread kamera.
- Nilai yang dilaporkan ke UI adalah hasil baca-balik.
- Kembalikan `Result<T>` dengan kode galat; log pesan yang dapat ditindaklanjuti (apa, kemungkinan penyebab, saran).
- Gunakan RAII untuk handle SDK; tutup handle di destruktor dan saat `stop()`.
- Gunakan kolam frame (`FramePool`) untuk menghindari alokasi per frame.

Jangan:
- Mengubah atau menormalisasi kecerahan data frame (tidak ada auto-stretch di driver).
- Menyimulasikan kontrol di software (mis. mengalikan piksel untuk "gain") dan melaporkannya sebagai hardware.
- Menelan kesalahan SDK (`catch(...)` kosong), atau mengembalikan sukses tanpa memeriksa kode SDK.
- Memuat DLL dari `PATH` atau direktori kerja; memakai `LoadLibrary` dengan nama saja.
- Membundel DLL sistem Windows (`winusb.dll`, `msvcr*.dll`) ke folder aplikasi.
- Mengarang nama fungsi, struktur, atau konstanta SDK. Jika dokumentasi tidak ada, berhenti dan tanyakan.
- Memanggil `wait()` atau `join()` tanpa batas waktu.

## Template / contoh output

Kerangka kelas (nama `<SDK_...>` diganti dari header resmi vendor):

```cpp
// src/camera/<nama>/<nama>_camera.h
#pragma once
#include "camera/icamera.h"

namespace sabit::camera {

class <Nama>Camera final : public ICamera {
public:
    explicit <Nama>Camera(CameraId id);
    ~<Nama>Camera() override;

    core::Result<CameraCaps> open(const CameraId&) override;
    CameraCaps caps() const override;
    std::future<core::Result<CameraSettings>> apply(CameraSettings s) override;
    core::Result<void> start(core::FrameSink&) override;
    void stop() override;
    CameraStatus status() const override;

private:
    void thread_main();                       // satu-satunya tempat memanggil <SDK_...>
    core::Result<CameraSettings> apply_in_thread(const CameraSettings&);

    CameraId id_;
    CameraCaps caps_;
    CommandQueue<Command> cmdq_;              // perintah dari thread lain
    std::thread worker_;
    std::atomic<bool> running_{false};
    core::FrameSink* sink_ = nullptr;
};

}  // namespace sabit::camera
```

Pola `apply_in_thread` (inti kontrak):

```cpp
core::Result<CameraSettings> <Nama>Camera::apply_in_thread(const CameraSettings& req) {
    CameraSettings applied;
    if (req.exposure_us) {
        if (!caps_.exposure_us.contains(*req.exposure_us))
            return core::Error{ErrorCode::OutOfRange, range_message(caps_.exposure_us)};
        if (auto rc = <SDK_SetExposure>(handle_, *req.exposure_us); rc != <SDK_OK>)
            return core::Error{ErrorCode::HardwareError, sdk_error_text(rc)};
        applied.exposure_us = <SDK_GetExposure>(handle_);   // baca balik
        if (*applied.exposure_us != *req.exposure_us)
            LOG_WARN("exposure dibulatkan {} -> {}", *req.exposure_us, *applied.exposure_us);
    }
    // ... gain, binning, bit_depth, roi, cooler dengan pola yang sama
    return applied;
}
```

## Contoh input -> output

**Contoh 1**
- Input: "Tambah driver QHY. Header dan dokumentasi SDK ada di `vendor/qhy/`."
- Output yang diharapkan:
  - `src/camera/qhy/qhy_camera.{h,cpp}` mengimplementasikan `ICamera`; `caps()` diisi dari pembacaan SDK.
  - Entri di `registry.cpp` dengan nama dari SDK.
  - Kasus ditambahkan ke `camera_contract_test.cpp` (SKIP bila perangkat tidak ada); uji unit pemetaan caps dan penerjemahan galat.
  - `docs/` diperbarui (kamera didukung, catatan lisensi).
  - Ringkasan: kemampuan yang tidak tersedia di SDK ditandai "tidak didukung" (bukan diisi nilai tebakan).

**Contoh 2**
- Input: "Slider gain di driver ZWO tidak mengubah gambar di kamera."
- Output yang diharapkan:
  - Diagnosis: apakah `apply()` memanggil SDK di thread kamera dan membaca balik; apakah UI menampilkan nilai baca-balik atau nilai slider.
  - Perbaikan: perintah masuk antrean, `apply_in_thread` memanggil SDK dan mengembalikan nilai baca-balik; UI diperbarui dari hasil `future`.
  - Uji regresi yang menyatakan nilai UI sama dengan nilai yang dibaca dari SDK/mock setelah `apply()`.

**Contoh 3 (tanpa dokumentasi)**
- Input: "Tambah dukungan kamera Z, SDK-nya belum ada di repo."
- Output yang diharapkan: tidak menulis kode yang memanggil fungsi tebakan. Mengajukan permintaan spesifik (header, DLL, dokumentasi, lisensi) dan membuat hanya kerangka kelas dengan `TODO` pada pemanggilan SDK, ditandai jelas di pesan.

## Penanganan error & edge case

| Situasi | Penanganan |
|---|---|
| SDK mengembalikan kode kesalahan tidak dikenal | `HardwareError` dengan kode mentah di pesan log |
| Kamera dicabut saat streaming | Hentikan thread, kirim `onDisconnected`, bebaskan handle |
| Timeout frame berulang | Hitung timeout; setelah 3 detik tanpa frame, status "Tidak ada sinyal" |
| Driver membulatkan nilai | Kembalikan nilai bulat, log selisih |
| Dua instance membuka kamera yang sama | Kembalikan `Busy` |
| Kamera mono dan warna memakai kelas yang sama | `caps().is_color` dari SDK; `bayer` diisi dari SDK |
| Tidak ada sensor suhu | `sensor_temp_c = NaN`; tidak menulis `CCD-TEMP` |
| `apply()` dipanggil sebelum `open()` | Kembalikan error `NotOpen` |

## Checklist verifikasi akhir

- [ ] `caps()` berasal dari SDK; tidak ada rentang tebakan
- [ ] Semua panggilan SDK di thread kamera
- [ ] `apply()` mengembalikan nilai baca-balik; selisih dicatat
- [ ] Frame 16-bit tanpa normalisasi; metadata dari hardware
- [ ] Putus koneksi terdeteksi ≤ 3 detik; tidak ada hang
- [ ] Tidak ada `wait()`/`join()` tanpa batas waktu
- [ ] DLL dimuat hanya dari folder aplikasi
- [ ] Tidak ada DLL sistem terbundel
- [ ] Uji kontrak dan uji unit lulus (`ctest`)
- [ ] Driver terdaftar tanpa entri palsu
- [ ] Dokumen dan catatan lisensi SDK diperbarui

## Referensi file pendukung

- `docs/02-Technical-Specification.md` (bagian 5 dan 11): kontrak dan error handling.
- `docs/05-Testing-QA-Checklist.md`: TC-001 sampai TC-009 dan TC-083 sampai TC-089.
- `src/camera/sim/`: implementasi referensi.
- `tests/integration/camera_contract_test.cpp`: suite kontrak.
- `vendor/<nama>/`: header, dokumentasi, catatan lisensi.
