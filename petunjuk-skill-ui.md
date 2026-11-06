# Petunjuk: Skill Desain, Hapus Emoji, dan Perbaiki Resize Sabit

Dokumen ini menjawab tiga hal: cara memasang skill dari tiga repo, aturan yang harus dibaca agent, dan langkah perbaikan UI berdasarkan screenshot Sabit 1.0.

Rujukan: 01-PRD.md (FR-28, FR-29, NFR-08, NFR-11, NFR-12), 03-User-Flow-Wireframe.md, `.antigravityrules`.

---

## Bagian 1: Yang saya temukan di screenshot

Berikut masalah yang terlihat di Image 1 (aplikasi) dan Image 2 (aturan dan panel agent). Urutannya dari yang paling berdampak.

1. **Layout tidak mengisi ruang.** Area gambar berhenti di sekitar x=1378 dan y=1163, sementara panel kanan mulai di x=1540. Ada margin hitam kosong di kanan dan bawah. Ini terjadi karena ukuran viewport dikunci, bukan mengikuti jendela.
2. **Teks dengan karakter nyasar.** `Citra _Alat` dan `Log Sistem _Event Live` punya underscore. Kemungkinan dari mnemonic Qt (`&`) yang salah ketik atau teks yang ditulis langsung dengan `_`. Periksa semua string di `*.cpp` dan `*.ts`.
3. **Ikon toolbar tidak seragam.** Ada lingkaran putih, lingkaran merah, ikon kamera, grid, dan kubus. Ikon rotasi di kiri tampak seperti glyph font. Ganti semuanya dengan SVG satu gaya (lihat Bagian 4).
4. **Dropdown profil punya tombol kosong di kanannya.** Kotak kecil di sisi kanan "Profil: Hilal Senja (Default)" tidak punya fungsi yang jelas. Hapus atau beri ikon panah dan fungsi.
5. **Kartu bertumpuk di panel kanan.** Lima seksi (Data Efemeris, Kamera, Pemrosesan Citra, Deteksi AI Hilal, Log Sistem) masing-masing dibungkus kartu dengan latar sendiri dan jarak sama. Ini pola "cookie-cutter" dari anti-slop-design. Hilangkan latar kartu; pakai satu garis pemisah tipis dan judul seksi biasa.
6. **Teks "Deteksi AI Hilal" di toolbar tampak seperti label, bukan tombol.** Tidak ada cara mengetahui bahwa ini bisa diklik atau menunjukkan statusnya (OFF, MEMUAT, ON seperti di SCR-03).
7. **Status bar terlalu padat.** Ada tujuh informasi dalam satu baris (koneksi, FPS, frame, rekam, drop, tulis, disk, NTP, UTC, WIB). Pisahkan menjadi dua kelompok: status kiri (kamera, FPS, drop) dan status kanan (disk, jam).
8. **Tema tidak sesuai kebutuhan lapangan.** Latar navy dan aksen cyan dipakai sebagai default. Crescent di preview berwarna putih terang. FR-28 menyatakan tema Night Red tidak boleh memiliki elemen putih terang. Pastikan tombol merah di toolbar benar-benar mengganti seluruh palet ke merah, dan jadikan Night Red default pertama kali dibuka.
9. **Lokasi default tidak konsisten.** Log menulis `Semarang_Hilal`, sedangkan dokumen menggunakan Jakarta sebagai preset. Pastikan lokasi aktif terbaca dari settings dan tampil di panel efemeris.
10. **Tidak resizable dengan benar.** Panel kanan dan viewport tampaknya memakai ukuran tetap. Penyebab umum: `setFixedWidth`, `setFixedSize`, atau `QSizePolicy::Fixed`. Lihat Bagian 5.
11. **Judul seksi Log memakai huruf kapital campur.** "Log Sistem _Event Live" dan "Deteksi AI Hilal" tidak konsisten dengan judul lain. Pakai satu gaya (sentence case).

