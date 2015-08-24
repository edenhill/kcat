#include "base64.h"

const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(char *binData, char *base64, int binLength)
{
        int i = 0;
        int j = 0;
        int current = 0;
        for (i = 0; i < binLength; i += 3) {
 

                current = (*(binData+i) >> 2) & 0x3F;
                *(base64 + j++) = base64char[current];
 

                current = (*(binData+i) << 4) & 0x30;
 

                if (binLength <= (i+1)) {
                        *(base64 + j++) = base64char[current];
                        *(base64 + j++) = '=';
                        *(base64 + j++) = '=';
                        break;
                }
 

                current |= (*(binData+i+1) >> 4 ) & 0xf;
                *(base64 + j++) = base64char[current];

                current = (*(binData+i+1) << 2 ) & 0x3c;
                if (binLength <= (i+2)) {
                        *(base64 + j++) = base64char[current];
                        *(base64 + j++) = '=';
                        break;
                }
 

                current |= (*(binData+i+2) >> 6) & 0x03;
                *(base64 + j++) = base64char[current];
 

                current = *(binData+i+2) & 0x3F;
                *(base64 + j++) = base64char[current];
        }
        *(base64+j) = '\0';
 
        return base64;
}
 
 
 
 
char *base64_decode(char const *base64Str, char *debase64Str, int encodeStrLen)
{
        int i = 0;
        int j = 0;
        int k = 0;
        char temp[4] = "";
 
        for (i = 0; i < encodeStrLen; i += 4) {
                for (j = 0; j < 64 ; j++) {
                        if (*(base64Str + i) == base64char[j]) {
                                temp[0] = j;
                        }
                }
 
                for (j = 0; j < 64 ; j++) {
                        if (*(base64Str + i + 1) == base64char[j]) {
                                temp[1] = j;
                        }
                }
 
 
                for (j = 0; j < 64 ; j++) {
                        if (*(base64Str + i + 2) == base64char[j]) {
                                temp[2] = j;
                        }
                }
 
 
                for (j = 0; j < 64 ; j++) {
                        if (*(base64Str + i + 3) == base64char[j]) {
                                temp[3] = j;
                        }
                }
 
                *(debase64Str + k++) = ((temp[0] << 2) & 0xFC) | ((temp[1]>>4) & 0x03);
                if ( *(base64Str + i + 2)  == '=' )
                        break;
 
                *(debase64Str + k++) = ((temp[1] << 4) & 0xF0) | ((temp[2]>>2) & 0x0F);
                if ( *(base64Str + i + 3) == '=' )
                        break;
 
                *(debase64Str + k++) = ((temp[2] << 6) & 0xF0) | (temp[3] & 0x3F);
        }
        return debase64Str;
}

void encode(FILE * fp_in, FILE * fp_out)
{
    unsigned char bindata[2050];
    char base64[4096];
    size_t bytes;
    while ( !feof( fp_in ) )
    {
        bytes = fread( bindata, 1, 2049, fp_in );
        base64_encode( bindata, base64, bytes );
        fprintf( fp_out, "%s", base64 );
    }
}

void decode(FILE * fp_in, FILE * fp_out)
{
    int i;
    unsigned char bindata[2050];
    char base64[4096];
    size_t bytes;
    while ( !feof( fp_in ) )
    {
        for ( i = 0 ; i < 2048 ; i ++ )
        {
            base64[i] = fgetc(fp_in);
            if ( base64[i] == EOF )
                break;
            else if ( base64[i] == '\n' || base64[i] == '\r' )
                i --;
        }
        bytes = base64_decode( base64, bindata,strlen(base64));
        fwrite( bindata, bytes, 1, fp_out );
    }
}
