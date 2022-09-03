#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define ON  1
#define OFF 0

// 1000000 / 60  (60Hz)
#define REFRESH_RATE 16666

// 1000000 / 700 (700 instructions per second)
#define CLOCK_RATE 1428

uint8_t mem[4096];
uint16_t reg[16];
uint16_t I;
uint8_t stack[1024];

uint16_t pc;
uint16_t sc;

char screen[SCREEN_HEIGHT][SCREEN_WIDTH];

uint8_t delayTimer;
uint8_t soundTimer;

uint8_t fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void LoadRom(char* filename)
{
    FILE* f = fopen(filename, "rb");
    int i = 512;
    char currByte; 
    fread(mem + 0x200, sizeof(char), 4096, f);
    fclose(f);
}

void DrawScreen()
{
    printf("\e[2J\e[H");

    int i, j;
    for (i = 0; i < SCREEN_HEIGHT; ++i)
    {
        for (j = 0; j < SCREEN_WIDTH; ++j)
        {
            if (screen[i][j] & 0x1)
                printf("\u2588\u2588");
            else
                printf("  ");
        }
        printf("\n");
    }
}

/* The following two functions are identical, but its clearer this way */
uint16_t BigToLittle(uint16_t big)
{
    return (big >> 8) & (big << 8);
}

uint16_t LittleToBig(uint16_t little)
{
    return (little >> 8) & (little << 8);
}

