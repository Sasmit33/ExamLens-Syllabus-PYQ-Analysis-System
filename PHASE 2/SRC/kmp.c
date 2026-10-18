#include "kmp.h"

void kmp_build_lps(const char *pattern, int m, int *lps)
{
    if (m <= 0) return;
    int len = 0;                       /* length of current matched prefix */
    lps[0] = 0;
    for (int i = 1; i < m;) {
        if (pattern[i] == pattern[len]) {
            lps[i++] = ++len;
        } else if (len > 0) {
            len = lps[len - 1];        /* fall back, do not advance i */
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
                j = lps[j - 1];        /* keep searching for further matches */
            }
        } else if (j > 0) {
            j = lps[j - 1];            /* text index i never moves backwards */
        } else {
            i++;
        }
    }
    return count;
}
