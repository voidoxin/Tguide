# 🛡️ tguide — Project Master Plan & Strategic Roadmap
**Author:** voidoxin  
**Core Stack:** C++17, SQLite3, libcurl, GGUF (Future)  
**Vision:** An elite, offline-first terminal assistant for cybersecurity professionals.

---

## 1. 🎯 Project Philosophy | رؤية المشروع
`tguide` is not just a command database; it is a **Terminal Ecosystem** designed for penetration testers. It solves the "internet dependency" problem during engagements by providing a lightning-fast, color-coded, and searchable reference for tools, flags, and exploit templates.

مشروع **tguide** ليس مجرد قاعدة بيانات للأوامر، بل هو مساعد رقمي لمختبري الاختراق يعمل بالكامل من سطر الأوامر، يتميز بالسرعة والقدرة على العمل في البيئات المعزولة.

---

## 2. 🏗️ Technical Architecture | المعمارية التقنية
The project follows a strict **Layered Design** to ensure maintainability:
* **L0-Core:** Filesystem paths, SQLite queries, and config management.
* **L1-Services:** The "Brain" (Logic, saved scripts, search scores).
* **L2-Interface Engine:** UI layer (ANSI colors, breadcrumbs, input normalization).

---

## 3. 🌐 Networking & OPSEC Modes | نظام الشبكة والتخفي
To respect **Operational Security (OPSEC)**, the tool provides four distinct connectivity levels:

| Mode | Frequency | Description |
| :--- | :--- | :--- |
| **Aggressive** | Every 1 Min | Fast updates, ignores stealth. |
| **Balanced** | Every 30 Min | Default. Minimal battery/network impact. |
| **Passive** | On-Demand | Only checks when triggered by the user. |
| **Stealth** | Disabled | **Zero Traffic.** 100% silent and offline. |

---

## 4. 🔄 Smart Update (Shadow Swap) | نظام التحديث الذكي
To prevent **Database Locking**, we use the **Shadow Swap** strategy:
1. **Background Download:** Updates are saved as `tguide.db.tmp`.
2. **User Notification:** A UI indicator appears: `[ Update Ready - Restart to Apply ]`.
3. **Atomic Swap:** Upon restart/exit, the code closes the DB connection, replaces the file, and re-opens the new DB.

---

## 5. 🔍 Randomized Connection Check | فحص الاتصال العشوائي
Instead of pinging sensitive IPs, `tguide` uses a **Randomized Lab Rotation**:
* Rotates between legal "scan-me" domains (e.g., `scanme.nmap.org`) and common public endpoints.
* Prevents IDS/Network monitors from flagging the tool as a persistent "beacon".

---

## 6. 🧠 AI Integration Roadmap | مسار الذكاء الاصطناعي
The future goal is to integrate a **Local Cyber-AI Assistant**:
* **Training:** Using **Unsloth** and **QLoRA** on **Google Colab** (Free tier).
* **Dataset:** Synthetic data generated from `tguide.db` content.
* **Engine:** Integrated using `llama.cpp` to keep the AI **100% offline**.

---

## 7. 🚀 Growth Strategy | خطة التنمية
* **Contribution:** YAML templates for community-driven tool updates.
* **Content:** Adding Cloud Security (AWS/Azure) and AI Hacking modules.
* **Distribution:** Deployment via `AUR` (Arch), `Homebrew` (macOS), and `Termux`.

---

### 🛠️ Essential Toolkit | صندوق الأدوات
* **Build System:** `CMake`.
* **Libraries:** `nlohmann/json`, `libcurl`, `SQLite3`.
* **AI Tools:** `Hugging Face`, `Unsloth AI`, `llama.cpp`.
