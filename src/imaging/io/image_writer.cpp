#include "src/imaging/io/image_writer.h"
#include <fstream>
#include <filesystem>
#include <cstring>
#include "src/core/safe_size.h"

namespace sabit::imaging::io {

// Little-endian writing helpers
static inline void write_u16_le(std::ostream& os, uint16_t v) {
    char b[2] = {static_cast<char>(v & 0xFF), static_cast<char>((v >> 8) & 0xFF)};
    os.write(b, 2);
}

static inline void write_u32_le(std::ostream& os, uint32_t v) {
    char b[4] = {
        static_cast<char>(v & 0xFF),
        static_cast<char>((v >> 8) & 0xFF),
        static_cast<char>((v >> 16) & 0xFF),
        static_cast<char>((v >> 24) & 0xFF)
    };
    os.write(b, 4);
}

static inline uint16_t read_u16_le(const char* p) {
    return static_cast<uint8_t>(p[0]) | (static_cast<uint16_t>(static_cast<uint8_t>(p[1])) << 8);
}

static inline uint32_t read_u32_le(const char* p) {
    return static_cast<uint8_t>(p[0]) |
           (static_cast<uint32_t>(static_cast<uint8_t>(p[1])) << 8) |
           (static_cast<uint32_t>(static_cast<uint8_t>(p[2])) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(p[3])) << 24);
}

core::Result<void> ImageWriter::write_tiff_16bit(const std::string& file_path, const core::Frame& frame) {
    if (!frame.is_valid()) {
        return core::Result<void>::err(core::ErrorCode::InvalidState, "Cannot write invalid frame to TIFF");
    }

    std::filesystem::path final_path(file_path);
    if (final_path.has_parent_path()) {
        std::filesystem::create_directories(final_path.parent_path());
    }

    std::string tmp_path = file_path + ".tmp";
    std::ofstream ofs(tmp_path, std::ios::binary);
    if (!ofs.is_open()) {
        return core::Result<void>::err(core::ErrorCode::PermissionDenied, "Failed to open temporary TIFF file");
    }

    uint32_t w = frame.width;
    uint32_t h = frame.height;
    uint32_t num_pixels = w * h;
    uint32_t strip_bytes = num_pixels * 2;

    // Header: 'II' (little endian) + 42 + offset to IFD (8)
    ofs.write("II", 2);
    write_u16_le(ofs, 42);
    write_u32_le(ofs, 8); // IFD offset immediately follows header

    // IFD: 10 tags
    uint16_t num_tags = 10;
    write_u16_le(ofs, num_tags);

    uint32_t ifd_size = 2 + num_tags * 12 + 4; // 2 + 120 + 4 = 126
    uint32_t extra_data_offset = 8 + ifd_size; // 134
    // RATIONAL resolution values (8 bytes each): 16 bytes
    uint32_t pixel_data_offset = extra_data_offset + 16; // 150

    auto write_tag = [&](uint16_t tag, uint16_t type, uint32_t count, uint32_t val_or_offset) {
        write_u16_le(ofs, tag);
        write_u16_le(ofs, type);
        write_u32_le(ofs, count);
        write_u32_le(ofs, val_or_offset);
    };

    // Tag 256: ImageWidth
    write_tag(256, 4, 1, w);
    // Tag 257: ImageLength
    write_tag(257, 4, 1, h);
    // Tag 258: BitsPerSample (16)
    write_tag(258, 3, 1, 16);
    // Tag 259: Compression (1 = uncompressed)
    write_tag(259, 3, 1, 1);
    // Tag 262: PhotometricInterpretation (1 = BlackIsZero)
    write_tag(262, 3, 1, 1);
    // Tag 273: StripOffsets
    write_tag(273, 4, 1, pixel_data_offset);
    // Tag 277: SamplesPerPixel (1)
    write_tag(277, 3, 1, 1);
    // Tag 278: RowsPerStrip (h)
    write_tag(278, 4, 1, h);
    // Tag 279: StripByteCounts
    write_tag(279, 4, 1, strip_bytes);
    // Tag 296: ResolutionUnit (2 = inch)
    write_tag(296, 3, 1, 2);

    // Next IFD offset: 0 (no more IFDs)
    write_u32_le(ofs, 0);

    // Extra data: XResolution & YResolution (72 / 1)
    write_u32_le(ofs, 72);
    write_u32_le(ofs, 1);
    write_u32_le(ofs, 72);
    write_u32_le(ofs, 1);

    // Pixel data (16-bit Little-Endian)
    const uint16_t* px = frame.pixels.get();
    ofs.write(reinterpret_cast<const char*>(px), strip_bytes);
    ofs.flush();
    ofs.close();

    std::error_code ec;
    std::filesystem::rename(tmp_path, final_path, ec);
    if (ec) {
        return core::Result<void>::err(core::ErrorCode::HardwareError, "Atomic rename failed: " + ec.message());
    }

    return core::Result<void>::ok();
}

