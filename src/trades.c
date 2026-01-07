#include "types.h"
#include <stdio.h>
#include <string.h>

/*
 * trades.c - Core trade execution primitives
 *
 * Pure mechanics: buy, sell, position tracking.
 * No strategy logic - just executes trades in memory.
 */

static const char *outcome_name(Outcome o) {
    switch (o) {
        case TEAM1: return "TEAM1";
        case DRAW:  return "DRAW";
        case TEAM2: return "TEAM2";
    }
    return "?";
}

void portfolio_init(Portfolio *p, double starting_cash) {
    memset(p, 0, sizeof(Portfolio));
    p->cash = starting_cash;
    printf("[PORTFOLIO] init: $%.2f\n", starting_cash);
}

static MatchPosition *get_or_create_match_pos(Portfolio *p, const char *event_id) {
    for (uint32_t i = 0; i < p->count; i++) {
        if (strcmp(p->positions[i].event_id, event_id) == 0) {
            return &p->positions[i];
        }
    }
    if (p->count >= MAX_MATCHES) {
        fprintf(stderr, "[ERR] portfolio full, max %d positions\n", MAX_MATCHES);
        return NULL;
    }
    MatchPosition *mp = &p->positions[p->count++];
    memset(mp, 0, sizeof(MatchPosition));
    snprintf(mp->event_id, sizeof(mp->event_id), "%s", event_id);
    return mp;
}

int buy(Portfolio *p, const Match *m, Outcome outcome, double dollars) {
    if (dollars <= 0) {
        fprintf(stderr, "[ERR] buy: amount must be positive (got $%.2f)\n", dollars);
        return -1;
    }
    if (dollars > p->cash) {
        fprintf(stderr, "[ERR] buy: need $%.2f, have $%.2f\n", dollars, p->cash);
        return -1;
    }
    double ask = m->markets[outcome].ask;
    if (ask <= 0) {
        fprintf(stderr, "[ERR] buy: no ask for %s %s\n", m->event_id, outcome_name(outcome));
        return -1;
    }

    double shares = dollars / ask;
    MatchPosition *mp = get_or_create_match_pos(p, m->event_id);
    if (!mp) return -1;

    mp->pos[outcome].shares += shares;
    mp->pos[outcome].cost_basis += dollars;
    p->cash -= dollars;

    printf("[BUY] %s %s: $%.2f @ %.3f -> %.2f shares\n",
           m->event_id, outcome_name(outcome), dollars, ask, shares);
    return 0;
}

int sell(Portfolio *p, const Match *m, Outcome outcome, double shares_to_sell) {
    if (shares_to_sell <= 0) {
        fprintf(stderr, "[ERR] sell: shares must be positive\n");
        return -1;
    }
    MatchPosition *mp = get_or_create_match_pos(p, m->event_id);
    if (!mp) return -1;

    Position *pos = &mp->pos[outcome];
    if (shares_to_sell > pos->shares) {
        fprintf(stderr, "[ERR] sell: have %.2f shares, tried %.2f\n", pos->shares, shares_to_sell);
        return -1;
    }
    double bid = m->markets[outcome].bid;
    if (bid <= 0) {
        fprintf(stderr, "[ERR] sell: no bid for %s %s\n", m->event_id, outcome_name(outcome));
        return -1;
    }

    double proceeds = shares_to_sell * bid;
    double fraction = shares_to_sell / pos->shares;
    double cost_sold = pos->cost_basis * fraction;

    pos->shares -= shares_to_sell;
    pos->cost_basis -= cost_sold;
    p->cash += proceeds;

    printf("[SELL] %s %s: %.2f shares @ %.3f -> $%.2f\n",
           m->event_id, outcome_name(outcome), shares_to_sell, bid, proceeds);
    return 0;
}

int sell_all(Portfolio *p, const Match *m, Outcome outcome) {
    MatchPosition *mp = get_or_create_match_pos(p, m->event_id);
    if (!mp || mp->pos[outcome].shares <= 0) {
        fprintf(stderr, "[ERR] sell_all: no position in %s %s\n", m->event_id, outcome_name(outcome));
        return -1;
    }
    return sell(p, m, outcome, mp->pos[outcome].shares);
}

Position *get_position(Portfolio *p, const char *event_id, Outcome outcome) {
    for (uint32_t i = 0; i < p->count; i++) {
        if (strcmp(p->positions[i].event_id, event_id) == 0) {
            return &p->positions[i].pos[outcome];
        }
    }
    return NULL;
}

double position_value(const Position *pos, double bid) {
    return pos->shares * bid;
}

double position_pnl(const Position *pos, double bid) {
    return position_value(pos, bid) - pos->cost_basis;
}

void portfolio_print(const Portfolio *p, const MatchBook *book) {
    printf("\n=== Portfolio ===\n");
    printf("Cash: $%.2f\n\n", p->cash);

    double total_value = p->cash;
    double total_cost = 0;

    for (uint32_t i = 0; i < p->count; i++) {
        const MatchPosition *mp = &p->positions[i];
        const Match *m = NULL;
        for (uint32_t j = 0; j < book->count; j++) {
            if (strcmp(book->matches[j].event_id, mp->event_id) == 0) {
                m = &book->matches[j];
                break;
            }
        }
        if (!m) {
            fprintf(stderr, "[WARN] event %s not in book\n", mp->event_id);
            continue;
        }

        int has_pos = 0;
        for (int o = 0; o < 3; o++) if (mp->pos[o].shares > 0) has_pos = 1;
        if (!has_pos) continue;

        printf("%s vs %s (%s)\n", m->team1, m->team2, m->date);
        const char *labels[] = {m->team1, "Draw", m->team2};
        for (int o = 0; o < 3; o++) {
            const Position *pos = &mp->pos[o];
            if (pos->shares > 0) {
                double val = position_value(pos, m->markets[o].bid);
                double pnl = position_pnl(pos, m->markets[o].bid);
                printf("  %-25s %.2f sh @ $%.2f | Val: $%.2f | PnL: %+.2f\n",
                       labels[o], pos->shares, pos->cost_basis, val, pnl);
                total_value += val;
                total_cost += pos->cost_basis;
            }
        }
        printf("\n");
    }

    printf("Position Value: $%.2f | Cost: $%.2f | PnL: %+.2f | Total: $%.2f\n",
           total_value - p->cash, total_cost, (total_value - p->cash) - total_cost, total_value);
}
