#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"

void matchbook_init(MatchBook *book);
void matchbook_print(const MatchBook *book);
int parse_events(const char *json_str, MatchBook *book);

/* Dump raw data structure to JSON file */
static void dump_raw(const MatchBook *book, const char *filename) {
    cJSON *root = cJSON_CreateArray();
    for (uint32_t i = 0; i < book->count; i++) {
        const Match *m = &book->matches[i];
        cJSON *match = cJSON_CreateObject();

        /* Match-level identifiers */
        cJSON_AddStringToObject(match, "event_id", m->event_id);
        cJSON_AddStringToObject(match, "ticker", m->ticker);
        cJSON_AddStringToObject(match, "neg_risk_market_id", m->neg_risk_market_id);

        /* Teams & timing */
        cJSON_AddStringToObject(match, "team1", m->team1);
        cJSON_AddStringToObject(match, "team2", m->team2);
        cJSON_AddStringToObject(match, "date", m->date);
        cJSON_AddStringToObject(match, "kickoff_iso", m->kickoff_iso);
        cJSON_AddNumberToObject(match, "kickoff_ts", (double)m->kickoff_ts);
        cJSON_AddNumberToObject(match, "total_liquidity", m->total_liquidity);

        /* Markets */
        cJSON *markets = cJSON_CreateArray();
        const char *labels[] = {"TEAM1", "DRAW", "TEAM2"};
        for (int j = 0; j < 3; j++) {
            const Market *mkt = &m->markets[j];
            cJSON *o = cJSON_CreateObject();
            cJSON_AddStringToObject(o, "outcome", labels[j]);
            cJSON_AddStringToObject(o, "id", mkt->id);
            cJSON_AddStringToObject(o, "condition_id", mkt->condition_id);
            cJSON_AddStringToObject(o, "token_id", mkt->token_id);
            cJSON_AddNumberToObject(o, "bid", mkt->bid);
            cJSON_AddNumberToObject(o, "ask", mkt->ask);
            cJSON_AddNumberToObject(o, "last", mkt->last);
            cJSON_AddNumberToObject(o, "prob", mkt->prob);
            cJSON_AddNumberToObject(o, "liquidity", mkt->liquidity);
            cJSON_AddNumberToObject(o, "min_order_size", mkt->min_order_size);
            cJSON_AddNumberToObject(o, "tick_size", mkt->tick_size);
            cJSON_AddItemToArray(markets, o);
        }
        cJSON_AddItemToObject(match, "markets", markets);
        cJSON_AddItemToArray(root, match);
    }

    char *str = cJSON_Print(root);
    FILE *f = fopen(filename, "w");
    fprintf(f, "%s\n", str);
    fclose(f);
    printf("Raw data saved to %s\n", filename);

    free(str);
    cJSON_Delete(root);
}

typedef struct { char *data; size_t size; } Buffer;

static size_t write_cb(void *ptr, size_t size, size_t n, void *userp) {
    Buffer *buf = userp;
    size_t len = size * n;
    char *tmp = realloc(buf->data, buf->size + len + 1);
    if (!tmp) return 0;
    buf->data = tmp;
    memcpy(buf->data + buf->size, ptr, len);
    buf->size += len;
    buf->data[buf->size] = '\0';
    return len;
}

static char *fetch(const char *url) {
    Buffer buf = {0};
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return buf.data;
}

int main(void) {
    char *json = fetch("https://gamma-api.polymarket.com/events?series_id=10188&sportsMarketType=moneyline&active=true&closed=false");
    if (!json) { printf("Fetch failed\n"); return 1; }

    MatchBook book;
    matchbook_init(&book);
    if (parse_events(json, &book) < 0) { printf("Parse failed\n"); free(json); return 1; }

    matchbook_print(&book);
    dump_raw(&book, "matchbook_raw.json");
    free(json);
    return 0;
}
