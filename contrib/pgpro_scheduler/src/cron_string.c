#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cron_string.h"
#include "postgres.h"
#include "c.h"
#include "port.h"


char *cps_month_subst_data[12] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
char *cps_wday_subst_data[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

int cps_error = 0;
static char cps_error_buffer[512];

void _cps_set_error(int num, const char *message, ...)
{
	va_list arglist;

	cps_error = num;
	va_start(arglist, message);
	pvsnprintf(cps_error_buffer, 512, message, arglist);
	va_end(arglist);
}

char *get_cps_error()
{
	if(cps_error)
	{
		return cps_error_buffer;
	}
	return NULL;
}

int _cps_string_has(char *str, char c)
{
	int i;
	int n = strlen(str);

	for(i=0; i < n; i++)
	{
		if(c == str[i]) return 1;
	}
	return 0;
}

char *_cps_append_string(char *str, char *to_add)
{
	int end = str ? strlen(str): 0;
	int len = strlen(to_add);

	str = realloc(str, end + len + 1);
	memcpy(str+end, to_add, len);
	str[end+len] = 0;

	return str;
}

char *_cps_make_array(int *src, int len)
{
	char *out = NULL;
	int i = 0;
	char buff[100];

	out = _cps_append_string(out, "[");
	for(i=0; i<len; i++)
	{
		sprintf(buff, "%d,", src[i]);
		if(len == (i+1)) buff[strlen(buff)-1] = 0;
		out = _cps_append_string(out, buff);
	}
	return _cps_append_string(out, "]");
}

int _cps_dig_digit(char *str)
{
	int len = strlen(str);
	char *endptr = NULL;
	int res;

	if(len == 1 && str[0] == '*')
	{
		return -1;
	}
	res = (int)(strtol(str, &endptr, 10));
	if(endptr == str)
	{
		cps_error = 1;
		_cps_set_error(44, "the %s should be digit", str);
		return -2;
	}
	return res;
}

void _cps_clean_charchar(char **data, int len)
{
	int i;

	for(i=0; i < len; i++)
	{
		free(data[i]);
	}
	free(data);
}

char **_cps_split_by(char sep, char *src, int *N)
{
	int len = strlen(src);
	char **res = NULL;
	int i, n = 1, ri = 0, ti = 0;
	char tmp[1024];

	for(i=0; i<len; i++)
	{
		if(src[i] == sep) n++;
	}
	res = malloc(sizeof(char *)*n);
	*N = n;

	for(i=0; i < len; i++)
	{
		if(src[i] == sep)
		{
			tmp[ti++] = 0;
			res[ri] = (char *)malloc(sizeof(char)*(ti));
			memcpy(res[ri++], tmp, ti);
			ti = 0;
		}
		else
		{
			if(ti < 1024) tmp[ti++] = src[i];
		}
	}
	tmp[ti++] = 0;
	res[ri] = malloc(sizeof(char)*(ti+1));
	memcpy(res[ri], tmp, ti+1);

	return res;
}

int *_cps_parse_range(char *src, int start, int end, int *len)
{
	char *values = NULL;
	int hasStar = 0;
	int range_len = end-start+1;
	int i, a, b, cur;
	int ni = 0;
	char **parts;
	int e_len = 0, nparts = 0;
	char **elements;
	int isError = 0;
	int *ptr = NULL;

	*len = 0;

	values = malloc(sizeof(char)*range_len);
	memset(values, 0, sizeof(char)*range_len);

	parts = _cps_split_by(',', src, &nparts);

	for(i=0; i < nparts; i++)
	{
		if(strlen(parts[i]) == 1 && parts[i][0] == '*')
		{
			hasStar = 1;
			break;
		}
	}
	if(hasStar)
	{
		memset(values, 1, sizeof(char)*range_len);
	}
	else
	{
		for(i=0; i < nparts; i++)
		{
			if(_cps_string_has(parts[i], '-'))
			{
				elements = _cps_split_by('-', parts[i], &e_len);
				if(e_len == 2)
				{
					a = _cps_dig_digit(elements[0]);
					if(a < 0)
					{
						_cps_clean_charchar(elements, e_len);
						isError = 1;
						if(a == -1) _cps_set_error(1, "star is not allowed in range");
						break;
					}
					b = _cps_dig_digit(elements[1]);
					if(b < 0)
					{
						_cps_clean_charchar(elements, e_len);
						isError = 2;
						if(b == -1) _cps_set_error(2, "star is not allowed in range");
						break;
					}
					if(b < a)
					{
						_cps_clean_charchar(elements, e_len);
						isError = 3;
						_cps_set_error(2, "bad  range: %d - %d", a, b);
						break;
					}
					for(i = 0; i < range_len; i++)
					{
						if(i >= (a-start) && i <= (b-start)) values[i] = 1;
					}
					_cps_clean_charchar(elements, e_len);
				}
				else
				{
					_cps_clean_charchar(elements, e_len);
					isError = 4;
					_cps_set_error(4, "range must be of 2 elements: %s", parts[i]);
					break;
				}
			}
			else if(_cps_string_has(parts[i], '/'))
			{
				elements = _cps_split_by('/', parts[i], &e_len);
				if(e_len == 2)
				{
					a = _cps_dig_digit(elements[0]);
					if(a < -1)
					{
						isError = 5;
						_cps_clean_charchar(elements, e_len);
						break;
					}
					if(a == -1) a = start;
					b = _cps_dig_digit(elements[1]);
					if(b <= 0)
					{
						isError = 6;
						_cps_clean_charchar(elements, e_len);
						if(b == -1) _cps_set_error(6, "star is not allowed as period: %s", parts[i]);
						if(b == 0) _cps_set_error(6, "period must be greater than zero: %s", parts[i]);
						break;
					}
					cur = a -start;
					while(cur <= (end+start))
					{
						values[cur] = 1;
						cur += b;
					}
				}
				else
				{
					isError = 10;
					_cps_set_error(10, "wrong period format: %s", parts[i]);
					_cps_clean_charchar(elements, e_len);
					break;
				}
				_cps_clean_charchar(elements, e_len);
			}
			else
			{
				a = _cps_dig_digit(parts[i]);
				if(a < start)
				{
					isError = 11;
					if(a > -1) _cps_set_error(11, "%d is out of range", a);
					break;
				}
				if(a > end)
				{
					isError = 12;
					_cps_set_error(12, "%d is out of range", a);
					break;
				}
				values[a-start] = 1;
			}
		}
	}

	if(isError == 0)
	{
		for(i =0; i < range_len; i++)
		{
			if(values[i]) (*len)++;
		}

		if(*len > 0)
		{
			ptr = malloc(sizeof(int)*(*len));
		}
		else
		{
			ptr = NULL;
		}

		for(i = 0; i < range_len; i++)
		{
			if(values[i]) ptr[ni++] = i+start;
		}
	}
	else
	{
		ptr = NULL;
		ni = 0;
	}
	free(values);
	_cps_clean_charchar(parts, nparts);

	return ptr;
}

char *_cps_subst_str(char *str, char **subst_array, int subst_len, int step)
{
	int len = strlen(str);
	char *new_str =  NULL;
	int *candidats = (int *)malloc(sizeof(int)*subst_len);
	int *new_cands = (int *)malloc(sizeof(int)*subst_len);
	int cands_num = 0;
	int new_cands_num = 0;
	int this_clen;
	char *this_cand = NULL;
	int has_match = -1;
	int i,j;
	int cand_step = 0;
	match_ent_t *matches = NULL;
	int matches_count = 0;
	int new_len = 0, new_curr = 0, curr = 0;
	char buff[100];

	for(i=0; i < subst_len; i++)
	{
		candidats[i] = i;
	}
	cands_num = subst_len;

	for(i=0; i < len; i++)
	{
		for(j=0; j < cands_num; j++)
		{
			this_cand = subst_array[candidats[j]];
			this_clen = strlen(this_cand);
			if(this_clen > cand_step)
			{
				if(this_cand[cand_step] == str[i])
				{
					if(cand_step ==  this_clen - 1)
					{
						has_match = candidats[j];
					}
					else
					{
						new_cands[new_cands_num++] = candidats[j];
					}
				}
			}
		}
		if(has_match >= 0)
		{
			for(j=0; j < subst_len; j++)
			{
				candidats[j] = j;
			}
			matches_count++;
			matches = realloc(matches, (sizeof(match_ent_t) * matches_count));
			matches[matches_count-1].start = i - cand_step;
			matches[matches_count-1].end = i;

			matches[matches_count-1].subst_len = sprintf(buff, "%d", has_match + step);
			matches[matches_count-1].subst = malloc(sizeof(matches[matches_count-1].subst_len));
			memcpy(matches[matches_count-1].subst, buff, matches[matches_count-1].subst_len);

			cands_num = subst_len;
			new_cands_num = 0;
			cand_step = 0;
			has_match = -1;
		}
		else if(new_cands_num == 0)
		{
			i -= cand_step;
			cand_step = 0;
			for(j=0; j < subst_len; j++)
			{
				candidats[j] = j;
			}
			cands_num = subst_len;
			new_cands_num = 0;
		}
		else
		{
			cand_step++;
			cands_num = new_cands_num;
			for(j=0; j < new_cands_num; j++)
			{
				candidats[j] = new_cands[j];
			}
			new_cands_num = 0;
		}
	}
	free(candidats);
	free(new_cands);

	if(matches_count > 0)
	{
		for(i=0; i < matches_count; i++)
		{
			new_len += (matches[i].end - matches[i].start + 1) - matches[i].subst_len ;
		}
		new_len = len - new_len;
		new_str = malloc(sizeof(char) * new_len+1);

		for(i=0; i < matches_count; i++)
		{
			memcpy(&(new_str[new_curr]), &(str[curr]), matches[i].start - curr);

			new_curr += (matches[i].start - curr);
			memcpy(&new_str[new_curr], matches[i].subst, matches[i].subst_len);
			new_curr += matches[i].subst_len;
			curr = matches[i].end+1;

			free(matches[i].subst);
		}
		if(curr < (len-1))
		{
			memcpy(&new_str[new_curr], &(str[curr]), len - curr);
			new_curr += (len - curr);
		}
		new_str[new_curr] = 0;
		free(matches);
		free(str);
		return new_str;
	}

	return str;
}

void destroyCronEnt(cron_ent_t *e)
{
	if(e->mins) free(e->mins);
	if(e->hour) free(e->hour);
	if(e->day) free(e->day);
	if(e->month) free(e->month);
	if(e->wdays) free(e->wdays);

	free(e);
}

cron_ent_t *parse_crontab(char *cron_str)
{
	cron_ent_t *CE;
	/* min, hour, day, month, wd */
	char *entrs[5] = {NULL, NULL, NULL, NULL, NULL};
	int N = strlen(cron_str);
	char tmp[256];
	int i, ti =0, en = 0 ;

	if(N == 7 && strcmp(cron_str, "@reboot") == 0)
	{
		CE = (cron_ent_t *)malloc(sizeof(cron_ent_t));
		memset((void *)CE, 0, sizeof(cron_ent_t));
		CE->onstart = 1;

		return CE;
	}

	for(i=0; i < N; i++)
	{
		if(cron_str[i] != ' ' && cron_str[i] != '\t')
		{
			tmp[ti++] = cron_str[i];
		}
		else
		{
			tmp[ti++] = 0;
			entrs[en] = malloc(sizeof(char) * ti);
			memcpy(entrs[en], tmp, ti);
			ti = 0;
			en++;
		}
	}
	if(ti)
	{
		tmp[ti++] = 0;
		entrs[en] = malloc(sizeof(char) * ti);
		memcpy(entrs[en], tmp, ti);
	}
	if(en != 4)
	{
		_cps_set_error(55, "cron string wrong format");
		for(i=0; i < 5; i++)
		{
			if(entrs[i]) free(entrs[i]);
		}
		return NULL;
	}

	CE = (cron_ent_t *)malloc(sizeof(cron_ent_t));
	memset((void *)CE, 0, sizeof(cron_ent_t));


	entrs[3] = _cps_subst_str(entrs[3], cps_month_subst_data, 12, 1);
	entrs[4] = _cps_subst_str(entrs[4], cps_wday_subst_data, 7, 0);

	CE->mins = _cps_parse_range(entrs[0], 0, 59, &(CE->mins_len));
	if(CE->mins) CE->hour = _cps_parse_range(entrs[1], 0, 23, &(CE->hour_len));
	if(CE->hour) CE->day = _cps_parse_range(entrs[2], 1, 31, &(CE->day_len));
	if(CE->day) CE->month = _cps_parse_range(entrs[3], 1, 12, &(CE->month_len));
	if(CE->month) CE->wdays = _cps_parse_range(entrs[4], 0, 6, &(CE->wdays_len));

	for(i=0; i < 5; i++)
	{
		if(entrs[i]) free(entrs[i]);
	}

	if(CE->wdays) return CE;

	destroyCronEnt(CE);
	return NULL;
}

char *parse_crontab_to_json_text(char *cron_str)
{
	char *out = NULL;
	char *tmp_out;
	cron_ent_t *cron;

	cron = parse_crontab(cron_str);

	if(cron)
	{
		out = _cps_append_string(out, "{ \"crontab\": \"");
		out = _cps_append_string(out, cron_str);
		out = _cps_append_string(out, "\", ");
		if(cron->onstart)
		{
			out = _cps_append_string(out, "\"onstart\": 1");
		}
		else
		{
			out = _cps_append_string(out, "\"minutes\": ");
			tmp_out = _cps_make_array(cron->mins, cron->mins_len);
			out = _cps_append_string(out, tmp_out);
			free(tmp_out);
			out = _cps_append_string(out, ", ");

			out = _cps_append_string(out, "\"hours\": ");
			tmp_out = _cps_make_array(cron->hour, cron->hour_len);
			out = _cps_append_string(out, tmp_out);
			free(tmp_out);
			out = _cps_append_string(out, ", ");

			out = _cps_append_string(out, "\"days\": ");
			tmp_out = _cps_make_array(cron->day, cron->day_len);
			out = _cps_append_string(out, tmp_out);
			free(tmp_out);
			out = _cps_append_string(out, ", ");

			out = _cps_append_string(out, "\"months\": ");
			tmp_out = _cps_make_array(cron->month, cron->month_len);
			out = _cps_append_string(out, tmp_out);
			free(tmp_out);
			out = _cps_append_string(out, ", ");

			out = _cps_append_string(out, "\"wdays\": ");
			tmp_out = _cps_make_array(cron->wdays, cron->wdays_len);
			out = _cps_append_string(out, tmp_out);
			free(tmp_out);
		}
		out = _cps_append_string(out, "}");

		destroyCronEnt(cron);
	}

	return out;
}

