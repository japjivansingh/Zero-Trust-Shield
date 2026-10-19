# Zero-Trust Database Access & Query Anomaly Gateway

## 1. Vision Document....

### Project Name & Overview
**Project Name:** Zero-Trust Database Access & Query Anomaly Gateway  
**Overview:** This project is a custom Man-in-the-Middle (MITM) proxy built in C that sits between client applications and a database. It uses Docker to enforce strict network isolation, ensuring that clients can only access the database through the security proxy.

### Problem it Solves
Normally, if an application or user has the correct database credentials, the database trusts them completely. If an app has an SQL injection vulnerability, the database will just blindly execute the malicious code. This project fixes that by intercepting the raw network traffic and dropping connections that contain dangerous payloads (like `DROP TABLE`) before they ever reach the database.

### Target Users (Personas)
*   **Security & DevOps Engineers:** Looking for ways to add an extra layer of defense without modifying their existing apps.
*   **Database Administrators (DBAs):** Needing a fail-safe to prevent accidental or malicious destructive queries.
*   **Students/Researchers:** Anyone studying network protocols, raw socket programming, or container security.

### Vision Statement
To build a transparent, lightweight security layer that assumes no client is fully trusted, inspecting every database query at the network level before it runs.

### Key Features / Goals
*   **Transparent TCP Interception:** Acts as a middleman without requiring changes to the database or client tools.
*   **Real-Time Blocking:** Scans raw network packets in memory and instantly drops banned SQL commands.
*   **Strict Docker Isolation:** Puts the database on an internal Docker network so it literally cannot be accessed directly from the outside.
*   **Low-Level Implementation:** Written entirely in C for fast socket manipulation and low memory usage.

### Success Metrics
*   Successfully intercepts and kills TCP connections when banned payloads are sent.
*   Adds minimal delay to normal, safe queries.
*   The whole environment builds and runs cleanly with a single `docker-compose up` command.

### Assumptions & Constraints
*   **Assumptions:** Assumes the traffic between the client and proxy is unencrypted (plaintext TCP) so the proxy can actually read the packets.
*   **Constraints:** The current version relies on basic string matching for anomaly detection and is built specifically for the MariaDB protocol.

---

## 2. Quick Start – Local Development

This project uses Docker to easily spin up both the isolated database and the custom C gateway.

### Prerequisites
*   Docker (and `docker-compose`) installed.
*   A standard database client (like the `mysql` CLI tool).

### Installation & Execution
1. Clone this repo and open the folder in your terminal.
2. Build the C proxy and start the containers:
   ```bash
   docker-compose up --build

## Branching Strategy

This project follows the **GitHub Flow** branching model to ensure clean, isolated feature development:

1. **`main` Branch:** Represents stable, production-ready code. Direct commits to `main` are restricted.
2. **`feature/*` Branches:** All new capabilities (such as proxy socket handling, rule engines, or dashboard UIs) are developed in dedicated feature branches branched off `main`.
3. **Pull Requests & Code Review:** Once a feature is complete, a Pull Request (PR) is opened against `main` for review and testing before merging.

### Example Active Branch
* `feature/sql-anomaly-detector` – Implements payload inspection and connection termination logic.
