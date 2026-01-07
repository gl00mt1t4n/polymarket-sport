#include "types.h"
#include <stdio.h>

/*
 * simulation.c - Orchestration layer
 *
 * Initializes portfolio, runs simulation, handles live updates.
 * Calls strategy.c for decisions, which calls trades.c for execution.
 */

/* From trades.c */
void portfolio_init(Portfolio *p, double starting_cash);
void portfolio_print(const Portfolio *p, const MatchBook *book);
int buy(Portfolio *p, const Match *m, Outcome outcome, double dollars);

/* From strategy.c */
Outcome find_favored(const Match *m);

static int buy_favored(Portfolio *p, const Match *m, double dollars) {
    Outcome fav = find_favored(m);
    printf("[STRATEGY] %s vs %s: favored=%s (%.0f%%)\n",
           m->team1, m->team2,
           (fav == TEAM1) ? m->team1 : m->team2,
           m->markets[fav].prob * 100);
    return buy(p, m, fav, dollars);
}

/* Run a simple simulation: buy favored team for N matches */
void run_simulation(MatchBook *book, double starting_cash, int num_matches, double bet_size) {
    printf("\n=== Starting Simulation ===\n");
    printf("Cash: $%.2f | Matches: %d | Bet: $%.2f each\n\n",
           starting_cash, num_matches, bet_size);

    Portfolio portfolio;
    portfolio_init(&portfolio, starting_cash);

    int count = 0;
    for (uint32_t i = 0; i < book->count && count < num_matches; i++) {
        Match *m = &book->matches[i];
        if (buy_favored(&portfolio, m, bet_size) == 0) {
            count++;
        }
    }

    printf("\n=== Simulation Complete ===\n");
    printf("Placed %d bets\n", count);

    portfolio_print(&portfolio, book);
}

/* TODO: Live simulation loop */
/*
 * Future:
 * - run_live(book, portfolio) - continuous loop
 * - on_price_update(book, market_id, new_quote) - handle price changes
 * - check_hedges(portfolio, book) - evaluate and execute hedges
 */
