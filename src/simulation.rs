use crate::strategy::find_favored;
use crate::trades::{buy, portfolio_init, portfolio_print};
use crate::types::{Match, MatchBook, Outcome};

fn buy_favored(
    p: &mut crate::types::Portfolio,
    m: &Match,
    dollars: f64,
) -> Result<(), String> {
    let fav = find_favored(m);
    let team_name = if fav == Outcome::Team1 {
        &m.team1
    } else {
        &m.team2
    };
    println!(
        "[STRATEGY] {} vs {}: favored={} ({:.0}%)",
        m.team1,
        m.team2,
        team_name,
        m.markets[fav.as_index()].prob * 100.0
    );
    buy(p, m, fav, dollars)
}

/// Run a simple simulation: buy favored team for N matches
pub fn run_simulation(
    book: &mut MatchBook,
    starting_cash: f64,
    num_matches: i32,
    bet_size: f64,
) {
    println!("\n=== Starting Simulation ===");
    println!(
        "Cash: ${:.2} | Matches: {} | Bet: ${:.2} each\n",
        starting_cash, num_matches, bet_size
    );

    let mut portfolio = portfolio_init(starting_cash);

    let mut count = 0;
    for m in &book.matches {
        if count >= num_matches {
            break;
        }
        if buy_favored(&mut portfolio, m, bet_size).is_ok() {
            count += 1;
        }
    }

    println!("\n=== Simulation Complete ===");
    println!("Placed {} bets", count);

    portfolio_print(&portfolio, book);
}

// TODO: Live simulation loop
//
// Future:
// - run_live(book, portfolio) - continuous loop
// - on_price_update(book, market_id, new_quote) - handle price changes
// - check_hedges(portfolio, book) - evaluate and execute hedges