Catatan tambahan dari `.antigravityrules`: baris 6 menyebut `Onyx_OS.zip`, sedangkan file yang diunggah sebelumnya bernama `Onyx_Test.zip`. Pastikan nama yang dipakai di aturan sesuai file yang benar-benar ada.

---

## Bagian 2: Pasang tiga skill

Tiga repo ini sudah terlihat di `.design_skills/` pada Explorer. Jika ingin memasang ulang dari nol, jalankan dari root repo Sabit:

```powershell
# Windows PowerShell, dari root repo sabit
bash tools/setup-design-skills.sh
```

Skrip ini melakukan hal berikut:
1. Meng-clone ketiga repo ke `.design_skills/` (atau melakukan `git pull` bila sudah ada).
2. Menghapus folder `.git` di dalam tiap clone agar tidak menjadi repo bersarang.
3. Menghapus `skills/zzprobe-in-working-repo-5e18` dari `anti-slop-design`.

Perintah manual bila tidak memakai skrip:

```powershell
mkdir .design_skills; cd .design_skills
gh repo clone nextlevelbuilder/ui-ux-pro-max-skill
gh repo clone wwewtech/anti-slop-design
gh repo clone miqdadbadjuber/anti-slop
cd ..
Remove-Item -Recurse -Force .design_skills\anti-slop-design\skills\zzprobe-in-working-repo-5e18
Get-ChildItem .design_skills -Directory -Recurse -Force -Filter .git | Remove-Item -Recurse -Force
```

### Mengapa skill `zzprobe` dihapus

Folder `zzprobe-in-working-repo-5e18` dari `anti-slop-design` berisi teks "probe" dan isinya tidak relevan dengan desain. Penulisnya menjelaskan bahwa skill ini hanya dipakai untuk menguji apakah statistik install dihitung per skill. Tidak ada kode berbahaya di sana, tetapi skill ini hanya menambah kebisingan ke agent.

### Audit singkat yang sudah saya lakukan

- Tidak ada `eval`, `fetch` ke host lain, atau perintah shell yang tersembunyi dalam file SKILL.md ketiga repo.
- `anti-slop/.claude-plugin/contrast-mcp-launcher.mjs` menjalankan server Python lokal untuk cek kontras. Ini hanya relevan jika Anda memakai plugin Claude/Cline. Untuk Antigravity cukup pakai file SKILL.md.
- `ui-ux-pro-max` menyediakan `scripts/search.py` yang berjalan lokal dan membaca CSV. Butuh Python 3.

Sebelum rilis, ulangi audit ini setelah setiap `git pull` pada skill.

---

## Bagian 3: Peran tiap skill

| Skill | Fungsi untuk Sabit | Dipakai untuk |
|---|---|---|
| `anti-slop-design` (`anti-slop-design/skills/anti-slop-design/SKILL.md`) | Daftar larangan (Banned Matrix) dan checklist pra-kirim. Termasuk "Raw Emojis as Icons", "Cookie-Cutter Card Grid", "Screaming Eyebrow Labels", "Missing Interactive States" | Aturan utama anti-slop. Baca sebelum mengubah UI |
| `anti-slop` (`anti-slop/skills/antislop-ui/SKILL.md`, plus `antislop/SKILL.md` sebagai inti) | Aturan R-XX dengan format Tell / Why / Fix, dan Delivery Gate | Review UI sebelum merge. Pakai checklist "UI Skill Checklist" di akhir file |
| `ui-ux-pro-max` (`.claude/skills/ui-ux-pro-max/SKILL.md`) | Prioritas aturan (aksesibilitas, target sentuh, layout responsif, tipografi, warna) dan pencarian data lokal | Hierarki, spasi, dan aksesibilitas. Gunakan `scripts/search.py` untuk mencari aturan spesifik |

### Batasan yang harus dipahami sebelum memakai output generator

