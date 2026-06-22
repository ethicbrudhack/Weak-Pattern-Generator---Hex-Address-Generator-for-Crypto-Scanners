========================================
WEAK PATTERN GENERATOR - HEX ADDRESS GENERATOR
========================================

DESCRIPTION:
This tool generates massive TXT files with hexadecimal patterns that are
converted by AllChainScanner into cryptocurrency addresses (BTC, ETH, ZEC, etc.).
Generates up to 10GB of patterns with 37 different hex sequences.

FEATURES:
- 🚀 High speed up to 100 MB/s
- 📁 Supports 100 MB, 1 GB, 10 GB files
- 🔄 37 different hex patterns with prefixes/suffixes
- 🎲 Random or Sequential mode
- 📊 Real-time progress bar with ETA
- 💾 Buffer optimized writing

PATTERN TYPES:
Basic:      0123456789abcdef0123456789abcdef...
With Prefix: f0123456789abcdeff0123456789abcdef...
With Suffix: 0123456789abcdeff0123456789abcdeffa...
With Both:   ff0123456789abcdef00ff0123456789abcdef00...
Special:     dead0123456789abcdefdead0123456789abcdef...

INTEGRATION WITH ALLCHAINSCANNER:
1. Generate patterns: ./pattern_generator (choose option 2)
2. Run scanner: ./AllChainScanner -f wzorce_1gb.txt -o found.txt
3. Scanner converts 64-char hex → Private Keys → BTC/ETH Addresses

COMPILATION:
g++ -std=c++11 -O3 -pthread -o pattern_generator pattern_generator.cpp

USAGE:
./pattern_generator

MENU OPTIONS:
  1. Generate 100 MB TXT (random patterns)
  2. Generate 1 GB TXT (random patterns)
  3. Generate 10 GB TXT (random patterns)
  4. Generate 100 MB TXT (sequential patterns)
  5. Generate 1 GB TXT (sequential patterns)
  6. Exit

OUTPUT FILES:
Option 1: wzorce_100mb.txt     (100 MB, random)
Option 2: wzorce_1gb.txt       (1 GB, random)
Option 3: wzorce_10gb.txt      (10 GB, random)
Option 4: wzorce_100mb_seq.txt (100 MB, sequential)
Option 5: wzorce_1gb_seq.txt   (1 GB, sequential)

PERFORMANCE:
100 MB → ~2-5 seconds  (20-50 MB/s)
1 GB   → ~20-40 seconds (25-50 MB/s)
10 GB  → ~3-5 minutes   (30-50 MB/s)

WHY WEAK PATTERNS?
AllChainScanner searches for:
- Weak private keys (insecure generation)
- Brain wallets (passphrase-based keys)
- Predictable patterns (sequential/pattern-based keys)
- Keys from vulnerable RNGs
- Historical wallet generation flaws

COMPATIBILITY WITH ALLCHAINSCANNER:
✅ Bitcoin (1..., 3..., bc1...)
✅ Ethereum (0x...)
✅ Zcash (t1..., t3...)
✅ Litecoin (L..., M...)
✅ Dogecoin (D...)
✅ Dash (X...)
✅ Bitcoin Cash (bitcoincash:...)

REQUIRED FILES:
- pattern_generator.cpp (this file)
- g++ compiler
- AllChainScanner (for scanning generated patterns)

EXAMPLE WORKFLOW:
1. ./pattern_generator → select option 2
2. ./AllChainScanner -f wzorce_1gb.txt -o found.txt -t all
3. cat found.txt
4. Found: 0123456789abcdef... → 1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa → BALANCE: 0.5 BTC

DISCLAIMER:
This tool is for educational and research purposes only.
Only test on addresses you own. Do not use for illegal activities.

========================================
LINK: https://github.com/ethicbrudhack/AllChainScanner-Bitcoin-eth-zcash-etc..-
========================================
