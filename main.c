#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EXTENSION_SIZE 128
#define MIN(A, B)          ((A) < (B) ? (A) : (B))
#define MAX(A, B)          ((A) > (B) ? (A) : (B))

struct __attribute__((__packed__)) WavHeader {
  // Master RIFF chunk
  uint32_t file_type_bloc_id;
  uint32_t file_size;
  uint32_t file_format_id;

  // Chunk describing the data format
  uint32_t format_bloc_id;
  uint32_t bloc_size;
  uint16_t audio_format;
  uint16_t nbr_channels;
  uint32_t frequency;
  uint32_t byte_per_sec;
  uint16_t byte_per_bloc;
  uint16_t bits_per_sample;

  // Chunk containing the sampled data
  uint32_t data_bloc_id;
  uint32_t data_size;
};

struct Vec2 {
  int x;
  int y;
};

struct Digit {
  struct Vec2 table;
  char ch;
};

struct Dft {
  double re;
  double im;
};

void get_file_extension(char *path, char *extension) {
  char *extension_start = strrchr(path, '.');
  char *dir_start = strrchr(path, '/');
  if (!extension_start || (dir_start && dir_start > extension_start)) {
    extension[0] = '\0';
    return;
  }
  strncpy(extension, extension_start, strlen(extension_start));
}

int get_harmonic(int frequency, int resolution) { return frequency / resolution; }
double get_magnitude(struct Dft harmonic) { return sqrt(harmonic.re * harmonic.re + harmonic.im * harmonic.im); }

char get_digit(struct Digit *harmonics_to_digit, size_t count, int harmonic1, int harmonic2) {
  for (int i = 0; i < count; i++) {
    int x = harmonics_to_digit[i].table.x, y = harmonics_to_digit[i].table.y;
    if (MIN(harmonic1, harmonic2) == MIN(x, y) && MAX(harmonic1, harmonic2) == MAX(x, y)) return harmonics_to_digit[i].ch;
  }
  return NULL;
}

struct Dft *calculate_dft(uint8_t *data, size_t N, int *harmonics, size_t count) {
  struct Dft *res = (struct Dft *)malloc(count * sizeof(struct Dft));
  for (int i = 0; i < count; i++) {
    int harmonic = harmonics[i];
    double sum_x = 0, sum_y = 0;
    for (int n = 0; n < N; n++) {
      double angle = (2 * M_PI * harmonic * n) / N;
      sum_x += data[n] * cos(angle);
      sum_y += data[n] * sin(angle);
    }
    res[i].re = sum_x / N;
    res[i].im = sum_y / N;
  }
  return res;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("You must specify the audio file\n");
    return -1;
  }
  char *path = argv[1];
  FILE *sound = fopen(path, "rb");
  if (!sound) {
    printf("Audio file not found\n");
    return -1;
  }
  char extension[MAX_EXTENSION_SIZE];
  get_file_extension(path, extension);
  if (!strlen(extension) || strcmp(extension, ".wav")) {
    printf("Only wav files are supported\n");
    fclose(sound);
    return -1;
  }
  struct WavHeader header;
  fread(&header, sizeof(header), 1, sound);

  // CONSTANTS
  struct Digit frequencies_to_digit[] = {
      {{697, 1209}, '1'}, {{770, 1209}, '4'}, {{852, 1209}, '7'}, {{941, 1209}, '*'}, {{697, 1336}, '2'}, {{770, 1336}, '5'},
      {{852, 1336}, '8'}, {{941, 1336}, '0'}, {{697, 1477}, '3'}, {{770, 1477}, '6'}, {{852, 1477}, '9'}, {{941, 1477}, '#'},
      {{697, 1633}, 'A'}, {{770, 1633}, 'B'}, {{852, 1633}, 'C'}, {{941, 1633}, 'D'},
  };
  static const size_t DIGITS_COUNT = sizeof(frequencies_to_digit) / sizeof(frequencies_to_digit[0]);

  int frequencies[] = {697, 770, 852, 941, 1209, 1336, 1477, 1633};
  static const size_t FREQUENCIES_COUNT = sizeof(frequencies) / sizeof(frequencies[0]);

  const int SAMPLES_COUNT = header.data_size;
  const int HARMONIC_DETECTION_MAGNITUDE = 5;
  const int HARMONIC_RESOLUTION = 35;
  const int N = header.frequency / HARMONIC_RESOLUTION;
  const int SHIFT_INTERVAL = N / 3;

  int harmonics[FREQUENCIES_COUNT];
  for (int i = 0; i < FREQUENCIES_COUNT; i++) {
    harmonics[i] = get_harmonic(frequencies[i], HARMONIC_RESOLUTION);
  }

  struct Digit harmonics_to_digit[DIGITS_COUNT];
  for (int i = 0; i < DIGITS_COUNT; i++) {
    harmonics_to_digit[i].table.x = get_harmonic(frequencies_to_digit[i].table.x, HARMONIC_RESOLUTION);
    harmonics_to_digit[i].table.y = get_harmonic(frequencies_to_digit[i].table.y, HARMONIC_RESOLUTION);
    harmonics_to_digit[i].ch = frequencies_to_digit[i].ch;
  }

  // Encoding
  uint8_t samples[SAMPLES_COUNT];
  fseek(sound, sizeof(struct WavHeader), SEEK_SET);
  fread(samples, SAMPLES_COUNT, 1, sound);
  size_t curr = 0;
  while (curr + N < SAMPLES_COUNT) {
    // TODO: переписать сортировку, вместо множества массивов должен быть массив объектов "harmonic: magnitude"
    struct Dft *dft = calculate_dft(samples + curr, N, harmonics, FREQUENCIES_COUNT);
    double magnitudes[FREQUENCIES_COUNT];
    for (int i = 0; i < FREQUENCIES_COUNT; i++) {
      magnitudes[i] = get_magnitude(dft[i]);
    }
    for (int i = 0; i < FREQUENCIES_COUNT - 1; i++) {
      for (int j = i; j < FREQUENCIES_COUNT - 1; j++) {
        if (magnitudes[i] > magnitudes[i + 1]) continue;
        double temp = magnitudes[i];
        magnitudes[i] = magnitudes[i + 1];
        magnitudes[i + 1] = temp;
      }
    }
    if (magnitudes[0] > HARMONIC_DETECTION_MAGNITUDE && magnitudes[1] > HARMONIC_DETECTION_MAGNITUDE) {}
  }

  fclose(sound);
}
