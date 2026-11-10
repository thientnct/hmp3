/* ***** BEGIN LICENSE BLOCK *****
 * Helix MP3 Encoder C-API Header
 * Modeled after the LAME API for clean, standard integration into FFmpeg
 * ***** END LICENSE BLOCK ***** */

#ifndef _HMP3_H_
#define _HMP3_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum HMP3_mode_e {
    HMP3_STEREO = 0,
    HMP3_JOINT_STEREO = 1,
    HMP3_DUAL_CHANNEL = 2,
    HMP3_MONO = 3
} HMP3_mode;

typedef enum HMP3_vbr_mode_e {
    HMP3_VBR_OFF = 0,
    HMP3_VBR_DEFAULT = 1,
    HMP3_VBR_ABR = 2
} HMP3_vbr_mode;

struct hmp3_global_struct;
typedef struct hmp3_global_struct hmp3_global_flags;
typedef hmp3_global_flags *hmp3_t;

/* Initialize encoder context with default settings */
hmp3_global_flags *hmp3_init(void);

/* Parameter configuration functions */
int hmp3_set_num_channels(hmp3_global_flags *gfp, int channels);
int hmp3_get_num_channels(const hmp3_global_flags *gfp);

int hmp3_set_in_samplerate(hmp3_global_flags *gfp, int samplerate);
int hmp3_set_out_samplerate(hmp3_global_flags *gfp, int samplerate);
int hmp3_get_out_samplerate(const hmp3_global_flags *gfp);

int hmp3_set_mode(hmp3_global_flags *gfp, HMP3_mode mode);
HMP3_mode hmp3_get_mode(const hmp3_global_flags *gfp);

int hmp3_set_quality(hmp3_global_flags *gfp, int quality);
int hmp3_set_brate(hmp3_global_flags *gfp, int brate_kbps);
int hmp3_set_VBR(hmp3_global_flags *gfp, HMP3_vbr_mode vbr);
int hmp3_set_VBR_quality(hmp3_global_flags *gfp, float quality);
int hmp3_set_VBR_mean_bitrate_kbps(hmp3_global_flags *gfp, int brate_kbps);
int hmp3_set_VBR_mnr(hmp3_global_flags *gfp, int mnr);

int hmp3_set_lowpassfreq(hmp3_global_flags *gfp, int freq);
int hmp3_set_bWriteVbrTag(hmp3_global_flags *gfp, int bWriteVbrTag);
int hmp3_set_disable_reservoir(hmp3_global_flags *gfp, int disable);
int hmp3_set_copyright(hmp3_global_flags *gfp, int copyright);
int hmp3_set_original(hmp3_global_flags *gfp, int original);

int hmp3_set_hf_flag(hmp3_global_flags *gfp, int hf_flag);
int hmp3_set_short_block_threshold(hmp3_global_flags *gfp, int sbt);
int hmp3_set_vbr_delta_mnr(hmp3_global_flags *gfp, int delta_mnr);
int hmp3_set_vbr_br_limit(hmp3_global_flags *gfp, int br_limit_kbps);
int hmp3_set_nsbstereo(hmp3_global_flags *gfp, int nsbstereo);
int hmp3_set_tx(hmp3_global_flags *gfp, int tx);

/* Finalize initialization and instantiate the encoder engine */
int hmp3_init_params(hmp3_global_flags *gfp);

/* Query encoder properties */
int hmp3_get_encoder_delay(const hmp3_global_flags *gfp);
int hmp3_get_framesize(const hmp3_global_flags *gfp);

/* Encoding buffer routines */
int hmp3_encode_buffer_float(hmp3_global_flags *gfp,
                            const float *buffer_l,
                            const float *buffer_r,
                            int nsamples,
                            unsigned char *mp3buf,
                            int mp3buf_size);

int hmp3_encode_buffer(hmp3_global_flags *gfp,
                       const short int *buffer_l,
                       const short int *buffer_r,
                       int nsamples,
                       unsigned char *mp3buf,
                       int mp3buf_size);

int hmp3_encode_buffer_int(hmp3_global_flags *gfp,
                           const int *buffer_l,
                           const int *buffer_r,
                           int nsamples,
                           unsigned char *mp3buf,
                           int mp3buf_size);

/* Flush remaining data from encoder (returns bytes written to mp3buf) */
int hmp3_encode_flush(hmp3_global_flags *gfp,
                      unsigned char *mp3buf,
                      int mp3buf_size);

/* Close and free encoder */
int hmp3_close(hmp3_global_flags *gfp);

#ifdef __cplusplus
}
#endif

#endif /* _HMP3_H_ */
