#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unicode.h"

// Abandon all hope, ye who enter here

// Convert a UTF16 encoded array into a UTF8 encoded string, stopping at a null terminator.
// This properly null terminates, however you have to be sure your strings are 0d beforehand
void wtoa(char *out, u16 *in)
{
    u8 lastByte = 1; // Initialize to 1 because
    while(*in != 0 || lastByte != 0)
    {
        lastByte = *in;
        if (*in == 0) continue;
        *out = *in;
        out++;
        in++;
    }
    *out = 0;
}

// Convert standard ascii to a UTF16 encoded array.
// Return the number of characters. Should be equal to strlen(in)
ssize_t atow(u16 *out, char *in)
{
    u8 i = 0;
    for (i = 0; i < strlen(in); i++) 
    {
        out[i] = in[i];
    }
    return i;
}


// Remove a file extension on a UTF16 encoded array. 
// Returns the number of characters in the array. Should be equal to len - 4 in most cases.
// Warning: This assumes everything after the first '.' is an extension and trashes it.
ssize_t trim_extension(u16 *out, u16 *in, ssize_t len)
{
    for (int i = 0; i < len; i++)
    {
        if (in[i] == '.')
        {
            out[i] = '\0';
            return i;
        }
        out[i] = in[i];
    }
    return len;
}

// Removes all Unicode characters (and extended ASCII) and replaces them with nothing
// Returns a boolean indicated whether or not there's any change in  
bool strip_unicode(u16 *out, u16 *in, ssize_t len)
{
    bool changed = false;
    int n = 0;
    for (int i = 0; i < len; i++)
    {
        if (in[i] == 0)
        {
            return changed;
        } else if (in[i] > 127) // This isn't Chinese checkers - this isn't even regular checkers!
        {
            changed = true;
            printf("Changed on character: %c\n", in[i]);
        } else {
            out[n] = in[i];
            n++;
        }
    }
    return changed;
}