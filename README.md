# gr-opendsp_ofdm

GNU Radio 3.10 Out-of-Tree module providing GRC blocks for the
[opendsp-ofdm](https://github.com/halcyontyranny/opendsp-ofdm) adaptive HF
OFDM modem.

---

## Blocks

### OpenDSP OFDM TX Source (`opendsp_ofdm_tx_source`)

Float32 source block. Receives text messages via a GRC message port and outputs
modulated OFDM samples at **8000 Hz**. Output is silent (zeros) when no
message is queued; messages back-to-back are transmitted consecutively.

| Port / Parameter | Type | Description |
|---|---|---|
| `msg_in` | message in | PMT symbol, string, or PDU pair containing text to transmit |
| `out` | float stream | Modulated OFDM audio at 8000 sps |
| Callsign | string | Your callsign, embedded in every frame (up to 6 chars) |
| ACM Tier | int | 0–8 to force a tier, -1 for adaptive ACM |
| Verbose | bool | Print per-transmission status to console |

**Message formats accepted on `msg_in`:**
- `pmt.intern("hello")` — PMT symbol (from QT GUI Message Edit Box)
- `pmt.cons(meta, pmt.intern("hello"))` — PDU pair with symbol payload
- `pmt.cons(meta, pmt.init_u8vector(n, data))` — PDU pair with byte vector

### OpenDSP OFDM RX Sink (`opendsp_ofdm_rx_sink`)

Float32 sink block. Demodulates 8000 Hz OFDM samples and posts decoded frames
to a GRC message port.

| Port / Parameter | Type | Description |
|---|---|---|
| `in` | float stream | OFDM audio at 8000 sps |
| `frame_out` | message out | PDU pair: `pmt.cons(meta_dict, text_symbol)` |
| Callsign | string | Local callsign (informational) |
| ACM Tier | int | 0–8 to lock receive tier, -1 to follow sender's ACM header |
| Verbose | bool | Print per-symbol SNR, ACM status, and decoded frames to console |

**Output PDU format on `frame_out`:**
```
pmt.cons(meta_dict, pmt.string_to_symbol(decoded_text))
```
`meta_dict` keys: `"callsign"` (string), `"tier"` (long), `"fec_ok"` (bool).

---

## ACM Tier Reference

| Tier | BW | Mod | Code | Passes | Net bps | Min SNR |
|------|----|-----|------|--------|---------|---------|
| 0 | 50 Hz | BPSK | 1/3 | 4 | ~2 | −20 dB |
| 1 | 500 Hz | BPSK | 1/3 | 1 | ~88 | −12 dB |
| 2 | 1 kHz | BPSK | 1/2 | 1 | ~265 | −8 dB |
| 3 | 1.5 kHz | QPSK | 1/2 | 1 | ~796 | −5 dB |
| 4 | 2 kHz | QPSK | 1/2 | 1 | ~1.1k | −2 dB |
| 5 | 2.5 kHz | QPSK | 1/2 | 1 | ~1.3k | +1 dB |
| 6 | 3 kHz | 8-PSK | 3/4 | 1 | ~3.6k | +4 dB |
| 7 | 3.2 kHz | 8-PSK | 3/4 | 1 | ~3.8k | +7 dB |
| 8 | 3.5 kHz | 16-QAM | 3/4 | 1 | ~5.6k | +10 dB |

Tier 0 ("survival mode") takes ~100 seconds to transmit a short message and
decodes down to approximately −25 dB SNR in a 2500 Hz reference bandwidth.

---

## Dependencies

```bash
# Ubuntu / Debian
sudo apt install gnuradio gnuradio-dev libcodec2-dev libfftw3-dev \
                 pybind11-dev cmake build-essential
```

The opendsp-ofdm library must be built first:

```bash
git clone https://github.com/halcyontyranny/opendsp-ofdm
cd opendsp-ofdm && mkdir build && cd build
cmake .. -DWITH_CODEC2=ON \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_POSITION_INDEPENDENT_CODE=ON
make -j$(nproc)
```

> **Important:** `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` is required.
> Without it, the static library cannot be linked into the GNU Radio shared
> object and the build will fail with a relocation error.

---

## Build and Install

```bash
cd gr-opendsp_ofdm && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DOPENDSP_OFDM_DIR=/path/to/opendsp-ofdm \
         -DOPENDSP_OFDM_BUILD_DIR=/path/to/opendsp-ofdm/build
make -j$(nproc)
sudo make install
sudo ldconfig
```

If `opendsp-ofdm` is in the sibling directory (`../opendsp-ofdm`) the path
arguments can be omitted — that is the default.

---

## Typical GRC Flowgraph

### TX
```
[QT GUI Message Edit Box] ──msg──> [OpenDSP OFDM TX Source] ──float──> [Audio Sink @ 8000 sps]
```

### RX
```
[Audio Source @ 8000 sps] ──float──> [OpenDSP OFDM RX Sink] ──msg──> [QT GUI Message Edit Box]
```

For SDR hardware, replace the Audio Source/Sink with an appropriate SDR block
(e.g. osmocom Source/Sink) and set the sample rate to 8000.

---

## License

MIT — see LICENSE file in the opendsp-ofdm repository.
