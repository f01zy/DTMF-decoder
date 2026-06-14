# Building
```bash
git clone https://github.com/f01zy/DTMF-decoder decoder && cd decoder
gcc main.c -lm -o decoder
```

# How to use
You can generate WAV file [here](https://onlinesound.net/dtmf-generator). Pulse length and Gap length shoud be 40ms. Then run the program:

```bash
./decoder sound.wav
```


# Decoding table
![frequencies](assets/table.png)
