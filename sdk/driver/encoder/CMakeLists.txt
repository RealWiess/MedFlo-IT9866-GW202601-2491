add_library(encoder)
target_sources(encoder
PRIVATE
    mmp_encoder.c
    encoder_memory.c
    encoder_hardware.c
    encoder_bitstream.c
)