core::Result<core::Frame> ImageWriter::read_tiff_16bit(const std::string& file_path) {
    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs.is_open()) {
        return core::Result<core::Frame>::err(core::ErrorCode::NotFound, "Failed to open TIFF file");
    }

    char hdr[8];
    if (!ifs.read(hdr, 8) || (hdr[0] != 'I' || hdr[1] != 'I')) {
        return core::Result<core::Frame>::err(core::ErrorCode::SerializationError, "Not a valid little-endian TIFF file");
    }

    uint32_t ifd_offset = read_u32_le(hdr + 4);
    ifs.seekg(ifd_offset);

    char num_tags_buf[2];
    if (!ifs.read(num_tags_buf, 2)) {
        return core::Result<core::Frame>::err(core::ErrorCode::SerializationError, "Failed to read IFD tag count");
    }
    uint16_t num_tags = read_u16_le(num_tags_buf);

    uint32_t w = 0, h = 0, strip_offset = 0;
    for (uint16_t i = 0; i < num_tags; ++i) {
        char tag_buf[12];
        ifs.read(tag_buf, 12);
        uint16_t tag = read_u16_le(tag_buf);
        uint32_t val = read_u32_le(tag_buf + 8);

        if (tag == 256) w = val;
        else if (tag == 257) h = val;
        else if (tag == 273) strip_offset = val;
    }

    if (w == 0 || h == 0 || strip_offset == 0) {
        return core::Result<core::Frame>::err(core::ErrorCode::SerializationError, "Missing required TIFF tags");
    }
    if (!core::frame_bytes(w, h, 1, 2).has_value()) {
        return core::Result<core::Frame>::err(core::ErrorCode::SerializationError, "TIFF dimensions exceed safe allocation limit");
    }

    core::Frame frame = core::Frame::create_allocated(w, h, 1, 16);
    ifs.seekg(strip_offset);
    ifs.read(reinterpret_cast<char*>(frame.pixels.get()), w * h * 2);

    return frame;
}

core::Result<void> ImageWriter::write_pgm_16bit(const std::string& file_path, const core::Frame& frame) {
    if (!frame.is_valid()) {
        return core::Result<void>::err(core::ErrorCode::InvalidState, "Cannot write invalid frame to PGM");
    }

    std::filesystem::path final_path(file_path);
    if (final_path.has_parent_path()) {
        std::filesystem::create_directories(final_path.parent_path());
    }

    std::string tmp_path = file_path + ".tmp";
    std::ofstream ofs(tmp_path, std::ios::binary);
    if (!ofs.is_open()) {
        return core::Result<void>::err(core::ErrorCode::PermissionDenied, "Failed to open temporary PGM file");
    }

    // PGM header (P5 = binary grayscale)
    ofs << "P5\n" << frame.width << " " << frame.height << "\n65535\n";

    // PGM 16-bit requires Big-Endian bytes
    size_t count = frame.pixel_count();
    const uint16_t* px = frame.pixels.get();
    std::vector<char> buf(count * 2);
    for (size_t i = 0; i < count; ++i) {
        uint16_t v = px[i];
        buf[i * 2 + 0] = static_cast<char>((v >> 8) & 0xFF);
        buf[i * 2 + 1] = static_cast<char>(v & 0xFF);
    }

    ofs.write(buf.data(), buf.size());
    ofs.flush();
    ofs.close();

    std::error_code ec;
    std::filesystem::rename(tmp_path, final_path, ec);
    if (ec) {
        return core::Result<void>::err(core::ErrorCode::HardwareError, "Atomic rename failed: " + ec.message());
    }

    return core::Result<void>::ok();
}

core::Result<core::Frame> ImageWriter::read_pgm_16bit(const std::string& file_path) {
    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs.is_open()) {
        return core::Result<core::Frame>::err(core::ErrorCode::NotFound, "Failed to open PGM file");
    }

    std::string magic;
    int w = 0, h = 0, max_val = 0;
    ifs >> magic >> w >> h >> max_val;
    ifs.get(); // skip single newline/whitespace char after max_val

    if (magic != "P5" || w <= 0 || h <= 0 || max_val <= 0) {
        return core::Result<core::Frame>::err(core::ErrorCode::SerializationError, "Invalid PGM header");
    }
    if (!core::frame_bytes(w, h, 1, 2).has_value()) {
        return core::Result<core::Frame>::err(core::ErrorCode::SerializationError, "PGM dimensions exceed safe allocation limit");
    }

    core::Frame frame = core::Frame::create_allocated(w, h, 1, 16);
    size_t count = static_cast<size_t>(w) * h;
    std::vector<char> buf(count * 2);
    ifs.read(buf.data(), count * 2);

    uint16_t* dst = frame.pixels.get();
    for (size_t i = 0; i < count; ++i) {
        uint8_t b0 = static_cast<uint8_t>(buf[i * 2 + 0]);
        uint8_t b1 = static_cast<uint8_t>(buf[i * 2 + 1]);
        dst[i] = (static_cast<uint16_t>(b0) << 8) | b1;
    }

    return frame;
}

core::Result<void> ImageWriter::write_ppm_8bit(const std::string& file_path, int width, int height, int channels, const uint8_t* data) {
    if (width <= 0 || height <= 0 || !data) {
        return core::Result<void>::err(core::ErrorCode::InvalidState, "Invalid arguments for PPM write");
    }

    std::filesystem::path final_path(file_path);
    if (final_path.has_parent_path()) {
        std::filesystem::create_directories(final_path.parent_path());
    }

    std::string tmp_path = file_path + ".tmp";
    std::ofstream ofs(tmp_path, std::ios::binary);
    if (!ofs.is_open()) {
        return core::Result<void>::err(core::ErrorCode::PermissionDenied, "Failed to open temporary PPM file");
    }

    if (channels == 1) {
        ofs << "P5\n" << width << " " << height << "\n255\n";
        ofs.write(reinterpret_cast<const char*>(data), width * height);
    } else {
        ofs << "P6\n" << width << " " << height << "\n255\n";
        ofs.write(reinterpret_cast<const char*>(data), width * height * 3);
    }
    ofs.flush();
    ofs.close();

    std::error_code ec;
    std::filesystem::rename(tmp_path, final_path, ec);
    if (ec) {
        return core::Result<void>::err(core::ErrorCode::HardwareError, "Atomic rename failed: " + ec.message());
    }

    return core::Result<void>::ok();
}

} // namespace sabit::imaging::io
