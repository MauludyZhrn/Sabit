#include "src/imaging/io/mp4_writer.h"
#include <filesystem>
#include <algorithm>
#include <cstring>

namespace sabit::imaging::io {

static inline void write_be32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
}

static inline void write_be16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
}

static inline void write_tag(std::vector<uint8_t>& buf, const char* tag) {
    buf.push_back(static_cast<uint8_t>(tag[0]));
    buf.push_back(static_cast<uint8_t>(tag[1]));
    buf.push_back(static_cast<uint8_t>(tag[2]));
    buf.push_back(static_cast<uint8_t>(tag[3]));
}

static inline void write_be32_stream(std::ostream& os, uint32_t val) {
    char b[4] = {
        static_cast<char>((val >> 24) & 0xFF),
        static_cast<char>((val >> 16) & 0xFF),
        static_cast<char>((val >> 8) & 0xFF),
        static_cast<char>(val & 0xFF)
    };
    os.write(b, 4);
}

static inline void write_tag_stream(std::ostream& os, const char* tag) {
    os.write(tag, 4);
}

Mp4Writer::~Mp4Writer() {
    close();
}

core::Result<void> Mp4Writer::open(const std::string& file_path, int width, int height, double fps) {
    close();

    if (width <= 0 || height <= 0) {
        return core::Result<void>::err(core::ErrorCode::OutOfRange, "Dimensi video tidak valid untuk MP4");
    }

    try {
        std::filesystem::path p(file_path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (const std::exception& e) {
        return core::Result<void>::err(core::ErrorCode::SerializationError, std::string("Gagal membuat direktori MP4: ") + e.what());
    }

    file_.open(file_path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
        return core::Result<void>::err(core::ErrorCode::SerializationError, "Gagal membuka file MP4 untuk penulisan");
    }

    file_path_ = file_path;
    width_ = width;
    height_ = height;
    fps_ = (fps > 0.0) ? fps : 30.0;
    frame_count_ = 0;
    frame_data_size_ = static_cast<uint32_t>(width_ * height_ * 3);
    frame_buffer_.assign(frame_data_size_, 0);
    chunk_offsets_.clear();

    // 1. Write 'ftyp' box (24 bytes)
    write_be32_stream(file_, 24);
    write_tag_stream(file_, "ftyp");
    write_tag_stream(file_, "isom");
    write_be32_stream(file_, 0x00000200);
    write_tag_stream(file_, "isom");
    write_tag_stream(file_, "mp42");

    // 2. Write 'mdat' box header (size updated on close)
    pos_mdat_size_ = file_.tellp();
    write_be32_stream(file_, 0); // Placeholder for mdat size
    write_tag_stream(file_, "mdat");
    pos_mdat_data_start_ = file_.tellp();

    is_open_ = true;
    return core::Result<void>::ok();
}

core::Result<void> Mp4Writer::write_frame(const core::Frame& frame) {
    if (!is_open_) {
        return core::Result<void>::err(core::ErrorCode::InvalidState, "File MP4 belum dibuka");
    }

    if (frame.width != width_ || frame.height != height_) {
        return core::Result<void>::err(core::ErrorCode::OutOfRange, "Dimensi frame tidak cocok dengan konfigurasi MP4");
    }

    const uint16_t* src_u16 = frame.pixels ? frame.pixels.get() : nullptr;
    const uint8_t* src_u8 = reinterpret_cast<const uint8_t*>(src_u16);
    bool is_16bit = (frame.bit_depth == 16);
    int ch = frame.channels;
    size_t total_px = static_cast<size_t>(width_) * static_cast<size_t>(height_);

    for (size_t i = 0; i < total_px; ++i) {
        uint8_t r = 0, g = 0, b = 0;
        size_t px_idx = i * static_cast<size_t>(ch);
        if (is_16bit && src_u16) {
            if (ch == 1) {
                r = g = b = static_cast<uint8_t>(src_u16[px_idx] >> 8);
            } else if (ch >= 3) {
                r = static_cast<uint8_t>(src_u16[px_idx + 0] >> 8);
                g = static_cast<uint8_t>(src_u16[px_idx + 1] >> 8);
                b = static_cast<uint8_t>(src_u16[px_idx + 2] >> 8);
            }
        } else if (src_u8) {
            if (ch == 1) {
                r = g = b = src_u8[px_idx];
            } else if (ch >= 3) {
                r = src_u8[px_idx + 0];
                g = src_u8[px_idx + 1];
                b = src_u8[px_idx + 2];
            }
        }
        frame_buffer_[i * 3 + 0] = r;
        frame_buffer_[i * 3 + 1] = g;
        frame_buffer_[i * 3 + 2] = b;
    }

    std::streampos sample_pos = file_.tellp();
    chunk_offsets_.push_back(static_cast<uint32_t>(static_cast<std::streamoff>(sample_pos)));

    file_.write(reinterpret_cast<const char*>(frame_buffer_.data()), frame_data_size_);
    ++frame_count_;

    return core::Result<void>::ok();
}

void Mp4Writer::close() {
    if (!is_open_) return;

    std::streampos mdat_end = file_.tellp();
    uint32_t mdat_size = static_cast<uint32_t>(static_cast<std::streamoff>(mdat_end) - static_cast<std::streamoff>(pos_mdat_size_));

    // Update mdat size
    file_.seekp(pos_mdat_size_);
    write_be32_stream(file_, mdat_size);
    file_.seekp(mdat_end);

    uint32_t timescale = 1000;
    uint32_t sample_delta = static_cast<uint32_t>(1000.0 / fps_);
    if (sample_delta == 0) sample_delta = 33;
    uint32_t duration = frame_count_ * sample_delta;

    // Build 'stsd' (Sample Description Box)
    std::vector<uint8_t> stsd;
    write_be32(stsd, 0); // size placeholder
    write_tag(stsd, "stsd");
    write_be32(stsd, 0); // version & flags
    write_be32(stsd, 1); // entry_count = 1
    // VisualSampleEntry 'raw ' (86 bytes)
    write_be32(stsd, 86);
    write_tag(stsd, "raw ");
    for (int i = 0; i < 6; ++i) stsd.push_back(0); // reserved
    write_be16(stsd, 1); // data_reference_index
    write_be16(stsd, 0); // pre_defined
    write_be16(stsd, 0); // reserved
    write_be32(stsd, 0); write_be32(stsd, 0); write_be32(stsd, 0); // pre_defined[3]
    write_be16(stsd, static_cast<uint16_t>(width_));
    write_be16(stsd, static_cast<uint16_t>(height_));
    write_be32(stsd, 0x00480000); // horizresolution 72 dpi
    write_be32(stsd, 0x00480000); // vertresolution 72 dpi
    write_be32(stsd, 0); // reserved
    write_be16(stsd, 1); // frame_count = 1
    for (int i = 0; i < 32; ++i) stsd.push_back(0); // compressorname
    write_be16(stsd, 24); // depth = 24-bit RGB
    write_be16(stsd, 0xFFFF); // pre_defined = -1
    uint32_t stsd_sz = static_cast<uint32_t>(stsd.size());
    stsd[0] = (stsd_sz >> 24) & 0xFF; stsd[1] = (stsd_sz >> 16) & 0xFF;
    stsd[2] = (stsd_sz >> 8) & 0xFF;  stsd[3] = stsd_sz & 0xFF;

    // Build 'stts' (Time-to-Sample Box)
    std::vector<uint8_t> stts;
    write_be32(stts, 24);
    write_tag(stts, "stts");
    write_be32(stts, 0);
    write_be32(stts, 1);
    write_be32(stts, frame_count_);
    write_be32(stts, sample_delta);

    // Build 'stsc' (Sample-to-Chunk Box)
    std::vector<uint8_t> stsc;
    write_be32(stsc, 28);
    write_tag(stsc, "stsc");
    write_be32(stsc, 0);
    write_be32(stsc, 1);
    write_be32(stsc, 1); // first_chunk = 1
    write_be32(stsc, 1); // samples_per_chunk = 1
    write_be32(stsc, 1); // sample_description_index = 1

    // Build 'stsz' (Sample Size Box)
    std::vector<uint8_t> stsz;
    write_be32(stsz, 20);
    write_tag(stsz, "stsz");
    write_be32(stsz, 0);
    write_be32(stsz, frame_data_size_);
    write_be32(stsz, frame_count_);

    // Build 'stco' (Chunk Offset Box)
    std::vector<uint8_t> stco;
    write_be32(stco, static_cast<uint32_t>(16 + chunk_offsets_.size() * 4));
    write_tag(stco, "stco");
    write_be32(stco, 0);
    write_be32(stco, static_cast<uint32_t>(chunk_offsets_.size()));
    for (uint32_t off : chunk_offsets_) {
        write_be32(stco, off);
    }

    // Build 'stbl'
    std::vector<uint8_t> stbl;
    uint32_t stbl_sz = static_cast<uint32_t>(8 + stsd.size() + stts.size() + stsc.size() + stsz.size() + stco.size());
    write_be32(stbl, stbl_sz);
    write_tag(stbl, "stbl");
    stbl.insert(stbl.end(), stsd.begin(), stsd.end());
    stbl.insert(stbl.end(), stts.begin(), stts.end());
    stbl.insert(stbl.end(), stsc.begin(), stsc.end());
    stbl.insert(stbl.end(), stsz.begin(), stsz.end());
    stbl.insert(stbl.end(), stco.begin(), stco.end());

    // Build 'vmhd' (20 bytes) & 'dinf' (36 bytes)
    std::vector<uint8_t> vmhd;
    write_be32(vmhd, 20);
    write_tag(vmhd, "vmhd");
    write_be32(vmhd, 1); // version 0, flags 1
    write_be16(vmhd, 0); // graphicsmode
    write_be16(vmhd, 0); write_be16(vmhd, 0); write_be16(vmhd, 0); // opcolor

    std::vector<uint8_t> dinf;
    write_be32(dinf, 36);
    write_tag(dinf, "dinf");
    write_be32(dinf, 28);
    write_tag(dinf, "dref");
    write_be32(dinf, 0);
    write_be32(dinf, 1);
    write_be32(dinf, 12);
    write_tag(dinf, "url ");
    write_be32(dinf, 1); // flag 1 = self-contained

    // Build 'minf'
    std::vector<uint8_t> minf;
    uint32_t minf_sz = static_cast<uint32_t>(8 + vmhd.size() + dinf.size() + stbl.size());
    write_be32(minf, minf_sz);
    write_tag(minf, "minf");
    minf.insert(minf.end(), vmhd.begin(), vmhd.end());
    minf.insert(minf.end(), dinf.begin(), dinf.end());
    minf.insert(minf.end(), stbl.begin(), stbl.end());

    // Build 'mdhd' (32 bytes) & 'hdlr' (45 bytes)
    std::vector<uint8_t> mdhd;
    write_be32(mdhd, 32);
    write_tag(mdhd, "mdhd");
    write_be32(mdhd, 0); // version & flags
    write_be32(mdhd, 0); // creation_time
    write_be32(mdhd, 0); // modification_time
    write_be32(mdhd, timescale);
    write_be32(mdhd, duration);
    write_be16(mdhd, 0x55C4); // language 'und'
    write_be16(mdhd, 0);      // pre_defined

    std::vector<uint8_t> hdlr;
    write_be32(hdlr, 45);
    write_tag(hdlr, "hdlr");
    write_be32(hdlr, 0);
    write_be32(hdlr, 0);
    write_tag(hdlr, "vide");
    write_be32(hdlr, 0); write_be32(hdlr, 0); write_be32(hdlr, 0);
    const char* hname = "VideoHandler";
    for (int i = 0; i < 13; ++i) hdlr.push_back(static_cast<uint8_t>(hname[i]));

    // Build 'mdia'
    std::vector<uint8_t> mdia;
    uint32_t mdia_sz = static_cast<uint32_t>(8 + mdhd.size() + hdlr.size() + minf.size());
    write_be32(mdia, mdia_sz);
    write_tag(mdia, "mdia");
    mdia.insert(mdia.end(), mdhd.begin(), mdhd.end());
    mdia.insert(mdia.end(), hdlr.begin(), hdlr.end());
    mdia.insert(mdia.end(), minf.begin(), minf.end());

    // Build 'tkhd' (92 bytes)
    std::vector<uint8_t> tkhd;
    write_be32(tkhd, 92);
    write_tag(tkhd, "tkhd");
    write_be32(tkhd, 0x00000003); // flags: track_enabled | track_in_movie
    write_be32(tkhd, 0); // creation_time
    write_be32(tkhd, 0); // modification_time
    write_be32(tkhd, 1); // track_ID = 1
    write_be32(tkhd, 0); // reserved
    write_be32(tkhd, duration);
    write_be32(tkhd, 0); write_be32(tkhd, 0); // reserved[2]
    write_be16(tkhd, 0); // layer
    write_be16(tkhd, 0); // alternate_group
    write_be16(tkhd, 0); // volume (0 for video)
    write_be16(tkhd, 0); // reserved
    // Unity matrix (9 * 4 bytes)
    write_be32(tkhd, 0x00010000); write_be32(tkhd, 0); write_be32(tkhd, 0);
    write_be32(tkhd, 0); write_be32(tkhd, 0x00010000); write_be32(tkhd, 0);
    write_be32(tkhd, 0); write_be32(tkhd, 0); write_be32(tkhd, 0x40000000);
    write_be32(tkhd, static_cast<uint32_t>(width_) << 16);
    write_be32(tkhd, static_cast<uint32_t>(height_) << 16);

    // Build 'trak'
    std::vector<uint8_t> trak;
    uint32_t trak_sz = static_cast<uint32_t>(8 + tkhd.size() + mdia.size());
    write_be32(trak, trak_sz);
    write_tag(trak, "trak");
    trak.insert(trak.end(), tkhd.begin(), tkhd.end());
    trak.insert(trak.end(), mdia.begin(), mdia.end());

    // Build 'mvhd' (108 bytes)
    std::vector<uint8_t> mvhd;
    write_be32(mvhd, 108);
    write_tag(mvhd, "mvhd");
    write_be32(mvhd, 0); // version & flags
    write_be32(mvhd, 0); // creation_time
    write_be32(mvhd, 0); // modification_time
    write_be32(mvhd, timescale);
    write_be32(mvhd, duration);
    write_be32(mvhd, 0x00010000); // rate = 1.0
    write_be16(mvhd, 0x0100);     // volume = 1.0
    write_be16(mvhd, 0);          // reserved
    write_be32(mvhd, 0); write_be32(mvhd, 0); // reserved[2]
    // Unity matrix
    write_be32(mvhd, 0x00010000); write_be32(mvhd, 0); write_be32(mvhd, 0);
    write_be32(mvhd, 0); write_be32(mvhd, 0x00010000); write_be32(mvhd, 0);
    write_be32(mvhd, 0); write_be32(mvhd, 0); write_be32(mvhd, 0x40000000);
    for (int i = 0; i < 6; ++i) write_be32(mvhd, 0); // pre_defined[6]
    write_be32(mvhd, 2); // next_track_ID = 2

    // Build 'moov'
    std::vector<uint8_t> moov;
    uint32_t moov_sz = static_cast<uint32_t>(8 + mvhd.size() + trak.size());
    write_be32(moov, moov_sz);
    write_tag(moov, "moov");
    moov.insert(moov.end(), mvhd.begin(), mvhd.end());
    moov.insert(moov.end(), trak.begin(), trak.end());

    file_.write(reinterpret_cast<const char*>(moov.data()), static_cast<std::streamsize>(moov.size()));
    file_.flush();
    file_.close();
    is_open_ = false;
}

} // namespace sabit::imaging::io
