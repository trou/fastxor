#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

void errmsg(const char *error)
{
    if (error) {
        fprintf(stderr, "Error: %s\n", error);
    }
    fprintf(stderr,
            "Usage: fastxor -x hexkey|-f file_key input_file ouput_file\n");
    exit(EXIT_FAILURE);
}

int hex2bin(const char *hex, int len, uint8_t *bin)
{
    int i;
    char tmp[3] = {0};

    // Reset errno
    errno = 0;
    for (i = 0; i < len / 2; i++) {
        memcpy(tmp, &hex[2 * i], 2);
        // Check we have hex
        if (!isxdigit(tmp[0]) || !isxdigit(tmp[1])) {
            return -1;
        }
        bin[i] = strtol(tmp, NULL, 16);
        if (errno) {
            return -1;
        }
    }
    return i;
}

int main(int argc, char *argv[])
{
    uint8_t *key = NULL, *mapped = NULL, *output_map = NULL;
    int hex_keylen = 0, opt, keylen = 0;
    int verb = 0, fd;
    off_t file_size;

    while ((opt = getopt(argc, argv, "vx:f:")) != -1) {
        switch (opt) {
        /* Hex key on command line */
        case 'x':
            hex_keylen = strlen(optarg);
            if (hex_keylen % 2 == 1 || hex_keylen == 0) {
                errmsg("Hex key length is zero or not even");
            }
            key = (uint8_t *)malloc(hex_keylen / 2);
            if (!key) {
                errmsg("malloc failed");
            }
            keylen = hex2bin(optarg, hex_keylen, key);
            if (keylen <= 0) {
                free(key);
                if (errno) {
                    perror("Conversion failed !");
                }
                errmsg("Invalid arg");
            }
            break;
        /* Read file as key */
        case 'f':
            fd = open(optarg, O_RDONLY);
            if(fd < 0) {
                errmsg("Could not open keyfile");
            }
            keylen = lseek(fd, 0, SEEK_END);
            if(keylen == 0 || keylen == (off_t) -1) {
                errmsg("Error getting key file size or empty file");
            }
            mapped = mmap(0, keylen, PROT_READ, MAP_PRIVATE, fd, 0);
            if (mapped == MAP_FAILED) {
                errmsg("mmap failed");
            }
            key = (uint8_t *)malloc(keylen);
            if (!key) {
                errmsg("malloc failed");
            }
            memcpy(key, mapped, keylen);
            munmap(mapped, keylen);
            close(fd);
            break;
        case 'v':
            verb = 1;
            break;
        default: /* '?' */
            errmsg(NULL);
        }
    }
    
    if(argc-optind != 2)    {
        free(key);
        errmsg("Missing file arguments");
    }

    int input_fd = open(argv[optind], O_RDONLY);
    if(input_fd < 0) {
        free(key);
        errmsg("Could not open input file");
    }
    file_size = lseek(input_fd, 0, SEEK_END);
    mapped = mmap(0, file_size, PROT_READ, MAP_PRIVATE, input_fd, 0);
    if(mapped == MAP_FAILED) {
        free(key);
        errmsg("could not mmap input file");
    }

    int output_fd = open(argv[optind+1], O_RDWR|O_CREAT, 00600);
    if(output_fd < 0) {
        free(key);
        errmsg("Could not open output file");
    }
    /* Set file size */
    if(ftruncate(output_fd, file_size) < 0) {
        free(key);
        errmsg("could not resize output file");
    }
    output_map = mmap(0, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, output_fd, 0);
    if(output_map == MAP_FAILED) {
        free(key);
        errmsg("could not mmap output file");
    }

    for(off_t i = 0; i < file_size; i++) {
        output_map[i] = mapped[i] ^ key[i%keylen];    
    }
    munmap(mapped, file_size);
    close(input_fd);
    munmap(output_map, file_size);
    close(output_fd);

    if (verb) {
        printf("Key (%d bytes): ", keylen);
        for (int i = 0; i < keylen; i++) {
            printf("%02x", key[i]);
        }
        printf("\n");
    }

    free(key);
    return EXIT_SUCCESS;
}
