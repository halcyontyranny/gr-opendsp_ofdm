#include "tx_source_impl.h"
#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

using namespace opendsp;

namespace gr {
namespace opendsp_ofdm {

tx_source::sptr tx_source::make(const std::string& callsign,
                                 int  force_tier,
                                 bool verbose)
{
    return gnuradio::make_block_sptr<tx_source_impl>(callsign, force_tier, verbose);
}

tx_source_impl::tx_source_impl(const std::string& callsign,
                                int  force_tier,
                                bool verbose)
    : gr::sync_block("opendsp_ofdm_tx_source",
                     gr::io_signature::make(0, 0, 0),
                     gr::io_signature::make(1, 1, sizeof(float))),
      d_callsign(callsign),
      d_verbose(verbose)
{
    if (force_tier >= 0)
        d_acm.force_tier(force_tier);

    message_port_register_in(pmt::intern("msg_in"));
    set_msg_handler(pmt::intern("msg_in"),
        [this](pmt::pmt_t msg) { handle_msg(msg); });
}

void tx_source_impl::set_callsign(const std::string& cs)
{
    std::lock_guard<std::mutex> lock(d_mtx);
    d_callsign = cs;
}

void tx_source_impl::set_force_tier(int tier)
{
    if (tier >= 0)
        d_acm.force_tier(tier);
}

// ── message handler ───────────────────────────────────────────────────────────

void tx_source_impl::handle_msg(pmt::pmt_t msg)
{
    std::string text;

    if (pmt::is_symbol(msg)) {
        text = pmt::symbol_to_string(msg);
    } else if (pmt::is_pair(msg)) {
        pmt::pmt_t data = pmt::cdr(msg);
        if (pmt::is_symbol(data))
            text = pmt::symbol_to_string(data);
        else if (pmt::is_u8vector(data)) {
            auto v = pmt::u8vector_elements(data);
            text = std::string(v.begin(), v.end());
        }
    }

    if (text.empty()) return;
    enqueue_frame(text);
}

// ── frame builder ─────────────────────────────────────────────────────────────
//
// Mirrors build_tx_frame() in main.cpp; outputs float samples into d_queue.

void tx_source_impl::enqueue_frame(const std::string& text)
{
    OFDMParams    params = d_acm.current_params();
    OFDMModulator mod(params);
    Framer        framer;

    // Assemble frame header
    Frame f;
    {
        std::lock_guard<std::mutex> lock(d_mtx);
        f.callsign = d_callsign;
    }
    f.header.tier_index  = static_cast<uint8_t>(d_acm.state().tier_index);
    f.header.max_bw_code = static_cast<uint8_t>(d_acm.num_tiers() - 1);
    f.header.frame_type  = FrameType::DATA;
    for (char c : text) f.payload.push_back(static_cast<uint8_t>(c));

    // FEC encode — compute num_blocks, pad, encode each block N passes
    int num_passes  = passes_for_tier(d_acm.state().tier_index);
    FECCodec fec(fec_code_for_tier(d_acm.state().tier_index));
    int block_bytes   = fec.data_bits() / 8;
    int content_bytes = ACM_HDR_BYTES + CALLSIGN_BYTES
                      + static_cast<int>(f.payload.size()) + CRC32_BYTES;
    int num_blocks    = (content_bytes + block_bytes - 1) / block_bytes;
    int target_bytes  = num_blocks * block_bytes;

    f.header.num_blocks = static_cast<uint8_t>(num_blocks);
    auto raw_bytes = framer.build(f, target_bytes);
    auto all_bits  = bytes_to_bits(raw_bytes);

    std::vector<uint8_t> coded_bits;
    for (int off = 0; off < static_cast<int>(all_bits.size()); off += fec.data_bits()) {
        std::vector<uint8_t> block(all_bits.begin() + off,
                                    all_bits.begin() + off + fec.data_bits());
        auto codeword = fec.encode(block);
        for (int p = 0; p < num_passes; p++)
            coded_bits.insert(coded_bits.end(), codeword.begin(), codeword.end());
    }

    // Pad bit stream to integer OFDM symbols
    int data_sc      = params.num_subcarriers - params.num_subcarriers / params.pilot_interval;
    int bits_per_sym = data_sc * params.bits_per_symbol;
    while (static_cast<int>(coded_bits.size()) % bits_per_sym != 0)
        coded_bits.push_back(0);

    // Preamble: Zadoff-Chu
    CxVec   zc = zadoff_chu(params.fft_size / 4);
    RealVec output;
    for (auto& s : zc) output.push_back(s.real() * 0.5);
    while (static_cast<int>(output.size()) < params.fft_size + params.cp_len)
        output.push_back(0.0);

    // Pilot OFDM symbol
    CxVec pilot_sc(params.num_subcarriers);
    mod.insert_pilots(pilot_sc, 0);
    auto pilot_samples = mod.modulate_symbol(pilot_sc);
    for (auto s : pilot_samples) output.push_back(s);

    // Data symbols
    int bit_idx = 0;
    while (bit_idx < static_cast<int>(coded_bits.size())) {
        CxVec sc(params.num_subcarriers);
        int local_bit = bit_idx;
        for (int i = 0; i < params.num_subcarriers; i++) {
            if (i % params.pilot_interval == 0) { sc[i] = cx(1, 0); continue; }
            uint8_t sym_bits = 0;
            for (int b = 0; b < params.bits_per_symbol; b++) {
                if (local_bit < static_cast<int>(coded_bits.size()))
                    sym_bits |= (coded_bits[local_bit++] & 1) << (params.bits_per_symbol-1-b);
            }
            sc[i] = map_symbol(sym_bits, params.bits_per_symbol);
        }
        mod.insert_pilots(sc, static_cast<int>(output.size() / (params.fft_size + params.cp_len)));
        auto sym_samples = mod.modulate_symbol(sc);
        for (auto s : sym_samples) output.push_back(s);
        bit_idx = local_bit;
    }

    // Normalise to ±0.9
    double peak = 0.0;
    for (auto s : output) peak = std::max(peak, std::abs(s));
    if (peak > 0.0) for (auto& s : output) s *= 0.9 / peak;

    if (d_verbose)
        std::cout << "[TX] \"" << text << "\" — " << output.size() << " samples ("
                  << std::fixed << std::setprecision(1)
                  << static_cast<double>(output.size()) / SAMPLE_RATE << " s) "
                  << d_acm.status_string() << "\n";

    // Push to output queue
    std::lock_guard<std::mutex> lock(d_mtx);
    for (double s : output) d_queue.push_back(static_cast<float>(s));
}

// ── work ──────────────────────────────────────────────────────────────────────

int tx_source_impl::work(int noutput_items,
                          gr_vector_const_void_star& /*input_items*/,
                          gr_vector_void_star& output_items)
{
    float* out = static_cast<float*>(output_items[0]);

    std::lock_guard<std::mutex> lock(d_mtx);
    int n = std::min(noutput_items, static_cast<int>(d_queue.size()));
    for (int i = 0; i < n; i++) {
        out[i] = d_queue.front();
        d_queue.pop_front();
    }
    // Silence while idle
    for (int i = n; i < noutput_items; i++) out[i] = 0.0f;

    return noutput_items;
}

} // namespace opendsp_ofdm
} // namespace gr
