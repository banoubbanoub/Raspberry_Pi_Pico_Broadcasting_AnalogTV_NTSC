


/**

 *
 * Description: Channel 3 RF video using RP2040 PIO and DMA.
 */

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/pio_instructions.h"

#include "video_broadcast.h"
#include "broadcast_tables.h"
#include "CbTable.h"

#define VIDEO_OUT_PIN 21
#define LINE32LEN 159
#define LINE_SAMPLE_COUNT LINE32LEN
#define VIDEO_BIT_RATE 80000000.0f

#ifdef PAL
#define SHORT_SYNC_INTERVAL 5
#define LONG_SYNC_INTERVAL 75
#define NORMAL_SYNC_INTERVAL 10
#define COLORBURST_INTERVAL 10
#else
#define SHORT_SYNC_INTERVAL 6
#define LONG_SYNC_INTERVAL 73
#define SERRATION_PULSE_INT 67
#define NORMAL_SYNC_INTERVAL 12
#define COLORBURST_INTERVAL 4
#endif

int8_t jam_color = -1; 
int gline = 0;
volatile int gframe = 0;
static int linescratch;
uint16_t framebuffer[((FBW2/4)*(FBH))*2];

uint32_t * curdma;

uint8_t pixline; // line number currently being written out.

static int dma_channels[DMABUFFERDEPTH];
static uint32_t line_buffers[DMABUFFERDEPTH][LINE_SAMPLE_COUNT];
static const uint32_t *const table_start = premodulated_table;
static const uint32_t *const table_end =
    premodulated_table + PREMOD_ENTRIES * PREMOD_SIZE;
static const uint32_t *table_cursor = premodulated_table;

static inline uint32_t next_sample(uint8_t color)
{
    const uint32_t sample = table_cursor[color];
    table_cursor += PREMOD_SIZE;
    if (table_cursor >= table_end)
        table_cursor = table_start + (table_cursor - table_end);
    return sample;
}

static inline void fillwith(uint16_t qty, uint8_t color)
{
    while (qty--)
        *curdma++ = next_sample(color);
}

static inline void fill_pixel(uint8_t color)
{
    *curdma++ = next_sample(color);
}

