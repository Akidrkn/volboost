# volboost
изменения громкости через PulseAudio
## Компиляция

```bash id="qheknm"
gcc volboost.c -o volboost $(pkg-config --cflags --libs libpulse)
```

## Запуск

```bash id="jxx2hb"
./volboost
```
