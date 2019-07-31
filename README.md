Dumb tool to xor files.

Supports SSE2 for some cases.


```console
$ ./fastxor -h
Usage: fastxor -x hexkey|-f file_key input_file ouput_file
```

### Xor with hex string

```console
$ ./fastxor -x FF input output
$ ./fastxor -x 010203040506 input output
```

### Xor with a file

```console
$ hd keyfile
00000000  01 02 03 04 05 06                                 |......|
$ ./fastxor -f keyfile intput output
```
