#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _SSE2
    #include <emmintrin.h>
#endif

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

int gcd(int m, int n) {
  int tmp;
  while (m) {
    tmp = m;
    m = n % m;
    n = tmp;
  }
  return n;
}

int lcm(int m, int n) { return m / gcd(m, n) * n; }


void do_xor(const uint8_t *from, uint8_t *to, off_t len, const uint8_t *key, off_t keylen)
{
#ifdef _SSE2
    int sse2 = 0;
    uint8_t *realkey = NULL;

    /* Heuristic, don't optimize for files smaller than 1 MB */
    if(len < 1024*1024)
        goto next;

    __builtin_cpu_init ();
    sse2  = __builtin_cpu_supports("sse2");
    if(!sse2)
        goto next;

    /* First simple case : key is smaller than 16 bytes and 16%keysize == 0 */
    if (keylen <= 16 && 16%keylen == 0) {
        realkey = (uint8_t *)memalign(16, 16);
        for(int i = 0; i<(16/keylen); i++)
            memcpy(&realkey[i*keylen], key, keylen);
    } else {
        // TODO : implement more cases
        goto next;
    }

    printf("Using fast (SSE2) xor\n");

    off_t len_align = len-(len%16);

    /* Make sure we don't go too far */
    const __m128i *from_ptr = (__m128i *)from;
    const __m128i *from_end = (__m128i *)(from + len_align);

    __m128i *to_ptr = (__m128i *)to;
    __m128i xmm_key = _mm_load_si128((__m128i *)realkey);

    do {
        __m128i tmp_from = _mm_load_si128(from_ptr);

        __m128i tmp_xor = _mm_xor_si128(tmp_from, xmm_key); //  XOR  4 32-bit words
        _mm_store_si128(to_ptr, tmp_xor);

        ++from_ptr;
        ++to_ptr;
    } while (from_ptr < from_end);

    /* do the rest */
    for(off_t i = len_align; i < len; i++) {
        to[i] = from[i] ^ key[i%keylen];
    }

    free(realkey);
    return;

next:
#else
#endif
    for(off_t i = 0; i < len; i++) {
        to[i] = from[i] ^ key[i%keylen];
    }
    return;
}

int main(int argc, char *argv[])
{
    uint8_t *key = NULL, *mapped = NULL, *output_map = NULL;
    int hex_keylen = 0, opt, keylen = 0;
    int verb = 0, fd;
    off_t file_size;

    while ((opt = getopt(argc, argv, "hvx:f:")) != -1) {
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
        case 'h':
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

    do_xor(mapped, output_map, file_size, key, keylen);

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
