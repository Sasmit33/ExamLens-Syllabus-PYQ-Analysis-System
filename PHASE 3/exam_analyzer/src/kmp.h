#ifndef KMP_H
#define KMP_H

#ifdef __cplusplus
extern "C" {
#endif



void kmp_build_lps(const char *pattern, int m, int *lps);




int kmp_find_all(const char *text, int n, const char *pattern, int m,
                 const int *lps, int *positions, int max_positions);

#ifdef __cplusplus
}
#endif

#endif
