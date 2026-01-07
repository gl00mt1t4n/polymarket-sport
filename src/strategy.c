#include "types.h"
#include <stdio.h>

/*
 * strategy.c - Decision logic and heuristics
 *
 * Pure math/decision helpers. No trade execution.
 */

/* Find the favored team (highest prob, ignoring draw) */
Outcome find_favored(const Match *m) {
    return (m->markets[TEAM1].prob >= m->markets[TEAM2].prob) ? TEAM1 : TEAM2;
}

/* Find the underdog (lowest prob, ignoring draw) */
Outcome find_underdog(const Match *m) {
    return (m->markets[TEAM1].prob < m->markets[TEAM2].prob) ? TEAM1 : TEAM2;
}

/* Get probability sum (should be ~1.0, overround if >1) */
double prob_sum(const Match *m) {
    return m->markets[TEAM1].prob + m->markets[DRAW].prob + m->markets[TEAM2].prob;
}

/* TODO: Hedging logic will go here */
/*
 * Future functions:
 * - calculate_exposure(portfolio, match) -> exposure per outcome
 * - find_hedge_size(portfolio, match, target_pnl) -> optimal hedge
 * - should_hedge(portfolio, match) -> bool
 * - rebalance(portfolio, match) -> execute hedge trades
 */
