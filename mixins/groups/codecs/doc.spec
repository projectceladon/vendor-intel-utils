=== Overview

codecs is used to configure the video codec list on different platforms

=== Options

--- configurable
this option is used to select the platform and supported codec list

    --- parameters
        - platform:     select the media_codecs_performance.xml based on platform
        - codec_perf_xen:     select the media_codecs_performance.xml based on platform+xen
        - hw_vd_mp2:    select hw accelerated mpeg2 decoder
        - hw_vd_vc1:    select hw accelerated vc1 decoder
        - hw_vd_vp9:    select hw accelerated vp9 decoder
        - hw_vd_vp8:    select hw accelerated vp8 decoder
        - sw_vd_h265:   select sw h.265 decoder
        - hw_vd_h265:   select hw accelerated h.265 decoder
        - hw_vd_h265_secure:    select hw accelerated h.265 secure decoder
        - hw_ve_vp8:    select hw accelerated vp8 encoder
        - hw_ve_h265:   select hw accelerated h.265 encoder
        - hw_ve_h264:   select hw accelerated h.264 encoder
        - hw_vd_h264:   select hw accelerated h.264 decoder
        - hw_vd_h264_secure:    select hw accelerated h.264 secure decoder


--- default
when not explicitly selected in mixin spec file, the default option will be used.