Saya menjalankan `search.py --design-system` untuk Sabit. Hasilnya berguna sebagai contoh struktur, tetapi **jangan diterapkan mentah-mentah**:

- Style yang dihasilkan adalah **Glassmorphism** dengan `backdrop-filter` 10 sampai 20 px. Di Qt tidak ada padanan langsung, dan efek ini bertentangan dengan NFR-12 (kontras) serta FR-28 (tidak ada elemen terang di Night Red).
- Font yang disarankan adalah **Fira Code / Fira Sans dari Google Fonts** dengan `@import` dari CDN. Ini melanggar NFR-08 (berfungsi tanpa internet). Gunakan font yang dibundel di `assets/fonts/` dan dimuat lewat `QFontDatabase::addApplicationFont`.
- Palet yang dihasilkan (`#0F172A`, `#22C55E`) adalah palet Tailwind default. Ini persis kebalikan dari "Night Red". Ambil token warna dari `theme.py` Onyx (Dark Onyx, Night Vision Red, Light System) lalu pindahkan ke JSON token. Jangan pakai palet dari generator.
- Stack terdekat untuk desktop yang tersedia di repo adalah WPF, WinUI, dan Avalonia. **Tidak ada stack Qt.** Gunakan aturan umumnya, bukan kode contoh mereka.

Ringkasnya: ambil aturan dari skill, abaikan nilai default web-nya.

---

## Bagian 4: Hapus emoji dan ganti dengan ikon SVG

### 4.1 Cek dulu

Jalankan skrip pengecek. Skrip ini gagal (exit 1) bila menemukan emoji di kode atau UI:

```powershell
python tools/check_no_emoji.py src
python tools/check_no_emoji.py .
```

Skrip memindai `.cpp .h .hpp .qss .ui .qrc .qml .json .ts .md` dan melewati `vendor/`, `build/`, `dist/`, `Captures/`, dan `.design_skills/`. Panah teks seperti `→` tidak dihitung sebagai emoji.

Pasang di CI agar emoji tidak masuk lagi (tambahkan di `.github/workflows/ci.yml`):

```yaml
      - name: Cek emoji
        run: python tools/check_no_emoji.py .
```

### 4.2 Ganti dengan SVG

Pakai satu set ikon: **Lucide** (lisensi ISC) atau **Phosphor** (MIT). Pilih satu saja dan pakai untuk seluruh aplikasi. Simpan berkas SVG di `assets/icons/` dan daftarkan ke `resources.qrc`:

```xml
<RCC>
  <qresource prefix="/icons">
    <file>icons/camera.svg</file>
    <file>icons/record.svg</file>
    <file>icons/aperture.svg</file>
    <file>icons/moon.svg</file>
    <file>icons/refresh-cw.svg</file>
  </qresource>
</RCC>
```

Ikon harus memakai `stroke="currentColor"` agar warnanya bisa diganti dari token tema. Lucide sudah memakai itu.

Kode untuk memuat SVG sebagai `QIcon` dengan warna tema dan rasio DPI:

```cpp
// src/ui/theme/icon_loader.h
#pragma once
#include <QIcon>
#include <QColor>
#include <QSize>
#include <QString>

namespace sabit::ui {
// path contoh: ":/icons/camera.svg"; warna dari token tema aktif.
QIcon makeIcon(const QString& resourcePath, const QColor& color, QSize logicalSize, qreal dpr);
}
```

```cpp
// src/ui/theme/icon_loader.cpp
#include "ui/theme/icon_loader.h"
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

namespace sabit::ui {

QIcon makeIcon(const QString& resourcePath, const QColor& color, QSize logicalSize, qreal dpr) {
    QFile f(resourcePath);
    if (!f.open(QIODevice::ReadOnly)) return QIcon();   // jangan crash bila ikon hilang

    QByteArray svg = f.readAll();
    svg.replace("currentColor", color.name(QColor::HexRgb).toUtf8());

    QSvgRenderer renderer(svg);
    if (!renderer.isValid()) return QIcon();

    QPixmap pm(logicalSize * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    renderer.render(&p, QRectF(QPointF(0, 0), logicalSize));
    p.end();
    return QIcon(pm);
}

}  // namespace sabit::ui
```

