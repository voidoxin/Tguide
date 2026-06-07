Technical Proposal: Database Integrity, Connectivity Verification & Recovery

Project: tguide
Original submission by: voidoxin
Supplemental review by: Ext. Contributor
Date: June 2026
Status: Open for Review

Note from supplemental reviewer: The proposals in this document originate from
two sources. Proposals 1 and 2 were submitted by the project author. Proposals
3 and 4 are supplemental observations added by an external contributor following
a separate review session. All four proposals are presented as suggestions for
team discussion — none of them are mandatory directives. The team and project
lead retain full authority over implementation decisions.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Proposal 1 — Replace Static Hash with Dynamic Version Manifest

Problem

The current database integrity system relies on two compile-time constants
defined in db_cache_manager.h:

    static constexpr const char* DB_OFFICIAL_HASH = ...
    static constexpr const char* DB_DOWNLOAD_URL  = ...

The expected SHA-256 hash of the official database is embedded directly inside
the compiled binary. This creates a structural contradiction: the system is
designed to support database updates, yet the hash it validates against cannot
change without recompiling and redistributing a new binary.

Risk

Every time the database is updated — whether to add new tools, fix
vulnerabilities, or expand content — the hash of the new file will not match
the value baked into existing binaries. The resolver will treat a legitimate,
signed update as a tampered file and refuse to accept it.

Additionally, if a user manually modifies their local database for custom
entries or research, the mismatch between their modified file and the static
hash will trigger the conflict prompt on every launch, regardless of intent.

Consequence if Not Addressed

• Shipping DB_OFFICIAL_HASH as a non-empty value before release permanently
  locks the application to a single database version.
• Any future database update silently breaks for all existing users until they
  manually reinstall or recompile.
• The download and integrity system becomes a liability rather than a feature
  — it blocks legitimate use while providing no meaningful protection against
  a sufficiently motivated attacker who can simply patch the binary.

Proposed Solution

Replace the static hash with a lightweight version manifest hosted alongside
the database on GitHub. The manifest is a small JSON file (under 300 bytes)
that the application fetches before any database operation requiring network
access.

Proposed manifest structure:

    {
        "version": "1.2.0",
        "db_hash": "a3f8c29d1e...",
        "db_url": "https://raw.githubusercontent.com/{user}/...",
        "min_app_version": "1.0.0"
    }

Proposed resolver logic:

    1. Fetch manifest.json over HTTPS (timeout: 5s)
    2. Parse and validate structure — abort on malformed JSON
    3. Compare manifest["version"] against local cached version
    4. If version is newer:
         a. Download tguide.db from manifest["db_url"]
         b. Verify downloaded file hash against manifest["db_hash"]
         c. Accept on match — replace local database
         d. Delete file and abort on mismatch
    5. Remove DB_OFFICIAL_HASH from compiled binary
    6. Trust is anchored in HTTPS transport layer

This is the same model used by Homebrew, apt, and most modern package managers.
The security guarantee comes from HTTPS — a manifest served over
raw.githubusercontent.com cannot be intercepted or modified in transit without
breaking the TLS connection.

Required changes:

• Add fetchManifest() to DBResolver — fetches and parses manifest JSON
• Store last-seen manifest version in .db_cache alongside the existing hash
  record
• Remove DB_OFFICIAL_HASH constant — replaced entirely by manifest-provided
  hash
• Keep DB_DOWNLOAD_URL as a fallback pointing to the manifest, not directly
  to the database file

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Proposal 2 — Replace Connectivity Check with DNS-Only Verification

Problem

The current architecture implies checking internet connectivity before
attempting database download. The natural implementation — pinging a
third-party host such as archlinux.org, google.com or any public server —
carries unintended consequences for a tool positioned as a security research
and penetration testing assistant.

Risk

Sending unsolicited ICMP or HTTP requests to external hosts, even publicly
accessible ones, raises two distinct concerns.

Operational security. Users running tguide during a penetration testing
engagement may be operating under strict traffic controls or behind a VPN, Tor,
or isolated lab network. Any automatic outbound traffic to a third-party server
reveals the user's real IP, timestamps their session, and may violate the
engagement's rules of engagement. The tool's own documentation emphasizes
OPSEC — automatic background pings directly contradict this.

