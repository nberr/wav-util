/*
 * wav-look: a simple command line utility to view wav files
 * Nicholas Berriochoa
 * 14 February 2021
 */
#include <stdio.h> /* io functions */
#include <stdint.h> /* uint types */
#include <stdlib.h> /* mem allocation */
#include <string.h> /* strcmp */

#ifndef DEBUG
#define DEBUG 0
#endif

#define ID_LEN 4 /* chunk IDs */

#define BLOCK 4096 /* how much data we will read in at a time */
#define MAX_BLOCKS 524288 /* wav files are capped at 2gb */

#define BITS_PER_BYTE 8

/* used for programming part B */
#define D_START 100
#define D_END 30

const char *sample_name  = "sample.wav";
const char *silence_name = "silence.wav"; 

/* RIFF definitions */
const char *RIFF_ID = "RIFF";
const char *RIFF_FMT = "WAVE";
struct riff_chunk {
   char chunkID[ID_LEN];
   uint32_t chunkSize;
   char format[ID_LEN];
};

/* fmt definitions */
const char *FMT_ID = "fmt ";
struct fmt_chunk {
   char chunkID[ID_LEN];
   uint32_t chunkSize;
   uint16_t audioFormat;
   uint16_t numChannels;
   uint32_t sampleRate;
   uint32_t byteRate;
   uint16_t blockAlign;
   uint16_t bitsPerSample;
};

/* data definitions */
const char *DATA_ID = "data";
struct data_chunk {
   char chunkID[ID_LEN];
   uint32_t chunkSize;
};

/* the wav file containing the 3 chunks */
typedef struct wav_header_t {
   struct riff_chunk r;
   struct fmt_chunk f;
   struct data_chunk d;
}wav_header;

const size_t HEADER_SIZE = sizeof(wav_header);

/*
 * this function is used to verify that the file entered
 * is in fact a wav file. If it is not, the program
 * will terminate. Some wav files can contain extra chunks
 * such as JUNK but this program will only accept wav files
 * with the 3 chunks we discussed in lecture
 */
int verify_file(wav_header *input) {
   int error = 0;
   /* check the RIFF id */
   if (strncmp(input->r.chunkID, RIFF_ID, ID_LEN)) {
      printf("riff chunk could not be verified: %s\n", input->r.chunkID);
      error++;
   }

   /* check the RIFF format */
   if (strncmp(input->r.format, RIFF_FMT, ID_LEN)) {
      printf("riff format could not be verified: %s\n", input->r.format);
      error++;
   }

   /* check the fmt ID */
   if (strncmp(input->f.chunkID, FMT_ID, ID_LEN)) {
      printf("format chunk could not be verified: %s\n", input->f.chunkID);
      error++;
   }

   /* check the data ID */
   if (strncmp(input->d.chunkID, DATA_ID, ID_LEN)) {
      printf("data chunk could not be verified: %s\n", input->d.chunkID);
      error++;
   }

   return error;
}

/* 
 * This function displays info about the wav file to the user
 */
void print(wav_header *input) {
   printf("+------------+\n");
   printf("| RIFF CHUNK |\n");
   printf("+____________+\n");

   printf("ID\t%.4s\n",     input->r.chunkID);
   printf("Size\t%d\n",     input->r.chunkSize);
   printf("Format\t%.4s\n", input->r.format);

   printf("+-----------+\n");
   printf("| FMT CHUNK |\n");
   printf("+-----------+\n");

   printf("ID\t\t%.4s\n",            input->f.chunkID);
   printf("Size\t\t%d\n",            input->f.chunkSize);
   printf("Format\t\t%d\n",          input->f.audioFormat);
   printf("Channels\t%d\n",        input->f.numChannels);
   printf("Sample rate\t%d\n",     input->f.sampleRate);
   printf("Byte rate\t%d\n",       input->f.byteRate);
   printf("Block align\t%d\n",     input->f.blockAlign);
   printf("Bits per sample\t%d\n", input->f.bitsPerSample);

   printf("+------------+\n");
   printf("| DATA CHUNK |\n");
   printf("+------------+\n");
   printf("ID\t%.4s\n",     input->d.chunkID);
   printf("Size\t%d\n",     input->d.chunkSize);
}

/*
 * This function creates a new wav file and writes the original header
 * to that file unless the name is "sample.wav". in that case, the header
 * is modified for programming Part A
 */
FILE* create_file (const char *name, wav_header header) {
   FILE* f = NULL;

   /* modifiy the header for Programming part A */
   if (strcmp(name, "sample.wav") == 0) {
      /* this will cause the playback to be at 50% speed */
      header.f.sampleRate /= 2;
   }

   /* create the file */
   if (!(f = fopen(name, "w"))) {
      fprintf(stderr, "Failed to create %s\n", name);
      exit(EXIT_FAILURE);
   }

   /* write the header to the new file */
   size_t bytes;
   if ((bytes = fwrite(&header, HEADER_SIZE, 1, f)) != 1) {
      fprintf(stderr, "Writing header to %s failed. bytes written: %zu\n", name, bytes);
      exit(EXIT_FAILURE);
   }

   /* return the file pointer to main */
   return f;
}

