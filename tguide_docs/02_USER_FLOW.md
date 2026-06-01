# tguide — User Flow & Navigation Guide
**Author:** voidoxin  
**Read after:** `01_PROJECT_OVERVIEW.md`

---

## 1. First Run

On the very first launch, before anything else is shown:

```
╔═════════════════════════════════════════════════════════════╗
║               TGUIDE  —  LEGAL DISCLAIMER                   ║
╚═════════════════════════════════════════════════════════════╝

  This tool is intended solely for authorized security testing...
  ...
  → agree / exit
```

- Type `agree` → disclaimer accepted, saved to `config.json`, never shown again
- Type `exit` / `q` / `quit` → program closes immediately
- Any other input → disclaimer redisplays

---

## 2. Main Menu

After the disclaimer (or on all subsequent runs):

```
  ████████╗ ██████╗ ██╗   ██╗██╗██████╗ ███████╗
  ...
          security tools reference — by voidoxin

  tguide › main menu

  ├─ ⊞  Tools             — flags, templates, usage           [1]
  ├─ ◎  Script Generator  — build commands interactively      [2]
  ├─ ◈  Saved Commands    — your personal command library     [3]
  ├─ ▦  Saved Scripts     — your generated scripts            [4]
  ├─ ⊙  Settings          — configure tguide behavior         [5]
  └─ ✕  Exit                                                  [6]
  →
```

The user can type a number or the option name (case-insensitive).

---

## 3. Tools Flow

### 3.1 Tools Entry Screen

```
  tguide › tools

  ├─ ◉  Browse by Category   [1]
  ├─ ⌕  Search               [2]
  ├─ ⊟  Filter               [3]
  └─ ←  Back                 [0]
  →
```

---

### 3.2 Path A — Browse by Category

```
  tguide › tools › categories

  ├─ [1]  Network Scanning
  ├─ [2]  Web Application
  ├─ [3]  Password Attacks
  ├─ [4]  Exploitation
  ├─ [5]  Post-Exploitation
  ├─ [6]  OSINT
  ...
  [page 1/2]  next / prev / back
  →
```

User types a number or category name → moves to tool list:

```
  tguide › tools › Network Scanning

  ├─ [1]  NMAP
  │        fast network scanner and host discovery tool
  ├─ [2]  MASSCAN
  │        high-speed port scanner
  ...
  [page 1/1]  back
  →
```

User types a number or tool name → opens Tool Detail Screen (§3.5)

---

### 3.3 Path B — Search

```
  tguide › tools › search

  enter search query (or 0 to go back):
  → <input>

  searching...

  ┌──────────────────────────────────────────────┐
  │  results for: "sql injection"                │
  └──────────────────────────────────────────────┘

  [TODO: search algorithm not yet implemented]
  no results found for this query.
```

> **Note:** The search UI is fully built. The algorithm that performs actual matching is a future task. The screen accepts input and returns an empty result set in the current version.

---

### 3.4 Path C — Filter

```
  tguide › tools › filter

  available filters:
    protocol  — e.g. protocol=tcp  or  protocol=http
    root      — e.g. root=yes  or  root=no
    loud      — e.g. loud=yes  or  loud=no

  usage: protocol=tcp,root=yes
  multiple filters separated by comma

  enter filter expression (or 0 to cancel):
  →
```

After input → results displayed as paginated tool list → Tool Detail Screen (§3.5)

---

### 3.5 Tool Detail Screen

```
  tguide › tools › NMAP

  ├─ ◉  Description              [A]
  ├─ ⊞  All Flags                [B]
  ├─ ⊟  Flags with Filter        [C]
  ├─ ▦  Templates                [D]
  ├─ ▦  Templates with Filter    [E]
  └─ ←  Back                     [0]
  →
```

**Special case — metasploit:**
```
  tguide › tools › METASPLOIT

  ├─ ◉  Description              [A]
  ├─ ⊞  All Flags                [B]
  ├─ ⊟  Flags with Filter        [C]
  ├─ ▦  Templates                [D]
  ├─ ▦  Templates with Filter    [E]
  ├─ ⚡  Vulnerabilities          [F]   ← extra option
  └─ ←  Back                     [0]
  →
```

