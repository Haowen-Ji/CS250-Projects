#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

int word_size = 16;
int memory_size = 65535;
int block_size_max = 64;

struct block
{
    bool valid;
    int tag;
    unsigned char bytes[64];
    int LRU;
    bool dirty_bit;
};

void print_number(unsigned char *str, int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%02x", str[i]);
    }
    printf("\n");
}

void hex_transfer(char *hex, int input, unsigned char *new_chars)
{
    int len = input * 2;
    for (int i = 0, j = 0; j < input; i += 2, j += 1)
    {
        *(new_chars + j) = (hex[i + 1] % 32 + 9) % 25 + (hex[i] % 32 + 9) % 25 * 16;
    }
}

int main(int argc, char *argv[])
{
    char *file;
    char *p;
    bool write_back = 1;
    bool write_through = 0;
    file = argv[1];
    long size_KB = strtol(argv[2], &p, 10);
    long ways = strtol(argv[3], &p, 10);
    long block_size = strtol(argv[5], &p, 10);
    int frames = size_KB * 1024 / block_size;
    int sets = frames / ways;
    int offset = log2(block_size);
    int index = log2(sets);
    int tag = word_size - offset - index;
    if (argv[4][1] == 't')
    {
        write_back = 0;
        write_through = 1;
    }
    // initialize cache
    struct block *cache[ways];
    for (int i = 0; i < ways; i++)
        cache[i] = (struct block *)malloc(sets * sizeof(struct block));
    for (int i = 0; i < ways; i++)
        for (int j = 0; j < sets; j++)
        {
            cache[i][j].dirty_bit = 0;
            cache[i][j].valid = 0;
            cache[i][j].LRU = ways - i - 1;
        }
    // initialize memory
    char memory[memory_size];
    for (int i = 0; i < memory_size; i++)
    {
        memory[i] = 0;
    }

    // Get the file
    FILE *fptr;
    fptr = fopen(file, "r");
    if (fptr == NULL)
        return 0;

    int line_length = 100;
    char line[line_length];
    while (fgets(line, line_length, fptr) != NULL)
    {
        int address, associativity;
        // The load function
        if (line[0] == 'l')
        {
            char *addr = line + 5;
            *(addr + 4) = '\0';
            char *acc = line + 10;
            address = (int)strtol(addr, NULL, 16);
            associativity = (int)strtol(acc, NULL, 16);
            bool isHit = 0;
            int block_addr = address / block_size;
            int check = address % block_size;
            unsigned char result[associativity];
            int offset_bit = log2(block_size);
            int index_bit = log2(sets);
            int offset = address & (((1 << offset_bit) - 1));
            int index = (address >> offset_bit) & ((1 << index_bit) - 1);
            int tag = address >> (offset_bit + index_bit);

            for (int i = 0; i < ways; i++)
            {
                for (int j = 0; j < sets; j++)
                {
                    if (cache[i][j].tag == block_addr && cache[i][j].valid == 1)
                    {
                        isHit = 1;
                        for (int k = check; k < check + associativity; k++)
                            result[k - check] = cache[i][j].bytes[k];
                        // update LRU
                        if (cache[i][j].LRU != 0)
                        {
                            int prev_LRU = cache[i][j].LRU;
                            cache[i][j].LRU = 0;
                            for (int m = 0; m < ways; m++)
                            {
                                if ((cache[m][j].LRU < prev_LRU) && (m != i))
                                    cache[m][j].LRU++;
                            }
                        }
                        // print result
                        printf("load %.4x hit ", address);
                        print_number(result, associativity);
                    }
                }
            }

            if (isHit == 0)
            {
                int page = block_addr * block_size;
                for (int k = check; k < check + associativity; k++)
                {
                    result[k - check] = memory[page + k];
                }
                // update LRU
                int set_id = block_addr % sets;
                for (int i = 0; i < ways; i++)
                {
                    if (cache[i][set_id].LRU == ways - 1)
                    {
                        cache[i][set_id].LRU = 0;
                        cache[i][set_id].valid = 1;
                        if (cache[i][set_id].dirty_bit == 1)
                        {
                            // for (int j = cache[i][set_id].tag * block_size; j < block_size; j++)
                            for (int j = cache[i][set_id].tag; j < block_size; j++)
                            {
                                // memory[j] = cache[i][set_id].bytes[j - cache[i][set_id].tag * block_size];
                                memory[j] = cache[i][set_id].bytes[j - cache[i][set_id].tag];
                            }
                            cache[i][set_id].dirty_bit = 0;
                        }

                        cache[i][set_id].tag = block_addr;

                        for (int j = page; j < page + block_size; j++)
                        {
                            cache[i][set_id].bytes[j - page] = memory[j];
                        }
                        for (int k = 0; k < ways; k++)
                        {
                            if (k != i && cache[k][set_id].LRU < ways - 1)
                            {
                                cache[k][set_id].LRU++;
                            }
                        }
                        break;
                    }
                }
                // print result
                printf("load %.4x miss ", address);
                print_number(result, associativity);
            }
        }

        else
        {
            char *addr = line + 6;
            *(addr + 4) = '\0';
            char *acc;
            char *split = line + 10;
            int i = 0;
            bool isHit = 0;

            while (true)
            {
                split++;
                if (*(split) == ' ')
                {
                    split++;
                    break;
                }
                i++;
            }
            acc = (char *)malloc(i * sizeof(char));
            i = 11;
            while (true)
            {
                *(acc + i - 11) = *(line + i);
                i++;
                if (*(line + i) == ' ')
                {
                    break;
                }
            }

            address = (int)strtol(addr, NULL, 16);
            associativity = (int)strtol(acc, NULL, 16);

            unsigned char *new_chars = (unsigned char *)malloc((associativity + 1) * sizeof(*new_chars));
            hex_transfer(split, associativity, new_chars);

            int block_addr = address / block_size;
            int check = address % block_size;
            unsigned char result[associativity];
            int offset_bit = log2(block_size);
            int index_bit = log2(sets);
            int tag_bit = word_size - offset - index;
            int offset = address & (((1 << offset_bit) - 1));
            int index = (address >> offset_bit) & ((1 << index_bit) - 1);
            int tag = address >> (offset_bit + index_bit);
            // int offset = address & (((1 << offset_bit) - 1));
            // int index = (address >> offset_bit) & ((1 << index_bit) - 1);
            // int tag = address >> (offset_bit + index_bit);

            if (write_through == 1)
            {
                for (int i = 0; i < ways; i++)
                {
                    for (int j = 0; j < sets; j++)
                    {
                        if (cache[i][j].tag == block_addr && cache[i][j].valid == 1)
                        {
                            for (int k = check; k < check + associativity; k++)
                            {
                                cache[i][j].bytes[k] = new_chars[k - check];
                                memory[address + k - check] = new_chars[k - check];
                            }

                            // update LRU
                            if (cache[i][j].LRU != 0)
                            {
                                int prev_LRU = cache[i][j].LRU;
                                cache[i][j].LRU = 0;
                                for (int m = 0; m < ways; m++)
                                {
                                    if (m != i && cache[m][j].LRU < prev_LRU)
                                        cache[m][j].LRU++;
                                }
                            }
                            // print result
                            printf("store %.4x hit\n", address);
                            isHit = 1;
                        }
                    }
                }

                if (isHit == 0)
                {
                    printf("store %.4x miss\n", address);

                    for (int i = address; i < address + associativity; i++)
                    {
                        memory[i] = new_chars[i - address];
                    }
                }
            }

            if (write_back == 1)
            {
                for (int i = 0; i < ways; i++)
                {
                    for (int j = 0; j < sets; j++)
                    {
                        if (cache[i][j].tag == block_addr && cache[i][j].valid == 1)
                        {
                            if (cache[i][j].dirty_bit == 1)
                            {
                                int address_id = (tag_bit << (offset_bit + index_bit)) + (index << offset_bit);
                                // for (int k = cache[i][j].tag * block_size; k < block_size; k++)
                                // for (int k = cache[i][j].tag; k < block_size; k++)
                                // {
                                //     memory[k] = cache[i][j].bytes[k - cache[i][j].tag];
                                // }
                                for (int k = address_id; k < (address_id + block_size); k++)
                                {
                                    memory[k] = cache[i][j].bytes[k - address_id];
                                }
                            }
                            for (int k = check; k < check + associativity; k++)
                            {
                                cache[i][j].bytes[k] = new_chars[k - check];
                            }

                            if (cache[i][j].LRU != 0)
                            {
                                int prev_LRU = cache[i][j].LRU;
                                cache[i][j].LRU = 0;
                                for (int k = 0; k < ways; k++)
                                {
                                    if (k != i && cache[k][j].LRU < prev_LRU)
                                        cache[k][j].LRU++;
                                }
                            }
                            printf("store %.4x hit\n", address);
                            isHit = 1;
                        }
                    }
                }

                if (isHit == 0)
                {
                    printf("store %.4x miss\n", address);
                    int set_id = block_addr % sets;

                    for (int i = 0; i < ways; i++)
                    {
                        if (cache[i][set_id].LRU == ways - 1)
                        {
                            cache[i][set_id].valid = 1;
                            cache[i][set_id].LRU = 0;
                            if (cache[i][set_id].dirty_bit == 1)
                            {
                                // // for (int j = cache[i][set_id].tag * block_size; j < block_size; j++)
                                // for (int j = cache[i][set_id].tag; j < block_size; j++)
                                // {
                                //     // memory[j] = cache[i][set_id].bytes[j - cache[i][set_id].tag * block_size];
                                //     memory[j] = cache[i][set_id].bytes[j - cache[i][set_id].tag];
                                // }
                                int address_id = (tag_bit << (offset_bit + index_bit)) + (index << offset_bit);
                                for (int k = address_id; k < address_id + block_size; k++)
                                {
                                    memory[k] = cache[i][set_id].bytes[k - address_id];
                                }
                                cache[i][set_id].dirty_bit = 0;
                            }

                            cache[i][set_id].tag = block_addr;

                            for (int j = check; j < check + associativity; j++)
                            {
                                cache[i][set_id].bytes[j] = new_chars[j - check];
                            }

                            // update the LRU
                            for (int k = 0; k < ways; k++)
                            {
                                if (k != i && cache[k][set_id].LRU < ways - 1)
                                    cache[k][set_id].LRU++;
                            }
                            break;
                        }
                    }
                }
            }
            free(new_chars);
        }
    }
    for (int i = 0; i < ways; i++)
    {
        free(cache[i]);
    }
    return 0;
}