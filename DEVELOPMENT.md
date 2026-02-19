# Development Guide

This document covers the architecture, design patterns, build system, C++ internals, and extension points for the Option Strategy Pricer.

---

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────────┐
│  React Dashboard  (src/frontend)                                 │
│  App.js → MultiLegStrategy.js  →  services/api.js (axios)       │
└───────────────────────────┬──────────────────────────────────────┘
                            │  HTTP / JSON  (port 8080)
┌───────────────────────────▼──────────────────────────────────────┐
│  REST Server  (src/cpp/src/main_server.cpp + cpp-httplib)        │
│  POST /api/price          → PricingEndpoint::handlePriceRequest  │
│  POST /api/strategy/price → PricingEndpoint::handleStrategyRequest│
│  POST /api/portfolio/price→ PricingEndpoint::handlePortfolioRequest│
│  GET  /api/greeks/surface → PricingEndpoint::handleGreeksSurface │
│  GET  /api/strategies     → PricingEndpoint::handleStrategiesList│
│  GET  /health                                                    │
└───────────────────────────┬──────────────────────────────────────┘
                            │  C++ standard library calls
┌───────────────────────────▼──────────────────────────────────────┐
│  Pricing Engine  (src/cpp/src + src/cpp/include)                 │
│  models/BlackScholes   — closed-form BS + Greeks                 │
│  models/Greeks         — aggregate Greeks utilities              │
│  options/EuropeanOption — Black-Scholes option pricing           │
│  options/AmericanOption — CRR binomial tree option pricing       │
│  strategy/BullCall, IronCondor, …  — composite strategies        │
└──────────────────────────────────────────────────────────────────┘
```

---

## C++ Design Patterns

### Strategy / Polymorphism

```cpp
// Option base (src/cpp/include/options/Option.h)
class Option {
protected:
    double spot_, strike_, rate_, sigma_, time_;
    std::string type_;
public:
    virtual double price() const = 0;
    virtual double delta() const = 0;
    virtual double gamma() const = 0;
    virtual double vega()  const = 0;
    virtual double theta() const = 0;
    virtual double rho()   const = 0;
};

// Closed-form implementation
class EuropeanOption : public Option { /* Black-Scholes */ };

// Strategy base (src/cpp/include/strategy/Strategy.h)
struct Leg {
    std::shared_ptr<Option> option;
    int quantity;          // positive = long, negative = short
    double initialPremium; // per-unit price at construction time
};

class Strategy {
protected:
    std::vector<Leg> legs_;
public:
    void addLeg(std::shared_ptr<Option> opt, int qty, double premium = 0.0);
    virtual double price() const;   // sums leg prices × quantities
    virtual double delta() const;   // sums leg deltas
    // … Gamma, Vega, Theta, Rho
};
```

### Factory pattern

```cpp
// src/cpp/include/options/OptionFactory.h
auto opt = OptionFactory::create("european", S, K, r, sigma, T, "call");

// src/cpp/include/strategy/StrategyFactory.h
auto strat = StrategyFactory::create("iron_condor", S, K, r, sigma, T, true);
```

### Composite pattern

Multi-leg `Strategy` holds a `std::vector<Leg> legs_` where each `Leg` wraps an `Option` pointer, a signed quantity, and an initial premium.  
Greeks aggregate linearly across legs; payoffs sum pointwise over a spot grid.

---

## Black-Scholes Implementation

**File:** `src/cpp/include/models/BlackScholes.h` / `src/cpp/src/models/BlackScholes.cpp`

```cpp
// Key formulas (all Greeks closed-form):
static double callPrice(double S, double K, double r, double sigma, double T);
static double putPrice (double S, double K, double r, double sigma, double T);
static double delta    (double S, double K, double r, double sigma, double T, const std::string& type);
static double gamma    (double S, double K, double r, double sigma, double T);
static double vega     (double S, double K, double r, double sigma, double T);  // per 1% move
static double theta    (double S, double K, double r, double sigma, double T, const std::string& type); // per day
static double rho      (double S, double K, double r, double sigma, double T, const std::string& type);
```

Normal CDF is computed via `std::erfc()` — no external math library required.

---

## REST API Layer

**Entry point:** `src/cpp/src/main_server.cpp`  
**Logic:** `src/cpp/src/api/PricingEndpoint.cpp`  
**Serialisation:** `src/cpp/src/api/JsonSerializer.cpp` (nlohmann/json)

CORS headers are injected by `setCorsHeaders()` on every response, allowing the React dev server to call the API without a proxy.

### Request / response JSON shapes

**`POST /api/price`**

```json
// Request
{ "type": "call", "spot": 100, "strike": 100, "rate": 0.05, "volatility": 0.2, "time": 1.0 }