Catatan: `QSvgRenderer` membutuhkan modul `Svg` di CMake (`find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)` dan `target_link_libraries(... Qt6::Svg)`). Ukuran ikon yang disarankan: 16 px untuk status bar, 20 px untuk toolbar, 24 px untuk tombol utama. Ukuran tampilan harus mengikuti token jarak, bukan angka acak.

Pemakaian:

```cpp
auto* recordBtn = new QToolButton(this);
recordBtn->setIcon(makeIcon(":/icons/record.svg", theme.danger, {20, 20}, devicePixelRatio()));
recordBtn->setToolTip(tr("Rekam (Ctrl+R)"));
recordBtn->setAccessibleName(tr("Rekam"));   // wajib untuk pembaca layar (NFR-12)
```

Ikon tanpa teks harus punya `toolTip` dan `accessibleName`. Itu bagian dari aturan aksesibilitas di ui-ux-pro-max.

### 4.3 Hal yang tidak boleh ada

- Emoji di label, tooltip, pesan log, atau judul menu.
- Karakter Unicode sebagai ikon (misalnya `⟳`, `●`, `⌄`). Ganti dengan SVG.
- Ikon berwarna penuh. Gunakan satu warna dari token tema.

---

## Bagian 5: Perbaiki resize

Target: NFR-11 menyebut jendela minimum 1120x720 dan skala DPI 100% sampai 200%. Uji di 1366x768, 1920x1080, dan 2560x1440.

### 5.1 Aturan

1. Jangan memakai `setFixedSize`, `setFixedWidth`, atau `setFixedHeight` untuk area konten. Gunakan `setMinimumWidth` dan `setMaximumWidth` hanya bila benar-benar diperlukan.
2. Area video dan panel samping dipisah dengan `QSplitter`, bukan dengan lebar tetap.
3. Viewport video memakai `QSizePolicy::Expanding` dan mengisi seluruh sisa ruang.
4. Gambar dipasang dengan `KeepAspectRatio`, sehingga letterbox hanya muncul di sisi yang memang perlu.
5. Panel kanan punya lebar minimum (misalnya 300 px) dan tidak bisa dikecilkan sampai menutup viewport. Gunakan `setCollapsible(index, false)`.
6. Gunakan layout, bukan posisi absolut (`move`, `setGeometry` dengan angka tetap).

### 5.2 Kode: layout utama

```cpp
// src/ui/main_window/main_window.cpp (potongan)
setMinimumSize(1120, 720);                       // NFR-11

auto* split = new QSplitter(Qt::Horizontal, central);
split->setChildrenCollapsible(false);

auto* viewport = new VideoView(split);           // QGraphicsView dengan fitInView
viewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

auto* side = new SidePanel(split);
side->setMinimumWidth(300);
side->setMaximumWidth(480);

split->addWidget(viewport);
split->addWidget(side);
split->setStretchFactor(0, 1);                   // viewport menerima sisa ruang
split->setStretchFactor(1, 0);                   // panel samping tetap lebarnya
split->setSizes({ 1000, 360 });                  // ukuran awal; lalu bebas diubah pengguna
```

### 5.3 Kode: gambar mengisi viewport dan mempertahankan rasio

```cpp
// src/ui/widgets/video_view.h
#pragma once
#include <QGraphicsView>
#include <QGraphicsPixmapItem>

class VideoView : public QGraphicsView {
public:
    explicit VideoView(QWidget* parent = nullptr);
    void setFrame(const QPixmap& pm);          // dipanggil dari LatestSlot (GUI thread)

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    QGraphicsScene scene_;
    QGraphicsPixmapItem* item_ = nullptr;
};
```

