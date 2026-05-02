#pragma once
#include <gnuradio/opendsp_ofdm/rx_sink.h>
#include <pmt/pmt.h>
#include <memory>
#include <vector>
#include <string>
#include "acm.h"
#include "ofdm_core.h"
#include "fec.h"
#include "framer.h"

namespace gr {
namespace opendsp_ofdm {

class rx_sink_impl : public rx_sink
{
public:
    rx_sink_impl(const std::string& callsign, int force_tier, bool verbose);

    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;

    void set_force_tier(int tier) override;
    void set_verbose(bool v)      override { d_verbose = v; }

private:
    void update_params();
    void process();   // run sync/demod loop on d_sample_buf

    std::string d_callsign;
    bool        d_verbose;
    int         d_force_tier;

    ::opendsp::ACMEngine                            d_acm;
    ::opendsp::OFDMParams                           d_params;
    std::unique_ptr<::opendsp::OFDMDemodulator>     d_demod;
    ::opendsp::ChannelEstimate                      d_ch_est;
    ::opendsp::CxVec                                d_zc_ref;
    ::opendsp::RealVec                              d_sample_buf;
    int                                             d_sym_count = 0;

    // FEC / accumulator state
    std::unique_ptr<::opendsp::FECAccumulator>      d_accum;
    std::vector<float>                              d_pass_llrs;
    std::vector<uint8_t>                            d_block_buf;
    int                                             d_blocks_received = 0;
    int                                             d_expected_blocks = 1;
};

} // namespace opendsp_ofdm
} // namespace gr
