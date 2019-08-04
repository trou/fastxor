Fast tool to xor files.

```console
$ ./fastxor -h
Usage: fastxor -x hexkey|-f file_key [input_file ouput_file]
```
When no input and output file is specified, uses stdin and stdout.

### Xor with hex string

```console
$ ./fastxor -x FF input output
$ ./fastxor -x 010203040506 input output
$ cat input | ./fastxor -x 11 > output
```

### Xor with a file

```console
$ hd keyfile
00000000  01 02 03 04 05 06                                 |......|
$ ./fastxor -f keyfile intput output
$ cat input | ./fastxor -f keyfile > output
```