```cpp
// src/ui/widgets/video_view.cpp
#include "ui/widgets/video_view.h"

VideoView::VideoView(QWidget* parent) : QGraphicsView(parent) {
    setScene(&scene_);
    item_ = scene_.addPixmap(QPixmap());
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setBackgroundBrush(Qt::black);
    setFrameShape(QFrame::NoFrame);
}

void VideoView::setFrame(const QPixmap& pm) {
    const bool sizeChanged = item_->pixmap().size() != pm.size();
    item_->setPixmap(pm);
    if (sizeChanged) {
        scene_.setSceneRect(pm.rect());
        fitInView(scene_.sceneRect(), Qt::KeepAspectRatio);
    }
}

void VideoView::resizeEvent(QResizeEvent* e) {
    QGraphicsView::resizeEvent(e);
    fitInView(scene_.sceneRect(), Qt::KeepAspectRatio);   // ikut ukuran jendela
}
```

Dengan ini letterbox hanya ada di sisi yang kosong karena rasio gambar, bukan karena ukuran tetap.

### 5.4 Kode: batas ukuran panel dan tinggi seksi

Seksi-seksi di panel kanan harus bisa diperkecil tanpa memotong isi. Pakai `QScrollArea` untuk seluruh panel samping:

```cpp
auto* scroll = new QScrollArea(side);
scroll->setWidgetResizable(true);                 // isi mengikuti lebar area
scroll->setFrameShape(QFrame::NoFrame);
scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
```

Jangan memberi tinggi tetap pada `QTextEdit` log. Biarkan `QTextEdit` memakai `Expanding` vertikal dan batasi jumlah baris dengan `setMaximumBlockCount(2000)`.

---

## Bagian 6: Aturan agent di `.antigravityrules`

Tambahkan blok ini di bagian atas `.antigravityrules`. Agent akan membacanya setiap sesi.

```markdown
## Aturan Desain Sabit (wajib dibaca sebelum mengubah UI)

1. Baca dulu `.design_skills/anti-slop-design/skills/anti-slop-design/SKILL.md` (Banned Matrix) dan `.design_skills/anti-slop/skills/antislop-ui/SKILL.md` (UI Skill Checklist).
2. Jangan gunakan emoji atau karakter Unicode pictografik di kode, teks UI, tooltip, atau log. Jalankan `python tools/check_no_emoji.py .` sebelum selesai. Hasil harus 0 temuan.
3. Ikon hanya SVG (Lucide). Simpan di `assets/icons/`, muat dengan `makeIcon()` di `src/ui/theme/icon_loader.cpp`, warnanya dari token tema.
4. Jangan pakai `setFixedSize`, `setFixedWidth`, atau `QSizePolicy::Fixed` untuk area konten. Layout utama memakai `QSplitter`; viewport memakai `QGraphicsView` dengan `fitInView(KeepAspectRatio)`.
5. Jendela minimum 1120x720. Uji resize ke 1120x720, 1366x768, 1920x1080, dan 2560x1440 pada DPI 100% dan 150%.
6. Tema Night Red adalah default pertama kali dibuka. Tidak boleh ada elemen putih terang atau biru terang saat tema ini aktif.
7. Font dibundel di `assets/fonts/` dan dimuat dengan `QFontDatabase::addApplicationFont`. Jangan memuat font atau CSS dari internet.
8. Jangan memakai glassmorphism, `backdrop-filter`, gradien dekoratif, atau kartu bayangan tebal. Pisahkan seksi dengan garis tipis dan jarak dari token.
9. Jangan menghasilkan palet dari generator skill. Token warna hanya dari `theme.py` (Onyx) yang sudah dipindahkan ke JSON token.
10. Hapus underscore atau karakter nyasar di teks UI. Judul seksi memakai sentence case.
11. Setiap ikon tanpa teks wajib punya `toolTip` dan `accessibleName`.
```

