#ifndef CRON_STRING_PARSE_H
#define CRON_STRING_PARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

extern int cps_error;

#define CE_MINUTES_LEN	60
#define CE_HOURS_LEN	24
#define CE_DAYS_LEN		31
#define CE_MONTHS_LEN	12
#define CE_DOWS_LEN		7

typedef struct {
	int start;
	int end;
	char *subst;
	int subst_len;
} match_ent_t;

typedef struct {
	int mins_len;
	int *mins;
	
	int hour_len;
	int *hour;

	int day_len;
	int *day;

	int month_len;
	int *month;

	int wdays_len;
	int *wdays;

	unsigned char onstart;
} cron_ent_t;

void destroyCronEnt(cron_ent_t *);
void _cps_set_error(int num, const char *message, ...) 
#ifdef __GNUC__
__attribute__ ((format (gnu_printf, 2, 3)))
#endif
;;
char *get_cps_error(void);
int _cps_string_has(char *str, char c);
char *_cps_append_string(char *str, char *to_add);
char *_cps_make_array(int *src, int len);
int _cps_dig_digit(char *str);
void _cps_clean_charchar(char **data, int len);
char **_cps_split_by(char sep, char *src, int *N);
int *_cps_parse_range(char *src, int start, int end, int *len);
char *_cps_subst_str(char *str, char **subst_array, int subst_len, int step);
cron_ent_t *parse_crontab(char *cron_str);
char *parse_crontab_to_json_text(char *cron_str);

#endif
