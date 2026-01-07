#include "types.h"
#include <string.h>
#include <stdio.h>

void matchbook_init(MatchBook *book) {
    book->count = 0;
}

Match *matchbook_add(MatchBook *book, const Match *match) {
    if (book->count >= MAX_MATCHES) return NULL;
    book->matches[book->count] = *match;
    return &book->matches[book->count++];
}

void matchbook_print(const MatchBook *book) {
    printf("=== %u Matches ===\n\n", book->count);
    for (uint32_t i = 0; i < book->count; i++) {
        const Match *m = &book->matches[i];
        printf("%s vs %s (%s)\n", m->team1, m->team2, m->date);
        printf("\t %-25s  %.2f / %.2f  (%.0f%%)\n", m->team1, m->markets[TEAM1].bid, m->markets[TEAM1].ask, m->markets[TEAM1].prob * 100);
        printf("\t %-25s  %.2f / %.2f  (%.0f%%)\n", "Draw", m->markets[DRAW].bid, m->markets[DRAW].ask, m->markets[DRAW].prob * 100);
        printf("\t %-25s  %.2f / %.2f  (%.0f%%)\n", m->team2, m->markets[TEAM2].bid, m->markets[TEAM2].ask, m->markets[TEAM2].prob * 100);
        printf("\n");
    }
}
