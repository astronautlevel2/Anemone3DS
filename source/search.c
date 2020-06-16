/* C implementation of Boyer-Moore Pattern Searching taken from https://www.geeksforgeeks.org/boyer-moore-algorithm-for-pattern-searching/ */

# include <limits.h>
# include <string.h>
# include <stdio.h>

# define NO_OF_CHARS 256

int max (int a, int b) { return (a > b)? a: b; }


void badCharHeuristic( char *str, int size,
                        int badchar[NO_OF_CHARS])
{
    int i;

    for (i = 0; i < NO_OF_CHARS; i++)
         badchar[i] = -1;

    for (i = 0; i < size; i++)
         badchar[(int) str[i]] = i;
}


int BM_search(char *txt, int n, char *pat, int m)
{

    int badchar[NO_OF_CHARS];

    badCharHeuristic(pat, m, badchar);

    int s = 0;

    while(s <= (n - m))
    {
        int j = m-1;

        while(j >= 0 && pat[j] == txt[s+j])
            j--;

        if (j < 0)
        {
            return s;

            s += (s+m < n)? m-badchar[txt[s+m]] : 1;

        }

        else
            s += max(1, j - badchar[txt[s+j]]);
    }
    
    return -1;
}
