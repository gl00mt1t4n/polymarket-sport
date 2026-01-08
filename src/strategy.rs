use crate::types::{Match, Outcome};

/// Find the favored team (highest prob, ignoring draw)
pub fn find_favored(m: &Match) -> Outcome {
    if m.markets[Outcome::Team1.as_index()].prob >= m.markets[Outcome::Team2.as_index()].prob {
        Outcome::Team1
    } else {
        Outcome::Team2
    }
}

/// Find the underdog (lowest prob, ignoring draw)
pub fn find_underdog(m: &Match) -> Outcome {
    if m.markets[Outcome::Team1.as_index()].prob < m.markets[Outcome::Team2.as_index()].prob {
        Outcome::Team1
    } else {
        Outcome::Team2
    }
}

/// Get probability sum (should be ~1.0, overround if >1)
pub fn prob_sum(m: &Match) -> f64 {
    m.markets[Outcome::Team1.as_index()].prob
        + m.markets[Outcome::Draw.as_index()].prob
        + m.markets[Outcome::Team2.as_index()].prob
}

// TODO: Hedging logic will go here
//
// Future functions:
// - calculate_exposure(portfolio, match) -> exposure per outcome
// - find_hedge_size(portfolio, match, target_pnl) -> optimal hedge
// - should_hedge(portfolio, match) -> bool
// - rebalance(portfolio, match) -> execute hedge trades
