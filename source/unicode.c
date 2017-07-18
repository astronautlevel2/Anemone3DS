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
ssize_t trim_extension(u16 *out, u16 *in)
{
    ssize_t i = 0;
    while (true)
    {
        if (in[i] == '.' || in[i] == 0)
        {
            out[i] = '\0';
            return i;
        }
        out[i] = in[i];
        i++;
    }
    return i;
}

// Removes all Unicode characters (and extended ASCII) and replaces them with nothing
// Returns a boolean indicated whether or not there's any change in the string
bool strip_unicode(u16 *out, u16 *in, ssize_t len)
{
    bool changed = false;
    int n = 0;
    for (int i = 0; i < len; i++)
    {
        if (in[i] == 0)
        {
            out[i] = 0;
            return changed;
        } else if (in[i] > 127) // This isn't Chinese checkers - this isn't even regular checkers!
        {
            changed = true;
        } else {
            out[n] = in[i];
            n++;
        }
    }
    return changed;
}


// Copy one u16* into another u16*, without regard for the length.
// source *must* be properly null terminated for this to work!
u16 *strucpy(u16 *destination, const u16 *source)
{
    u16 pos = 0;
    while (true)
    {
        if (source[pos] == 0)
        {
            break;  
        }
        destination[pos] = source[pos];
        pos++;
    }

    return destination;
}

// Get the length of a UTF16 string
// As always, the string must be properly null terminated
ssize_t strulen(const u16 *str)
{
    ssize_t len = 0;
    while (true) if (str[++len] == 0) return len; 
}

// Concatenate one u16* onto another.
// Source and destination both must be properly null terminated to start with or this will *not* work!
u16 *strucat(u16 *destination, const u16 *source)
{
    ssize_t dest_len = strulen(destination);

    ssize_t source_len = strulen(source);

    memcpy(&destination[dest_len], source, source_len * sizeof(u16));
    return destination;
}