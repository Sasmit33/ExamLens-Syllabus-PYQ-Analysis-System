#include "kmp.h"

void kmp_build_lps(const char *pattern, int m, int *lps)
{
    if (m <= 0) return;
    int len = 0;                       
    lps[0] = 0;
    for (int i = 1; i < m;) {
        if (pattern[i] == pattern[len]) {
            lps[i++] = ++len;
        } else if (len > 0) {
            len = lps[len - 1];        
        } else {
            lps[i++] = 0;
        }
    }
}

int kmp_find_all(const char *text, int n, const char *pattern, int m,
                 const int *lps, int *positions, int max_positions)
{
    int count = 0;
    if (m <= 0 || n < m) return 0;

    for (int i = 0, j = 0; i < n && count < max_positions;) {
        if (text[i] == pattern[j]) {
            i++;
            j++;
            if (j == m) {
                int start = i - m;
                int left_ok  = (start == 0 || text[start - 1] == ' ');
                int right_ok = (i == n || text[i] == ' ');
                if (left_ok && right_ok) positions[count++] = start;
                j = lps[j - 1];        
            }
        } else if (j > 0) {
            j = lps[j - 1];            
        } else {
            i++;
        }
    }
    return count;
}
