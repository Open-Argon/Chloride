// win32_strptime.h
#pragma once
#ifdef _WIN32

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

static const char *_days_full[]  = { "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" };
static const char *_days_abbr[]  = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
static const char *_months_full[] = { "January","February","March","April","May","June","July","August","September","October","November","December" };
static const char *_months_abbr[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

static int _strptime_match(const char *s, const char **table, int count) {
    for (int i = 0; i < count; i++)
        if (strncasecmp(s, table[i], strlen(table[i])) == 0)
            return i;
    return -1;
}

static char *strptime(const char *s, const char *fmt, struct tm *tm) {
    while (*fmt && *s) {
        if (*fmt != '%') {
            if (*fmt != *s) return NULL;
            fmt++; s++;
            continue;
        }
        fmt++; // skip %
        int val, match;
        switch (*fmt++) {
            case 'Y': tm->tm_year = atoi(s) - 1900; while (isdigit(*s)) s++; break;
            case 'y': val = atoi(s); tm->tm_year = val < 69 ? val + 100 : val; while (isdigit(*s)) s++; break;
            case 'm': tm->tm_mon  = atoi(s) - 1; while (isdigit(*s)) s++; break;
            case 'd':
            case 'e': tm->tm_mday = atoi(s); while (isdigit(*s)) s++; break;
            case 'H': tm->tm_hour = atoi(s); while (isdigit(*s)) s++; break;
            case 'I': tm->tm_hour = atoi(s) % 12; while (isdigit(*s)) s++; break;
            case 'M': tm->tm_min  = atoi(s); while (isdigit(*s)) s++; break;
            case 'S': tm->tm_sec  = atoi(s); while (isdigit(*s)) s++; break;
            case 'j': tm->tm_yday = atoi(s) - 1; while (isdigit(*s)) s++; break;
            case 'w': tm->tm_wday = atoi(s); while (isdigit(*s)) s++; break;
            case 'p':
            case 'P':
                if (strncasecmp(s, "pm", 2) == 0 && tm->tm_hour < 12) tm->tm_hour += 12;
                while (isalpha(*s)) s++;
                break;
            case 'A':
                match = _strptime_match(s, _days_full, 7);
                if (match < 0) return NULL;
                tm->tm_wday = match;
                s += strlen(_days_full[match]);
                break;
            case 'a':
                match = _strptime_match(s, _days_abbr, 7);
                if (match < 0) return NULL;
                tm->tm_wday = match;
                s += strlen(_days_abbr[match]);
                break;
            case 'B':
                match = _strptime_match(s, _months_full, 12);
                if (match < 0) return NULL;
                tm->tm_mon = match;
                s += strlen(_months_full[match]);
                break;
            case 'b':
            case 'h':
                match = _strptime_match(s, _months_abbr, 12);
                if (match < 0) return NULL;
                tm->tm_mon = match;
                s += strlen(_months_abbr[match]);
                break;
            case 'n':
            case 't': while (isspace(*s)) s++; break;
            case '%': if (*s != '%') return NULL; s++; break;
            default: return NULL;
        }
    }
    return (char *)s;
}

#endif // _WIN32