find_package(pybind11 REQUIRED)

pybind11_add_module(opendsp_ofdm_python
    python_bindings.cc)

target_link_libraries(opendsp_ofdm_python PRIVATE
    gnuradio-opendsp_ofdm
    gnuradio::gnuradio-runtime
    gnuradio::gnuradio-pmt)

install(TARGETS opendsp_ofdm_python
    DESTINATION ${GR_PYTHON_DIR}/gnuradio/opendsp_ofdm)