**Special case — recon-ng:**
```
  tguide › tools › RECON-NG

  ├─ ◉  Description              [A]
  ├─ ⊞  All Flags                [B]
  ├─ ⊟  Flags with Filter        [C]
  ├─ ▦  Templates                [D]
  ├─ ▦  Templates with Filter    [E]
  ├─ ◈  Modules                  [F]   ← extra option
  └─ ←  Back                     [0]
  →
```

---

### 3.6 Option A — Description

```
  tguide › tools › NMAP › description

  Nmap ("Network Mapper") is a free and open source utility
  for network discovery and security auditing. It uses raw IP
  packets in novel ways to determine what hosts are available
  on the network, what services those hosts are offering...

  ──────────────────────────────────────────────────────────
  press enter to continue...
```

---

### 3.7 Option B — All Flags

```
  tguide › tools › NMAP › flags

  ├─ [1]  -sV          — version detection
  │        protocols: TCP  |  root: no  |  loud: medium
  ├─ [2]  -sS          — SYN stealth scan
  │        protocols: TCP  |  root: yes  |  loud: low
  ...
  [page 1/3]  next / prev / back
  →
```

---

### 3.8 Option C — Flags with Filter

```
  tguide › tools › NMAP › flags › filter

  available filters:
    protocol  — e.g. protocol=tcp
    root      — e.g. root=yes
    loud      — e.g. loud=low

  usage: root=yes,protocol=tcp

  enter filter expression (or 0 to cancel):
  →
```

Results displayed as paginated flag list (same style as B).

> **Note:** Implementation pending — marked TODO in svc_tools.

---

### 3.9 Option D — Templates

```
  tguide › tools › NMAP › templates

  ├─ [1]  Quick Host Discovery
  │        ping sweep to discover live hosts on a subnet
  ├─ [2]  Full Port Scan
  │        scan all 65535 ports on a target
  ├─ [3]  Service Version Detection
  │        identify services and versions on open ports
  ...
  [page 1/1]  back
  →
```

User picks a template → Template Detail (§3.11)

---

### 3.10 Option E — Templates with Filter

```
  tguide › tools › NMAP › templates › filter

  available filters:
    root      — e.g. root=yes
    protocol  — e.g. protocol=tcp

  enter filter expression (or 0 to cancel):
  →
```

Results displayed as paginated template list → Template Detail (§3.11)

> **Note:** Implementation pending — marked TODO in svc_tools.

---

### 3.11 Template Detail — Fill Values + Save

```
  tguide › tools › NMAP › Service Version Detection

  nmap -sV -p <port> <target_ip>

  fill in the required values:
  → target_ip:  192.168.1.1
  → port:       80

  ┌─────────────────────────────────────────────────────────┐
  │  nmap -sV -p 80 192.168.1.1                            │
  └─────────────────────────────────────────────────────────┘

  ├─ [S]  Save to Saved Commands
  └─ [0]  Back without saving
  →
```

If **S** is chosen:
```
  enter a short note for this command:
  → version scan on target web server

  ✓ command saved.
  press enter to continue...
```

---

### 3.12 Option F — Vulnerabilities (metasploit only)

```
  tguide › tools › METASPLOIT › vulnerabilities

  ├─ ⌕  Search               [1]
  ├─ ⊟  Filter               [2]
  ├─ ⊞  Show All             [3]
  └─ ←  Back                 [0]
  →
```

**Search:**
```
  enter search query (or 0 to go back):
  → vsftpd

  [TODO: search algorithm not yet implemented]
  no results found for this query.
```

**Filter:**
```
  available filters:
    severity   — e.g. severity=critical  or  severity=high
    service    — e.g. service=ftp
    platform   — e.g. platform=linux
    access     — e.g. access=remote

  usage: severity=critical,service=ftp

  enter filter expression (or 0 to cancel):
  →
```

**Show All:**
```
  tguide › tools › METASPLOIT › vulnerabilities › all

  ├─ [1]  vsftpd 2.3.4 Backdoor
  │        severity: critical  |  service: ftp  |  platform: linux
  ├─ [2]  EternalBlue (MS17-010)
  │        severity: critical  |  service: smb  |  platform: windows
  ...
  [page 1/4]  next / prev / back
  →
```

---

### 3.13 Option F — Modules (recon-ng only)

```
  tguide › tools › RECON-NG › modules

  ├─ ⌕  Search               [1]
  ├─ ⊟  Filter               [2]
  ├─ ⊞  Show All             [3]
  └─ ←  Back                 [0]
  →
```