---

## Bagian 7: Prompt siap pakai untuk agent

Salin prompt ini ke Antigravity setelah skill terpasang dan aturan diperbarui:

```
Baca .antigravityrules, lalu baca skill di .design_skills/anti-slop-design dan .design_skills/anti-slop.

Kerjakan dalam urutan ini dan laporkan hasil tiap langkah:
1. Jalankan `python tools/check_no_emoji.py .` dan perbaiki semua temuan. Ganti dengan ikon SVG Lucide dari assets/icons/.
2. Perbaiki teks nyasar: "Citra _Alat" dan "Log Sistem _Event Live". Cari semua string dengan underscore atau & yang salah di src/ dan *.ts.
3. Hapus latar kartu dari seksi di panel kanan. Pakai garis pemisah tipis dan judul sentence case.
4. Ubah layout utama ke QSplitter dengan viewport QGraphicsView (fitInView, KeepAspectRatio) dan panel samping dalam QScrollArea. Hapus semua setFixedSize/setFixedWidth di area konten.
5. Jadikan Night Red default pertama kali dibuka. Periksa semua layar dan pastikan tidak ada elemen putih atau biru terang.
6. Pisahkan status bar menjadi dua kelompok (kiri: kamera, FPS, drop; kanan: disk, NTP, jam).
7. Jadikan "Deteksi AI Hilal" tombol dengan status OFF, MEMUAT, dan ON.
8. Hapus tombol kosong di sisi kanan dropdown profil.

Batasan: jangan pakai Google Fonts atau CDN. Jangan memakai palet atau glassmorphism dari generator skill. Jangan mengubah logika kamera, pipeline, atau penulisan berkas.

Setelah selesai: jalankan build, uji offscreen (`ctest`), dan laporkan hasil check_no_emoji dan ukuran minimum jendela.
```

---

## Bagian 8: Verifikasi

Lakukan secara berurutan. Jangan lanjut bila langkah sebelumnya gagal.

1. `python tools/check_no_emoji.py .` menghasilkan `0 temuan emoji.`
2. `ctest --preset windows-release --output-on-failure` lulus, termasuk uji layar offscreen.
3. Uji manual resize:
   - Jendela ke 1120x720: panel samping tidak terpotong, viewport tetap terlihat, tidak ada scrollbar horizontal.
   - Jendela ke 2560x1440: gambar mengisi viewport, letterbox hanya di sisi yang kosong.
   - Geser splitter sampai batas minimum: panel samping tidak bisa menutup viewport.
4. Uji DPI 150%: ikon tajam, teks tidak terpotong.
5. Uji Night Red: tangkap layar dan cek tidak ada piksel putih atau biru terang. Kontras teks tetap minimal 4,5:1 (NFR-12).
6. Uji kriteria fungsional: pengaturan exposure dan gain tetap berjalan setelah perubahan UI (TC-004).
7. Ambil tangkapan layar baru dan bandingkan dengan Image 1. Jika masih terlihat seperti kartu bertumpuk dan ikon tidak seragam, ulangi langkah 3 sampai 8 di Bagian 7.

---

## Ringkasan singkat

- Pasang skill dengan `tools/setup-design-skills.sh`. Hapus `zzprobe`.
- Pakai `anti-slop-design` untuk larangan, `anti-slop` untuk checklist, dan `ui-ux-pro-max` untuk hierarki dan aksesibilitas.
- Abaikan nilai default web dari generator (glassmorphism, Google Fonts, palet Tailwind). Itu bertentangan dengan offline dan Night Red.
- Emoji dilarang. Cek dengan `tools/check_no_emoji.py`, pasang di CI, ganti dengan SVG Lucide.
- Resize: QSplitter, QGraphicsView dengan fitInView, tanpa ukuran tetap.
