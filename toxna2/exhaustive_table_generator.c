#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include "rules.h"
#include "deht.h"


/*
 * populate the DEHT with passwords generated by the given rule.
 * this function will generate all passwords until the rule is exhausted.
 */
static int populate_deht(DEHT * deht, rule_info_t * rule)
{
	int res;
	int pwlength;
	int digest_size;
	char password[MAX_INPUT_BUFFER];
	unsigned char digest[MAX_DIGEST_LENGTH_IN_BYTES];
#ifdef TABLE_SHOW_PROGRESS
	int added_passwords = 0;
	char buf[MAX_INPUT_BUFFER];
#endif

	if (strcmp(rule->hashname, "MD5") == 0) {
		digest_size = MD5_OUTPUT_LENGTH_IN_BYTES;
	}
	else if (strcmp(rule->hashname, "SHA1") == 0) {
		digest_size = SHA1_OUTPUT_LENGTH_IN_BYTES;
	}
	else {
		/* this should never happen, as rule_load already validated the hash name */
		fprintf(stderr, "invalid hash name '%s'\n", rule->hashname);
		return -1;
	}

	while (1) {
		res = rule_generate_next_password(rule, password, sizeof(password));
		if (res == RULE_STATUS_EXHAUSTED) {
			/* exhausted all passwords */
#ifdef TABLE_SHOW_PROGRESS
			printf("Added %d passwords\n", added_passwords);
#endif
			return 0;
		} else if (res != RULE_STATUS_OK) {
			/* error message is printed by rule_generate_password */
			return -1;
		}
		pwlength = strlen(password);
		if (rule->hashfunc((unsigned char*)password, pwlength, digest) < 0) {
			fprintf(stderr, "%s of generated password failed\n", rule->hashname);
			return -1;
		}

		/* Instructions say:
		 *    When receiving parameter �flag = �n�� in .ini file [...] you should use insert_uniquely_DEHT
		 *    to avoid inserting same password many times. When �flag = �all�� you should use add_DEHT. */
		if (rule->limit < 0) {
			res = add_DEHT(deht, digest, digest_size, (unsigned char*)password, pwlength);
		}
		else {
			res = insert_uniquely_DEHT(deht, digest, digest_size, (unsigned char*)password, pwlength);
		}
		if (res == DEHT_STATUS_FAIL) {
			/* error message is printed by insert_uniquely_DEHT */
			return -1;
		}
#ifdef TABLE_SHOW_PROGRESS
		added_passwords++;
		if (added_passwords % 500 == 0) {
			printf("%d\n", added_passwords);
			binary2hexa(digest, digest_size, buf, sizeof(buf));
			printf("%s : %s\n", buf, password);
		}
#endif
	}
}


int main(int argc, const char** argv)
{
	int res = 1;
	rule_info_t rule;
	DEHT * deht = NULL;
	char ini_file[MAX_INPUT_BUFFER];

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <prefix>\n", argv[0]);
		return 1;
	}

	/* load rule */
	strncpy(ini_file, argv[1], sizeof(ini_file) - 5);
	strcat(ini_file, ".ini");
	if (rule_load_from_file(&rule, ini_file) != 0) {
		/* error message printed by rule_load_from_file */
		return 1;
	}

	/* create deht with parameters from intructions */
	deht = create_empty_DEHT(argv[1], my_hash_func, my_valid_func,
			65536, /* numEntriesInHashTable */
			10, /* nPairsPerBlock */
			8, /* nBytesPerKey */
			rule.hashname);
	if (deht == NULL) {
		/* error message printed by create_empty_DEHT */
		goto cleanup;
	}

	/* instructions:
	 *    Unlike querying, during the creation of the table (exhaustive_table_generator), you should
	 *    hold the relevant table-of-pointers (both head and tails) in memory as it is frequently
	 *    accessed during creation.
	 */

	/* enable caching to speed up insertion */
	if (read_DEHT_pointers_table(deht) == DEHT_STATUS_FAIL) {
		/* error message printed by read_DEHT_pointers_table */
		goto cleanup;
	}

	/* populate the table */
	if (populate_deht(deht, &rule) != 0) {
		/* error message printed by create_empty_DEHT */
		goto cleanup;
	}

	/* success */
	res = 0;

cleanup:
	if (deht != NULL) {
		close_DEHT_files(deht);
	}
	rule_finalize(&rule);
	return res;
}
