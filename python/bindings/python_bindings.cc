#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#include <gnuradio/opendsp_ofdm/tx_source.h>
#include <gnuradio/opendsp_ofdm/rx_sink.h>

PYBIND11_MODULE(opendsp_ofdm_python, m)
{
    // Pull in GR base class types so pybind11 knows about gr::sync_block.
    py::module::import("gnuradio.gr");

    // ── tx_source ─────────────────────────────────────────────────────────────
    py::class_<gr::opendsp_ofdm::tx_source,
               gr::sync_block,
               std::shared_ptr<gr::opendsp_ofdm::tx_source>>(m, "tx_source")
        .def(py::init([](const std::string& callsign, int force_tier, bool verbose) {
                 return gr::opendsp_ofdm::tx_source::make(callsign, force_tier, verbose);
             }),
             py::arg("callsign")    = "N0CALL",
             py::arg("force_tier") = -1,
             py::arg("verbose")    = false)
        .def("set_callsign",  &gr::opendsp_ofdm::tx_source::set_callsign)
        .def("set_force_tier",&gr::opendsp_ofdm::tx_source::set_force_tier)
        .def("set_verbose",   &gr::opendsp_ofdm::tx_source::set_verbose);

    // ── rx_sink ───────────────────────────────────────────────────────────────
    py::class_<gr::opendsp_ofdm::rx_sink,
               gr::sync_block,
               std::shared_ptr<gr::opendsp_ofdm::rx_sink>>(m, "rx_sink")
        .def(py::init([](const std::string& callsign, int force_tier, bool verbose) {
                 return gr::opendsp_ofdm::rx_sink::make(callsign, force_tier, verbose);
             }),
             py::arg("callsign")    = "N0CALL",
             py::arg("force_tier") = -1,
             py::arg("verbose")    = false)
        .def("set_force_tier",&gr::opendsp_ofdm::rx_sink::set_force_tier)
        .def("set_verbose",   &gr::opendsp_ofdm::rx_sink::set_verbose);
}