Open source exposure. As an open-source project, tguide will be inspected and
used by a wide audience. Automatically pinging external servers without explicit
user consent or disclosure is a pattern commonly flagged in security audits,
package reviews, and distribution inclusion criteria (AUR, Homebrew, Debian).
It can delay or prevent inclusion in official repositories.

Consequence if Not Addressed

• tguide will be rejected by distribution maintainers during package review.
• Security-conscious users will distrust the tool and fork or avoid it.
• Pentesting users may inadvertently expose their identity during engagements.
• The tool's own OPSEC-focused branding becomes internally inconsistent.

Proposed Solution

Replace any host-ping based connectivity check with a DNS-only resolution
against well-known public resolver hostnames. DNS queries are handled entirely
by the system's local resolver — no packet is sent directly to the target host,
no HTTP connection is opened, and the check leaves no trace on any external
server.

Proposed implementation:

    #ifndef _WIN32
        #include <netdb.h>
        #include <sys/socket.h>
    #else
        #include <ws2tcpip.h>
    #endif

    bool hasInternetAccess() {
        // Resolves well-known DNS hostnames
        // No packets are sent to these hosts
        static const char* probes[] = {
            "dns.google",
            "one.one.one.one",
            "resolver.opendns.com"
        };

        struct addrinfo hints{}, *res = nullptr;
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        for (const char* host : probes) {
            if (getaddrinfo(host, nullptr, &hints, &res) == 0) {
                freeaddrinfo(res);
                return true;
            }
        }
        return false;
    }

Why this approach is acceptable:

• No outbound packets reach any external server.
• Works transparently through VPN and proxy configurations.
• Requires no additional dependencies — getaddrinfo is available on Linux,
  Termux, macOS, and Windows.
• Passes distribution security audits and open-source compliance reviews.
• Consistent with the project's stated OPSEC design principles.

Recommended placement: call hasInternetAccess() only immediately before
fetchManifest() is invoked. Do not call it at startup, on every launch, or in
any background context.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Proposal 3 — GitHub URL Stability & Revised Threat Model for Account Compromise

Submitted by: Ext. Contributor

Observation

During review, a point was raised that deserves formal acknowledgment: raw
GitHub URLs are structurally permanent for the lifetime of a repository. The
URL pattern:

    https://raw.githubusercontent.com/{user}/...

does not change unless the repository is renamed, deleted, or the branch is
removed — all of which are actions visible to the public and recoverable. This
is a meaningful stability guarantee that the current design does not account
for, and it simplifies the threat model considerably.

Revised Threat Assessment for Account Compromise

The concern often raised is: what happens if the GitHub account is compromised?
Under the manifest-based system proposed in Proposal 1, the answer is bounded
and manageable.

An attacker who gains access to the GitHub account can modify tguide.db and
update manifest.json to reflect a new hash. The application would download the
modified database and accept it — because the hash in the manifest would match
the file.

However, this is data corruption, not code execution. The worst outcome is a
database with incorrect, missing, or misleading tool entries. The application
binary itself is unaffected. No malicious code runs on the user's machine as a
result of a compromised database file.

This is a materially lower risk than it initially appears, and it is worth
stating explicitly so the team does not over-engineer the integrity system in
response to a threat that the architecture already naturally contains.

Recommendation

Document this threat boundary clearly in the project's security notes. The
manifest system from Proposal 1 is sufficient for the database update use case.
No additional cryptographic signing of the database file is required at this
stage, though it remains an option for future hardening if the project grows to
a scale where it warrants the operational overhead.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Proposal 4 — Database Rollback & Recovery System

Submitted by: Ext. Contributor

Problem

The current update flow in DBResolver replaces the local database with a newly
downloaded version and discards the previous one. If the downloaded database
passes hash verification but contains structural issues — incomplete data, a
schema regression introduced by a bad release, or content errors noticed only
after deployment — the user has no recovery path without reinstalling the
application.

Proposed Solution

Implement a lightweight N-1 backup and rollback system with a three-state user
prompt for recovery decisions.

──────────────────────────────────────────────────────────────────────────────

Part A — Automatic Backup on Every Update

Before replacing the active database with a newly downloaded version, the
resolver saves the current database as a backup file in the same directory.

    ~/.local/share/tguide/
    ├── tguide.db        ← active database
    └── tguide.db.bak   ← previous version

