#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
char *base64_encode(char *binData, char *base64, int binLength);
char *base64_decode(char const *base64Str, char *debase64Str, int encodeStrLen);
void encode(FILE * fp_in, FILE * fp_out);
void decode(FILE * fp_in, FILE * fp_out);
