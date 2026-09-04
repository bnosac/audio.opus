#include <R.h>
#include <Rinternals.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opusfile.h>

/* Patch the WAV data-chunk and RIFF sizes after decoding */
static void finalize_wav_header(FILE *fout, long data_bytes) {
    unsigned int data_size = (unsigned int)data_bytes;
    unsigned int riff_size = 36 + data_size;
    fseek(fout, 4,  SEEK_SET); fwrite(&riff_size, 4, 1, fout);
    fseek(fout, 40, SEEK_SET); fwrite(&data_size,  4, 1, fout);
}

#define MAX_FRAME_SIZE 120*48   /* 120 ms at 48 kHz */




/* WAV header writer */
static void write_wav_header(FILE *fout, int rate, int channels, int fp) {
    int bits_per_sample = fp ? 32 : 16;
    int byte_rate       = rate * channels * bits_per_sample / 8;
    int block_align     = channels * bits_per_sample / 8;
    /* Placeholder sizes; updated at end */
    unsigned int data_size = 0;
    unsigned int riff_size = 36 + data_size;

    fwrite("RIFF",            1, 4, fout);
    fwrite(&riff_size,        4, 1, fout);
    fwrite("WAVE",            1, 4, fout);
    fwrite("fmt ",            1, 4, fout);
    unsigned int fmt_size = 16;
    fwrite(&fmt_size,         4, 1, fout);
    unsigned short audio_fmt = fp ? 3 : 1;   /* 3 = IEEE float, 1 = PCM */
    fwrite(&audio_fmt,        2, 1, fout);
    unsigned short ch = (unsigned short)channels;
    fwrite(&ch,               2, 1, fout);
    unsigned int sr = (unsigned int)rate;
    fwrite(&sr,               4, 1, fout);
    unsigned int br = (unsigned int)byte_rate;
    fwrite(&br,               4, 1, fout);
    unsigned short ba = (unsigned short)block_align;
    fwrite(&ba,               2, 1, fout);
    unsigned short bps = (unsigned short)bits_per_sample;
    fwrite(&bps,              2, 1, fout);
    fwrite("data",            1, 4, fout);
    fwrite(&data_size,        4, 1, fout);
}


#define MAX_FRAME_SIZE 120*48   /* 120 ms at 48 kHz */

/*
 * .Call entry point
 *
 * Arguments (all SEXP):
 *   in_file   – character(1)  path to .opus file (or "-" for stdin)
 *   out_file  – character(1)  path to output .wav file
 *   rate      – integer(1)    target sample rate (0 = keep original / 48000)
 *   stereo    – logical(1)    force stereo output
 *   gain_db   – numeric(1)    manual gain in dB (0 = none)
 *   use_float – logical(1)    write 32-bit float WAV instead of 16-bit PCM
 */
SEXP C_opus_decode(SEXP in_file, SEXP out_file,
                   SEXP rate_sexp, SEXP stereo_sexp,
                   SEXP gain_sexp, SEXP float_sexp)
{
    const char *inFile  = CHAR(STRING_ELT(in_file,  0));
    const char *outFile = CHAR(STRING_ELT(out_file, 0));
    int    rate         = asInteger(rate_sexp);
    int    force_stereo = asLogical(stereo_sexp);
    float  manual_gain  = (float)asReal(gain_sexp);
    int    fp           = asLogical(float_sexp);

    OggOpusFile *st = NULL;
    FILE        *fout = NULL;
    float       *output = NULL;
    int          exit_code = 0;
    long         data_bytes = 0;

    /* ── open input ───────────────────────────────────────────────── */
    if (strcmp(inFile, "-") == 0) {
        OpusFileCallbacks cb = {NULL, NULL, NULL, NULL};
        st = op_open_callbacks(op_fdopen(&cb, fileno(stdin), "rb"),
                               &cb, NULL, 0, NULL);
    } else {
        st = op_open_url(inFile, NULL, NULL);
        if (!st) st = op_open_file(inFile, NULL);
    }
    if (!st) {
        REprintf("opus_decode: failed to open '%s'.\n", inFile);
        return ScalarInteger(1);
    }

    /* ── optional manual gain ─────────────────────────────────────── */
    if (manual_gain != 0.f) {
        op_set_gain_offset(st, OP_HEADER_GAIN,
                           (int)(manual_gain * 256.f));
    }

    /* ── determine output rate / channels ────────────────────────── */
    const OpusHead *head = op_head(st, 0);

    if (rate == 0) {
        rate = (int)head->input_sample_rate;
        if (rate == 0) rate = 48000;
    }
    if (rate < 8000 || rate > 192000) {
        REprintf("opus_decode: unusual rate %d, falling back to 48000.\n",
                 rate);
        rate = 48000;
    }

    int channels = force_stereo ? 2 : head->channel_count;

    /* ── open output WAV ──────────────────────────────────────────── */
    fout = fopen(outFile, "wb");
    if (!fout) {
        REprintf("opus_decode: cannot open output file '%s'.\n", outFile);
        op_free(st);
        return ScalarInteger(1);
    }
    write_wav_header(fout, rate, channels, fp);

    /* ── allocate decode buffer ───────────────────────────────────── */
    output = (float *)malloc(sizeof(float) * MAX_FRAME_SIZE * channels);
    if (!output) {
        REprintf("opus_decode: memory allocation failure.\n");
        exit_code = 1;
        goto cleanup;
    }

    /* ── decode loop ─────────────────────────────────────────────── */
    while (1) {
        int nb_read;

        if (force_stereo)
            nb_read = op_read_float_stereo(st, output,
                                           MAX_FRAME_SIZE * channels);
        else
            nb_read = op_read_float(st, output,
                                    MAX_FRAME_SIZE * channels, NULL);

        if (nb_read < 0) {
            if (nb_read == OP_HOLE) {
                REprintf("opus_decode: warning – hole in data.\n");
                continue;
            }
            REprintf("opus_decode: decoding error (%d).\n", nb_read);
            exit_code = 1;
            break;
        }
        if (nb_read == 0) break;   /* EOF */

        int total_samples = nb_read * channels;

        if (fp) {
            /* 32-bit IEEE float */
            fwrite(output, sizeof(float), total_samples, fout);
            data_bytes += (long)(sizeof(float) * total_samples);
        } else {
            /* 16-bit PCM with simple clip */
            short *pcm = (short *)malloc(sizeof(short) * total_samples);
            if (!pcm) {
                REprintf("opus_decode: memory allocation failure.\n");
                exit_code = 1;
                break;
            }
            for (int i = 0; i < total_samples; i++) {
                float s = output[i];
                if      (s >  1.f) s =  1.f;
                else if (s < -1.f) s = -1.f;
                pcm[i] = (short)(s * 32767.f);
            }
            fwrite(pcm, sizeof(short), total_samples, fout);
            data_bytes += (long)(sizeof(short) * total_samples);
            free(pcm);
        }
    }

    /* ── patch WAV header with real sizes ────────────────────────── */
    if (exit_code == 0) finalize_wav_header(fout, data_bytes);

cleanup:
    if (output) free(output);
    if (fout)   fclose(fout);
    if (st)     op_free(st);

    return ScalarInteger(exit_code);
}