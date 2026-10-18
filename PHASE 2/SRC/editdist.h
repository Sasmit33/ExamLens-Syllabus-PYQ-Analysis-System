#ifndef EDITDIST_H
#define EDITDIST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Levenshtein distance: minimum insertions, deletions and substitutions.
   Dynamic programming, O(n*m) time, O(m) space (two rows). */

/* Character level: "travesal" vs "traversal" = 1. Used for typo-tolerant keywords. */
int edit_distance_str(const char *a, const char *b);

/* Word level: each word is one unit. Used to detect reworded repeat questions. */
int edit_distance_tokens(char *const *a, int n, char *const *b, int m);

#ifdef __cplusplus
}
#endif

#endif
