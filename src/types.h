#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define MAX_MATCHES 1024

typedef enum { TEAM1 = 0, DRAW = 1, TEAM2 = 2 } Outcome;

/* Market - Single outcome (Team1 wins / Draw / Team2 wins) */
typedef struct {
    /* Identifiers */
    char id[16];                  /* Market ID for API calls */
    char condition_id[68];        /* On-chain condition ID */
    char token_id[96];            /* CLOB token for order execution */

    /* Pricing */
    double bid;
    double ask;
    double last;                  /* Last trade price */
    double prob;                  /* Implied probability */

    /* Sizing */
    double liquidity;
    double min_order_size;        /* Minimum order size */
    double tick_size;             /* Price tick size (usually 0.01) */
} Market;

/* Match - One game with 3 outcomes */
typedef struct {
    /* Identifiers */
    char event_id[16];            /* Event ID for API calls */
    char ticker[64];              /* e.g. "epl-eve-wol-2026-01-07" */
    char neg_risk_market_id[68];  /* Shared across all 3 markets */

    /* Teams & timing */
    char team1[64];
    char team2[64];
    char date[16];                /* "2026-01-07" */
    char kickoff_iso[32];         /* Full ISO timestamp */
    uint64_t kickoff_ts;          /* Unix timestamp (seconds) */

    /* The 3 outcome markets */
    Market markets[3];            /* [TEAM1, DRAW, TEAM2] */

    /* Aggregate */
    double total_liquidity;
} Match;

/* MatchBook - All matches */
typedef struct {
    Match matches[MAX_MATCHES];
    uint32_t count;
} MatchBook;

/* Position - Holdings for one outcome */
typedef struct {
    double shares;      /* Number of shares held */
    double cost_basis;  /* Total $ spent to acquire */
} Position;

/* MatchPosition - All positions for one match */
typedef struct {
    char event_id[16];        /* Links to Match */
    Position pos[3];          /* [TEAM1, DRAW, TEAM2] */
} MatchPosition;

/* Portfolio - All positions across all matches */
typedef struct {
    MatchPosition positions[MAX_MATCHES];
    uint32_t count;
    double cash;              /* Available cash balance */
} Portfolio;

#endif
