use crate::types::{Match, MatchBook, MatchPosition, Outcome, Portfolio, Position, MAX_MATCHES};

pub fn portfolio_init(starting_cash: f64) -> Portfolio {
    println!("[PORTFOLIO] init: ${:.2}", starting_cash);
    Portfolio {
        positions: Vec::new(),
        cash: starting_cash,
    }
}

fn get_or_create_match_pos<'a>(p: &'a mut Portfolio, event_id: &str) -> Option<&'a mut MatchPosition> {
    // Check if position already exists
    let exists = p.positions.iter().any(|mp| mp.event_id == event_id);

    if exists {
        // Find and return the existing position
        p.positions.iter_mut().find(|mp| mp.event_id == event_id)
    } else {
        // Create new position
        if p.positions.len() >= MAX_MATCHES {
            eprintln!("[ERR] portfolio full, max {} positions", MAX_MATCHES);
            return None;
        }
        let mp = MatchPosition {
            event_id: event_id.to_string(),
            pos: Default::default(),
        };
        p.positions.push(mp);
        p.positions.last_mut()
    }
}

pub fn buy(p: &mut Portfolio, m: &Match, outcome: Outcome, dollars: f64) -> Result<(), String> {
    if dollars <= 0.0 {
        return Err(format!(
            "[ERR] buy: amount must be positive (got ${:.2})",
            dollars
        ));
    }
    if dollars > p.cash {
        return Err(format!(
            "[ERR] buy: need ${:.2}, have ${:.2}",
            dollars, p.cash
        ));
    }
    let ask = m.markets[outcome.as_index()].ask;
    if ask <= 0.0 {
        return Err(format!(
            "[ERR] buy: no ask for {} {}",
            m.event_id,
            outcome.name()
        ));
    }

    let shares = dollars / ask;
    let mp = get_or_create_match_pos(p, &m.event_id).ok_or("[ERR] buy: portfolio full")?;

    mp.pos[outcome.as_index()].shares += shares;
    mp.pos[outcome.as_index()].cost_basis += dollars;
    p.cash -= dollars;

    println!(
        "[BUY] {} {}: ${:.2} @ {:.3} -> {:.2} shares",
        m.event_id,
        outcome.name(),
        dollars,
        ask,
        shares
    );
    Ok(())
}

pub fn sell(
    p: &mut Portfolio,
    m: &Match,
    outcome: Outcome,
    shares_to_sell: f64,
) -> Result<(), String> {
    if shares_to_sell <= 0.0 {
        return Err("[ERR] sell: shares must be positive".to_string());
    }

    let mp = get_or_create_match_pos(p, &m.event_id).ok_or("[ERR] sell: portfolio full")?;

    let pos = &mp.pos[outcome.as_index()];
    if shares_to_sell > pos.shares {
        return Err(format!(
            "[ERR] sell: have {:.2} shares, tried {:.2}",
            pos.shares, shares_to_sell
        ));
    }
    let bid = m.markets[outcome.as_index()].bid;
    if bid <= 0.0 {
        return Err(format!(
            "[ERR] sell: no bid for {} {}",
            m.event_id,
            outcome.name()
        ));
    }

    let proceeds = shares_to_sell * bid;
    let fraction = shares_to_sell / pos.shares;
    let cost_sold = pos.cost_basis * fraction;

    let pos = &mut mp.pos[outcome.as_index()];
    pos.shares -= shares_to_sell;
    pos.cost_basis -= cost_sold;
    p.cash += proceeds;

    println!(
        "[SELL] {} {}: {:.2} shares @ {:.3} -> ${:.2}",
        m.event_id,
        outcome.name(),
        shares_to_sell,
        bid,
        proceeds
    );
    Ok(())
}

pub fn sell_all(p: &mut Portfolio, m: &Match, outcome: Outcome) -> Result<(), String> {
    let shares = {
        let mp = p.positions.iter().find(|mp| mp.event_id == m.event_id);
        match mp {
            Some(mp) if mp.pos[outcome.as_index()].shares > 0.0 => {
                mp.pos[outcome.as_index()].shares
            }
            _ => {
                return Err(format!(
                    "[ERR] sell_all: no position in {} {}",
                    m.event_id,
                    outcome.name()
                ));
            }
        }
    };
    sell(p, m, outcome, shares)
}

pub fn get_position<'a>(
    p: &'a Portfolio,
    event_id: &str,
    outcome: Outcome,
) -> Option<&'a Position> {
    p.positions
        .iter()
        .find(|mp| mp.event_id == event_id)
        .map(|mp| &mp.pos[outcome.as_index()])
}

pub fn position_value(pos: &Position, bid: f64) -> f64 {
    pos.shares * bid
}

pub fn position_pnl(pos: &Position, bid: f64) -> f64 {
    position_value(pos, bid) - pos.cost_basis
}

pub fn portfolio_print(p: &Portfolio, book: &MatchBook) {
    println!("\n=== Portfolio ===");
    println!("Cash: ${:.2}\n", p.cash);

    let mut total_value = p.cash;
    let mut total_cost = 0.0;

    for mp in &p.positions {
        let m = book.matches.iter().find(|m| m.event_id == mp.event_id);
        let m = match m {
            Some(m) => m,
            None => {
                eprintln!("[WARN] event {} not in book", mp.event_id);
                continue;
            }
        };

        let has_pos = mp.pos.iter().any(|p| p.shares > 0.0);
        if !has_pos {
            continue;
        }

        println!("{} vs {} ({})", m.team1, m.team2, m.date);
        let labels = [m.team1.as_str(), "Draw", m.team2.as_str()];
        for o in 0..3 {
            let pos = &mp.pos[o];
            if pos.shares > 0.0 {
                let val = position_value(pos, m.markets[o].bid);
                let pnl = position_pnl(pos, m.markets[o].bid);
                println!(
                    "  {:<25} {:.2} sh @ ${:.2} | Val: ${:.2} | PnL: {:+.2}",
                    labels[o], pos.shares, pos.cost_basis, val, pnl
                );
                total_value += val;
                total_cost += pos.cost_basis;
            }
        }
        println!();
    }

    println!(
        "Position Value: ${:.2} | Cost: ${:.2} | PnL: {:+.2} | Total: ${:.2}",
        total_value - p.cash,
        total_cost,
        (total_value - p.cash) - total_cost,
        total_value
    );
}
