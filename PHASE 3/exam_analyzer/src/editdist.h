#ifndef EDITDIST_H
#define EDITDIST_H

#ifdef __cplusplus
extern "C" {
#endif





int edit_distance_str(const char *a, const char *b);


int edit_distance_tokens(char *const *a, int n, char *const *b, int m);

#ifdef __cplusplus
}
#endif

#endif
