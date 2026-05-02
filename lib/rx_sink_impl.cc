#include "rx_sink_impl.h"
#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace opendsp;

namespace gr {
namespace opendsp_ofdm {

rx_sink::sptr rx_sink::make(const std::string& callsign,
                             int  force_tier,
                             bool verbose)
{
    return gnuradio::make_block_sptr<rx_sink_impl>(callsign, force_tier, verbose);
}

rx_sink_impl::rx_sink_impl(const std::string& callsign,
                            int  force_tier,
                            bool verbose)
    : gr::sync_block("opendsp_ofdm_rx_sink",
                     gr::io_signature::make(1, 1, sizeof(float)),
                     gr::io_signature::make(0, 0, 0)),
      d_callsign(callsign),
      d_verbose(verbose),
      d_force_tier(force_tier)
{
    if (force_tier >= 0)
        d_acm.force_tier(force_tier);

    message_port_register_out(pmt::intern("frame_out"));
    update_params();
}

void rx_sink_impl::set_force_tier(int tier)
{
    d_force_tier = tier;
    if (tier >= 0) {
        d_acm.force_tier(tier);
        update_params();
    }
}

// ── update_params ─────────────────────────────────────────────────────────────

void rx_sink_impl::update_params()
{
    d_params = d_acm.current_params();
    d_demod  = std::make_unique<OFDMDemodulator>(d_params);
    d_zc_ref = zadoff_chu(d_params.fft_size / 4);

    int tier = d_acm.state().tier_index;
    d_accum  = std::make_unique<FECAccumulator>(fec_code_for_tier(tier),
                                                 passes_for_tier(tier));
    d_pass_llrs.clear();
    d_block_buf.clear();
    d_blocks_received = 0;
    d_expected_blocks = 1;
}

// ── work ──────────────────────────────────────────────────────────────────────

int rx_sink_impl::work(int noutput_items,
                        gr_vector_const_void_star& input_items,
                        gr_vector_void_star& /*output_items*/)
{
    const float* in = static_cast<const float*>(input_items[0]);
    for (int i = 0; i < noutput_items; i++)
        d_sample_buf.push_back(static_cast<double>(in[i]));

    process();
    return noutput_items;
}

// ── process ───────────────────────────────────────────────────────────────────
//
// Mirrors RXPipeline::on_samples() from main.cpp, adapted for GR message output.

void rx_sink_impl::process()
{
    int sym_len    = d_params.fft_size + d_params.cp_len;
    int search_len = sym_len * 4;

    while (static_cast<int>(d_sample_buf.size()) >= search_len) {
        RealVec window(d_sample_buf.begin(),
                       d_sample_buf.begin() + search_len);

        int sync_pos = d_demod->find_sync(window, d_zc_ref);
        if (sync_pos < 0) {
            d_sample_buf.erase(d_sample_buf.begin(),
                               d_sample_buf.begin() + sym_len / 2);
            return;
        }

        int data_start = sync_pos + static_cast<int>(d_zc_ref.size());
        if (data_start + sym_len > static_cast<int>(d_sample_buf.size())) return;

        RealVec sym_samples(d_sample_buf.begin() + data_start,
                            d_sample_buf.begin() + data_start + sym_len);

        CxVec rx_sc = d_demod->demodulate_symbol(sym_samples);
        d_demod->update_channel_estimate(rx_sc, d_sym_count, d_ch_est);
        auto llr = d_demod->equalise_and_demap(rx_sc, d_ch_est);

        // Accumulate LLRs pass-by-pass
        for (auto v : llr) d_pass_llrs.push_back(static_cast<float>(v));

        while (d_accum && static_cast<int>(d_pass_llrs.size()) >= d_accum->coded_bits()) {
            std::vector<float> one_pass(d_pass_llrs.begin(),
                                        d_pass_llrs.begin() + d_accum->coded_bits());
            d_pass_llrs.erase(d_pass_llrs.begin(),
                              d_pass_llrs.begin() + d_accum->coded_bits());

            if (d_accum->add_pass(one_pass)) {
                std::vector<uint8_t> decoded_bits;
                bool parity_ok = d_accum->decode(decoded_bits);
                d_accum->reset();
                auto decoded_bytes = bits_to_bytes(decoded_bits);

                // First block: extract num_blocks from header byte 3
                if (d_blocks_received == 0 && decoded_bytes.size() > 3) {
                    d_expected_blocks = decoded_bytes[3];
                    if (d_expected_blocks == 0) d_expected_blocks = 1;
                    if (d_expected_blocks > 1 && d_verbose) {
                        int data_sc  = d_params.num_subcarriers
                                     - d_params.num_subcarriers / d_params.pilot_interval;
                        int bps      = data_sc * d_params.bits_per_symbol;
                        int np       = passes_for_tier(d_acm.state().tier_index);
                        int syms     = (d_accum->coded_bits() + bps - 1) / bps;
                        double secs  = d_expected_blocks * np * syms
                                     * static_cast<double>(sym_len) / SAMPLE_RATE;
                        std::cout << "[RX] " << d_expected_blocks << "-block frame (~"
                                  << std::fixed << std::setprecision(1)
                                  << secs << " s) — hold TX\n";
                    }
                }

                d_block_buf.insert(d_block_buf.end(),
                                   decoded_bytes.begin(), decoded_bytes.end());
                d_blocks_received++;

                if (d_blocks_received >= d_expected_blocks) {
                    Framer framer;
                    Frame  frame = framer.parse(d_block_buf);

                    if (frame.crc_ok) {
                        // Build decoded text string
                        std::string text(frame.payload.begin(), frame.payload.end());

                        // Build metadata dict
                        pmt::pmt_t meta = pmt::make_dict();
                        meta = pmt::dict_add(meta, pmt::intern("callsign"),
                                             pmt::string_to_symbol(frame.callsign));
                        meta = pmt::dict_add(meta, pmt::intern("tier"),
                                             pmt::from_long(frame.header.tier_index));
                        meta = pmt::dict_add(meta, pmt::intern("fec_ok"),
                                             pmt::from_bool(parity_ok));

                        message_port_pub(pmt::intern("frame_out"),
                                         pmt::cons(meta, pmt::string_to_symbol(text)));

                        if (d_verbose)
                            std::cout << "[RX] " << frame.callsign << ": \""
                                      << text << "\" [FEC:"
                                      << (parity_ok ? "OK" : "err") << "]\n";
                    }

                    d_block_buf.clear();
                    d_blocks_received = 0;
                    d_expected_blocks = 1;
                }
            }
        }

        // Update ACM with measured SNR
        bool tier_changed = d_acm.update(d_ch_est.snr_db, 0.0);
        if (tier_changed && d_force_tier < 0) {
            if (d_verbose)
                std::cout << "[ACM] " << d_acm.status_string() << "\n";
            update_params();
            return;  // params changed; restart on next work() call
        }

        if (d_verbose)
            std::cout << "\r[RX] sym=" << d_sym_count
                      << " SNR=" << std::fixed << std::setprecision(1)
                      << d_ch_est.snr_db << " dB  " << d_acm.status_string()
                      << std::flush;

        d_sym_count++;
        d_sample_buf.erase(d_sample_buf.begin(),
                           d_sample_buf.begin() + data_start + sym_len);
    }
}

} // namespace opendsp_ofdm
} // namespace gr