**Filter:**
```
  available filters:
    type      — e.g. type=recon
    platform  — e.g. platform=linux
    mode      — e.g. mode=active  or  mode=passive
    loud      — e.g. loud=yes

  usage: type=recon,mode=passive

  enter filter expression (or 0 to cancel):
  →
```

**Show All:**
```
  tguide › tools › RECON-NG › modules › all

  ├─ [1]  recon/domains-hosts/google_site_web
  │        type: recon  |  mode: passive  |  loud: no
  ├─ [2]  recon/hosts-ports/shodan_ip
  │        type: recon  |  mode: passive  |  loud: no
  ...
  [page 1/3]  next / prev / back
  →
```

---

## 4. Script Generator Flow

```
  tguide › script generator

  step 1 — pick a tool
  →  nmap

  step 2 — pick templates (type "done" when finished)
  tool: NMAP

  ├─ [1]  Quick Host Discovery
  ├─ [2]  Full Port Scan
  ├─ [3]  Service Version Detection
  →  1

  filling values for: Quick Host Discovery
  nmap -sn <target_ip>/<subnet>
  → target_ip:  192.168.1.0
  → subnet:     24

  add another template? (number / done / back)
  →  3

  filling values for: Service Version Detection
  nmap -sV -p <port> <target_ip>
  → port:       1-1000
  → target_ip:  192.168.1.1

  add another template? (number / done / back)
  →  done

  ──────────────────────────────────────────────
  step 3 — review commands
  ──────────────────────────────────────────────
  [1]  nmap -sn 192.168.1.0/24
  [2]  nmap -sV -p 1-1000 192.168.1.1

  remove a command by number, or confirm to proceed:
  → confirm

  step 4 — name the script
  → recon_scan

  ┌──────────────────────────────────────────────────────────┐
  │  script will be saved to:                               │
  │  ~/.local/share/tguide/scripts/recon_scan.sh            │
  └──────────────────────────────────────────────────────────┘

  ├─ [G]  Generate and save
  └─ [0]  Cancel
  →  G

  ✓ script saved: ~/.local/share/tguide/scripts/recon_scan.sh
  press enter to continue...
```

---

## 5. Saved Commands Flow

```
  tguide › saved commands

  ├─ [1]  NMAP — version scan on target web server
  │        nmap -sV -p 80 192.168.1.1
  ├─ [2]  HYDRA — ssh brute force attempt
  │        hydra -l root -P /usr/share/wordlists/rockyou.txt 10.0.0.1 ssh
  ...
  [page 1/1]  back
  →  1

  tguide › saved commands › #1

  ┌─────────────────────────────────────────────────────────┐
  │  nmap -sV -p 80 192.168.1.1                            │
  └─────────────────────────────────────────────────────────┘
  tool: NMAP
  note: version scan on target web server

  ├─ [V]  View full command
  ├─ [D]  Delete
  └─ [0]  Back
  →
```

---

## 6. Saved Scripts Flow

```
  tguide › saved scripts

  ├─ [1]  recon_scan
  │        ~/.local/share/tguide/scripts/recon_scan.sh
  │        full recon on 192.168.1.0/24
  ...
  [page 1/1]  back
  →  1

  ├─ [P]  Print script content
  ├─ [D]  Delete record
  └─ [0]  Back
  →  P

  #!/bin/bash
  # generated by tguide — voidoxin
  # recon_scan
  nmap -sn 192.168.1.0/24
  nmap -sV -p 1-1000 192.168.1.1

  press enter to continue...
```

> **Note:** Delete removes the record from `saved_scripts.json` only. The `.sh` file on disk is not deleted.

---

## 7. Settings Flow

```
  tguide › settings

  ├─ ◉  Colors   [enabled]   [1]
  └─ ←  Back     [0]
  →  1

  colors disabled.
  press enter to continue...
```

Toggle is applied immediately to the current session and saved to `config.json`.

---

## 8. Global Navigation Reference

Available at **every prompt** without exception:

| Type this | What happens |
|-----------|-------------|
| `0` | Go back to previous screen |
| `back` | Go back to previous screen |
| `q` | Close program immediately |
| `quit` | Close program immediately |
| `exit` | Close program immediately |
| `next` or `n` | Next page (paginated lists only) |
| `prev` or `p` | Previous page (paginated lists only) |
| A number | Select item by position |
| An option name | Select item by name (case-insensitive) |

Screen is cleared on every navigation action. The breadcrumb at the top always shows the current location.