// Line Generation Functions
static void FT_STA() { // Short Sync
    pixline = 0; 
    fillwith(SHORT_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(LONG_SYNC_INTERVAL, BLACK_LEVEL);
    fillwith(SHORT_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(LINE32LEN - (SHORT_SYNC_INTERVAL+LONG_SYNC_INTERVAL+SHORT_SYNC_INTERVAL), BLACK_LEVEL);
}

static void FT_STB() { // Long Sync
    #ifdef PAL
        #define FT_STB_BLACK_INTERVAL SHORT_SYNC_INTERVAL
    #else
        #define FT_STB_BLACK_INTERVAL NORMAL_SYNC_INTERVAL
    #endif
    fillwith(LONG_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(FT_STB_BLACK_INTERVAL, BLACK_LEVEL);
    fillwith(LONG_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(LINE32LEN - (LONG_SYNC_INTERVAL+FT_STB_BLACK_INTERVAL+LONG_SYNC_INTERVAL), BLACK_LEVEL);
}

static void FT_B() { // Black
    fillwith(NORMAL_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(2, BLACK_LEVEL);
    fillwith(COLORBURST_INTERVAL, COLORBURST_LEVEL);
    fillwith(LINE32LEN-NORMAL_SYNC_INTERVAL-2-COLORBURST_INTERVAL, (pixline < 1) ? GRAY_LEVEL : BLACK_LEVEL);
}

static void FT_SRA() { // Short to long
    fillwith(SHORT_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(LONG_SYNC_INTERVAL, BLACK_LEVEL);
    #ifdef PAL
    fillwith(LONG_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(LINE32LEN - (SHORT_SYNC_INTERVAL+LONG_SYNC_INTERVAL+LONG_SYNC_INTERVAL), BLACK_LEVEL);
    #else
    fillwith(SERRATION_PULSE_INT, SYNC_LEVEL);
    fillwith(LINE32LEN - (SHORT_SYNC_INTERVAL+LONG_SYNC_INTERVAL+SERRATION_PULSE_INT), BLACK_LEVEL);
    #endif
}

static void FT_SRB() { // Long to short
    #ifdef PAL
    fillwith(LONG_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(SHORT_SYNC_INTERVAL, BLACK_LEVEL);
    fillwith(SHORT_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(LINE32LEN - (LONG_SYNC_INTERVAL+SHORT_SYNC_INTERVAL+SHORT_SYNC_INTERVAL), BLACK_LEVEL);
    #else
    fillwith(SERRATION_PULSE_INT, SYNC_LEVEL);
    fillwith(NORMAL_SYNC_INTERVAL, BLACK_LEVEL);
    fillwith(SHORT_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(LINE32LEN - (SERRATION_PULSE_INT+NORMAL_SYNC_INTERVAL+SHORT_SYNC_INTERVAL), BLACK_LEVEL);
    #endif
}

static void FT_LIN() { // Line Signal
    fillwith(NORMAL_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(1, BLACK_LEVEL);
    fillwith(COLORBURST_INTERVAL, COLORBURST_LEVEL);
    fillwith(11, BLACK_LEVEL);
    #define HDR_SPD (NORMAL_SYNC_INTERVAL+1+COLORBURST_INTERVAL+11)

    int fframe = gframe & 1;
    uint16_t * fbs = (uint16_t*)(&framebuffer[(((pixline * (FBW2/2)) + (((FBW2/2)*(FBH))*(fframe))) / 2)]);

    for (linescratch = 0; linescratch < FBW2/4; linescratch++) {
        uint16_t fbb = fbs[linescratch];
        fill_pixel((fbb >> 0) & 0x0f);
        fill_pixel((fbb >> 4) & 0x0f);
        fill_pixel((fbb >> 8) & 0x0f);
        fill_pixel((fbb >> 12) & 0x0f);
    }

    fillwith(LINE32LEN - (HDR_SPD+FBW2), BLACK_LEVEL);
    pixline++;
}

static void FT_CLOSE_M() { // End Frame
    #ifdef PAL
    fillwith(SHORT_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(LONG_SYNC_INTERVAL, BLACK_LEVEL);
    fillwith(SHORT_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(LINE32LEN - (SHORT_SYNC_INTERVAL+LONG_SYNC_INTERVAL+SHORT_SYNC_INTERVAL), BLACK_LEVEL);
    #else
    fillwith(NORMAL_SYNC_INTERVAL, SYNC_LEVEL);
    fillwith(2, BLACK_LEVEL);
    fillwith(4, COLORBURST_LEVEL);
    fillwith(LINE32LEN-NORMAL_SYNC_INTERVAL-6, WHITE_LEVEL);
    #endif
    gline = -1;
    gframe++;
}

void (*CbTable[FT_MAX_d])() = { FT_STA, FT_STB, FT_B, FT_SRA, FT_SRB, FT_LIN, FT_CLOSE_M };

static void generate_line(uint32_t *buffer)
{
    curdma = buffer;
    if (jam_color >= 0) {
        fillwith(LINE32LEN, (uint8_t)jam_color);
        return;
    }

    const uint32_t lookup_index = (uint32_t)gline >> 1;
    const uint8_t packed_types = CbLookup[lookup_index];
    const uint8_t line_type = (gline & 1) ? packed_types >> 4 : packed_types & 0x0f;
    CbTable[line_type]();
    gline++;
}

static void dma_complete_handler(void)
{
    const uint32_t pending = dma_hw->ints0;
    dma_hw->ints0 = pending;

    for (uint i = 0; i < DMABUFFERDEPTH; i++) {
        if (pending & (1u << dma_channels[i])) {
            generate_line(line_buffers[i]);
            dma_channel_set_read_addr(dma_channels[i], line_buffers[i], false);
            dma_channel_set_trans_count(dma_channels[i], LINE_SAMPLE_COUNT, false);
        }
    }
}

void video_init(void)
{
    const uint16_t video_program_instruction = pio_encode_out(pio_pins, 1);
    const struct pio_program video_program = {
        .instructions = &video_program_instruction,
        .length = 1,
        .origin = -1
    };

    for (uint i = 0; i < DMABUFFERDEPTH; i++) {
        generate_line(line_buffers[i]);
    }

    const uint program_offset = pio_add_program(pio0, &video_program);
    const uint sm = pio_claim_unused_sm(pio0, true);
    pio_gpio_init(pio0, VIDEO_OUT_PIN);

    pio_sm_config sm_config = pio_get_default_sm_config();
    sm_config_set_wrap(&sm_config, program_offset, program_offset);
    sm_config_set_out_pins(&sm_config, VIDEO_OUT_PIN, 1);
    sm_config_set_out_shift(&sm_config, false, true, 32);
    sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(
        &sm_config,
        (float)clock_get_hz(clk_sys) / VIDEO_BIT_RATE
    );
    pio_sm_init(pio0, sm, program_offset, &sm_config);
    pio_sm_set_consecutive_pindirs(pio0, sm, VIDEO_OUT_PIN, 1, true);

    // Prime the serializer before enabling autopull, so the first video word
    // starts at the beginning of the first generated line.
    pio_sm_put_blocking(pio0, sm, line_buffers[0][0]);
    pio_sm_exec(pio0, sm, pio_encode_pull(false, true));

    dma_channels[0] = dma_claim_unused_channel(true);
    dma_channels[1] = dma_claim_unused_channel(true);
    dma_channel_config config[DMABUFFERDEPTH];
    for (uint i = 0; i < DMABUFFERDEPTH; i++) {
        config[i] = dma_channel_get_default_config(dma_channels[i]);
        channel_config_set_transfer_data_size(&config[i], DMA_SIZE_32);
        channel_config_set_read_increment(&config[i], true);
        channel_config_set_write_increment(&config[i], false);
        channel_config_set_dreq(&config[i], pio_get_dreq(pio0, sm, true));
        channel_config_set_chain_to(&config[i], dma_channels[(i + 1) % DMABUFFERDEPTH]);

        dma_channel_configure(
            dma_channels[i],
            &config[i],
            &pio0->txf[sm],
            line_buffers[i] + (i == 0 ? 1 : 0),
            LINE_SAMPLE_COUNT - (i == 0 ? 1 : 0),
            false
        );
        dma_channel_set_irq0_enabled(dma_channels[i], true);
    }

    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_start(dma_channels[0]);
    pio_sm_set_enabled(pio0, sm, true);
}