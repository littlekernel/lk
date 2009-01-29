/*
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __PLATFORM_MDDI_H
#define __PLATFORM_MDDI_H

struct fbcon_config;

struct __attribute__((packed)) mddi_client_caps 
{
    unsigned short length;
    unsigned short type;
    unsigned short client_id;

    unsigned short protocol_ver;
    unsigned short min_protocol_ver;
    unsigned short data_rate_cap;
    unsigned char interface_type_cap;
    unsigned char num_alt_displays;
    unsigned short postcal_data_rate;
    unsigned short bitmap_width;
    unsigned short bitmap_height;
    unsigned short display_window_width;
    unsigned short display_window_height;
    unsigned cmap_size;
    unsigned short cmap_rgb_width;
    unsigned short rgb_cap;
    unsigned char mono_cap;
    unsigned char reserved1;
    unsigned short ycbcr_cap;
    unsigned short bayer_cap;
    unsigned short alpha_cursor_planes;
    unsigned client_feature_cap;
    unsigned char max_video_frame_rate_cap;
    unsigned char min_video_frame_rate_cap;
    unsigned short min_sub_frame_rate;
    unsigned short audio_buf_depth;
    unsigned short audio_channel_cap;
    unsigned short audio_sampe_rate_rap;
    unsigned char audio_sample_res;
    unsigned char mic_audio_sample_res;
    unsigned short mic_sample_rate_cap;
    unsigned char keyboard_data_fmt;
    unsigned char pointing_device_data_fmt;
    unsigned short content_protection_type;
    unsigned short manufacturer_name;
    unsigned short product_code;
    unsigned short reserved3;
    unsigned serial_no;
    unsigned char week_of_manufacture;
    unsigned char year_of_manufacture;

    unsigned short crc;    
};

void mddi_remote_write(unsigned val, unsigned reg);
struct fbcon_config *mddi_init(void);

#endif /* __PLATFORM_MDDI_H */