// Response
{ "price": 10.45, "delta": 0.6368, "gamma": 0.0188, "vega": 37.52, "theta": -6.41, "rho": 53.23 }
```

**`POST /api/portfolio/price`**

Each leg accepts an optional `"type"` field (`"european"` or `"american"`; default `"european"`).
American legs also accept an optional `"steps"` field (binomial-tree steps, default 100).

```json
// Request (ATM straddle — European call + European put)
{
  "spot": 100, "rate": 0.05, "payoff_steps": 100,
  "legs": [
    { "type": "european", "optionType": "call", "strike": 100, "volatility": 0.2, "time": 1.0, "quantity": 1 },
    { "type": "european", "optionType": "put",  "strike": 100, "volatility": 0.2, "time": 1.0, "quantity": 1 }
  ]
}
// Response
{
  "portfolio": {
    "totalPrice": 16.02,
    "greeks": { "delta": 0.274, "gamma": 0.038, "vega": 75.04, "theta": -12.82, "rho": 10.47 },
    "legs": [
      { "model": "european", "type": "call", "price": 10.45, "delta": 0.637, "gamma": 0.019, "vega": 37.52, "theta": -6.41, "rho": 53.23 },
      { "model": "european", "type": "put",  "price": 5.57,  "delta": -0.363, "gamma": 0.019, "vega": 37.52, "theta": -6.41, "rho": -42.76 }
    ],
    "payoff": { "spot_prices": [...], "payoffs": [...] }
  }
}
```

> ATM straddle delta ≈ 2·N(d₁)−1 ≈ 0.274 when r = 5%. Payoff values are **net P&L** (intrinsic at expiry minus premium paid).

---

## Build System

**File:** `CMakeLists.txt`

Two CMake targets:

| Target           | Sources                                            | Purpose            |
| ---------------- | -------------------------------------------------- | ------------------ |
| `pricing_server` | `CORE_SOURCES` + `main_server.cpp`                 | HTTP server binary |
| `test_runner`    | `CORE_SOURCES` + `tests/cpp/test_blackscholes.cpp` | Model validation   |

`CORE_SOURCES` includes all `.cpp` files under `src/cpp/src/`.  
Include search paths: `src/cpp/include`, `src/cpp/include/nlohmann`, `tests/cpp`, `tests/cpp/fixtures`.

### Build commands

```bash
# Configure and build (from repo root)
mkdir build && cd build
cmake ..
cmake --build .

# Run unit tests
./build/test_runner.exe         # Windows
./build/test_runner             # Linux / macOS

# Start the pricing server
./build/pricing_server.exe      # Windows
./build/pricing_server          # Linux / macOS
```

All output goes directly to `build/` — there is no `Release/` subdirectory.

### Dependencies (header-only — no Conan required)

| Library       | Location                        | Purpose            |
| ------------- | ------------------------------- | ------------------ |
| cpp-httplib   | `third_party/httplib/httplib.h` | HTTP server        |
| nlohmann/json | `third_party/nlohmann/json.hpp` | JSON serialisation |

Do **not** move or rename these — include paths in `CMakeLists.txt` point directly to these locations.

---

## Frontend

**Framework:** React 18 + axios + Plotly.js  
**Entry:** `src/frontend/src/App.js` → `components/MultiLegStrategy.js`  
**API calls:** `src/frontend/src/services/api.js` (axios, `baseURL: http://localhost:8080`)

Key environment variable:

```bash
REACT_APP_API_URL=http://localhost:8080   # default; override in .env.local
```

