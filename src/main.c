#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint8_t mem[4096];
uint16_t reg[16];
uint8_t stack[1024];

void LoadRom(char* filename)
{
    FILE* f = fopen(filename, "r");
    int i = 512;
    uint8_t currByte; 
    while (currByte = fgetc(f) != -1)
        mem[i++] = currByte;
    fclose(f);
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Invalid usage.\n%s [rom]\n", argv[0]);
        return -1;
    }
    LoadRom(argv[1]);

    return 0;
}
