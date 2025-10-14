use std::fmt;

use anyhow::{Context, Result};
use chrono::{DateTime, FixedOffset};
use gix::bstr::ByteSlice;
use reqwest::{
    blocking::{Client, ClientBuilder},
    header::{HeaderMap, HeaderValue},
};
use tracing::{info, warn};

#[derive(Debug)]
pub struct ManytaskClient {
    base_url: String,
    client: Client,
}

impl ManytaskClient {
    pub fn new(base_url: impl Into<String>, tester_token: impl fmt::Display) -> Result<Self> {
        let r: Result<_> = try {
            let mut default_headers = HeaderMap::new();
            default_headers.insert(
                "Authorization",
                HeaderValue::from_str(&format!("Bearer {tester_token}"))?,
            );
            let client = ClientBuilder::new()
                .default_headers(default_headers)
                .build()
                .context("creating http client")?;

            Self {
                base_url: base_url.into(),
                client,
            }
        };
        r.context("trying to create manytask client")
    }

    pub fn report_score(
        &self,
        course: &str,
        student: &str,
        task: &str,
        commit_ts: DateTime<FixedOffset>,
    ) -> Result<()> {
        let r: Result<()> = try {
            let resp = self
                .client
                .post(format!("{}/api/{}/report", self.base_url, course))
                .form(&[
                    ("task", task),
                    ("username", student),
                    (
                        "submit_time",
                        &commit_ts.format("%Y-%m-%d %H:%M:%S%z").to_string(),
                    ),
                ])
                .send()?
                .error_for_status()?;

            let body = match resp.bytes() {
                Ok(b) => b,
                Err(e) => {
                    warn!("Failed to get response body: {e}");
                    return Ok(());
                }
            };

            info!("Response: {}", body.to_str_lossy());
        };
        r.with_context(|| format!("trying to report {}, {}, {}", course, student, task))
    }

    pub fn report_score_with_retries(
        &self,
        course: &str,
        student: &str,
        task: &str,
        commit_ts: DateTime<FixedOffset>,
    ) -> Result<()> {
        let mut i = 0;
        loop {
            let r = self.report_score(course, student, task, commit_ts);
            i += 1;
            if i == 3 {
                return r;
            }
            match r {
                Ok(v) => return Ok(v),
                Err(e) => warn!("Failed to report score: {e:?}"),
            }
            std::thread::sleep(std::time::Duration::from_secs(1 << i));
        }
    }
}
