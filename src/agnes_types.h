#ifndef agnes_types_h
#define agnes_types_h

#ifndef AGNES_AMALGAMATED
#include "common.h"
#include "agnes.h"
#endif

/************************************ CPU ************************************/

typedef enum {
    INTERRPUT_NONE = 0,
    INTERRUPT_NMI = 1,
    INTERRUPT_IRQ = 2
} cpu_interrupt_t;

typedef struct cpu {
    struct agnes *agnes;
    uint16_t pc;
    uint8_t sp;
    uint8_t acc;
    uint8_t x;
    uint8_t y;
    uint8_t flag_carry;
    uint8_t flag_zero;
    uint8_t flag_dis_interrupt;
    uint8_t flag_decimal;
    uint8_t flag_overflow;
    uint8_t flag_negative;
    uint32_t stall;
    uint64_t cycles;
    cpu_interrupt_t interrupt;
} cpu_t;

/************************************ PPU ************************************/

typedef struct {
    uint8_t y_pos;
    uint8_t tile_num;
    uint8_t attrs;
    uint8_t x_pos;
} sprite_t;

typedef struct ppu {
    struct agnes *agnes;

    uint8_t nametables[4 * 1024];
    uint8_t palette[32];

    uint8_t screen_buffer[AGNES_SCREEN_HEIGHT * AGNES_SCREEN_WIDTH];

    int scanline;
    int dot;

    uint8_t ppudata_buffer;
    uint8_t last_reg_write;

    struct {
        uint16_t v;
        uint16_t t;
        uint8_t x;
        uint8_t w;
    } regs;

    struct {
        bool show_leftmost_bg;
        bool show_leftmost_sprites;
        bool show_background;
        bool show_sprites;
    } masks;

    uint8_t nt;
    uint8_t at;
    uint8_t at_latch;
    uint16_t at_shift;
    uint8_t bg_hi;
    uint8_t bg_lo;
    uint16_t bg_hi_shift;
    uint16_t bg_lo_shift;

    struct {
        uint16_t addr_increment;
        uint16_t sprite_table_addr;
        uint16_t bg_table_addr;
        bool use_8x16_sprites;
        bool nmi_enabled;
    } ctrl;

    struct {
        bool in_vblank;
        bool sprite_overflow;
        bool sprite_zero_hit;
    } status;

    bool is_odd_frame;

    uint8_t oam_address;
    uint8_t oam_data[256];
    sprite_t sprites[8];
    int sprite_ixs[8];
    int sprite_ixs_count;
} ppu_t;

/********************************** MAPPERS **********************************/

typedef enum {
    MIRRORING_MODE_NONE,
    MIRRORING_MODE_SINGLE_LOWER,
    MIRRORING_MODE_SINGLE_UPPER,
    MIRRORING_MODE_HORIZONTAL,
    MIRRORING_MODE_VERTICAL,
    MIRRORING_MODE_FOUR_SCREEN
} mirroring_mode_t;

typedef struct mapper0 {
    struct agnes *agnes;

    unsigned prg_bank_offsets[2];
    bool use_chr_ram;
    uint8_t chr_ram[8 * 1024];
} mapper0_t;

typedef struct mapper1 {
    struct agnes *agnes;

    uint8_t shift;
    int shift_count;
    uint8_t control;
    int prg_mode;
    int chr_mode;
    int chr_banks[2];
    int prg_bank;
    unsigned chr_bank_offsets[2];
    unsigned prg_bank_offsets[2];
    bool use_chr_ram;
    uint8_t chr_ram[8 * 1024];
    uint8_t prg_ram[8 * 1024];
} mapper1_t;

typedef struct mapper2 {
    struct agnes *agnes;

    unsigned prg_bank_offsets[2];
    uint8_t chr_ram[8 * 1024];
} mapper2_t;

typedef struct mapper4 {
    struct agnes *agnes;

    unsigned prg_mode;
    unsigned chr_mode;
    bool irq_enabled;
    int reg_ix;
    uint8_t regs[8];
    uint8_t counter;
    uint8_t counter_reload;
    unsigned chr_bank_offsets[8];
    unsigned prg_bank_offsets[4];
    uint8_t prg_ram[8 * 1024];
    bool use_chr_ram;
    uint8_t chr_ram[8 * 1024];
} mapper4_t;

/********************************* GAMEPACK **********************************/

typedef struct {
    const uint8_t *data;
    unsigned prg_rom_offset;
    unsigned chr_rom_offset;
    int prg_rom_banks_count;
    int chr_rom_banks_count;
    bool has_prg_ram;
    unsigned char mapper;
} gamepack_t;

/******************************** CONTROLLER *********************************/

typedef struct controller {
    uint8_t state;
    uint8_t shift;
} controller_t;

/*********************************** APU ***********************************/

