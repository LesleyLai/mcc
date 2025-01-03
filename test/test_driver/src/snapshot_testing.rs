use colored::{Color, Colorize};
use similar::{ChangeTag, TextDiff};
use std::fmt::Display;
use std::path::{Path, PathBuf};
use std::sync::Arc;

pub struct SnapshotError {
    pub actual: Arc<str>,
    pub expected: std::io::Result<String>,
    pub expected_path: PathBuf,
}

struct Line(Option<usize>);

impl Display for Line {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self.0 {
            None => write!(f, "    "),
            Some(idx) => write!(f, "{:<4}", idx + 1),
        }
    }
}

impl Display for SnapshotError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match &self.expected {
            Ok(expected) => {
                let diff = TextDiff::from_lines(expected.as_ref(), self.actual.as_ref());

                for change in diff.iter_all_changes() {
                    let (sign, color) = match change.tag() {
                        ChangeTag::Delete => ("-", Some(Color::Red)),
                        ChangeTag::Insert => ("+", Some(Color::Green)),
                        ChangeTag::Equal => (" ", None),
                    };
                    write!(
                        f,
                        "{} {} |",
                        Line(change.old_index()),
                        Line(change.new_index())
                    )?;

                    let line = format!("{}{}", sign, change);
                    match color {
                        Some(c) => write!(f, "{}", line.color(c))?,
                        None => write!(f, "{}", line)?,
                    }
                }
            }
            Err(err) => {
                writeln!(f, "failed to open {}", self.expected_path.display())?;
                writeln!(f, "{}", err)?;
            }
        }

        Ok(())
    }
}

pub async fn snapshot_match<'a>(
    actual: Arc<str>,
    expected_path: &Path,
) -> Result<(), SnapshotError> {
    let expected = tokio::fs::read_to_string(&expected_path).await;

    if expected
        .as_ref()
        .is_ok_and(|expected| expected == actual.as_ref())
    {
        Ok(())
    } else {
        Err(SnapshotError {
            actual,
            expected,
            expected_path: expected_path.to_path_buf(),
        })
    }
}