Copy `src/frontend/.env.example` to `src/frontend/.env.local` to customise the API URL without touching tracked files.

### Expired-leg filtering

`MultiLegStrategy.js` exposes an **as-of date** picker. Any leg whose `expiry` date is ≤ the as-of date is considered expired:

- The leg card is dimmed to 45% opacity and its inputs are disabled.
- A yellow warning banner lists the expired tickers.
- The leg is stripped from both the `/api/portfolio/price` call and the Greeks/payoff `useEffect` — it has zero impact on pricing.

### Running the frontend

```bash
cd src/frontend
npm install        # first time
npm start          # dev server on port 3000
npm run build      # production bundle → src/frontend/build/
```

---

## Testing

### C++ unit tests

Located in `tests/cpp/`. Built as the `test_runner` CMake target.

| File                    | What it tests                                      |
| ----------------------- | -------------------------------------------------- |
| `test_blackscholes.cpp` | ATM call price ≈ $10.45, delta ≈ 0.64              |
| `test_greeks.cpp`       | Delta bounds (−1 to 1), put-call parity for Greeks |
| `test_options.cpp`      | European call/put pricing bounds                   |
| `test_strategies.cpp`   | Straddle, Bull Call, Iron Condor payoffs           |

Reference values come from `tests/cpp/fixtures/test_data.h`.

### Python integration tests

All Python integration tests live in `tests/test_api.py` (97 tests). `tests/conftest.py` provides the session-scoped `server_url` fixture; server-dependent tests are auto-skipped when nothing is listening on port 8080.

```bash
# Start the server first, then in another terminal:
python -m venv .venv
# Windows:  .venv\Scripts\activate
# Unix:     source .venv/bin/activate
pip install -r requirements.txt

pytest tests/ -v                 # run all 97 tests
pytest tests/ -v -k "portfolio" # run a subset by keyword
```

| Test section            | Description                                                    |
| ----------------------- | -------------------------------------------------------------- |
| Smoke                   | Server health check                                            |
| Single option           | 10 parametrised call/put cases                                 |
| ATM accuracy            | Compare against scipy Black-Scholes reference                  |
| Error handling          | HTTP 400 for missing / invalid params                          |
| Portfolio strategies    | Straddle, strangle, bull spread, iron condor                   |
| Greeks sensitivity      | Delta direction, vega positivity, theta decay                  |
| Greeks surface          | 3-D vol surface data shape                                     |
| Black-Scholes pure math | Put-call parity, straddle delta, time decay (no server needed) |
| American options        | Binomial-tree put ≥ European put (early-exercise premium)      |

---

## Adding a New Strategy

1. **Header** — `src/cpp/include/strategy/MyStrategy.h`  
   Derive from `Strategy`, declare constructor and any override methods.

2. **Implementation** — `src/cpp/src/strategy/MyStrategy.cpp`  
   Push legs into `legs_` in the constructor:

   ```cpp
   addLeg(std::make_shared<EuropeanOption>(S, K_low,  r, sigma, T, "call"), +1);
   addLeg(std::make_shared<EuropeanOption>(S, K_high, r, sigma, T, "call"), -1);
   ```

3. **Factory** — Register the name in `StrategyFactory::create()`.

4. **CMakeLists.txt** — Add `src/cpp/src/strategy/MyStrategy.cpp` to `CORE_SOURCES`.

5. **Tests** — Add cases to `tests/cpp/test_strategies.cpp`.

---

## Potential Extensions

| Feature                      | Notes                                                              |
| ---------------------------- | ------------------------------------------------------------------ |
| American options             | Fully implemented — CRR binomial tree, `steps` param (default 100) |
| Implied volatility           | Newton-Raphson solver; header stub in `include/models/`            |
| Greeks surface 3D            | Plotly surface chart in `MultiLegStrategy.js`                      |
| Butterfly / Calendar spreads | Follow the strategy pattern above                                  |
| Historical backtesting       | Python script consuming `/api/portfolio/price`                     |
| SIMD Greeks arrays           | Replace scalar loops in `BlackScholes.cpp` with AVX2               |