/* 
 * this function will silence a portion of the data for Programming Part B 
 */
void silence_section(uint8_t *data, int block, wav_header header) {
   
   /* 
    * calculate the start and end to silence 
    * this function will only work if the number of samples is less than or equal to
    * the max value of a 32bit unsigned integer 
    */
   uint32_t num_samples = header.d.chunkSize / (header.f.bitsPerSample / BITS_PER_BYTE);
   uint32_t dstart = (num_samples / 2) + (num_samples / D_START);
   uint32_t dend   = (num_samples / 2) + (num_samples / D_END);

   if (DEBUG) {
      fprintf(stderr, "dstart: %d\n", dstart);
      fprintf(stderr, "dend:   %d\n", dend);
   }

   /* step through the block of audio data and silence the portion as needed */
   int i;
   for (i = 0; i < BLOCK; i++) {
      int pos = (block * BLOCK) + i;

      if ((pos >= dstart) && (pos <= dend)) {
         data[i] = 0;

         if (DEBUG) {
            fprintf(stderr, "pos: %d\n", pos);
         }
      }
   }
}

/*
 * this function writes the audio data to the newly created wav files
 */
void write_data(wav_header header, FILE* original, FILE* sample, FILE* silence) {
   size_t bytes;

   /* allocate data to read in the audio data portion of the file */
   uint8_t *data = (uint8_t *)calloc(BLOCK, sizeof(uint8_t));
   if (data == NULL) {
      fprintf(stderr, "Data block allocation failed\n");
      exit(EXIT_FAILURE);
   }

   /* allocate another block to manipulate the audio data */
   uint8_t *silence_data = (uint8_t *)calloc(BLOCK, sizeof(uint8_t));
   if (silence_data == NULL) {
      fprintf(stderr, "Silence block allocation failed\n");
      exit(EXIT_FAILURE);
   }

   /* read a chunk of data from the original file */
   size_t data_bytes;
   int num_blocks = 0;
   while ((bytes = fread(data, sizeof(uint8_t), BLOCK, original)) > 0) {
      num_blocks++;
      /* modify the data and write to the new files */
      memcpy(silence_data, data, BLOCK);
      silence_section(silence_data, num_blocks, header);

      if (DEBUG) {
         fprintf(stderr, "Bytes read: %zu\n", bytes);
      }

      /* 
       * write original audio data to the sample wav file 
       * The original audio data can be used because changing the sample
       * rate is handled in the header of the file
       */
      if ((data_bytes = fwrite(data, sizeof(uint8_t), bytes, sample)) != bytes) {
         fprintf(stderr, "Writing audio data to sample.wav failed. bytes written: %zu\n", data_bytes);
         exit(EXIT_FAILURE);
      }

      /* write the modified audio data to the silence wav file */
      if ((data_bytes = fwrite(silence_data, sizeof(uint8_t), bytes, silence)) != bytes) {
         fprintf(stderr, "Writing audio data to silence.wav failed. bytes written: %zu\n", data_bytes);
         exit(EXIT_FAILURE);
      }
   }

   if (DEBUG) {
      fprintf(stderr, "%d blocks read in\n", num_blocks);
   }
}

int main(int argc, char **argv) {
   FILE *original;
   wav_header header;

   /* check command line usage */
   if (argc == 1) {
      printf("please provide a file: ./wav-look <filename|path>\n");
      exit(EXIT_FAILURE);
   }
   else if (argc > 2) {
      printf("too many arguments: ./wav-look <filename|path>\n");
      exit(EXIT_FAILURE);
   }

   /* try to open the file */
   if (!(original = fopen(argv[1], "rb"))) {
      fprintf(stderr, "failed to open file: %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }

   /* try to read in the header */
   size_t header_read = fread(&header, HEADER_SIZE, 1, original);
   if (header_read < 1) {
      fprintf(stderr, "reading file header failed. bytes read: %zu\n", header_read);
      exit(EXIT_FAILURE);
   }

   /* check to make sure the file is a wav file */
   if (verify_file(&header)) {
      fprintf(stderr, "Input file could not be verified\n");
      exit(EXIT_FAILURE);
   }

   /* print the header information */
   print(&header);

   /* create the sample and silence files */
   FILE *sample = create_file(sample_name,   header);
   FILE *silence = create_file(silence_name, header);

   /* write the audio data to the new files */
   write_data(header, original, sample, silence);

   /* close the new files */
   fclose(sample);
   fclose(silence);

   /* close the original file */
   fclose(original);

   return EXIT_SUCCESS;
}
