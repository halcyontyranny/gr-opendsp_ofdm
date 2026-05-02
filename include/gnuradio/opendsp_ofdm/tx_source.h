#pragma once
#include <gnuradio/sync_block.h>
#include "api.h"
#include <string>
#include <memory>

namespace gr {
namespace opendsp_ofdm {

// TX Source block — modulates text messages into a float32 OFDM sample stream.
//
// Connect a PMT string message source (e.g. QT GUI Message Edit Box) to the
// "msg_in" message port.  Connect the float output to an Audio Sink or SDR
// sink at 8000 Hz sample rate.
//
// Messages on msg_in may be:
//   - A PMT symbol/string:              pmt::intern("hello")
//   - A PDU pair (meta, data):          pmt::cons(meta, pmt::intern("hello"))
//   - A PDU pair with u8vector data:    pmt::cons(meta, pmt::init_u8vector(...))
//
// Multiple queued messages are transmitted back-to-back.  The output is silent
// (all zeros) when no message is pending.
class OPENDSP_OFDM_API tx_source : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<tx_source> sptr;

    // callsign  : operator callsign embedded in every frame (up to 6 chars)
    // force_tier: ACM tier 0–8, or -1 to let the engine adapt automatically
    // verbose   : print per-frame ACM status to stdout
    static sptr make(const std::string& callsign = "N0CALL",
                     int  force_tier             = -1,
                     bool verbose                = false);

    virtual void set_callsign(const std::string& callsign) = 0;
    virtual void set_force_tier(int tier) = 0;
    virtual void set_verbose(bool v) = 0;
};

} // namespace opendsp_ofdm
} // namespace gr
