#ifndef KMP_H
#define KMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* lps[i] = length of the longest proper prefix of pattern[0..i] that is also a
   suffix of it. Built once per pattern in O(m). */
void kmp_build_lps(const char *pattern, int m, int *lps);

/* Find all WHOLE-WORD occurrences of pattern in text in O(n + m).
   "sort" matches in "merge sort" but not in "sorted".
   Writes start offsets into positions (up to max_positions); returns count. */
int kmp_find_all(const char *text, int n, const char *pattern, int m,
                 const int *lps, int *positions, int max_positions);

#ifdef __cplusplus
}
#endif

#endif
