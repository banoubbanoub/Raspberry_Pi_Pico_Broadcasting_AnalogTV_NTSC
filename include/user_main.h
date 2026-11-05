/**
 * RP2040 Port of Channel 3 Video Demo Application
 * Adapted from Charles Lohr's ESP8266 Channel 3 Demo
 */

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "video_broadcast.h"
#include "3d.h"
#include "CbTable.h"
#include "broadcast_tables.h"

#define INITIAL_SHOW_STATE 2
#define FRAMES_PER_STATE 300

extern volatile int gframe;
extern uint16_t framebuffer[];
extern uint8_t *frontframe;

char lastct[256];
uint8_t showstate = INITIAL_SHOW_STATE;
int framessostate = 0;

int16_t Height(int x, int y, int l)
{
    return tdCOS((x*x + y*y) + l);
}

void SetupMatrix()
{
    tdIdentity(ProjectionMatrix);
    tdIdentity(ModelviewMatrix);
    Perspective(600, 250, 50, 8192, ProjectionMatrix);
}

void DrawFrame()
{
    int x = 0;
    int y = 0;
    int i;
    
    CNFGPenX = 14;
    CNFGPenY = 20;
    memset(frontframe, 0x00, ((FBW/4)*FBH));
    tdIdentity(ModelviewMatrix);
    tdIdentity(ProjectionMatrix);
    // Palette entries 3..7 carry distinct NTSC chroma phases.
    CNFGColor(3 + (framessostate % 5));

    switch(showstate)
    {
        case 11: // Display test boxes
        {
            for(i = 0; i < 16; i++)
            {
                int x = i % 4;
                int y = i / 4;
                x *= (FBW/4);
                y *= (FBH/4);
                CNFGColor(i);
                CNFGTackRectangle(x, y, x+(FBW/4)-1, y+(FBH/4)-1);
            }
        } break;
        
        case 10:
        {
            for(i = 0; i < 16; i++)
            {
                CNFGPenX = 14;
                CNFGPenY = (i+1) * 12;
                CNFGColor(i);
                CNFGDrawText("Hello world", 3);
            }

            SetupMatrix();
            tdRotateEA(ProjectionMatrix, -20, 0, 0);
            tdRotateEA(ModelviewMatrix, framessostate, 0, 0);

            for(y = 3; y >= 0; y--)
            for(x = 0; x < 4; x++)
            {
                CNFGColor(x + y*4);
                ModelviewMatrix[11] = 1000 + tdSIN((x + y)*40 + framessostate*2);
                ModelviewMatrix[3] = 600*x - 850;
                ModelviewMatrix[7] = 600*y + 800 - 850;
                DrawGeoSphere();
            }

        } break;

        case 9:
        {
            const char *s = "Direct modulation.\nDMA through PIO Bus!\nTry it yourself!";
            size_t visible = (size_t)framessostate;
            const size_t text_length = strlen(s);
            if(visible > text_length) visible = text_length;
            memcpy(lastct, s, visible);
            lastct[visible] = 0;
            CNFGDrawText(lastct, 2);
        } break;

        case 8:
        {
            CNFGDrawText("Dynamic 3D Meshes", 2);
            SetupMatrix();
            tdRotateEA(ProjectionMatrix, -20, 0, 0);
            tdRotateEA(ModelviewMatrix, 0, 0, framessostate);

            for(y = -18; y < 18; y++)
            for(x = -18; x < 18; x++)
            {
                int o = -framessostate*2;
                int t = Height(x, y, o)*2 + 2000;
                CNFGColor(((t/100)%15) + 1);
                int nx = Height(x+1, y, o)*2 + 2000;
                int ny = Height(x, y+1, o)*2 + 2000;
                int16_t p0[3] = {
                    (int16_t)(x * 140), (int16_t)(y * 140), (int16_t)t
                };
                int16_t p1[3] = {
                    (int16_t)((x + 1) * 140), (int16_t)(y * 140), (int16_t)nx
                };
                int16_t p2[3] = {
                    (int16_t)(x * 140), (int16_t)((y + 1) * 140), (int16_t)ny
                };
                Draw3DSegment(p0, p1);
                Draw3DSegment(p0, p2);
            }

        } break;
    
        case 7:
        {
            CNFGPenX = (FBW2 - 23 * 3) / 2;
            CNFGDrawText("Matrix-based 3D engine.", 2);
            SetupMatrix();
            tdRotateEA(ProjectionMatrix, -20, 0, 0);
            tdRotateEA(ModelviewMatrix, framessostate, 0, 0);
            int sphereset = (framessostate / 120);
            if(sphereset > 2) sphereset = 2;
            for(y = -sphereset; y <= sphereset; y++)
            for(x = -sphereset; x <= sphereset; x++)
            {
                if(y == 2) continue;
                ModelviewMatrix[11] = 1000 + tdSIN((x + y)*40 + framessostate*2);
                ModelviewMatrix[3] = 500*x;
                ModelviewMatrix[7] = 500*y + 800;
                DrawGeoSphere();
            }
        } break;
    
        case 6:
        {
            static int line_segments[350][5];
            CNFGPenX = (FBW2 - 25 * 3) / 2;
            CNFGDrawText("Double-buffered graphics", 2);
            if(framessostate == 0)
            {
                for(i = 0; i < 350; i++)
                {
                    line_segments[i][0] = rand() % 16;
                    line_segments[i][1] = rand() % FBW2;
                    line_segments[i][2] = rand() % (FBH - 30) + 30;
                    line_segments[i][3] = rand() % FBW2;
                    line_segments[i][4] = rand() % (FBH - 30) + 30;
                }
            }
            for(i = 0; i < 350; i++)
            {
                CNFGColor(line_segments[i][0]);
                CNFGTackSegment(
                    line_segments[i][1], line_segments[i][2],
                    line_segments[i][3], line_segments[i][4]
                );
            }
        } break;

        case 5:
        {
            CNFGColor(7);
            CNFGTackRectangle(20, 90, FBW - 20, 130);
            CNFGColor(3);
            CNFGPenX = 28;
            CNFGPenY = 100;
            CNFGDrawText("HELLO WORLD", 2);
        } break;

        case 4:
        {
            CNFGPenX = (FBW2 - 15 * 6) / 2;
            CNFGPenY += 14 * 7;
            CNFGDrawText("  38x14 TEXT MODE", 2);

            CNFGPenY += 14;
            CNFGPenX = (FBW2 - 14 * 6) / 2;
            CNFGDrawText("  ...on graphics", 2);

        } break;

        case 3:
        {
            CNFGPenX = (FBW2 - 10 * 6) / 2;
            CNFGDrawText(" WELCOME TO", 2);
            CNFGPenY += 18;
            CNFGPenX = (FBW2 - 17 * 6) / 2;
            CNFGDrawText("  RP2040 VIDEO MODE", 2);
            CNFGPenY += 18;
            CNFGPenX = (FBW2 - 14 * 6) / 2;
            CNFGDrawText("  ANALOG TV CH 3", 2);
        } break;

        case 2:
        {
            const char *features = " RP2040 Features:\n Dual ARM Cortex-M0+ @133MHz\n 264kB SRAM\n PIO State Machines\n DMA Channels\n Analog Broadcast TV\n";
            size_t visible = (size_t)framessostate / 2;
            const size_t text_length = strlen(features);
            if(visible > text_length) visible = text_length;
            memcpy(lastct, features, visible);
            lastct[visible] = 0;
            CNFGDrawText(lastct, 2);
        } break;

        case 1:
        {
            CNFGPenX = (FBW2 - 14 * 6) / 2;
            CNFGDrawText("  Analog TV demo", 2);
            CNFGPenX = (FBW2 - 9 * 6) / 2;
            CNFGPenY += 24;
            CNFGDrawText("  Channel 3", 2);
        } break;
        case 0:
        {
            char status[32];
            CNFGDrawText("  Channel 3 Broadcasting", 2);
            CNFGPenY += 18;
            snprintf(status, sizeof(status), "Frame: %d", gframe);
            CNFGDrawText(status, 2);
            CNFGPenY += 18;
            CNFGDrawText("  RP2040 Core Active", 2);
        } break;
    }

    if(framessostate >= FRAMES_PER_STATE - 1)
    {
        showstate = (showstate + 1) % 12;
        framessostate = 0;
        lastct[0] = 0;
    }
    else
    {
        framessostate++;
    }
}

// Core 1 Entry for Background Rendering & State Machine
void render_core_entry()
{
    SetupMatrix();
    static uint8_t lastframe = 0;

    while(true)
    {
        uint8_t tbuffer = !(gframe & 1);
        if(lastframe != tbuffer)
        {
            frontframe = (uint8_t*)&framebuffer[((FBW2/4)*FBH)*tbuffer];
            DrawFrame();
            lastframe = tbuffer;
        }
        tight_loop_contents();
    }
}

void EnterCritical() {}
void ExitCritical() {}
