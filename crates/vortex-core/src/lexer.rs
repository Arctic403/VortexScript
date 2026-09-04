use crate::diagnostic::{Diagnostic, Span};

#[derive(Debug, Clone, PartialEq)]
pub struct Token {
    pub kind: TokenKind,
    pub span: Span,
}

#[derive(Debug, Clone, PartialEq)]
pub enum TokenKind {
    Ident(String),
    Number(f64),
    String(String),
    LBrace,
    RBrace,
    LessEqual,
    Equal,
    Semicolon,
    Eof,
}

pub fn lex(source: &str) -> Result<Vec<Token>, Diagnostic> {
    let bytes = source.as_bytes();
    let mut tokens = Vec::new();
    let mut i = 0;

    while i < bytes.len() {
        match bytes[i] {
            b' ' | b'\t' | b'\r' | b'\n' => {
                i += 1;
            }
            b'/' if i + 1 < bytes.len() && bytes[i + 1] == b'/' => {
                i += 2;
                while i < bytes.len() && bytes[i] != b'\n' {
                    i += 1;
                }
            }
            b'{' => {
                tokens.push(token(TokenKind::LBrace, i, i + 1));
                i += 1;
            }
            b'}' => {
                tokens.push(token(TokenKind::RBrace, i, i + 1));
                i += 1;
            }
            b';' => {
                tokens.push(token(TokenKind::Semicolon, i, i + 1));
                i += 1;
            }
            b'=' => {
                tokens.push(token(TokenKind::Equal, i, i + 1));
                i += 1;
            }
            b'<' if i + 1 < bytes.len() && bytes[i + 1] == b'=' => {
                tokens.push(token(TokenKind::LessEqual, i, i + 2));
                i += 2;
            }
            b'"' => {
                let start = i;
                i += 1;
                let mut value = String::new();
                let mut closed = false;

                while i < bytes.len() {
                    if bytes[i] == b'"' {
                        i += 1;
                        closed = true;
                        break;
                    }

                    if bytes[i] == b'\\' {
                        i += 1;
                        if i >= bytes.len() {
                            break;
                        }
                        match bytes[i] {
                            b'n' => value.push('\n'),
                            b't' => value.push('\t'),
                            b'"' => value.push('"'),
                            b'\\' => value.push('\\'),
                            other => value.push(other as char),
                        }
                        i += 1;
                        continue;
                    }

                    let ch = source[i..].chars().next().expect("valid UTF-8 source");
                    value.push(ch);
                    i += ch.len_utf8();
                }

                if !closed {
                    return Err(Diagnostic::new(
                        "unterminated string literal",
                        Span::new(start, bytes.len()),
                    ));
                }

                tokens.push(token(TokenKind::String(value), start, i));
            }
            b'-' if i + 1 < bytes.len() && bytes[i + 1].is_ascii_digit() => {
                let (number, end) = scan_number(source, i)?;
                tokens.push(token(TokenKind::Number(number), i, end));
                i = end;
            }
            b if b.is_ascii_digit() => {
                let (number, end) = scan_number(source, i)?;
                tokens.push(token(TokenKind::Number(number), i, end));
                i = end;
            }
            b if is_ident_start(b) => {
                let start = i;
                i += 1;
                while i < bytes.len() && is_ident_continue(bytes[i]) {
                    i += 1;
                }
                tokens.push(token(
                    TokenKind::Ident(source[start..i].to_owned()),
                    start,
                    i,
                ));
            }
            _ => {
                let ch = source[i..].chars().next().expect("valid UTF-8 source");
                return Err(Diagnostic::new(
                    format!("unexpected character '{ch}'"),
                    Span::new(i, i + ch.len_utf8()),
                ));
            }
        }
    }

    tokens.push(token(TokenKind::Eof, bytes.len(), bytes.len()));
    Ok(tokens)
}

fn scan_number(source: &str, start: usize) -> Result<(f64, usize), Diagnostic> {
    let bytes = source.as_bytes();
    let mut i = start;
    if bytes[i] == b'-' {
        i += 1;
    }

    let mut seen_dot = false;
    while i < bytes.len() {
        if bytes[i].is_ascii_digit() {
            i += 1;
        } else if bytes[i] == b'.' && !seen_dot {
            seen_dot = true;
            i += 1;
        } else {
            break;
        }
    }

    source[start..i]
        .parse::<f64>()
        .map(|number| (number, i))
        .map_err(|_| Diagnostic::new("invalid number literal", Span::new(start, i)))
}

fn token(kind: TokenKind, start: usize, end: usize) -> Token {
    Token {
        kind,
        span: Span::new(start, end),
    }
}

const fn is_ident_start(byte: u8) -> bool {
    byte.is_ascii_alphabetic() || byte == b'_'
}

const fn is_ident_continue(byte: u8) -> bool {
    is_ident_start(byte) || byte.is_ascii_digit()
}
