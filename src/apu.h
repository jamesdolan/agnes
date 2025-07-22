#ifndef apu_h
#define apu_h

#ifndef AGNES_AMALGAMATED
#include "common.h"
#include "agnes_types.h"
#endif

// Function declarations
void apu_init(apu_t *apu, agnes_t *agnes);
void apu_tick(apu_t *apu);
void apu_write_register(apu_t *apu, apu_register_t addr, uint8_t val);
uint8_t apu_read_register(apu_t *apu, apu_register_t addr);
void apu_get_audio_samples(const apu_t *apu, int16_t *samples, int count);
void apu_clear_audio_buffer(apu_t *apu);

// Internal functions
AGNES_INTERNAL void apu_tick_square_channel(square_channel_t *channel);
AGNES_INTERNAL void apu_tick_triangle_channel(triangle_channel_t *channel);
AGNES_INTERNAL void apu_tick_noise_channel(noise_channel_t *channel);
AGNES_INTERNAL void apu_tick_dmc_channel(dmc_channel_t *channel);
AGNES_INTERNAL void apu_tick_frame_counter(apu_t *apu);
AGNES_INTERNAL void apu_update_length_counters(apu_t *apu);
AGNES_INTERNAL void apu_update_envelopes(apu_t *apu);
AGNES_INTERNAL void apu_update_linear_counter(apu_t *apu);
AGNES_INTERNAL void apu_update_sweeps(apu_t *apu);
AGNES_INTERNAL int16_t apu_mix_audio(apu_t *apu);

#endif /* apu_h */ 