#pragma once
#include <gnuradio/attributes.h>

#ifdef gnuradio_opendsp_ofdm_EXPORTS
#define OPENDSP_OFDM_API __GR_ATTR_EXPORT
#else
#define OPENDSP_OFDM_API __GR_ATTR_IMPORT
#endif
