mod matchbook;
mod parser;
mod simulation;
mod strategy;
mod trades;
mod types;

use matchbook::{matchbook_init, matchbook_print};
use parser::parse_events;
use simulation::run_simulation;
use serde_json::json;
use std::fs::File;
use std::io::Write;

/// Dump raw data structure to JSON file
fn dump_raw(book: &types::MatchBook, filename: &str) {
    let mut root = Vec::new();

    for m in &book.matches {
        let labels = ["TEAM1", "DRAW", "TEAM2"];
        let mut markets_json = Vec::new();

        for (j, label) in labels.iter().enumerate() {
            let mkt = &m.markets[j];
            markets_json.push(json!({
                "outcome": label,
                "id": mkt.id,
                "condition_id": mkt.condition_id,
                "token_id": mkt.token_id,
                "bid": mkt.bid,
                "ask": mkt.ask,
                "last": mkt.last,
                "prob": mkt.prob,
                "liquidity": mkt.liquidity,
                "min_order_size": mkt.min_order_size,
                "tick_size": mkt.tick_size
            }));
        }

        root.push(json!({
            "event_id": m.event_id,
            "ticker": m.ticker,
            "neg_risk_market_id": m.neg_risk_market_id,
            "team1": m.team1,
            "team2": m.team2,
            "date": m.date,
            "kickoff_iso": m.kickoff_iso,
            "kickoff_ts": m.kickoff_ts,
            "total_liquidity": m.total_liquidity,
            "markets": markets_json
        }));
    }

    let json_str = serde_json::to_string_pretty(&root).expect("Failed to serialize JSON");
    let mut f = File::create(filename).expect("Failed to create file");
    writeln!(f, "{}", json_str).expect("Failed to write to file");
    println!("Raw data saved to {}", filename);
}

fn fetch(url: &str) -> Result<String, Box<dyn std::error::Error>> {
    let response = reqwest::blocking::get(url)?;
    let body = response.text()?;
    Ok(body)
}

fn main() {
    let url = "https://gamma-api.polymarket.com/events?series_id=10188&sportsMarketType=moneyline&active=true&closed=false";
    let json = match fetch(url) {
        Ok(data) => data,
        Err(e) => {
            println!("Fetch failed: {}", e);
            std::process::exit(1);
        }
    };

    let mut book = matchbook_init();
    if parse_events(&json, &mut book).is_err() {
        println!("Parse failed");
        std::process::exit(1);
    }

    matchbook_print(&book);
    dump_raw(&book, "matchbook_raw.json");

    // Demo: simulate trading
    println!("\n--- Trade Simulation ---\n");

    run_simulation(&mut book, 1000.0, 3, 100.0);
}
