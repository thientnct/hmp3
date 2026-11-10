# Helix MP3 Encoder

| **Helix MP3 Encoder** | |
| :--- | :--- |
| **Developer(s)** | RealNetworks, [maikmerten](https://hydrogenaud.io/index.php?action=profile;u=1312) maintains GitHub repo |
| **Operating system** | Linux, Windows |
| **Use** | Encoder |
| **License** | [RPSL](https://github.com/maikmerten/hmp3/blob/main/hmp3/LICENSE.txt) |
| **Website** | [GitHub repo](https://github.com/maikmerten/hmp3) |

The Helix MP3 Encoder was open-sourced by RealNetworks ca. 2005 via the (long-defunct) Helix community project. It originated from the [Xing](https://wiki.hydrogenaudio.org/index.php?title=Xing) MP3 encoder, which was purchased by RealNetworks.

A current version ("hmp3"), with contributions from HydrogenAudio members, is available as source code over at [https://github.com/maikmerten/hmp3](https://github.com/maikmerten/hmp3). This Wiki page discusses that version.

## Features

* Encodes [MP3](https://wiki.hydrogenaudio.org/index.php?title=MP3) in MPEG-1 and MPEG-2 modes
  * 48 kHz, 44.1 kHz, 32 kHz (MPEG-1)
  * 24 kHz, 22.05 kHz, 16 kHz (MPEG-2)
* LAME headers for gapless playback
* [CBR](https://wiki.hydrogenaudio.org/index.php?title=CBR) and [VBR](https://wiki.hydrogenaudio.org/index.php?title=VBR) encoding

## Listening tests

The Helix MP3 encoder participated in several listening tests and demonstrated to be amongst the highest-quality encoders for MP3 available.

* [Public MP3 listening test, October 2008](https://listening-tests.hydrogenaud.io/sebastian/mp3-128-1/results.htm)
* [Personal listening test by Kamedo2, ~224 kbps, May 2013](https://hydrogenaud.io/index.php/topic,100896.0.html)
* [Personal listening test by Kamedo2, ~192 kbps, December 2016](https://hydrogenaud.io/index.php/topic,113324.0.html)

## Encoder switches

hmp3 is a command-line operated application. The most basic invocation to generate a MP3 file from WAV:

```
hmp3 input.wav output.mp3
```

This creates a ~128 kbps VBR file for 44.1 kHz stereo input.

*Encoder switches*

| Switch | Function | Example |
| :--- | :--- | :--- |
| `-B` | Set *per-channel* bitrate. Selects **CBR** encoding. | `-B64` for a 128 kbps stereo CBR file |
| `-F` | Frequency cutoff for the encoder lowpass filter. To actually encode anything beyond 16 kHz, also specify the -HF switch. | `-F19000` for a 19 kHz lowpass |
| `-HF` | Controls encoding of high frequency content (> 16 kHz). Disabled by default. Valid values are 0 (disabled), 1 (partial, only "mode-1 granules"), 2 (full, "all granules").<br><br>A value of "1" means that only frames with M/S stereo can include high frequency content. A value of "2" allows high-frequency content in all frame-types.<br><br>Note that high-frequency content will only be encoded if the psychoacoustic model deems encoding high frequencies as beneficial for the given bitrate/quality settings.<br><br>High frequencies will only be encoded if -V >= 80 or -B >= 96. | `HF2` for unrestricted high-frequency encoding |
| `-L` | Set *per-channel* bitrate limit (upper cap) in VBR mode. Defaults to no limit. | `-L80` to limit maximum VBR bitrate to 160 kbps for stereo files |
| `-M` | Stereo-mode/Mono selection. 0: stereo, 1: M/S stereo (default), 2: dual channel, 3: mono | `-M3` to downmix to mono |
| `-N` | Enable use of Intensity Stereo. Only works with CBR and makes the encoder use "Bit Allocator 1" (see section "Bit Allocators") | `-N8` to enable Intensity Stereo with 8 bands of M/S stereo |
| `-SBT` | Threshold for short-block decisions. *Lower* values mean *more* short-block usage. Default is 700. | `-SBT500` for more short-blocks (more responsive to transients, might increase bitrate in VBR) |
| `-T` | Bias for VBR quality scale. Ranges from -40 to 50, defaults to 0. In conjunction with the base VBR quality scale (-V, see below) this determines the bitrate and quality of VBR encodings. Can be used to reach bitrates usually not achievable with the base VBR quality setting alone. | `-V0 -T-40` for a lower than usually possible VBR bitrate |
| `-U` | Select assembly optimizations. 0: Only generic optimizations, 1: unused (was supposed to be AMD's 3Dnow!) 2: use SSE assembly optimizations (Intel Pentium 3).<br><br>This only has an effect if the encoder is compiled with Visual Studio (up to version 2015) for 32-bit Windows. No effect if the encoder is compiled for Linux, 64-bit Windows or, e.g., ARM processors. | `-U2` to use SSE assembly (where applicable) |
| `-V` | Quality setting for **VBR** encoding. Ranges from 0 to 150. Default is 50. | `-V115` for a ~180-200 kbps stereo VBR file |
| `-X` | Control writing of Xing/LAME header information. 0: No headers, 1: only basic Xing information header, 2: Xing header with VBR-TOC and LAME header (gapless information) (default) | `-X0` to disable headers (in very rare cases of incompatibility) |

### -HF switch

Due to a design decision in the MP3 format, encoding high-frequency content (frequencies beyond 16 kHz) can be comparatively costly in terms of bits, while these frequencies often do not contribute much to the perceived audio quality. For this reason, the Helix MP3 Encoder will not encode high-frequency content by default. This, however, can be controlled by the -HF switch.

With -HF1, only Mid-Side stereo (M/S-stereo) frames will contain high-frequency content, while other frame-types will not encode any high-frequency content. The encoder will automatically choose the most efficient stereo mode for each frame. Usually, around 80% of frames will be coded as M/S-stereo ("joint" stereo encoding), with 20% of frames being coded as simple-stereo (separate encoding of channels). With -HF1, the ~20% of frames not being encoded in M/S-stereo will not encode high-frequency content, which might be perceivable as "dropouts" in the high frequency bands in unfavorable conditions.

With -HF2, all frame types can encode high-frequency content, so no "dropouts" should occur.

The behavior of -HF1 and -HF2 can be observed in the following spectrographs:

-HF1: [![Helix-pink-noise-v110-hf1.png](https://wiki.hydrogenaudio.org/images/thumb/c/c3/Helix-pink-noise-v110-hf1.png/384px-Helix-pink-noise-v110-hf1.png)](https://wiki.hydrogenaudio.org/index.php?title=File:Helix-pink-noise-v110-hf1.png)

-HF2: [![Helix-pink-noise-v110-hf2.png](https://wiki.hydrogenaudio.org/images/thumb/6/65/Helix-pink-noise-v110-hf2.png/384px-Helix-pink-noise-v110-hf2.png)](https://wiki.hydrogenaudio.org/index.php?title=File:Helix-pink-noise-v110-hf2.png)

The rationale for having the -HF1 option in the first place may be that if M/S-stereo is not applicable for a given frame, many bits are already spent on encoding the stereo image with separate stereo encoding. If the bitrate cannot be increased (for instance, when doing CBR encoding), it might be preferable to spend the remaining bits on the usually more perceivable lower frequencies, omitting high-frequency content. This rationale appears less applicable to VBR encoding (as the encoder can choose a more fitting bit allocation) and thus using -HF1 in VBR encoding might not be recommendable.

### Magical Mystery Switch -TX

During encoder-development, apparently different experimental settings were exposed for easy tweaking during testing. The [BA_CONTROL data structure](https://github.com/maikmerten/hmp3/blob/0e895bf47fdb0de3093622465031c8dbc4acc0b6/hmp3/src/pub/bitallo.h#L65) includes the wonderfully named int variables "test1", "test2" and "test3". While "test2" and "test3" are unused, "test1" is used in the function [startup_adjustNT1B of bitallo3.cpp](https://github.com/maikmerten/hmp3/blob/0e895bf47fdb0de3093622465031c8dbc4acc0b6/hmp3/src/bitallo3.cpp#L1069). The value of "test1" can be set via the -TX command line parameter. The default value is 6.

The function startup_adjustNT1B adjusts (increases) the per-band "Noise Target" (NT). The value of "test1" provides a multiplier "f", which factors in how much the noise target is adjusted. A [per-band threshold table](https://github.com/maikmerten/hmp3/blob/0e895bf47fdb0de3093622465031c8dbc4acc0b6/hmp3/src/bitallo3.cpp#L1075) decides which bands receive adjustment.

From code comments, it appears that the default value of 6 was determined to generally work best. It's unclear in what situations modifying this value is beneficial.

## Reasonable Settings

Here's a short list of settings for different encoding needs. Note that while comparisons to LAME's VBR settings are provided, these are only *very rough estimates* to provide guidance regarding potential use cases. LAME and Helix are very different encoders and are expected to perform better and worse in comparison, depending on audio material.

*Overview of reasonable hmp3 settings*

| Setting | Approx. Bitrate | Description |
| :--- | :--- | :--- |
| `-HF2 -V150` | ~ 256 kbps | Maximum quality VBR encoding, with full audio spectrum. (ca. LAME -V 0) |
| `-F19000 -HF2 -V110` | ~ 195 kbps | High-quality VBR encoding, audio spectrum up to 19 kHz. (ca. LAME -V 2)<br><br>This should be close to transparent to most people in most situations. |
| `-F18000 -HF2 -V80` | ~ 160 kbps | Medium-quality VBR encoding, audio spectrum up to 18 kHz. (ca. LAME -V 4) |
| `-V50` | ~ 128 kbps | Low-medium-quality VBR encoding, audio spectrum up to 16 kHz. (ca. LAME -V 5-6)<br><br>Default setting of the Helix MP3 Encoder. Should be sufficient for casual listening on space-constrained devices, but is not expected to be universally transparent. |

## Technical details

The following bits might not be relevant for daily-use of the Helix MP3 Encoder, but might be interesting to developers.

### Bit allocators

The Helix MP3 Encoder, apparently for historical reasons, has two distinct bit allocators, which are selected depending on operating modes. **Bit Allocator 1** (bitallo1.cpp) appears to be the older one, most likely inherited from early Xing days, while **Bit Allocator 3** (bitallo3.cpp) is a newer, overall more-capable mechanism that is utilized by default.

| Feature | Bit Allocator 1 | Bit Allocator 3 |
| :--- | :--- | :--- |
| CBR | supported | supported |
| VBR | **not** supported | supported |
| >16 kHz encoding | **not** supported | supported |
| Long/Short block switching | **not** supported | supported |
| Stereo | supported | supported |
| M/S-Stereo | supported | supported |
| Dual Channel Stereo | supported | **not** supported |
| Intensity stereo | supported | **not** supported |

Bit Allocator 1 thus is mostly interesting for very low-bitrate CBR encodings, where intensity stereo can lead to bitrate savings to spend somewhere else. Example:

```
hmp3 input.wav output.mp3 -F16000 -B48 -N8
```

for somewhat bearable low-bitrate stereo-ish MP3 encoding (the -N parameter enables intensity stereo).

### Detection of transients

To detect transients that warrant a switch to short blocks, the Helix MP3 Encoder uses output from the 32-band polyphase filterbank. The encoder computes "energy" values from the filterbank output and compares current energy values with values for the previous granule (detect.c). This is accomplished by an "energy history" (defined as "attack_buf" in mp3enc.h).

If the energy values differ enough (above the threshold for short block detection), short blocks will be used. The encoder will use short blocks for both channels, even if only the signal of one channel triggered the transients detection.

### Assembly optimizations

In platform/win/i386, there are optimized assembly versions of speed-critical routines. These target 32-bit x86-CPUs in general (but usually optimized for the Pentium 2), with some routines also being available for SSE, targeting the Pentium 3 (assembly files starting with "x"). These SSE optimizations can be selected via the -U parameter.

These routines are somewhat outdated. They only work in Visual Studio up to version 2015, only for the Windows platform - and only for 32-bit targets. It has been demonstrated that modern compilers generate faster code from the pure C source code. As such, these assembly optimizations appear to be superfluous.

That modern compilers can generate faster code might (but this is speculation) have something to do with the hand-written assembly version mixing x87 FPU instructions (in the routines for general i386 CPUs) with SSE instructions (which have their own register set). Modern CPUs appear to prefer doing all (even scalar) floating point operations in SSE or AVX registers.

## External links

* [Helix MP3 encoder (Windows command-line executable)](https://www.rarewares.org/mp3-others.php#helix_enc) at RareWares
* [Resurrecting/Preserving the Helix MP3 encoder](https://hydrogenaud.io/index.php/topic,123331.0.html) on hydrogenaudio

---
**Categories:** [Software](https://wiki.hydrogenaudio.org/index.php?title=Category:Software) | [Encoder/Decoder](https://wiki.hydrogenaudio.org/index.php?title=Category:Encoder/Decoder) | [MP3](https://wiki.hydrogenaudio.org/index.php?title=Category:MP3)