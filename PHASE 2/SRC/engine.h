#ifndef ENGINE_H
#define ENGINE_H
#include "common.h"
#include "editdist.h"
#include "graph.h"
#include "hashmap.h"
#include "heap.h"
#include "kmp.h"
#include "queue.h"
#include "trie.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_YEARS 32

typedef struct {
    int  line_no;        /* line in the questions file (for error messages) */
    int  year;           /* -1 if missing                                   */
    char qid[16];        /* as written, e.g. "Q3-or"                        */
    int  qnum;           /* 3   (-1 if the id could not be parsed)          */
    char sub;            /* 'a', 'b' ... or 0                               */
    int  is_or;          /* this line is an "-or" alternative               */
    int  in_or_pair;     /* base or alternative of an OR choice             */
    int  marks;          /* -1 if missing or invalid                        */
    char *raw;           /* original text, for display                      */
    char *norm;          /* normalized + alias-expanded, for matching       */

    int  is_duplicate;   /* exact duplicate within the same year: ignored   */

    int    topic;        /* best topic index, -1 if unmapped                */
    double score;        /* best topic score                                */
    double confidence;   /* best / (best + second best), 0..1               */
    int    needs_review; /* low score or low confidence                     */

    int  group;          /* repetition group id, -1 if none                 */
} Question;

typedef struct {
    char   name[MAX_NAME];
    int    unit;
    /* analytics output (recomputed by engine_compute_analytics) */
    double w_count;      /* questions, OR alternatives count 0.5            */
    double w_marks;      /* marks, OR alternatives count 0.5                */
    double repeat_score; /* questions belonging to multi-year repeat groups */
    int    years[MAX_YEARS];
    int    nyears;
    int    last_year;
    double priority;     /* 0..1                                            */
} Topic;

typedef struct {         /* multi-word keyword, searched with KMP           */
    char *text;
    int   m;
    int   nwords;
    int  *lps;
    int   topic;
} Phrase;

typedef struct {
    Question *q;   int nq, capq;
    Topic    *t;   int nt, capt;
    Phrase   *ph;  int nph, capph;      
    char    **words; int nwords, capwords;

    HashMap  *alias;       
    HashMap  *topic_index;  
    HashMap  *stopwords;   
    TrieNode *trie;         
    Graph    *prereq;      
    IntQueue  review;       

    int  ngroups;
    int *group_size;
    int *group_years;      

    int *study_order;
    int  nstudy;

    int min_year, max_year;

    char **issues; int nissues, capissues;
    int n_missing_marks, n_missing_year, n_duplicates, n_malformed;
} Engine;
Engine *engine_create(void);
void    engine_free(Engine *e);
void    engine_add_issue(Engine *e, const char *fmt, ...) PRINTF_LIKE(2, 3);
void    engine_normalize(const Engine *e, const char *raw, char *out, size_t out_size);
int engine_load_syllabus(Engine *e, const char *path);
int engine_load_questions(Engine *e, const char *path);
int engine_load_prereqs(Engine *e, const char *path);
void   engine_validate(Engine *e);                   
void   engine_map_topics(Engine *e);                  
void   engine_set_topic(Engine *e, int question, int topic); 
void   engine_detect_repetition(Engine *e);            
void   engine_compute_analytics(Engine *e, int upto_year);
int    engine_rank_topics(const Engine *e, int by_marks_only, int *out);
int    engine_study_order(Engine *e);                 
double engine_backtest(Engine *e, int test_year, int k, double *baseline);

#ifdef __cplusplus
}
#endif

#endif
