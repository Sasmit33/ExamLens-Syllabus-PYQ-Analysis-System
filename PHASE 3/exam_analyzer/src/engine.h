#ifndef ENGINE_H
#define ENGINE_H




#include <stdio.h>

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
    int  line_no;        
    int  year;           
    char qid[16];        
    int  qnum;           
    char sub;            
    int  is_or;          
    int  in_or_pair;     
    int  marks;          
    char *raw;           
    char *norm;          

    int  is_duplicate;   

    int    topic;        
    double score;        
    double confidence;   
    int    needs_review; 

    int  group;          
} Question;

typedef struct {
    char   name[MAX_NAME];
    int    unit;
    
    double w_count;      
    double w_marks;      
    double repeat_score; 
    int    years[MAX_YEARS];
    int    nyears;
    int    last_year;
    double priority;     
} Topic;

typedef struct {         
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

    double weight[4];       

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

void   engine_set_weights(Engine *e, double marks, double coverage, double recency, double repeats);


int  engine_apply_overrides(Engine *e, const char *path);   
void engine_write_json(const Engine *e, FILE *f, double backtest, double baseline, int top_k); 

#ifdef __cplusplus
}
#endif

#endif
