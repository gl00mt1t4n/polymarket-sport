pub const MAX_MATCHES: usize = 1024;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Outcome {
    Team1 = 0,
    Draw = 1,
    Team2 = 2,
}

impl Outcome {
    pub fn name(&self) -> &'static str {
        match self {
            Outcome::Team1 => "TEAM1",
            Outcome::Draw => "DRAW",
            Outcome::Team2 => "TEAM2",
        }
    }

    pub fn as_index(&self) -> usize {
        *self as usize
    }
}

/// Market - Single outcome (Team1 wins / Draw / Team2 wins)
#[derive(Debug, Clone, Default)]
pub struct Market {
    // Identifiers
    pub id: String,
    pub condition_id: String,
    pub token_id: String,

    // Pricing
    pub bid: f64,
    pub ask: f64,
    pub last: f64,
    pub prob: f64,

    // Sizing
    pub liquidity: f64,
    pub min_order_size: f64,
    pub tick_size: f64,
}

/// Match - One game with 3 outcomes
#[derive(Debug, Clone, Default)]
pub struct Match {
    // Identifiers
    pub event_id: String,
    pub ticker: String,
    pub neg_risk_market_id: String,

    // Teams & timing
    pub team1: String,
    pub team2: String,
    pub date: String,
    pub kickoff_iso: String,
    pub kickoff_ts: u64,

    // The 3 outcome markets [TEAM1, DRAW, TEAM2]
    pub markets: [Market; 3],

    // Aggregate
    pub total_liquidity: f64,
}

/// MatchBook - All matches
#[derive(Debug, Clone, Default)]
pub struct MatchBook {
    pub matches: Vec<Match>,
}

/// Position - Holdings for one outcome
#[derive(Debug, Clone, Default)]
pub struct Position {
    pub shares: f64,
    pub cost_basis: f64,
}

/// MatchPosition - All positions for one match
#[derive(Debug, Clone, Default)]
pub struct MatchPosition {
    pub event_id: String,
    pub pos: [Position; 3],
}

/// Portfolio - All positions across all matches
#[derive(Debug, Clone, Default)]
pub struct Portfolio {
    pub positions: Vec<MatchPosition>,
    pub cash: f64,
}
