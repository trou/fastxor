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

/* Need to define WSIZE so that if __WORDSIZE is not defined, WSIZZ will be 0 */
#define WSIZE __WORDSIZE
#if WSIZE == 64
    typedef unsigned long long *data_ptr;
    typedef unsigned long long word_type;
#else
    typedef unsigned long *data_ptr;
    typedef unsigned long word_type;
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

size_t hex2bin(const char *hex, size_t len, uint8_t *bin)
{
    size_t i;
    char tmp[3] = {0};

    // Reset errno
    errno = 0;
    for (i = 0; i < len / 2; i++) {
        memcpy(tmp, &hex[2 * i], 2);
        // Check we have hex
        if (!isxdigit(tmp[0]) || !isxdigit(tmp[1])) {
            return (size_t)-1;
        }
        bin[i] = (uint8_t)strtol(tmp, NULL, 16);
        if (errno) {
            return (size_t)-1;
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

void do_simple_xor(const uint8_t *from, uint8_t *to, size_t len, const uint8_t *key, size_t keylen)
{
    size_t len_align = len-(len%keylen);
    data_ptr to_wptr = (data_ptr)to;
    data_ptr from_wptr = (data_ptr)from;
    data_ptr from_end = (data_ptr)(from+len_align);
    word_type key_word = *((data_ptr)key);

    while(from_wptr < from_end)
        *to_wptr++ = *from_wptr++ ^ key_word;

    /* do the rest */
    for(size_t i = len_align; i < len; i++) {
        to[i] = from[i] ^ key[i%keylen];
    }
    return;
}

void do_xor(const uint8_t *from, uint8_t *to, size_t len, const uint8_t *key, size_t keylen)
{
    uint8_t *realkey = NULL;
    data_ptr to_wptr = (data_ptr)to;
    data_ptr from_wptr = (data_ptr)from;
    const data_ptr key_wptr = (data_ptr)key;
    size_t word_size = sizeof(word_type);

    /* First simple case : key is smaller than word size and word_size%keysize == 0 */
    if (keylen <= word_size && word_size%keylen == 0) {
        realkey = (uint8_t *)memalign(16, word_size);
        for(size_t i = 0; i<(word_size/keylen); i++)
            memcpy(&realkey[i*keylen], key, keylen);
        do_simple_xor(from, to, len, realkey, word_size);
        free(realkey);
        return;
    }

    size_t keylen_in_words = keylen/word_size;
    size_t key_remain = keylen%word_size;

    size_t i, j;
    uint8_t *end = to+len;
    printf("'Slow' path: keylen = %ld, keylen_in_words = %ld, rem=%ld\n", keylen, keylen_in_words, key_remain);

    while((uint8_t *)to_wptr < end) {
        /* Do the words */
        for(i = 0; i < keylen_in_words && (uint8_t *)to_wptr < end; i++) {
            *to_wptr++ = *from_wptr++ ^ key_wptr[i];
        }
        /* Remaining bytes */
        to = (uint8_t *)to_wptr;
        from = (uint8_t *)from_wptr;
        for(j = 0; j < key_remain && (&to[j]) < end; j++)
            to[j] = from[j] ^ key[i*word_size+j];
        to_wptr = (data_ptr)(to+j);
        from_wptr = (data_ptr)(from+j);
    }
    return;
}

int main(int argc, char *argv[])
{
    uint8_t *key = NULL, *mapped = NULL, *output_map = NULL;
    size_t hex_keylen = 0, keylen = 0;
    int verb = 0, fd, opt;
    size_t file_size;

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
            if (keylen == 0 || keylen == (size_t) -1) {
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
            keylen = (size_t)lseek(fd, 0, SEEK_END);
            if(keylen == 0 || keylen == (size_t) -1) {
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
    file_size = (size_t)lseek(input_fd, 0, SEEK_END);
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
    if(ftruncate(output_fd, (off_t)file_size) < 0) {
        free(key);
        errmsg("could not resize output file");
    }
    output_map = mmap(0, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, output_fd, 0);
    if(output_map == MAP_FAILED) {
        free(key);
        errmsg("could not mmap output file");
    }

    if (verb) {
        printf("Key (%ld bytes): ", keylen);
        for (size_t i = 0; i < keylen; i++) {
            printf("%02x", key[i]);
        }
        printf("\n");
    }

    do_xor(mapped, output_map, file_size, key, keylen);

    munmap(mapped, file_size);
    close(input_fd);
    munmap(output_map, file_size);
    close(output_fd);


    free(key);
    return EXIT_SUCCESS;
}
