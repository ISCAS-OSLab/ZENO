#include "tee.hpp"

RID kv_int_add(RID left, RID right)
{
    int l = getInt(left);
    int r = getInt(right);

    RID key = putInt(INVALID_RID, l + r);

    return key;
}

RID kv_int_sub(RID left, RID right)
{
    int l = getInt(left);
    int r = getInt(right);

    RID key = putInt(INVALID_RID, l - r);

    return key;
}

RID kv_int_mult(RID left, RID right)
{
    int l = getInt(left);
    int r = getInt(right);

    RID key = putInt(INVALID_RID, l * r);

    return key;
}

RID kv_int_div(RID left, RID right)
{
    int l = getInt(left);
    int r = getInt(right);

    RID key = putInt(INVALID_RID, l / r);

    return key;
}

RID kv_int_pow(RID left, RID right)
{
    int l = getInt(left);
    int r = getInt(right);

    RID key = putInt(INVALID_RID, pow(l, r));

    return key;
}

RID kv_int_mod(RID left, RID right)
{
    int l = getInt(left);
    int r = getInt(right);

    RID key = putInt(INVALID_RID, l % r);

    return key;
}

int kv_int_cmp(RID left, RID right)
{
    int l = getInt(left);
    int r = getInt(right);

    return l > r ? 1 : (l < r ? -1 : 0);
}

RID kv_int_sum_bulk(size_t bulk_size, RID *bulk_data)
{
    int sum = 0;

    for (size_t i = 0; i < bulk_size; i++)
    {
        sum += getInt(bulk_data[i]);
    }

    RID key = putInt(INVALID_RID, sum);

#ifdef USE_LOCAL_STORE_CLEAR
    for (size_t i = 0; i < bulk_size; i++)
        freeLocalInt(bulk_data[i]);
#endif

    return key;
}

////////////////////////////////////////////////////////////////////////////////

RID kv_float_add(RID left, RID right)
{
    float l = getFloat(left);
    float r = getFloat(right);

    RID key = putFloat(INVALID_RID, l + r);
    return key;
}

RID kv_float_sub(RID left, RID right)
{
    float l = getFloat(left);
    float r = getFloat(right);

    RID key = putFloat(INVALID_RID, l - r);

    return key;
}

RID kv_float_mult(RID left, RID right)
{
    float l = getFloat(left);
    float r = getFloat(right);
    RID key = putFloat(INVALID_RID, l * r);
    return key;
}

RID kv_float_div(RID left, RID right)
{
    float l = getFloat(left);
    float r = getFloat(right);

    RID key = putFloat(INVALID_RID, l / r);

    return key;
}

RID kv_float_pow(RID left, RID right)
{
    float l = getFloat(left);
    float r = getFloat(right);

    RID key = putFloat(INVALID_RID, pow(l, r));

    return key;
}

RID kv_float_mod(RID left, RID right)
{
    float l = getFloat(left);
    float r = getFloat(right);

    RID key = putFloat(INVALID_RID, fmod(l, r));

    return key;
}

int kv_float_cmp(RID left, RID right)
{
    float l = getFloat(left);
    float r = getFloat(right);

    return l > r ? 1 : (l < r ? -1 : 0);
}

RID kv_float_sum_bulk(size_t bulk_size, RID *bulk_data)
{
    double sum = 0;
    for (size_t i = 0; i < bulk_size; i++)
    {
        sum += getFloat(bulk_data[i]);
    }

    RID key = putFloat(INVALID_RID, (float)sum);

#ifdef USE_LOCAL_STORE_CLEAR
    for (size_t i = 0; i < bulk_size; i++)
        freeLocalFloat(bulk_data[i]);
#endif

    return key;
}

////////////////////////////////////////////////////////////////////////////////

#define TMODULO(t, q, u)        \
    do                          \
    {                           \
        (q) = ((t) / (u));      \
        if ((q) != 0)           \
            (t) -= ((q) * (u)); \
    } while (0)