// APU register addresses
typedef enum {
    APU_SQ1_VOL = 0x4000,
    APU_SQ1_SWEEP = 0x4001,
    APU_SQ1_LO = 0x4002,
    APU_SQ1_HI = 0x4003,
    APU_SQ2_VOL = 0x4004,
    APU_SQ2_SWEEP = 0x4005,
    APU_SQ2_LO = 0x4006,
    APU_SQ2_HI = 0x4007,
    APU_TRI_LINEAR = 0x4008,
    APU_TRI_LO = 0x400A,
    APU_TRI_HI = 0x400B,
    APU_NOISE_VOL = 0x400C,
    APU_NOISE_LO = 0x400E,
    APU_NOISE_HI = 0x400F,
    APU_DMC_FREQ = 0x4010,
    APU_DMC_RAW = 0x4011,
    APU_DMC_START = 0x4012,
    APU_DMC_LEN = 0x4013,
    APU_STATUS = 0x4015,
    APU_FRAME_COUNTER = 0x4017
} apu_register_t;

// APU status register bits
typedef enum {
    APU_STATUS_DMC_IRQ = 0x80,
    APU_STATUS_FRAME_IRQ = 0x40,
    APU_STATUS_NOISE = 0x20,
    APU_STATUS_TRIANGLE = 0x10,
    APU_STATUS_SQUARE2 = 0x08,
    APU_STATUS_SQUARE1 = 0x04
} apu_status_bits_t;

// Frame counter modes
typedef enum {
    APU_FRAME_COUNTER_MODE_4STEP = 0x00,
    APU_FRAME_COUNTER_MODE_5STEP = 0x80
} apu_frame_counter_mode_t;

// Audio configuration constants
typedef enum {
    APU_SAMPLE_RATE = 44100,
    APU_BUFFER_SIZE = 1024
} apu_config_t;

// Square wave duty cycles
typedef enum {
    APU_DUTY_12_5 = 0,  // 12.5% duty cycle
    APU_DUTY_25 = 1,    // 25% duty cycle  
    APU_DUTY_50 = 2,    // 50% duty cycle
    APU_DUTY_75 = 3     // 75% duty cycle
} apu_duty_cycle_t;

typedef struct {
    // Square wave channel state
    apu_duty_cycle_t duty_cycle;
    uint8_t duty_step;
    uint16_t timer;
    uint16_t timer_reload;
    uint8_t volume;
    uint8_t constant_volume;
    bool use_constant_volume;
    bool enabled;
    
    // Sweep
    uint8_t sweep_shift;
    bool sweep_negate;
    uint8_t sweep_period;
    uint8_t sweep_counter;
    bool sweep_enabled;
    bool sweep_reload;
    
    // Length counter
    uint8_t length_counter;
    bool length_counter_halt;
    
    // Envelope
    uint8_t envelope_counter;
    uint8_t envelope_divider;
    bool envelope_start;
    bool envelope_loop;
    
    // Output
    int16_t output;
} square_channel_t;

typedef struct {
    // Triangle wave channel state
    uint16_t timer;
    uint16_t timer_reload;
    uint8_t linear_counter;
    uint8_t linear_counter_reload;
    bool linear_counter_control;
    bool linear_counter_reload_flag;
    bool enabled;
    
    // Length counter
    uint8_t length_counter;
    bool length_counter_halt;
    
    // Step counter for triangle wave
    uint8_t step_counter;
    
    // Output
    int16_t output;
} triangle_channel_t;

typedef struct {
    // Noise channel state
    uint16_t timer;
    uint16_t timer_reload;
    uint8_t volume;
    uint8_t constant_volume;
    bool use_constant_volume;
    bool enabled;
    bool mode;
    
    // Shift register
    uint16_t shift_register;
    
    // Length counter
    uint8_t length_counter;
    bool length_counter_halt;
    
    // Envelope
    uint8_t envelope_counter;
    uint8_t envelope_divider;
    bool envelope_start;
    bool envelope_loop;
    
    // Output
    int16_t output;
} noise_channel_t;

typedef struct {
    // DMC channel state
    struct agnes *agnes;
    uint16_t timer;
    uint16_t timer_reload;
    uint8_t output_level;
    uint8_t sample_buffer;
    uint8_t bits_remaining;
    uint8_t shift_register;
    bool enabled;
    bool irq_enabled;
    bool loop;
    
    // Sample address and length
    uint16_t sample_address;
    uint16_t sample_length;
    uint16_t current_address;
    uint16_t bytes_remaining;
    
    // Output
    int16_t output;
} dmc_channel_t;

typedef struct apu {
    struct agnes *agnes;
    
    // Channels
    square_channel_t square1;
    square_channel_t square2;
    triangle_channel_t triangle;
    noise_channel_t noise;
    dmc_channel_t dmc;
    
    // Frame counter
    apu_frame_counter_mode_t frame_counter_mode;
    uint16_t frame_counter;
    bool frame_irq_enabled;
    bool frame_irq_pending;
    bool dmc_irq_pending;
    
    // Status register
    uint8_t status;
    
    // Audio output
    int16_t audio_buffer[APU_BUFFER_SIZE];
    int audio_buffer_index;
    int audio_buffer_size;
    
    // Timing
    uint64_t cycles;
} apu_t;

/*********************************** AGNES ***********************************/
typedef struct agnes {
    cpu_t cpu;
    ppu_t ppu;
    apu_t apu;
    uint8_t ram[2 * 1024];
    gamepack_t gamepack;
    controller_t controllers[2];
    bool controllers_latch;

    union {
        mapper0_t m0;
        mapper1_t m1;
        mapper2_t m2;
        mapper4_t m4;
    } mapper;

    mirroring_mode_t mirroring_mode;
} agnes_t;

#endif /* agnes_types_h */
