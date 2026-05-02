add_library(gnuradio-opendsp_ofdm SHARED
    tx_source_impl.cc
    rx_sink_impl.cc)

target_include_directories(gnuradio-opendsp_ofdm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(gnuradio-opendsp_ofdm PUBLIC
    gnuradio::gnuradio-runtime
    gnuradio::gnuradio-pmt
    opendsp_ofdm_dep
    ${CODEC2_LIB}
    ${FFTW3_LIB})

set_target_properties(gnuradio-opendsp_ofdm PROPERTIES
    DEFINE_SYMBOL "gnuradio_opendsp_ofdm_EXPORTS")

install(TARGETS gnuradio-opendsp_ofdm
    LIBRARY DESTINATION lib)