#define INT64CONST(x) (x##L)
#define USECS_PER_DAY INT64CONST(86400000000)
#define POSTGRES_EPOCH_JDATE 2451545

static int timestamp_extract_year(int64_t timestamp)
{
    int64_t date;
    unsigned int quad;
    unsigned int extra;
    int year;

    TMODULO(timestamp, date, USECS_PER_DAY);
    if (timestamp < INT64CONST(0))
    {
        timestamp += USECS_PER_DAY;
        date -= 1;
    }

    date += POSTGRES_EPOCH_JDATE;

    if (date < 0 || date > (int64_t)INT_MAX)
        return -1;

    date += 32044;
    quad = date / 146097;
    extra = (date - quad * 146097) * 4 + 3;

    date += 60 + quad * 3 + extra / 146097;
    quad = date / 1461;
    date -= quad * 1461;

    year = date * 4 / 1461;
    year += quad * 4;
    return year - 4800;
}

RID kv_timestamp_extract_year(RID timestamp)
{
    TIMESTAMP t = getTimestamp(timestamp);

    int year = timestamp_extract_year(t);
    RID key = putInt(INVALID_RID, year);

    return key;
}

int kv_timestamp_cmp(RID left, RID right)
{
    TIMESTAMP l = getTimestamp(left);
    TIMESTAMP r = getTimestamp(right);

    return l > r ? 1 : (l < r ? -1 : 0);
}

////////////////////////////////////////////////////////////////////////////////

int kv_text_cmp(RID left, RID right)
{
    const char *l = getText(left);
    const char *r = getText(right);

    return strcmp(l, r);
}

RID kv_text_concat(RID left, RID right)
{
    const char *l = getText(left);
    const char *r = getText(right);

    char *res = (char *)malloc(strlen(l) + strlen(r) + 1);
    strcpy(res, l);
    strcat(res, r);

    RID key = putText(INVALID_RID, res);

    free(res);

    return key;
}

RID kv_text_substr(RID text, RID start, RID length)
{
    const char *t = getText(text);

    int s = getInt(start) - 1;
    int l = getInt(length);

    char *res = (char *)malloc(l + 1);
    strncpy(res, t + s, l);
    res[l] = '\0';

    RID key = putText(INVALID_RID, res);

    free(res);

    return key;
}

#define GETCHAR(t) (t)
#define LIKE_TRUE 1
#define LIKE_FALSE 0
#define LIKE_ABORT 0

#define CHAREQ(p1, p2) (*(p1) == *(p2))
#define NextByte(p, plen) ((p)++, (plen)--)
#define NextChar(p, plen) NextByte((p), (plen))
#define CopyAdvChar(dst, src, srclen) (*(dst)++ = *(src)++, (srclen)--)

