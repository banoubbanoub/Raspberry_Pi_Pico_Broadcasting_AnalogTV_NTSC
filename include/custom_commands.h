
/**
 * RP2040 Port of Custom Command Handler
 * Adapted from Charles Lohr's ESP8266 Channel 3 Demo
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "video_broadcast.h"
#include "broadcast_tables.h"

//extern uint8_t showstate;
extern uint8_t showallowadvance;
extern int8_t jam_color;

// Helper function to convert single hex character to value
static uint8_t rp2040_fromhex1(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

int CustomCommand(char *buffer, int retsize, char *pusrdata, unsigned short len)
{
    char *buffend = buffer;

    // Safety check for minimum length
    if (len < 2) return -1;

    switch (pusrdata[1])
    {
    case 'C': case 'c': // Custom command test
    {
        int written = snprintf(buffend, retsize - (buffend - buffer), "CC");
        if (written > 0) buffend += written;
        return buffend - buffer;
    }

    case 'o': case 'O': // co xxyy (where xx = current show state, yy = allow advancing)
    {
        // Show control parsing (uncommented and adapted)
        if (len >= 7) {
            char *bp = &pusrdata[3];
            uint8_t rh = rp2040_fromhex1(*(bp++));
         //   showstate = (rh << 4) | rp2040_fromhex1(*(bp++));
            rh = rp2040_fromhex1(*(bp++));
            showallowadvance = (rh << 4) | rp2040_fromhex1(*(bp++));
            if (len >= 11) {
                rh = rp2040_fromhex1(*(bp++));
                jam_color = (rh << 4) | rp2040_fromhex1(*(bp++));
            }
        }
        break;
    }

    case 'v': case 'V': // cv xxnnnnnnnnnnnnn (where xx is color #, nnnnnnnn is color data)
    {
        int i;
        if (len < 5) break;
        char *bp = &pusrdata[3];

        uint8_t ch = rp2040_fromhex1(*(bp++));
        ch = (ch << 4) | rp2040_fromhex1(*(bp++));

        if (ch >= PREMOD_SIZE)
        {
            int written = snprintf(buffend, retsize - (buffend - buffer), "!CV");
            if (written > 0) buffend += written;
            return buffend - buffer;
        }

        // Parse premodulated entries safely
        for (i = 0; i < PREMOD_ENTRIES; i++)
        {
            int k;
            uint32_t colval = 0;
            for (k = 0; k < 8; k++)
            {
                colval = (colval << 4) | rp2040_fromhex1(*(bp++));
            }
            premodulated_table[i * PREMOD_SIZE + ch] = colval;
        }

        // Add the overspill bits.
        for (i = PREMOD_ENTRIES; i < PREMOD_ENTRIES_WITH_SPILL; i++)
        {
            premodulated_table[i * PREMOD_SIZE + ch] = premodulated_table[(i - PREMOD_ENTRIES) * PREMOD_SIZE + ch];
        }
        
        break;
    }
    }
    return -1;
}