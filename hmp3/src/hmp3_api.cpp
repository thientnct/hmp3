/* ***** BEGIN LICENSE BLOCK *****
 * Helix MP3 Encoder C-API Implementation
 * Modeled after the LAME API for clean, standard integration into FFmpeg
 * ***** END LICENSE BLOCK ***** */

#include "hmp3.h"
#include "mp3enc.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FIFO_MAX_SAMPLES 16384

struct hmp3_global_struct {
    CMp3Enc *enc;
    E_CONTROL ec;

    int num_channels;
    int in_samplerate;
    int out_samplerate;
    HMP3_mode mode;
    int brate_kbps;
    HMP3_vbr_mode vbr_mode;
    float vbr_quality;
    int vbr_mnr;
    int cutoff;
    int quick;
    int bWriteVbrTag;
    int disable_reservoir;
    int copyright;
    int original;
    int hf_flag;
    int short_block_threshold;
    int vbr_delta_mnr;
    int vbr_br_limit;
    int nsbstereo;
    int tx;

    /* Internal PCM FIFO (float, planar) */
    float *fifo_l;
    float *fifo_r;
    int fifo_count;

    /* Interleaved scratch buffer for CMp3Enc (2 * 1152 = 2304) */
    float interleaved[2304];

    /* Flush tracking */
    int flush_state;
    int frames_in;
};

