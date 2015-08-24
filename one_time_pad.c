#include <stdlib.h>
#include <stdio.h>

#define KEY_LENGTH 31


int main(){
  unsigned char ch;
  FILE *fpIn, *fpOut;
  int i;
  unsigned char key[KEY_LENGTH] = {0xF2, 0x1A, 0x04, 0x9B,
				   0xD0, 0x73, 0x23, 0xC8,
				   0x39, 0x98, 0xCE, 0x09,
				   0x0E, 0xBC, 0x86, 0xDA,
				   0xC9, 0xE0, 0x39, 0x89,
				   0x2A, 0x5F, 0x72, 0x67,
				   0x83, 0xA5, 0x61, 0xFD,
				   0x25, 0xEE, 0x30};
 

  fpIn = fopen("messages.txt", "r");
  fpOut = fopen("ctexts.txt", "w");

  i=0;

  while (fscanf(fpIn, "%c", &ch) != EOF) {
    fprintf(fpOut, "%02X", ch^key[i]);
    i++;
    if (i==31) {
      fprintf(fpOut, "\n");
      i=0;
      fscanf(fpIn, "%c", &ch);
    }
  }
  fscanf(fpOut, "\n");

  fclose(fpIn);
  fclose(fpOut);

  exit(EXIT_SUCCESS);
}