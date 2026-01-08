use crate::types::{Match, MatchBook, Outcome, MAX_MATCHES};

pub fn matchbook_init() -> MatchBook {
    MatchBook {
        matches: Vec::new(),
    }
}

pub fn matchbook_add(book: &mut MatchBook, m: Match) -> Option<&Match> {
    if book.matches.len() >= MAX_MATCHES {
        return None;
    }
    book.matches.push(m);
    book.matches.last()
}

pub fn matchbook_print(book: &MatchBook) {
    println!("=== {} Matches ===\n", book.matches.len());
    for m in &book.matches {
        println!("{} vs {} ({})", m.team1, m.team2, m.date);
        println!(
            "\t {:<25}  {:.2} / {:.2}  ({:.0}%)",
            m.team1,
            m.markets[Outcome::Team1.as_index()].bid,
            m.markets[Outcome::Team1.as_index()].ask,
            m.markets[Outcome::Team1.as_index()].prob * 100.0
        );
        println!(
            "\t {:<25}  {:.2} / {:.2}  ({:.0}%)",
            "Draw",
            m.markets[Outcome::Draw.as_index()].bid,
            m.markets[Outcome::Draw.as_index()].ask,
            m.markets[Outcome::Draw.as_index()].prob * 100.0
        );
        println!(
            "\t {:<25}  {:.2} / {:.2}  ({:.0}%)",
            m.team2,
            m.markets[Outcome::Team2.as_index()].bid,
            m.markets[Outcome::Team2.as_index()].ask,
            m.markets[Outcome::Team2.as_index()].prob * 100.0
        );
        println!();
    }
}
