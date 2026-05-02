#pragma once
#include <gnuradio/opendsp_ofdm/tx_source.h>
#include <pmt/pmt.h>
#include <mutex>
#include <deque>
#include <memory>
#include <string>
#include "acm.h"
#include "ofdm_core.h"
#include "fec.h"
#include "framer.h"

namespace gr {
namespace opendsp_ofdm {

class tx_source_impl : public tx_source
{
public:
    tx_source_impl(const std::string& callsign, int force_tier, bool verbose);

    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;

    void set_callsign(const std::string& cs) override;
    void set_force_tier(int tier)            override;
    void set_verbose(bool v)                 override { d_verbose = v; }

private:
    void handle_msg(pmt::pmt_t msg);
    void enqueue_frame(const std::string& text);

    std::string               d_callsign;
    bool                      d_verbose;
    ::opendsp::ACMEngine      d_acm;

    std::mutex                d_mtx;
    std::deque<float>         d_queue;  // sample queue; drained by work()
};

} // namespace opendsp_ofdm
} // namespace gr
