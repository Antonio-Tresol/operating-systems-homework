#define _DEFAULT_SOURCE

#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int analize_arguments(int argc, char** arg, int* n, int* r, int* v);
int main(int argc, char** arg) {
  int64_t n = 0;  
  int64_t r = 0;
  int64_t v = 0;
  
}