#include <stdio.h>
#include <stdlib.h>

// This program sets up the ADC

int main(void) {
  char path[35] = "/sys/devices/bone_capemgr.9/slots";
  printf("Setting up ADC...");
  FILE *fd = fopen(path, "w");
  if (fd == NULL) {
    fprintf(stderr, "Could not open file: %s\n", path);
    return EXIT_FAILURE;
  }
  fprintf(fd, "%s", "cape-bone-iio");
  fclose (fd);
  printf("done\n");

  return EXIT_SUCCESS;
} 