extern "C" {

hmp3_global_flags *hmp3_init(void)
{
    hmp3_global_flags *gfp = (hmp3_global_flags *)calloc(1, sizeof(hmp3_global_flags));
    if (!gfp)
        return NULL;

    gfp->enc = NULL;
    gfp->frames_in = 0;
    gfp->num_channels = 2;
    gfp->in_samplerate = 44100;
    gfp->out_samplerate = 44100;
    gfp->mode = HMP3_JOINT_STEREO;
    gfp->brate_kbps = 128;
    gfp->vbr_mode = HMP3_VBR_OFF;
    gfp->vbr_quality = 4.0f;
    gfp->vbr_mnr = 50;
    gfp->cutoff = 0;
    gfp->quick = -1;
    gfp->bWriteVbrTag = 0;
    gfp->disable_reservoir = 0;
    gfp->copyright = 0;
    gfp->original = 1;
    gfp->hf_flag = 0;
    gfp->short_block_threshold = 700;
    gfp->vbr_delta_mnr = 0;
    gfp->vbr_br_limit = 0;
    gfp->nsbstereo = -1;
    gfp->tx = -1;

    gfp->fifo_l = (float *)calloc(FIFO_MAX_SAMPLES, sizeof(float));
    gfp->fifo_r = (float *)calloc(FIFO_MAX_SAMPLES, sizeof(float));
    gfp->fifo_count = 0;
    gfp->flush_state = 0;

    if (!gfp->fifo_l || !gfp->fifo_r) {
        hmp3_close(gfp);
        return NULL;
    }

    return gfp;
}

int hmp3_set_num_channels(hmp3_global_flags *gfp, int channels)
{
    if (!gfp || channels < 1 || channels > 2)
        return -1;
    gfp->num_channels = channels;
    if (channels == 1)
        gfp->mode = HMP3_MONO;
    return 0;
}

int hmp3_get_num_channels(const hmp3_global_flags *gfp)
{
    return gfp ? gfp->num_channels : 0;
}

int hmp3_set_in_samplerate(hmp3_global_flags *gfp, int samplerate)
{
    if (!gfp || samplerate <= 0)
        return -1;
    gfp->in_samplerate = samplerate;
    return 0;
}

int hmp3_set_out_samplerate(hmp3_global_flags *gfp, int samplerate)
{
    if (!gfp || samplerate <= 0)
        return -1;
    gfp->out_samplerate = samplerate;
    return 0;
}

int hmp3_get_out_samplerate(const hmp3_global_flags *gfp)
{
    return gfp ? gfp->out_samplerate : 0;
}

int hmp3_set_mode(hmp3_global_flags *gfp, HMP3_mode mode)
{
    if (!gfp)
        return -1;
    gfp->mode = mode;
    return 0;
}

HMP3_mode hmp3_get_mode(const hmp3_global_flags *gfp)
{
    return gfp ? gfp->mode : HMP3_STEREO;
}

int hmp3_set_quality(hmp3_global_flags *gfp, int quality)
{
    if (!gfp)
        return -1;
    // Map compression_level to quick mode:
    // quality <= 2 -> high quality (quick = 0)
    // quality > 2  -> fast (quick = 1)
    if (quality <= 2)
        gfp->quick = 0;
    else
        gfp->quick = 1;
    return 0;
}

int hmp3_set_brate(hmp3_global_flags *gfp, int brate_kbps)
{
    if (!gfp || brate_kbps <= 0)
        return -1;
    gfp->brate_kbps = brate_kbps;
    return 0;
}

int hmp3_set_VBR(hmp3_global_flags *gfp, HMP3_vbr_mode vbr)
{
    if (!gfp)
        return -1;
    gfp->vbr_mode = vbr;
    return 0;
}

int hmp3_set_VBR_quality(hmp3_global_flags *gfp, float quality)
{
    if (!gfp)
        return -1;
    gfp->vbr_quality = quality;
    // Map standard LAME quality (0.0 highest to 9.0 lowest) to Helix vbr_mnr (150 to 0)
    // Reference points from hmp3-info.md:
    // q = 0.0 -> MNR 150 (~256 kbps, LAME -V 0)
    // q = 2.0 -> MNR 110 (~195 kbps, LAME -V 2)
    // q = 4.0 -> MNR  80 (~160 kbps, LAME -V 4)
    // q = 6.0 -> MNR  50 (~128 kbps, LAME -V 5-6)
    // q = 9.0 -> MNR   0 (~100 kbps, LAME -V 9)
    float mnr_flt;
    if (quality <= 0.0f) {
        mnr_flt = 150.0f;
    } else if (quality <= 2.0f) {
        mnr_flt = 150.0f - quality * 20.0f;                 // 150 -> 110
    } else if (quality <= 4.0f) {
        mnr_flt = 110.0f - (quality - 2.0f) * 15.0f;         // 110 -> 80
    } else if (quality <= 6.0f) {
        mnr_flt = 80.0f - (quality - 4.0f) * 15.0f;          // 80 -> 50
    } else if (quality <= 9.0f) {
        mnr_flt = 50.0f - (quality - 6.0f) * (50.0f / 3.0f); // 50 -> 0
    } else {
        mnr_flt = 0.0f;
    }
    int mnr = (int)(mnr_flt + 0.5f);
    if (mnr < 0) mnr = 0;
    if (mnr > 150) mnr = 150;
    gfp->vbr_mnr = mnr;
    return 0;
}

int hmp3_set_VBR_mean_bitrate_kbps(hmp3_global_flags *gfp, int brate_kbps)
{
    if (!gfp || brate_kbps <= 0)
        return -1;
    gfp->brate_kbps = brate_kbps;
    return 0;
}

int hmp3_set_VBR_mnr(hmp3_global_flags *gfp, int mnr)
{
    if (!gfp || mnr < 0 || mnr > 150)
        return -1;
    gfp->vbr_mnr = mnr;
    return 0;
}

int hmp3_set_lowpassfreq(hmp3_global_flags *gfp, int freq)
{
    if (!gfp)
        return -1;
    gfp->cutoff = freq;
    return 0;
}

int hmp3_set_bWriteVbrTag(hmp3_global_flags *gfp, int bWriteVbrTag)
{
    if (!gfp)
        return -1;
    gfp->bWriteVbrTag = bWriteVbrTag;
    return 0;
}

int hmp3_set_disable_reservoir(hmp3_global_flags *gfp, int disable)
{
    if (!gfp)
        return -1;
    gfp->disable_reservoir = disable;
    return 0;
}

int hmp3_set_copyright(hmp3_global_flags *gfp, int copyright)
{
    if (!gfp)
        return -1;
    gfp->copyright = copyright;
    return 0;
}

int hmp3_set_original(hmp3_global_flags *gfp, int original)
{
    if (!gfp)
        return -1;
    gfp->original = original;
    return 0;
}

int hmp3_set_hf_flag(hmp3_global_flags *gfp, int hf_flag)
{
    if (!gfp)
        return -1;
    if (hf_flag <= 0)
        gfp->hf_flag = 0;
    else if (hf_flag == 1)
        gfp->hf_flag = 1;
    else
        gfp->hf_flag = 3; // 1 | 2 = 3: full spectrum on all frames
    return 0;
}

int hmp3_set_short_block_threshold(hmp3_global_flags *gfp, int sbt)
{
    if (!gfp || sbt <= 0)
        return -1;
    gfp->short_block_threshold = sbt;
    return 0;
}

int hmp3_set_vbr_delta_mnr(hmp3_global_flags *gfp, int delta_mnr)
{
    if (!gfp)
        return -1;
    if (delta_mnr < -40) delta_mnr = -40;
    if (delta_mnr > 50)  delta_mnr = 50;
    gfp->vbr_delta_mnr = delta_mnr;
    return 0;
}

int hmp3_set_vbr_br_limit(hmp3_global_flags *gfp, int br_limit_kbps)
{
    if (!gfp)
        return -1;
    gfp->vbr_br_limit = br_limit_kbps;
    return 0;
}

int hmp3_set_nsbstereo(hmp3_global_flags *gfp, int nsbstereo)
{
    if (!gfp)
        return -1;
    gfp->nsbstereo = nsbstereo;
    return 0;
}

int hmp3_set_tx(hmp3_global_flags *gfp, int tx)
{
    if (!gfp)
        return -1;
    gfp->tx = tx;
    return 0;
}

int hmp3_init_params(hmp3_global_flags *gfp)
{
    if (!gfp)
        return -1;

    if (gfp->enc) {
        delete gfp->enc;
        gfp->enc = NULL;
    }

    gfp->enc = new CMp3Enc();
    if (!gfp->enc)
        return -1;

    memset(&gfp->ec, 0, sizeof(E_CONTROL));

    gfp->ec.samprate = gfp->out_samplerate;
    gfp->ec.mode = (gfp->num_channels == 1) ? 3 : (int)gfp->mode;

    gfp->ec.vbr_flag = (gfp->vbr_mode != HMP3_VBR_OFF) ? 1 : 0;

    // In Helix MP3, VBR mode requires bitrate = -1 (let encoder choose)
    // In CBR mode, ec.bitrate is bitrate PER CHANNEL in kbps
    if (gfp->ec.vbr_flag) {
        gfp->ec.bitrate = -1;
    } else {
        if (gfp->num_channels == 1 || gfp->ec.mode == 3) {
            gfp->ec.bitrate = gfp->brate_kbps;
        } else {
            gfp->ec.bitrate = (gfp->brate_kbps > 0) ? (gfp->brate_kbps / 2) : -1;
        }
    }

    gfp->ec.vbr_mnr = gfp->vbr_mnr;
    gfp->ec.cr_bit = gfp->copyright ? 1 : 0;
    gfp->ec.original = gfp->original ? 1 : 0;
    gfp->ec.freq_limit = (gfp->cutoff > 0) ? gfp->cutoff : 24000;
    gfp->ec.quick = gfp->quick;

    // Standard defaults for Helix MP3 Layer III
    gfp->ec.layer = 3;
    gfp->ec.nsbstereo = gfp->nsbstereo;
    gfp->ec.filter_select = -1;
    gfp->ec.nsb_limit = -1;
    gfp->ec.hf_flag = gfp->hf_flag;

    // vbr_br_limit in Helix is per-channel kbps (max 160)
    if (gfp->vbr_br_limit > 0) {
        if (gfp->num_channels == 1 || gfp->ec.mode == 3)
            gfp->ec.vbr_br_limit = gfp->vbr_br_limit;
        else
            gfp->ec.vbr_br_limit = gfp->vbr_br_limit / 2;
    } else {
        gfp->ec.vbr_br_limit = 160;
    }

    gfp->ec.chan_add_f0 = 24000;
    gfp->ec.chan_add_f1 = 24000;
    gfp->ec.sparse_scale = -1;
    gfp->ec.vbr_delta_mnr = gfp->vbr_delta_mnr;
    gfp->ec.cpu_select = 0;
    gfp->ec.test1 = (gfp->tx >= 0) ? gfp->tx : -1;
    gfp->ec.test2 = 0;
    gfp->ec.test3 = 0;
    gfp->ec.short_block_threshold = gfp->short_block_threshold;
    for (int i = 0; i < 21; i++)
        gfp->ec.mnr_adjust[i] = 0;

    int ret = gfp->enc->L3_audio_encode_init(&gfp->ec);
    if (ret <= 0) {
        delete gfp->enc;
        gfp->enc = NULL;
        return -1;
    }

    gfp->fifo_count = 0;
    gfp->flush_state = 0;

    return 0;
}

int hmp3_get_encoder_delay(const hmp3_global_flags *gfp)
{
    (void)gfp;
    // Analysis lookahead delay of Helix MP3
    // Combined with decoder MDCT delay (528) and 1 sample rounding,
    // total delay matches 1152 + 528 = 1680 samples.
    return 1151;
}

int hmp3_get_framesize(const hmp3_global_flags *gfp)
{
    if (!gfp)
        return 1152;
    // 1152 samples for MPEG-1 (>= 32000 Hz), 576 samples for MPEG-2 (< 32000 Hz)
    return (gfp->out_samplerate < 32000) ? 576 : 1152;
}

static inline void fill_interleaved(hmp3_global_flags *gfp)
{
    if (gfp->num_channels == 1 || gfp->ec.mode == 3) {
        if (gfp->num_channels == 2) {
            for (int i = 0; i < 1152; i++) {
                gfp->interleaved[i] = 0.5f * (gfp->fifo_l[i] + gfp->fifo_r[i]);
            }
        } else {
            for (int i = 0; i < 1152; i++) {
                gfp->interleaved[i] = gfp->fifo_l[i];
            }
        }
    } else {
        for (int i = 0; i < 1152; i++) {
            gfp->interleaved[2 * i]     = gfp->fifo_l[i];
            gfp->interleaved[2 * i + 1] = gfp->fifo_r[i];
        }
    }
}

int hmp3_encode_buffer_float(hmp3_global_flags *gfp,
                            const float *buffer_l,
                            const float *buffer_r,
                            int nsamples,
                            unsigned char *mp3buf,
                            int mp3buf_size)
{
    if (!gfp || !gfp->enc || nsamples <= 0)
        return 0;

    if (!buffer_l)
        return -1;

    // Check available FIFO space
    if (gfp->fifo_count + nsamples > FIFO_MAX_SAMPLES)
        return -2; // FIFO overflow

    // Append to planar FIFO
    memcpy(gfp->fifo_l + gfp->fifo_count, buffer_l, nsamples * sizeof(float));
    if (gfp->num_channels == 2 && buffer_r) {
        memcpy(gfp->fifo_r + gfp->fifo_count, buffer_r, nsamples * sizeof(float));
    } else {
        memcpy(gfp->fifo_r + gfp->fifo_count, buffer_l, nsamples * sizeof(float));
    }
    gfp->fifo_count += nsamples;

    int total_bytes = 0;

    // CMp3Enc::L3_audio_encode consumes exactly 1152 samples per channel per call
    while (gfp->fifo_count >= 1152) {
        if (mp3buf && mp3buf_size - total_bytes < 4096)
            break; // Output buffer near capacity

        fill_interleaved(gfp);

        IN_OUT x = gfp->enc->L3_audio_encode(gfp->interleaved, mp3buf ? (mp3buf + total_bytes) : NULL);
        total_bytes += x.out_bytes;

        // Shift FIFO
        gfp->fifo_count -= 1152;
        if (gfp->fifo_count > 0) {
            memmove(gfp->fifo_l, gfp->fifo_l + 1152, gfp->fifo_count * sizeof(float));
            memmove(gfp->fifo_r, gfp->fifo_r + 1152, gfp->fifo_count * sizeof(float));
        }
    }

    return total_bytes;
}

int hmp3_encode_buffer(hmp3_global_flags *gfp,
                       const short int *buffer_l,
                       const short int *buffer_r,
                       int nsamples,
                       unsigned char *mp3buf,
                       int mp3buf_size)
{
    if (!gfp || !gfp->enc || nsamples <= 0 || !buffer_l)
        return 0;

    if (gfp->fifo_count + nsamples > FIFO_MAX_SAMPLES)
        return -2;

    float *dst_l = gfp->fifo_l + gfp->fifo_count;
    float *dst_r = gfp->fifo_r + gfp->fifo_count;

    for (int i = 0; i < nsamples; i++) {
        dst_l[i] = (float)buffer_l[i];
    }
    if (gfp->num_channels == 2 && buffer_r) {
        for (int i = 0; i < nsamples; i++) {
            dst_r[i] = (float)buffer_r[i];
        }
    } else {
        for (int i = 0; i < nsamples; i++) {
            dst_r[i] = dst_l[i];
        }
    }
    gfp->fifo_count += nsamples;

    int total_bytes = 0;

    while (gfp->fifo_count >= 1152) {
        if (mp3buf && mp3buf_size - total_bytes < 4096)
            break;

        fill_interleaved(gfp);

        IN_OUT x = gfp->enc->L3_audio_encode(gfp->interleaved, mp3buf ? (mp3buf + total_bytes) : NULL);
        total_bytes += x.out_bytes;

        gfp->fifo_count -= 1152;
        if (gfp->fifo_count > 0) {
            memmove(gfp->fifo_l, gfp->fifo_l + 1152, gfp->fifo_count * sizeof(float));
            memmove(gfp->fifo_r, gfp->fifo_r + 1152, gfp->fifo_count * sizeof(float));
        }
    }

    return total_bytes;
}

int hmp3_encode_buffer_int(hmp3_global_flags *gfp,
                           const int *buffer_l,
                           const int *buffer_r,
                           int nsamples,
                           unsigned char *mp3buf,
                           int mp3buf_size)
{
    if (!gfp || !gfp->enc || nsamples <= 0 || !buffer_l)
        return 0;

    if (gfp->fifo_count + nsamples > FIFO_MAX_SAMPLES)
        return -2;

    float *dst_l = gfp->fifo_l + gfp->fifo_count;
    float *dst_r = gfp->fifo_r + gfp->fifo_count;

    for (int i = 0; i < nsamples; i++) {
        dst_l[i] = (float)(buffer_l[i] >> 16);
    }
    if (gfp->num_channels == 2 && buffer_r) {
        for (int i = 0; i < nsamples; i++) {
            dst_r[i] = (float)(buffer_r[i] >> 16);
        }
    } else {
        for (int i = 0; i < nsamples; i++) {
            dst_r[i] = dst_l[i];
        }
    }
    gfp->fifo_count += nsamples;

    int total_bytes = 0;

    while (gfp->fifo_count >= 1152) {
        if (mp3buf && mp3buf_size - total_bytes < 4096)
            break;

        fill_interleaved(gfp);

        IN_OUT x = gfp->enc->L3_audio_encode(gfp->interleaved, mp3buf ? (mp3buf + total_bytes) : NULL);
        total_bytes += x.out_bytes;
        gfp->frames_in++;

        gfp->fifo_count -= 1152;
        if (gfp->fifo_count > 0) {
            memmove(gfp->fifo_l, gfp->fifo_l + 1152, gfp->fifo_count * sizeof(float));
            memmove(gfp->fifo_r, gfp->fifo_r + 1152, gfp->fifo_count * sizeof(float));
        }
    }

    return total_bytes;
}

int hmp3_encode_flush(hmp3_global_flags *gfp,
                      unsigned char *mp3buf,
                      int mp3buf_size)
{
    if (!gfp || !gfp->enc)
        return 0;

    int total_bytes = 0;

    // Step 1: If partial samples remain in FIFO, pad with silence to 1152 and encode
    if (gfp->fifo_count > 0) {
        memset(gfp->fifo_l + gfp->fifo_count, 0, (1152 - gfp->fifo_count) * sizeof(float));
        memset(gfp->fifo_r + gfp->fifo_count, 0, (1152 - gfp->fifo_count) * sizeof(float));

        fill_interleaved(gfp);

        IN_OUT x = gfp->enc->L3_audio_encode(gfp->interleaved, mp3buf ? (mp3buf + total_bytes) : NULL);
        total_bytes += x.out_bytes;
        gfp->fifo_count = 0;
        gfp->frames_in++;
        return total_bytes;
    }

    // Step 2: Feed silent frames until an MP3 frame is emitted to drain internal pipeline
    while (total_bytes == 0 && gfp->flush_state < 10) {
        memset(gfp->interleaved, 0, sizeof(gfp->interleaved));
        IN_OUT x = gfp->enc->L3_audio_encode(gfp->interleaved, mp3buf ? (mp3buf + total_bytes) : NULL);
        total_bytes += x.out_bytes;
        gfp->flush_state++;
    }

    return total_bytes;
}

int hmp3_close(hmp3_global_flags *gfp)
{
    if (!gfp)
        return 0;

    if (gfp->enc) {
        delete gfp->enc;
        gfp->enc = NULL;
    }

    if (gfp->fifo_l) {
        free(gfp->fifo_l);
        gfp->fifo_l = NULL;
    }
    if (gfp->fifo_r) {
        free(gfp->fifo_r);
        gfp->fifo_r = NULL;
    }

    free(gfp);
    return 0;
}

} // extern "C"
