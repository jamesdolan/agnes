#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef AGNES_AMALGAMATED
#include "apu.h"
#include "common.h"
#include "cpu.h"
#endif

// Square wave duty cycles (4-step patterns)
static const uint8_t square_duty_cycles[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
    {0, 1, 1, 0, 0, 0, 0, 0}, // 25%
    {0, 1, 1, 1, 1, 0, 0, 0}, // 50%
    {1, 0, 0, 1, 1, 1, 1, 1}  // 75%
};

// Triangle wave steps (32-step pattern)
static const uint8_t triangle_steps[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

// Noise period table
static const uint16_t noise_periods[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

// Length counter table
static const uint8_t length_counter_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

void apu_init(apu_t *apu, agnes_t *agnes) {
    memset(apu, 0, sizeof(*apu));
    apu->agnes = agnes;
    
    // Initialize shift register for noise channel
    apu->noise.shift_register = 1;
    
    // Initialize audio buffer
    apu->audio_buffer_index = 0;
    apu->audio_buffer_size = 0;
}

void apu_tick(apu_t *apu) {
    apu->cycles++;
    
    // APU runs at 1/2 CPU speed
    if (apu->cycles % 2 == 0) {
        apu_tick_square_channel(&apu->square1);
        apu_tick_square_channel(&apu->square2);
        apu_tick_triangle_channel(&apu->triangle);
        apu_tick_noise_channel(&apu->noise);
        apu_tick_dmc_channel(&apu->dmc);
        
        // Frame counter ticks every 7457 CPU cycles (14914 APU cycles)
        if (apu->cycles % 14914 == 0) {
            apu_tick_frame_counter(apu);
        }
    }
    
    // Generate audio samples at 44.1kHz
    // CPU runs at ~1.79MHz, so we need to generate samples every ~40.6 CPU cycles
    if (apu->cycles % 81 == 0) {
        int16_t sample = apu_mix_audio(apu);
        if (apu->audio_buffer_index < APU_BUFFER_SIZE) {
            apu->audio_buffer[apu->audio_buffer_index++] = sample;
            apu->audio_buffer_size = apu->audio_buffer_index;
        }
    }
}

void apu_write_register(apu_t *apu, apu_register_t addr, uint8_t val) {
    switch (addr) {
        case APU_SQ1_VOL: {
            apu->square1.duty_cycle = (apu_duty_cycle_t)((val >> 6) & 0x3);
            apu->square1.length_counter_halt = (val >> 5) & 0x1;
            apu->square1.envelope_loop = (val >> 5) & 0x1;
            apu->square1.use_constant_volume = (val >> 4) & 0x1;
            apu->square1.constant_volume = val & 0xf;
            break;
        }
            
        case APU_SQ1_SWEEP:
            apu->square1.sweep_enabled = (val >> 7) & 0x1;
            apu->square1.sweep_period = (val >> 4) & 0x7;
            apu->square1.sweep_negate = (val >> 3) & 0x1;
            apu->square1.sweep_shift = val & 0x7;
            apu->square1.sweep_reload = true;
            break;
            
        case APU_SQ1_LO:
            apu->square1.timer_reload = (apu->square1.timer_reload & 0xff00) | val;
            break;
            
        case APU_SQ1_HI:
            apu->square1.timer_reload = (apu->square1.timer_reload & 0x00ff) | ((val & 0x7) << 8);
            apu->square1.length_counter = length_counter_table[(val >> 3) & 0x1f];
            apu->square1.envelope_start = true;
            apu->square1.duty_step = 0;
            break;
            
        case APU_SQ2_VOL:
            apu->square2.duty_cycle = (apu_duty_cycle_t)((val >> 6) & 0x3);
            apu->square2.length_counter_halt = (val >> 5) & 0x1;
            apu->square2.envelope_loop = (val >> 5) & 0x1;
            apu->square2.use_constant_volume = (val >> 4) & 0x1;
            apu->square2.constant_volume = val & 0xf;
            break;
            
        case APU_SQ2_SWEEP:
            apu->square2.sweep_enabled = (val >> 7) & 0x1;
            apu->square2.sweep_period = (val >> 4) & 0x7;
            apu->square2.sweep_negate = (val >> 3) & 0x1;
            apu->square2.sweep_shift = val & 0x7;
            apu->square2.sweep_reload = true;
            break;
            
        case APU_SQ2_LO:
            apu->square2.timer_reload = (apu->square2.timer_reload & 0xff00) | val;
            break;
            
        case APU_SQ2_HI:
            apu->square2.timer_reload = (apu->square2.timer_reload & 0x00ff) | ((val & 0x7) << 8);
            apu->square2.length_counter = length_counter_table[(val >> 3) & 0x1f];
            apu->square2.envelope_start = true;
            apu->square2.duty_step = 0;
            break;
            
        case APU_TRI_LINEAR:
            apu->triangle.linear_counter_control = (val >> 7) & 0x1;
            apu->triangle.length_counter_halt = (val >> 7) & 0x1;
            apu->triangle.linear_counter_reload = val & 0x7f;
            break;
            
        case APU_TRI_LO:
            apu->triangle.timer_reload = (apu->triangle.timer_reload & 0xff00) | val;
            break;
            
        case APU_TRI_HI:
            apu->triangle.timer_reload = (apu->triangle.timer_reload & 0x00ff) | ((val & 0x7) << 8);
            apu->triangle.length_counter = length_counter_table[(val >> 3) & 0x1f];
            apu->triangle.linear_counter_reload_flag = true;
            break;
            
        case APU_NOISE_VOL:
            apu->noise.length_counter_halt = (val >> 5) & 0x1;
            apu->noise.envelope_loop = (val >> 5) & 0x1;
            apu->noise.use_constant_volume = (val >> 4) & 0x1;
            apu->noise.constant_volume = val & 0xf;
            break;
            
        case APU_NOISE_LO:
            apu->noise.mode = (val >> 7) & 0x1;
            apu->noise.timer_reload = noise_periods[val & 0xf];
            break;
            
        case APU_NOISE_HI:
            apu->noise.length_counter = length_counter_table[(val >> 3) & 0x1f];
            apu->noise.envelope_start = true;
            break;
            
        case APU_DMC_FREQ:
            apu->dmc.irq_enabled = (val >> 7) & 0x1;
            apu->dmc.loop = (val >> 6) & 0x1;
            apu->dmc.timer_reload = (val & 0xf) == 0 ? 428 : (val & 0xf) * 32;
            break;
            
        case APU_DMC_RAW:
            apu->dmc.output_level = val & 0x7f;
            break;
            
        case APU_DMC_START:
            apu->dmc.sample_address = 0xc000 | (val << 6);
            break;
            
        case APU_DMC_LEN:
            apu->dmc.sample_length = (val << 4) | 1;
            break;
            
        case APU_STATUS:
            apu->square1.enabled = (val >> 0) & 0x1;
            apu->square2.enabled = (val >> 1) & 0x1;
            apu->triangle.enabled = (val >> 2) & 0x1;
            apu->noise.enabled = (val >> 3) & 0x1;
            apu->dmc.enabled = (val >> 4) & 0x1;
            
            if (!apu->square1.enabled) apu->square1.length_counter = 0;
            if (!apu->square2.enabled) apu->square2.length_counter = 0;
            if (!apu->triangle.enabled) apu->triangle.length_counter = 0;
            if (!apu->noise.enabled) apu->noise.length_counter = 0;
            if (!apu->dmc.enabled) apu->dmc.bytes_remaining = 0;
            break;
            
        case APU_FRAME_COUNTER:
            apu->frame_counter_mode = (apu_frame_counter_mode_t)((val >> 7) & 0x1);
            apu->frame_irq_enabled = !((val >> 6) & 0x1);
            apu->frame_counter = 0;
            break;
            
        default:
            // Invalid register address - ignore write
            break;
    }
}

uint8_t apu_read_register(apu_t *apu, apu_register_t addr) {
    switch (addr) {
        case APU_STATUS: {
            uint8_t status = 0;
            status |= (apu->dmc.bytes_remaining > 0) << 4;
            status |= (apu->noise.length_counter > 0) << 3;
            status |= (apu->triangle.length_counter > 0) << 2;
            status |= (apu->square2.length_counter > 0) << 1;
            status |= (apu->square1.length_counter > 0) << 0;
            status |= apu->frame_irq_pending << 6;
            status |= apu->dmc_irq_pending << 7;
            
            // Clear frame IRQ flag
            apu->frame_irq_pending = false;
            
            return status;
        }
        
        default:
            // Invalid register address - return 0
            return 0;
    }
}

void apu_get_audio_samples(const apu_t *apu, int16_t *samples, int count) {
    int available = apu->audio_buffer_size;
    int to_copy = (count < available) ? count : available;
    
    memcpy(samples, apu->audio_buffer, to_copy * sizeof(int16_t));
    
    // Fill remaining with silence
    for (int i = to_copy; i < count; i++) {
        samples[i] = 0;
    }
}

void apu_clear_audio_buffer(apu_t *apu) {
    apu->audio_buffer_size = 0;
    apu->audio_buffer_index = 0;
}

AGNES_INTERNAL void apu_tick_square_channel(square_channel_t *channel) {
    if (channel->timer > 0) {
        channel->timer--;
    } else {
        channel->timer = channel->timer_reload;
        channel->duty_step = (channel->duty_step + 1) % 8;
    }
    
    // Generate output
    if (channel->length_counter > 0 && channel->timer_reload > 7) {
        uint8_t duty_value = square_duty_cycles[channel->duty_cycle][channel->duty_step];
        uint8_t volume = channel->use_constant_volume ? 
                        channel->constant_volume : channel->volume;
        
        channel->output = duty_value ? (volume * 1000) : 0;
    } else {
        channel->output = 0;
    }
}

AGNES_INTERNAL void apu_tick_triangle_channel(triangle_channel_t *channel) {
    if (channel->timer > 0) {
        channel->timer--;
    } else {
        channel->timer = channel->timer_reload;
        if (channel->linear_counter > 0 && channel->length_counter > 0) {
            channel->step_counter = (channel->step_counter + 1) % 32;
        }
    }
    
    // Generate output
    if (channel->length_counter > 0 && channel->linear_counter > 0 && channel->timer_reload > 1) {
        channel->output = (triangle_steps[channel->step_counter] - 7) * 200;
    } else {
        channel->output = 0;
    }
}

AGNES_INTERNAL void apu_tick_noise_channel(noise_channel_t *channel) {
    if (channel->timer > 0) {
        channel->timer--;
    } else {
        channel->timer = channel->timer_reload;
        
        // Update shift register
        uint16_t feedback = (channel->shift_register >> 0) ^ (channel->shift_register >> (channel->mode ? 6 : 1));
        channel->shift_register >>= 1;
        channel->shift_register |= (feedback & 1) << 14;
    }
    
    // Generate output
    if (channel->length_counter > 0) {
        uint8_t volume = channel->use_constant_volume ? 
                        channel->constant_volume : channel->volume;
        
        channel->output = (channel->shift_register & 1) ? 0 : (volume * 1000);
    } else {
        channel->output = 0;
    }
}

AGNES_INTERNAL void apu_tick_dmc_channel(dmc_channel_t *channel) {
    if (channel->timer > 0) {
        channel->timer--;
    } else {
        channel->timer = channel->timer_reload;
        
        if (channel->bits_remaining > 0) {
            // Output current bit
            if (channel->shift_register & 1) {
                if (channel->output_level <= 125) {
                    channel->output_level += 2;
                }
            } else {
                if (channel->output_level >= 2) {
                    channel->output_level -= 2;
                }
            }
            
            channel->shift_register >>= 1;
            channel->bits_remaining--;
        }
        
        // Load new byte if needed
        if (channel->bits_remaining == 0 && channel->bytes_remaining > 0) {
            // Read from CPU memory space
            channel->sample_buffer = cpu_read8(&channel->agnes->cpu, channel->current_address);
            channel->shift_register = channel->sample_buffer;
            channel->bits_remaining = 8;
            channel->current_address = (channel->current_address + 1) & 0xffff;
            channel->bytes_remaining--;
            
            if (channel->bytes_remaining == 0 && channel->loop) {
                channel->current_address = channel->sample_address;
                channel->bytes_remaining = channel->sample_length;
            } else if (channel->bytes_remaining == 0 && channel->irq_enabled) {
                channel->agnes->apu.dmc_irq_pending = true;
            }
        }
    }
    
    channel->output = (channel->output_level - 64) * 16;
}

AGNES_INTERNAL void apu_tick_frame_counter(apu_t *apu) {
    apu->frame_counter++;
    
    if (apu->frame_counter_mode == APU_FRAME_COUNTER_MODE_4STEP) {
        // 4-step sequence
        switch (apu->frame_counter) {
            case 1:
            case 3:
                apu_update_length_counters(apu);
                apu_update_sweeps(apu);
                break;
            case 2:
                apu_update_length_counters(apu);
                apu_update_sweeps(apu);
                apu_update_envelopes(apu);
                apu_update_linear_counter(apu);
                break;
            case 4:
                apu_update_length_counters(apu);
                apu_update_sweeps(apu);
                apu_update_envelopes(apu);
                apu_update_linear_counter(apu);
                if (apu->frame_irq_enabled) {
                    apu->frame_irq_pending = true;
                }
                apu->frame_counter = 0;
                break;
        }
    } else {
        // 5-step sequence
        switch (apu->frame_counter) {
            case 1:
            case 3:
                apu_update_length_counters(apu);
                apu_update_sweeps(apu);
                break;
            case 2:
            case 4:
                apu_update_length_counters(apu);
                apu_update_sweeps(apu);
                apu_update_envelopes(apu);
                apu_update_linear_counter(apu);
                break;
            case 5:
                apu_update_length_counters(apu);
                apu_update_sweeps(apu);
                apu_update_envelopes(apu);
                apu_update_linear_counter(apu);
                apu->frame_counter = 0;
                break;
        }
    }
}

AGNES_INTERNAL void apu_update_length_counters(apu_t *apu) {
    if (!apu->square1.length_counter_halt && apu->square1.length_counter > 0) {
        apu->square1.length_counter--;
    }
    if (!apu->square2.length_counter_halt && apu->square2.length_counter > 0) {
        apu->square2.length_counter--;
    }
    if (!apu->triangle.length_counter_halt && apu->triangle.length_counter > 0) {
        apu->triangle.length_counter--;
    }
    if (!apu->noise.length_counter_halt && apu->noise.length_counter > 0) {
        apu->noise.length_counter--;
    }
}

AGNES_INTERNAL void apu_update_envelopes(apu_t *apu) {
    // Square 1 envelope
    if (apu->square1.envelope_start) {
        apu->square1.envelope_start = false;
        apu->square1.envelope_counter = 15;
        apu->square1.envelope_divider = apu->square1.constant_volume;
    } else {
        if (apu->square1.envelope_divider == 0) {
            apu->square1.envelope_divider = apu->square1.constant_volume;
            if (apu->square1.envelope_counter > 0) {
                apu->square1.envelope_counter--;
            } else if (apu->square1.envelope_loop) {
                apu->square1.envelope_counter = 15;
            }
        } else {
            apu->square1.envelope_divider--;
        }
    }
    apu->square1.volume = apu->square1.envelope_counter;
    
    // Square 2 envelope
    if (apu->square2.envelope_start) {
        apu->square2.envelope_start = false;
        apu->square2.envelope_counter = 15;
        apu->square2.envelope_divider = apu->square2.constant_volume;
    } else {
        if (apu->square2.envelope_divider == 0) {
            apu->square2.envelope_divider = apu->square2.constant_volume;
            if (apu->square2.envelope_counter > 0) {
                apu->square2.envelope_counter--;
            } else if (apu->square2.envelope_loop) {
                apu->square2.envelope_counter = 15;
            }
        } else {
            apu->square2.envelope_divider--;
        }
    }
    apu->square2.volume = apu->square2.envelope_counter;
    
    // Noise envelope
    if (apu->noise.envelope_start) {
        apu->noise.envelope_start = false;
        apu->noise.envelope_counter = 15;
        apu->noise.envelope_divider = apu->noise.constant_volume;
    } else {
        if (apu->noise.envelope_divider == 0) {
            apu->noise.envelope_divider = apu->noise.constant_volume;
            if (apu->noise.envelope_counter > 0) {
                apu->noise.envelope_counter--;
            } else if (apu->noise.envelope_loop) {
                apu->noise.envelope_counter = 15;
            }
        } else {
            apu->noise.envelope_divider--;
        }
    }
    apu->noise.volume = apu->noise.envelope_counter;
}

AGNES_INTERNAL void apu_update_linear_counter(apu_t *apu) {
    if (apu->triangle.linear_counter_reload_flag) {
        apu->triangle.linear_counter = apu->triangle.linear_counter_reload;
    } else if (apu->triangle.linear_counter > 0) {
        apu->triangle.linear_counter--;
    }
    
    if (!apu->triangle.linear_counter_control) {
        apu->triangle.linear_counter_reload_flag = false;
    }
}

AGNES_INTERNAL void apu_update_sweeps(apu_t *apu) {
    // Square 1 sweep
    if (apu->square1.sweep_reload) {
        apu->square1.sweep_reload = false;
        apu->square1.sweep_counter = apu->square1.sweep_period;
    } else if (apu->square1.sweep_counter > 0) {
        apu->square1.sweep_counter--;
    } else {
        apu->square1.sweep_counter = apu->square1.sweep_period;
        if (apu->square1.sweep_enabled && apu->square1.sweep_shift > 0) {
            uint16_t change = apu->square1.timer_reload >> apu->square1.sweep_shift;
            if (apu->square1.sweep_negate) {
                apu->square1.timer_reload -= change;
            } else {
                apu->square1.timer_reload += change;
            }
        }
    }
    
    // Square 2 sweep
    if (apu->square2.sweep_reload) {
        apu->square2.sweep_reload = false;
        apu->square2.sweep_counter = apu->square2.sweep_period;
    } else if (apu->square2.sweep_counter > 0) {
        apu->square2.sweep_counter--;
    } else {
        apu->square2.sweep_counter = apu->square2.sweep_period;
        if (apu->square2.sweep_enabled && apu->square2.sweep_shift > 0) {
            uint16_t change = apu->square2.timer_reload >> apu->square2.sweep_shift;
            if (apu->square2.sweep_negate) {
                apu->square2.timer_reload -= change;
            } else {
                apu->square2.timer_reload += change;
            }
        }
    }
}

AGNES_INTERNAL int16_t apu_mix_audio(apu_t *apu) {
    int32_t mixed = 0;
    
    // Mix all channels
    mixed += apu->square1.output;
    mixed += apu->square2.output;
    mixed += apu->triangle.output;
    mixed += apu->noise.output;
    mixed += apu->dmc.output;
    
    // Apply some basic mixing (divide by number of channels)
    mixed /= 5;
    
    // Clamp to 16-bit range
    if (mixed > 32767) mixed = 32767;
    if (mixed < -32768) mixed = -32768;
    
    return (int16_t)mixed;
} 