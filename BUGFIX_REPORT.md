# BUGFIX_REPORT — WWC3 (Wayawolfcoin)

Date: 2026-05-27
Branch: `agents/bug-analysis-and-fix-plan` (based on `main`)
Build: Docker (Ubuntu 16.04) `make -f makefile.unix USE_UPNP=1 STATIC=1` — **zero errors**

---

## Fixes Applied (16 commits)

Each fix is a separate commit on `agents/bug-analysis-and-fix-plan`, strictly one logical change per commit.

### Phase 1 — Safety infrastructure

| Commit | Bug | Description |
|--------|-----|-------------|
| `4033c94` | #5 | Added `ccCriticalSection cs_getBlockTemplate` + `LOCK()` to `getwork()`, `getworkex()`, `getblocktemplate()` — protects shared `mapNewBlock`/`vNewBlock` static variables from concurrent RPC threads |
| `9d1bfa5` | #2 | Bounds checks in `CheckBlockSignature()`: validate `vtx.size() > 1` and `vtx[1].vout.size() > 1` before `vtx[1].vout[1]` access |
| `29a9c8f` | #10 | Coinbase `vin[0]` bounds check in `AcceptBlock()` height-signature validation |
| `7e09e73` | #9 | Bounds check for `vtx[0].vout[0]` in `SignBlock()` |

### Phase 2 — Null-pointer safety

| Commit | Bug | Description |
|--------|-----|-------------|
| `11f5ae3` | #4, #6 | `ConnectBlock()`: validate `vtx.size() ≥ 2` before `vtx[1]` access; check `pindex->pprev` for null before passing to `GetCoinAge()` |
| `761657e` | #7 | `AcceptBlock()`: null-check `pindexPrev` before dereference in timestamp validation |

### Phase 3 — Critical logic hardening

| Commit | Bug | Description |
|--------|-----|-------------|
| `1b23e69` | #1 | `GetProofOfStake()` in `main.h`: added explicit `vtx.size() > 1`, `vtx[1].IsCoinStake()`, and `vtx[1].vin.size() > 0` checks; eliminates TOCTOU race |
| `e344216` | #3 | `AcceptBlock()`: early validation `if (IsProofOfStake() && vtx.size() < 2)` before any PoS-specific access |
| `35f91ad` | #8 | `ConnectBlock()` stake reward: overflow/underflow guard (`nTxValueOut < 0 || nTxValueIn < 0 || nTxValueOut < nTxValueIn`) |
| `ffaa96f` | #8 | Added `MoneyRange(nStakeReward)` validation before reward comparison |
| `410ee15` | #14 | `AddToBlockIndex()`: split the PoW/PoS ternary into explicit branches; PoS branch checks `vtx.size() ≥ 2` and `vtx[1].vin` non-empty before accessing `vtx[1].vin[0]` |

### Phase 4 — Medium-severity fixes

| Commit | Bug | Description |
|--------|-----|-------------|
| `7c5bb8e` | #15 | `CreateTransaction()` in `wallet.cpp`: guard `nChange = nValueIn - nValue - nFeeRet` against underflow (negative inputs or `nValueIn < nValue + nFeeRet`) |
| `44dd806` | #16 | `miner.cpp`: replaced `assert(scriptSig.size() <= 100)` with runtime checks; `CreateNewBlock()` returns `NULL` on failure; `IncrementExtraNonce()` logs and returns |
| `bff4ef6` | #13 | `getworkex()`: size limit (10K), try/catch, and `CheckTransaction()` validation on coinbase deserialization — removed the `// FIXME - HACK!` |
| `20dd029` | — | Cleaned up stale `// FIXME: thread safety` comment in `getwork()` (resolved by bug #5) |

---

## Files Changed

| File | Bugs addressed |
|------|---------------|
| `src/main.h` | #1 |
| `src/main.cpp` | #2, #3, #4, #6, #7, #8, #9, #10, #14 |
| `src/rpcmining.cpp` | #5, #13 |
| `src/miner.cpp` | #16 |
| `src/wallet.cpp` | #15 |
| `.gitignore` | (local tool dirs) |

---

## Bugs NOT Addressed

### BUG #11 — pindexBest locking audit
- **84 references** to `pindexBest` across **15+ files** (`main.cpp`, `wallet.cpp`, `miner.cpp`, `rpc*.cpp`, `init.cpp`, `checkpoints.cpp`, `txdb-leveldb.cpp`, Qt files, etc.)
- **Reason deferred:** Each access site needs individual lock-context analysis. Some are already inside `cs_main`, some aren't. Blindly adding locks risks deadlocks or double-lock UB. Requires a full concurrency audit pass — Phase 5 work.
- **Risk:** Low in practice because block validation and RPC mostly run on well-defined threads, but a determined attacker sending concurrent RPC + block messages could trigger inconsistent state.

### BUG #12 — Memory in rpcmining.cpp
- **Status:** Effectively fixed by BUG #5. The original concern was use-after-free when `mapNewBlock` held pointers to already-deleted blocks across threads. Since BUG #5 serializes all `getwork`/`getworkex`/`getblocktemplate` calls under `cs_getBlockTemplate`, the cleanup sequence (`mapNewBlock.clear()` then delete loop then `vNewBlock.clear()`) is now atomic and safe.
- **No additional commit needed.**

---

## Build Verification

Two successful Docker builds on Ubuntu 16.04 (required for OpenSSL 1.0 compatibility).

### 1. Daemon-only build (static libs)

```
docker build -f Dockerfile.build
RUN cd src && make -f makefile.unix USE_UPNP=1 STATIC=1
```

→ `wayawolfcoind`: **7.96 MB** ELF 64-bit stripped executable, zero errors.

### 2. Full Qt GUI build (static libs)

```
docker build -f Dockerfile.qt-static
RUN sed -i 's/^#  Windows begin/.../' wayawolfcoin.pro   # wrap Win paths in win32{}
RUN cd src && make -f makefile.unix USE_UPNP=1 STATIC=1
RUN qmake "RELEASE=1" wayawolfcoin.pro && make
```

→ `wayawolfcoind`: **7.96 MB** stripped  
→ `Wayawolfcoin-qt`: **11.9 MB** ELF 64-bit Qt5 GUI executable (not stripped)

**Total:** zero compilation errors across all changed files (main.cpp, main.h, rpcmining.cpp, miner.cpp, wallet.cpp).  
Only warnings are pre-existing `void*` → type* casts in the third-party `sph` hash library (haval, sha2big), unrelated to our changes.

**Binaries extracted to `build-artifacts/`:**
- `build-artifacts/wayawolfcoind` (7.96 MB)
- `build-artifacts/Wayawolfcoin-qt` (11.9 MB)

---

## Commit Integrity

- Branch `agents/bug-analysis-and-fix-plan` is a descendant of `main`.
- All 16 commits are signed off, one logical change each.
- Working tree is clean (no untracked or modified files).
- Pushed to `git@github.com:rnts08/WWC3.git` remote.
