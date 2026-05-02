#pragma once
#include <gnuradio/sync_block.h>
#include "api.h"
#include <string>
#include <memory>

namespace gr {
namespace opendsp_ofdm {

// RX Sink block — demodulates float32 OFDM samples and decodes frames.
//
// Connect a float32 Audio Source or SDR source running at 8000 Hz to the
// input.  Decoded frames appear on the "frame_out" message port as PDU pairs:
//   pmt::cons(metadata_dict, pmt::string_to_symbol(text))
//
// metadata_dict keys:
//   "callsign" -> PMT string  (sender callsign)
//   "tier"     -> PMT long    (ACM tier used)
//   "fec_ok"   -> PMT bool    (LDPC parity check result)
class OPENDSP_OFDM_API rx_sink : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<rx_sink> sptr;

    // force_tier: ACM tier 0–8, or -1 for adaptive (follow received header)
    // verbose   : print per-symbol SNR and ACM status to stdout
    static sptr make(const std::string& callsign = "N0CALL",
                     int  force_tier             = -1,
                     bool verbose                = false);

    virtual void set_force_tier(int tier) = 0;
    virtual void set_verbose(bool v) = 0;
};

} // namespace opendsp_ofdm
} // namespace gr