Only one backup is retained at a time. On each successful update, the old .bak
is overwritten by the previous active database before the new one is written.
This keeps the storage footprint minimal — one extra database file — while
always providing a known-good fallback.

Proposed update sequence:

    1. Download new tguide.db to tguide.db.tmp
    2. Verify hash of tguide.db.tmp against manifest["db_hash"]
    3. On success:
         a. Copy tguide.db → tguide.db.bak
         b. Rename tguide.db.tmp → tguide.db
         c. Record update in .db_cache
    4. On failure:
         a. Delete tguide.db.tmp
         b. Abort — active database is untouched

──────────────────────────────────────────────────────────────────────────────

Part B — Automatic Detection at Boot

The existing schema validation in DBResolver::validateSchema() already detects
structural database problems. The proposal is to extend this: when schema
validation fails on the active database at boot, the application checks whether
a valid .bak file exists and, if so, triggers the recovery prompt rather than
a hard fatal error.

Proposed boot detection logic:

    Boot → validate tguide.db
            ├── valid → proceed normally
            └── invalid →
                    ├── tguide.db.bak exists →
                    │       trigger Part C prompt
                    └── no valid backup →
                            UI_fatal: "Database corrupted.
                                       Reinstall required."

──────────────────────────────────────────────────────────────────────────────

Part C — Three-State Recovery Prompt

When a database problem is detected and a backup exists, the application
presents a recovery prompt with three explicit options. The middle option —
deferral — is intentional. It acknowledges that not every user will immediately
know whether their database is causing problems, and forcing a binary yes/no
decision at an inconvenient moment produces worse outcomes than allowing the
user to defer.

Proposed prompt:

    tguide › database recovery

      A problem was detected with the current database.
      A previous version is available as a backup.

      ├── [1]  Restore previous database
      │         roll back to the last known-good version
      ├── [2]  Keep current database
      │         the backup will be deleted — this version treated as valid
      └── [3]  Ask me later
                skip for now — you will be asked again on next launch

      →

Behavior per choice:

    Choice       Action
    ─────────────────────────────────────────────────────────────────────────
    1 Restore    Copy .bak → active DB. Delete .bak.
                 Record rollback in .db_cache.
    2 Keep       Delete .bak. Record user confirmation in .db_cache.
                 Never prompt again for this version.
    3 Ask later  Do nothing. Proceed with current (possibly broken) database.
                 Prompt repeats on next launch.

The "ask later" path intentionally allows the user to continue using the
application even with a potentially degraded database. This is consistent with
the project's design principle that recoverable errors should not block the user
— only fatal, unrecoverable states should halt execution.

──────────────────────────────────────────────────────────────────────────────

Part D — Manual Rollback via Settings

In addition to the automatic boot-time prompt, a manual rollback option should
be accessible through the Settings screen. This allows users to initiate a
rollback at any time, not only when automatic detection fires.

Proposed placement:

    tguide › settings

    ├── ◉  Colors         [enabled]    [1]
    ├── ◈  Database                    [2]
    └── ←  Back                        [0]

    tguide › settings › database

      active version:   v1.3.0   (updated 2026-...)
      backup available: v1.2.0

      ├── [1]  Restore previous version (v1.2.0)
      ├── [2]  Delete backup
      └── [0]  Back

If no backup exists, option [1] is replaced with a dimmed line reading
no backup available.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Summary

    Feature                     Current State                  Proposed State
    ─────────────────────────────────────────────────────────────────────────────────────────────────
    Hash source                 Compiled into binary           Fetched from manifest.json over HTTPS
    Update detection            Requires recompile             Manifest version comparison
    Connectivity check          Unspecified / implicit ping    DNS-only resolution, called on demand
    External server exposure    Potential on every check       Zero — DNS stays local
    Account compromise impact   Undefined                      Bounded to data corruption only —
                                                               no code execution
    Previous DB preserved       No                             Yes — automatic .bak before every update
    Recovery on bad update      None — hard fatal              Three-state prompt: restore / keep / defer
    Manual rollback             Not available                  Settings → Database → Restore previous version
    Distribution readiness      Blocked                        Compatible with AUR, Homebrew, Debian

All proposals in this document are suggestions. The team is encouraged to adopt,
modify, or reject any of them based on implementation priorities and constraints.
No proposal here constitutes a required change.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Original proposals — voidoxin
Supplemental review (Proposals 3 & 4) — Ext. Contributor