static int MatchText(const char *t, int tlen, const char *p, int plen)
{
    /* Fast path for match-everything pattern */
    if (plen == 1 && *p == '%')
        return LIKE_TRUE;

    /* Since this function recurses, it could be driven to stack overflow */
    /*check_stack_depth();*/

    /*
     * In this loop, we advance by char when matching wildcards (and thus on
     * recursive entry to this function we are properly char-synced). On other
     * occasions it is safe to advance by byte, as the text and pattern will
     * be in lockstep. This allows us to perform all comparisons between the
     * text and pattern on a byte by byte basis, even for multi-byte
     * encodings.
     */
    while (tlen > 0 && plen > 0)
    {
        if (*p == '\\')
        {
            /* Next pattern byte must match literally, whatever it is */
            NextByte(p, plen);
            /* ... and there had better be one, per SQL standard */
            if (plen <= 0)
                return LIKE_ABORT;
            if (GETCHAR(*p) != GETCHAR(*t))
                return LIKE_FALSE;
        }
        else if (*p == '%')
        {
            char firstpat;

            /*
             * % processing is essentially a search for a text position at
             * which the remainder of the text matches the remainder of the
             * pattern, using a recursive call to check each potential match.
             *
             * If there are wildcards immediately following the %, we can skip
             * over them first, using the idea that any sequence of N _'s and
             * one or more %'s is equivalent to N _'s and one % (ie, it will
             * match any sequence of at least N text characters).  In this way
             * we will always run the recursive search loop using a pattern
             * fragment that begins with a literal character-to-match, thereby
             * not recursing more than we have to.
             */
            NextByte(p, plen);

            while (plen > 0)
            {
                if (*p == '%')
                    NextByte(p, plen);
                else if (*p == '_')
                {
                    /* If not enough text left to match the pattern, ABORT */
                    if (tlen <= 0)
                        return LIKE_ABORT;
                    NextChar(t, tlen);
                    NextByte(p, plen);
                }
                else
                    break; /* Reached a non-wildcard pattern char */
            }

            /*
             * If we're at end of pattern, match: we have a trailing % which
             * matches any remaining text string.
             */
            if (plen <= 0)
                return LIKE_TRUE;

            /*
             * Otherwise, scan for a text position at which we can match the
             * rest of the pattern.  The first remaining pattern char is known
             * to be a regular or escaped literal character, so we can compare
             * the first pattern byte to each text byte to avoid recursing
             * more than we have to.  This fact also guarantees that we don't
             * have to consider a match to the zero-length substring at the
             * end of the text.
             */
            if (*p == '\\')
            {
                if (plen < 2)
                    return LIKE_ABORT;
                firstpat = GETCHAR(p[1]);
            }
            else
                firstpat = GETCHAR(*p);

            while (tlen > 0)
            {
                if (GETCHAR(*t) == firstpat)
                {
                    int matched = MatchText(t, tlen, p, plen);

                    if (matched != LIKE_FALSE)
                        return matched; /* TRUE or ABORT */
                }

                NextChar(t, tlen);
            }

            /*
             * End of text with no match, so no point in trying later places
             * to start matching this pattern.
             */
            return LIKE_ABORT;
        }
        else if (*p == '_')
        {
            /* _ matches any single character, and we know there is one */
            NextChar(t, tlen);
            NextByte(p, plen);
            continue;
        }
        else if (GETCHAR(*p) != GETCHAR(*t))
        {
            /* non-wildcard pattern char fails to match text char */
            return LIKE_FALSE;
        }

        /*
         * Pattern and text match, so advance.
         *
         * It is safe to use NextByte instead of NextChar here, even for
         * multi-byte character sets, because we are not following immediately
         * after a wildcard character. If we are in the middle of a multibyte
         * character, we must already have matched at least one byte of the
         * character from both text and pattern; so we cannot get out-of-sync
         * on character boundaries.  And we know that no backend-legal
         * encoding allows ASCII characters such as '%' to appear as non-first
         * bytes of characters, so we won't mistakenly detect a new wildcard.
         */
        NextByte(t, tlen);
        NextByte(p, plen);
    }

    if (tlen > 0)
        return LIKE_FALSE; /* end of pattern, but not of text */

    /*
     * End of text, but perhaps not of pattern.  Match iff the remaining
     * pattern can match a zero-length string, ie, it's zero or more %'s.
     */
    while (plen > 0 && *p == '%')
        NextByte(p, plen);
    if (plen <= 0)
        return LIKE_TRUE;

    /*
     * End of text with no match, so no point in trying later places to start
     * matching this pattern.
     */
    return LIKE_ABORT;
}

int kv_text_like(RID text, RID pattern)
{
    const char *t = getText(text);
    const char *p = getText(pattern);

    return MatchText(t, strlen(t), p, strlen(p));
}

////////////////////////////////////////////////////////////////////////////////

#include <kv.hpp>

FileMapKVStoreOperator kv_ops;
LocalKVStoreOperator localkv_ops;

#ifdef USE_PG_WAL_FLUSH_HOOK
WALLogOperator wal_ops;
#endif
