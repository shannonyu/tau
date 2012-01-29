#ifndef RULES_H_INCLUDED
#define RULES_H_INCLUDED

#include "misc.h"
#include "iniloader.h"

#define RULE_STATUS_EXHAUSTED  (-2)
#define RULE_STATUS_ERROR      (-1)
#define RULE_STATUS_OK         (0)


/* the different kinds of rule terms */
typedef enum
{
	TERM_DIGIT, TERM_LATIN, TERM_ASCII, TERM_LEXICON
} term_type_t;

/* representes a rule term */
typedef struct
{
	term_type_t type;              /* term's type (e.g., '@' in "@7") */
	int count;                     /* term's count (e.g., 7 in "@7") */

	uint64_t limit;                /* total number of values generated by this term */
	int max_size;                  /* max size in bytes of the output that may be generated
								      by this term*/
} term_info_t;

typedef struct {
	int num_of_terms;               /* number of rule terms */
	term_info_t * terms;            /* array of terms (dynamically allocated) */
} rule_pattern_t;

/* representes a complete rule */
typedef struct
{
	int num_of_patterns;            /* number of patterns (separated by '&') */
	rule_pattern_t * patterns;      /* array of patterns (dynamically allocated) */
	uint64_t * pattern_offsets;     /* array of pattern offsets */

	int num_of_words;               /* number of words in lexicon */
	int longest_word;               /* longest word in the lexicon */
	char ** words;                  /* lexicon (dynamically allocated) */

	uint64_t num_of_passwords;      /* the size of the password space (good approximation) */
	int longest_password;           /* the longest possible password that this rule generates */
} rule_info_t;


/*
 * initializes the given rule object with the given parameters (which are loaded
 * from the INI file).
 * returns RULE_STATUS_OK on success, RULE_STATUS_ERROR on failure.
 */
int rule_init(rule_info_t * info, const char * pattern, const char * lexfilename);

/*
 * returns the k'th password in the password space defined by this rule
 * if allow_empty is 0, this function will not return empty passwords
 * (it will choose a different k until the password is non-empty). if you
 * set this argument to 1, you may get empty passwords.
 */
int rule_kth_password(const rule_info_t * info, uint64_t k, char * output,
					  int output_length, int allow_empty);

/*
 * releases all resources held by this rule object
 */
void rule_finalize(rule_info_t * info);



#endif /* RULES_H_INCLUDED */
