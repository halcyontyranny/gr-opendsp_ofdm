include(GrPython)

GR_PYTHON_INSTALL(
    FILES __init__.py
    DESTINATION ${GR_PYTHON_DIR}/gnuradio/opendsp_ofdm)

add_subdirectory(bindings)
