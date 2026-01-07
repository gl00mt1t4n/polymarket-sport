#include "types.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

Match *matchbook_add(MatchBook *book, const Match *match);

static void copy_str(char *dst, size_t n, const char *src) {
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, n, "%s", src);
}

/* Parse "Team A vs. Team B" */
static void parse_teams(const char *title, char *t1, char *t2) {
    const char *sep = strstr(title, " vs. ");
    if (sep) {
        size_t len = (size_t)(sep - title);
        if (len > 63) len = 63;
        memcpy(t1, title, len);
        t1[len] = '\0';
        copy_str(t2, 64, sep + 5);
    } else {
        copy_str(t1, 64, title);
        t2[0] = '\0';
    }
}

/* Extract first value from "[\"0.535\", \"0.465\"]" */
static double parse_prob(const char *s) {
    if (!s) return 0.0;
    const char *p = strchr(s, '"');
    return p ? atof(p + 1) : 0.0;
}

/* Extract first token from "[\"123...\", \"456...\"]" */
static void parse_token(const char *s, char *out, size_t out_size) {
    out[0] = '\0';
    if (!s) return;
    const char *start = strchr(s, '"');
    if (!start) return;
    start++;
    const char *end = strchr(start, '"');
    if (!end) return;
    size_t len = (size_t)(end - start);
    if (len >= out_size) len = out_size - 1;
    memcpy(out, start, len);
    out[len] = '\0';
}

/* Parse ISO timestamp to unix timestamp */
static uint64_t parse_iso_ts(const char *iso) {
    if (!iso) return 0;
    struct tm tm = {0};
    /* Parse "2026-01-07T19:30:00Z" */
    if (sscanf(iso, "%d-%d-%dT%d:%d:%d",
               &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
               &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6) {
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        return (uint64_t)timegm(&tm);
    }
    return 0;
}

/* Parse market, returns groupItemThreshold (0/1/2) or -1 */
static int parse_market(cJSON *j, Market *m) {
    cJSON *item;
    memset(m, 0, sizeof(Market));

    item = cJSON_GetObjectItem(j, "id");
    if (item) copy_str(m->id, sizeof(m->id), item->valuestring);

    item = cJSON_GetObjectItem(j, "conditionId");
    if (item) copy_str(m->condition_id, sizeof(m->condition_id), item->valuestring);

    item = cJSON_GetObjectItem(j, "clobTokenIds");
    if (item) parse_token(item->valuestring, m->token_id, sizeof(m->token_id));

    item = cJSON_GetObjectItem(j, "bestBid");
    if (item) m->bid = item->valuedouble;

    item = cJSON_GetObjectItem(j, "bestAsk");
    if (item) m->ask = item->valuedouble;

    item = cJSON_GetObjectItem(j, "lastTradePrice");
    if (item) m->last = item->valuedouble;

    item = cJSON_GetObjectItem(j, "outcomePrices");
    if (item) m->prob = parse_prob(item->valuestring);

    item = cJSON_GetObjectItem(j, "liquidityNum");
    if (item) m->liquidity = item->valuedouble;

    item = cJSON_GetObjectItem(j, "orderMinSize");
    if (item) m->min_order_size = item->valuedouble;

    item = cJSON_GetObjectItem(j, "orderPriceMinTickSize");
    if (item) m->tick_size = item->valuedouble;

    item = cJSON_GetObjectItem(j, "groupItemThreshold");
    return item ? atoi(item->valuestring) : -1;
}

static void parse_event(cJSON *j, Match *m) {
    cJSON *item;
    memset(m, 0, sizeof(Match));

    item = cJSON_GetObjectItem(j, "id");
    if (item) copy_str(m->event_id, sizeof(m->event_id), item->valuestring);

    item = cJSON_GetObjectItem(j, "ticker");
    if (item) copy_str(m->ticker, sizeof(m->ticker), item->valuestring);

    item = cJSON_GetObjectItem(j, "negRiskMarketID");
    if (item) copy_str(m->neg_risk_market_id, sizeof(m->neg_risk_market_id), item->valuestring);

    item = cJSON_GetObjectItem(j, "title");
    if (item) parse_teams(item->valuestring, m->team1, m->team2);

    item = cJSON_GetObjectItem(j, "endDate");
    if (item && item->valuestring) {
        copy_str(m->kickoff_iso, sizeof(m->kickoff_iso), item->valuestring);
        /* Extract just date portion */
        size_t len = strlen(item->valuestring);
        if (len > 10) len = 10;
        memcpy(m->date, item->valuestring, len);
        m->date[len] = '\0';
        /* Parse to timestamp */
        m->kickoff_ts = parse_iso_ts(item->valuestring);
    }

    item = cJSON_GetObjectItem(j, "liquidity");
    if (item) m->total_liquidity = item->valuedouble;

    cJSON *markets = cJSON_GetObjectItem(j, "markets");
    if (markets && cJSON_IsArray(markets)) {
        int n = cJSON_GetArraySize(markets);
        for (int i = 0; i < n && i < 3; i++) {
            Market mkt;
            int slot = parse_market(cJSON_GetArrayItem(markets, i), &mkt);
            if (slot >= 0 && slot < 3) {
                m->markets[slot] = mkt;
            }
        }
    }
}

/* Check if match has all 3 valid outcomes */
static int is_valid_moneyline(const Match *m) {
    return m->markets[TEAM1].bid > 0 &&
           m->markets[DRAW].bid > 0 &&
           m->markets[TEAM2].bid > 0;
}

int parse_events(const char *json_str, MatchBook *book) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root || !cJSON_IsArray(root)) {
        if (root) cJSON_Delete(root);
        return -1;
    }

    int n = cJSON_GetArraySize(root);
    for (int i = 0; i < n; i++) {
        Match m;
        parse_event(cJSON_GetArrayItem(root, i), &m);
        if (is_valid_moneyline(&m)) {
            matchbook_add(book, &m);
        }
    }

    cJSON_Delete(root);
    return (int)book->count;
}
