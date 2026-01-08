use crate::matchbook::matchbook_add;
use crate::types::{Market, Match, MatchBook, Outcome};
use chrono::{DateTime, Utc};
use serde_json::Value;

/// Parse "Team A vs. Team B" into (team1, team2)
fn parse_teams(title: &str) -> (String, String) {
    if let Some(pos) = title.find(" vs. ") {
        let t1 = title[..pos].to_string();
        let t2 = title[pos + 5..].to_string();
        (t1, t2)
    } else {
        (title.to_string(), String::new())
    }
}

/// Extract first value from "[\"0.535\", \"0.465\"]"
fn parse_prob(s: &str) -> f64 {
    if let Some(start) = s.find('"') {
        let rest = &s[start + 1..];
        if let Some(end) = rest.find('"') {
            return rest[..end].parse().unwrap_or(0.0);
        }
    }
    0.0
}

/// Extract first token from "[\"123...\", \"456...\"]"
fn parse_token(s: &str) -> String {
    if let Some(start) = s.find('"') {
        let rest = &s[start + 1..];
        if let Some(end) = rest.find('"') {
            return rest[..end].to_string();
        }
    }
    String::new()
}

/// Parse ISO timestamp to unix timestamp
fn parse_iso_ts(iso: &str) -> u64 {
    if let Ok(dt) = DateTime::parse_from_rfc3339(iso) {
        dt.timestamp() as u64
    } else {
        // Try parsing with manual format "2026-01-07T19:30:00Z"
        if let Ok(dt) = iso.parse::<DateTime<Utc>>() {
            dt.timestamp() as u64
        } else {
            0
        }
    }
}

/// Parse market, returns groupItemThreshold (0/1/2) or None
fn parse_market(j: &Value) -> (Market, Option<i32>) {
    let mut m = Market::default();

    if let Some(id) = j.get("id").and_then(|v| v.as_str()) {
        m.id = id.to_string();
    }

    if let Some(cid) = j.get("conditionId").and_then(|v| v.as_str()) {
        m.condition_id = cid.to_string();
    }

    if let Some(tokens) = j.get("clobTokenIds").and_then(|v| v.as_str()) {
        m.token_id = parse_token(tokens);
    }

    if let Some(bid) = j.get("bestBid").and_then(|v| v.as_f64()) {
        m.bid = bid;
    }

    if let Some(ask) = j.get("bestAsk").and_then(|v| v.as_f64()) {
        m.ask = ask;
    }

    if let Some(last) = j.get("lastTradePrice").and_then(|v| v.as_f64()) {
        m.last = last;
    }

    if let Some(prices) = j.get("outcomePrices").and_then(|v| v.as_str()) {
        m.prob = parse_prob(prices);
    }

    if let Some(liq) = j.get("liquidityNum").and_then(|v| v.as_f64()) {
        m.liquidity = liq;
    }

    if let Some(min_size) = j.get("orderMinSize").and_then(|v| v.as_f64()) {
        m.min_order_size = min_size;
    }

    if let Some(tick) = j.get("orderPriceMinTickSize").and_then(|v| v.as_f64()) {
        m.tick_size = tick;
    }

    let slot = j
        .get("groupItemThreshold")
        .and_then(|v| v.as_str())
        .and_then(|s| s.parse::<i32>().ok());

    (m, slot)
}

fn parse_event(j: &Value) -> Match {
    let mut m = Match::default();

    if let Some(id) = j.get("id").and_then(|v| v.as_str()) {
        m.event_id = id.to_string();
    }

    if let Some(ticker) = j.get("ticker").and_then(|v| v.as_str()) {
        m.ticker = ticker.to_string();
    }

    if let Some(nrm) = j.get("negRiskMarketID").and_then(|v| v.as_str()) {
        m.neg_risk_market_id = nrm.to_string();
    }

    if let Some(title) = j.get("title").and_then(|v| v.as_str()) {
        let (t1, t2) = parse_teams(title);
        m.team1 = t1;
        m.team2 = t2;
    }

    if let Some(end_date) = j.get("endDate").and_then(|v| v.as_str()) {
        m.kickoff_iso = end_date.to_string();
        // Extract just date portion
        let len = std::cmp::min(10, end_date.len());
        m.date = end_date[..len].to_string();
        // Parse to timestamp
        m.kickoff_ts = parse_iso_ts(end_date);
    }

    if let Some(liq) = j.get("liquidity").and_then(|v| v.as_f64()) {
        m.total_liquidity = liq;
    }

    if let Some(markets) = j.get("markets").and_then(|v| v.as_array()) {
        for market_json in markets.iter().take(3) {
            let (mkt, slot) = parse_market(market_json);
            if let Some(s) = slot {
                if s >= 0 && s < 3 {
                    m.markets[s as usize] = mkt;
                }
            }
        }
    }

    m
}

/// Check if match has all 3 valid outcomes
fn is_valid_moneyline(m: &Match) -> bool {
    m.markets[Outcome::Team1.as_index()].bid > 0.0
        && m.markets[Outcome::Draw.as_index()].bid > 0.0
        && m.markets[Outcome::Team2.as_index()].bid > 0.0
}

pub fn parse_events(json_str: &str, book: &mut MatchBook) -> Result<usize, &'static str> {
    let root: Value = serde_json::from_str(json_str).map_err(|_| "Failed to parse JSON")?;

    let arr = root.as_array().ok_or("Expected JSON array")?;

    for event_json in arr {
        let m = parse_event(event_json);
        if is_valid_moneyline(&m) {
            matchbook_add(book, m);
        }
    }

    Ok(book.matches.len())
}