void Execute(uint16_t opcode)
{
    uint8_t X       = (opcode & 0x0F00) >>  8;
    uint8_t Y       = (opcode & 0x00F0) >>  4;
    uint8_t N       = (opcode & 0x000F)      ;
    uint8_t NN      = (opcode & 0x00FF)      ;
    uint16_t NNN    = (opcode & 0x0FFF)      ;

    int i, j;
    switch (opcode & 0xF000)
    {
        case 0x0000:
            switch (NNN)
            {
                case 0x00E0:
                    memset(screen, 0x0, SCREEN_WIDTH * SCREEN_HEIGHT);
                    pc += 2;
                    break;
                case 0x00EE:
                    --sc;
                    pc = stack[sc];
                    break;
                default:
                    // Intentionally not implemented
                    pc += 2;
                    break;
            }
            break;
        case 0x1000:
            pc = NNN;
            break;
        case 0x2000:
            stack[sc] = pc;
            sc++;
            pc = NNN;
            break;
        case 0x3000:
            if (reg[X] == NN)
                pc += 2;
            pc += 2;
            break;
        case 0x4000:
            if (reg[X] != NN)
                pc += 2;
            pc += 2;
            break;
        case 0x5000:
            if (reg[X] == reg[Y])
                pc += 2;
            pc += 2;
            break;
        case 0x6000:
            reg[X] = NN;
            pc += 2;
            break;
        case 0x7000:
            reg[X] += NN;
            pc += 2;
            break;
        case 0x8000:
            switch (N)
            {
                case 0x0:
                    reg[X] = reg[Y];
                    pc += 2;
                    break;
                case 0x1:
                    reg[X] |= reg[Y];
                    pc += 2;
                    break;
                case 0x2:
                    reg[X] &= reg[Y];
                    pc += 2;
                    break;
                case 0x3:
                    reg[X] ^= reg[Y];
                    pc += 2;
                    break;
                case 0x4:
                    if (reg[X] + reg[Y] > 255)
                        reg[0xF] = 1;
                    else
                        reg[0xF] = 0;
                    reg[X] += reg[Y];
                    pc += 2;
                    break;
                case 0x5:
                    if (reg[X] >= reg[Y])
                        reg[0xF] = 1;
                    else
                        reg[0xF] = 0;
                    reg[X] -= reg[Y];
                    pc += 2;
                    break;
                case 0x6:
                    reg[0xF] = reg[X] & 0x1;
                    reg[X] >>= 1;
                    pc += 2;
                    break;
                case 0x7:
                    if (reg[Y] >= reg[X])
                        reg[0xF] = 1;
                    else
                        reg[0xF] = 0;
                    reg[X] = reg[Y] - reg[X];
                    pc += 2;
                    break;
                case 0xE:
                    reg[0xF] = reg[X] & 0x1000;
                    reg[X] <<= 1;
                    pc += 2;
                    break;
            }
            break;
        case 0x9000:
            if (reg[X] != reg[Y])
                pc += 2;
            pc += 2;
            break;
        case 0xA000:
            I = NNN;
            pc += 2;
            break;
        case 0xB000:
            pc = NNN + reg[0];
            break;
        case 0xC000:
            reg[X] = rand() & NN;
            pc += 2;
            break;
        case 0xD000:
            reg[0xF] = 0;
            uint16_t x = reg[X];
            uint16_t y = reg[Y];

            for (i = 0; i < N; ++i)
            {
                uint8_t row = mem[I + i];
                for (j = 0; j < 8; ++j)
                {
                    int set = 0;
                    if (screen[y + i][x + j] == 1)
                        set = 1;
                    screen[y + i][x + j] ^= (row >> (7 - j)) & 0x1;
                    if (screen[y + i][x + j] == 0 && set)
                        reg[0xF] = 1;
                }
            }
            pc += 2;
            break;
        case 0xE000:
            switch (NN)
            {
                case 0x9E:
                    //TODO
                    pc += 2;
                    break;
                case 0xA1:
                    //TODO
                    pc += 2;
                    break;
            }
            break;
        case 0xF000:
            switch (NN)
            {
                case 0x07:
                    reg[X] = delayTimer;
                    pc += 2;
                    break;
                case 0x0A:
                    //TODO
                    pc += 2;
                    break;
                case 0x15:
                    delayTimer = reg[X];
                    pc += 2;
                    break;
                case 0x18:
                    soundTimer = reg[X];
                    pc += 2;
                    break;
                case 0x1E:
                    I += reg[X];
                    pc += 2;
                    break;
                case 0x29:
                    //TODO
                    pc += 2;
                    break;
                case 0x33:
                    //TODO
                    pc += 2;
                    break;
                case 0x55:
                    for (i = 0; i < X; ++i)
                    {
                        *(mem + I + i * 2) = reg[i];
                    }
                    pc += 2;
                    break;
                case 0x65:
                    for (i = 0; i < X; ++i)
                    {
                        reg[i] = *(mem + I + i * 2);
                    }
                    pc += 2;
                    break;
            }
            break;
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Invalid usage.\n%s [rom]\n", argv[0]);
        return -1;
    }
    int i;
    for (i = 0; i < 80; ++i)
        mem[i] = fontset[i];
    LoadRom(argv[1]);
    struct timeval prevTime, currTime;
    gettimeofday(&prevTime, NULL);


    double deltaTime = 0;
    int refresh = 0;
    int clock = 0;
    pc = 0x200;
    sc = 0;
    while(1)
    {
        // Calculate deltatime
        gettimeofday(&currTime, NULL);
        deltaTime = (currTime.tv_sec - prevTime.tv_sec) * 1000000 +
            currTime.tv_usec - prevTime.tv_usec;
        prevTime = currTime;

        // Handle timers and clocks
        refresh += deltaTime;
        clock += deltaTime;

        // 60Hz events
        if (refresh >= REFRESH_RATE)
        {
            refresh = 0;
            //memset(screen, 1, SCREEN_WIDTH * SCREEN_HEIGHT);
            //screen[0][1] = 1;
            DrawScreen();
            if (soundTimer > 0)
                soundTimer--;
            if (delayTimer > 0)
                delayTimer--;
        }

        // Clock events
        if (clock >= CLOCK_RATE)
        {
            clock = 0;
            uint16_t opcode = (mem[pc] << 8) | mem[pc + 1];
            Execute(opcode);
            //pc += 2;
        }
    }
    
    return 0;
}
