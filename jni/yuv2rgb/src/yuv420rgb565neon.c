#include "yuv2rgb.h"
#include "chroma_neon.h"

void yuv420_2_rgb565(uint8_t  *dst_ptr,
               const uint8_t  *y_ptr,
               const uint8_t  *u_ptr,
               const uint8_t  *v_ptr,
                     int32_t   width,
                     int32_t   height,
                     int32_t   y_span,
                     int32_t   uv_span,
                     int32_t   dst_span,
               const uint32_t *tables,
                     int32_t   dither)
{
    struct yuv_pack out = { dst_ptr, dst_span };
    struct yuv_planes in = { y_ptr, u_ptr, v_ptr, y_span, uv_span };
    i420_rgb565_neon (&out, &in, width, height);	
}
