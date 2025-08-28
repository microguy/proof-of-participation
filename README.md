# GoldcoinPoP - Proof of Participation

**The Greatest Form of Money in the Galaxy**

## Overview

GoldcoinPoP is the evolution of Goldcoin (GLC) from Proof of Work to Proof of Participation - a revolutionary consensus mechanism that reduces energy consumption by 99.99% while maintaining true decentralization and security.

**Launch**: Hard fork at block 3,500,000  
**Creator**: MicroGuy  
**Philosophy**: Channeling Satoshi's Vision with Modern Innovation

## Key Features

### 🌍 Proof of Participation (PoP)
- **99.99% Energy Reduction** - No mining, no waste
- **Democratic Consensus** - One stake, one vote in the lottery
- **1000 GLC Minimum** - Accessible to everyone
- **VRF Lottery Selection** - Provably fair block producer selection
- **2-Minute Blocks** - Fast and reliable

### 💰 Hybrid Fee System
- **First 5% of Block**: FREE for high-priority transactions
- **Remaining 95%**: Optional fees during congestion
- **Priority Formula**: Satoshi's original (coin age × value / size)
- **32MB Blocks**: Massive capacity
- **Philosophy**: Everyone deserves free transactions

### 🤖 AI Autonomy (Phased Rollout)
- **Phase 1 (2025)**: 60% AI operation
- **Phase 2 (2026)**: 90% AI operation  
- **Phase 3 (2027)**: 100% autonomous - World's first self-governing money
- **No human intervention required**

### 🔒 Security & Preservation
- **All GLC Balances Protected** - Every holder transitions seamlessly
- **No Coins Lost** - 100% backward compatibility
- **Economic Security** - Attack cost makes it economically irrational
- **No 51% Attacks** - No mining power to concentrate

## Technical Specifications

- **Language**: C++23
- **Database**: Berkeley DB 18.1.40
- **GUI Framework**: Qt 6.9
- **Build System**: CMake 4.1.0
- **Compiler**: GCC 15.2
- **No Boost Dependencies** - Modern C++ standard library only

## Core Components

```
participation.cpp/h     - Proof of Participation consensus engine
vrf.h                  - Verifiable Random Function for lottery
hybridfee.cpp          - Hybrid fee system (5% free space)
ai_autonomy.cpp        - AI self-governance framework
consensus_preservation.cpp - Protects all existing balances
pop_security.cpp       - Security model for PoP
hardfork.cpp           - Smooth transition at block 3,500,000
```

## Building from Source

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake git
sudo apt-get install libssl-dev libdb-dev
sudo apt-get install qt6-base-dev qt6-tools-dev
```

### Build Instructions
```bash
# Clone repository
git clone https://github.com/microguy/proof-of-participation.git
cd proof-of-participation

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)

# Run
./goldcoinpop
```

## Consensus Rules

### Participation Requirements
- Minimum stake: 1000 GLC
- Stake maturity: 1440 blocks (2 days)
- Block time: 120 seconds
- Block reward: Standard Goldcoin schedule

### Transaction Priority
```
Priority = Σ(input_value × confirmations) / transaction_size
Free threshold = 57,600,000 (Satoshi's formula)
```

## Philosophy

> "What would Satoshi do?"

We channel Satoshi Nakamoto's original vision while solving Bitcoin's energy crisis. Every design decision asks this fundamental question.

### Core Principles
1. **No trusted third parties** - Pure peer-to-peer
2. **Permissionless participation** - No KYC, no identity
3. **Robust unstructured simplicity** - Elegant, not complex
4. **Economic security** - Cost makes attacks irrational
5. **Universal access** - Everyone can participate

## Roadmap

### ✅ Completed
- Core PoP consensus mechanism
- VRF lottery selection
- Hybrid fee system
- AI autonomy framework
- Consensus preservation logic

### 🚧 In Progress
- Qt 6.9 GUI implementation
- Testnet deployment
- Wallet migration tools

### 📋 Planned
- Block 3,500,000: PoP activation
- 2025: Phase 1 AI (60% autonomous)
- 2026: Phase 2 AI (90% autonomous)
- 2027: Phase 3 AI (100% autonomous)

## Contributing

We welcome contributions that align with Satoshi's principles and Goldcoin's vision. Please ensure:
- Code follows C++23 standards
- No Boost dependencies
- Maintains simplicity over complexity
- Preserves all existing GLC holder rights

## Original Whitepaper

The Proof of Participation whitepaper is available in the repository, detailing the complete consensus mechanism and economic model.

## Contact

**Creator**: MicroGuy  
**Email**: webmaster@microguy.net  
**Website**: [goldcoinproject.org](https://goldcoinproject.org)  
**Original Launch**: May 15, 2013

## License

MIT License - see LICENSE file for details

---

*"By 2027, Goldcoin will be the first money that governs itself."*  
**- MicroGuy**

*"The nature of Bitcoin is such that once version 0.1 was released, the core design was set in stone for the rest of its lifetime."*  
**- Satoshi Nakamoto**

*"We honor the past while building the future."*  
**- GoldcoinPoP**