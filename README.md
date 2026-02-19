# Option Strategy Pricer

A full-stack quantitative finance system for pricing options and multi-leg strategies using the Black-Scholes model. The C++ pricing engine exposes a REST API consumed by a React dashboard.

```
React Dashboard (port 3000)  ──HTTP/JSON──►  C++ REST Server (port 8080)  ──►  Black-Scholes Engine
```

---

## Project Structure

```
Option_Strategy_Pricer/
├── src/
│   ├── cpp/                        # C++ pricing engine & REST server
│   │   ├── include/
│   │   │   ├── models/             # BlackScholes.h, Greeks.h, RiskMeasures.h
│   │   │   ├── options/            # Option.h, EuropeanOption.h, OptionFactory.h
│   │   │   ├── strategy/           # Strategy.h, BullCall.h, IronCondor.h, ...
│   │   │   └── api/                # PricingEndpoint.h, RestServer.h, ...
│   │   └── src/
│   │       ├── models/             # BlackScholes.cpp, Greeks.cpp, RiskMeasures.cpp
│   │       ├── options/            # EuropeanOption.cpp, AmericanOption.cpp, OptionFactory.cpp
│   │       ├── strategy/           # Strategy.cpp, BullCall.cpp, IronCondor.cpp, StrategyFactory.cpp
│   │       ├── api/                # PricingEndpoint.cpp, RestServer.cpp, JsonSerializer.cpp
│   │       └── main_server.cpp     # HTTP server entry point (cpp-httplib)
│   └── frontend/                   # React dashboard
│       ├── package.json
│       ├── public/index.html
│       └── src/
│           ├── App.js              # Root component — renders MultiLegStrategy
│           ├── components/
│           │   └── MultiLegStrategy.js  # Portfolio builder, Greeks, payoff chart
│           └── services/
│               └── api.js          # Axios instance (baseURL: localhost:8080)
├── tests/
│   ├── cpp/                        # C++ unit tests (GoogleTest)
│   │   ├── fixtures/test_data.h    # Known Black-Scholes reference values
│   │   ├── test_blackscholes.cpp
│   │   ├── test_greeks.cpp
│   │   ├── test_options.cpp
│   │   └── test_strategies.cpp
│   ├── conftest.py                 # pytest fixtures (server_url, api_base — auto-skip if server down)
│   └── test_api.py                 # All Python integration + Black-Scholes reference tests
├── third_party/
│   ├── httplib/httplib.h           # cpp-httplib v0.30.1 (header-only)
│   └── nlohmann/json.hpp           # nlohmann/json v3.11.2 (header-only)
├── .github/
│   └── copilot-instructions.md     # AI agent guidance for this codebase
├── build/                          # CMake build output (generated)
├── CMakeLists.txt                  # Builds: pricing_server, test_runner
├── conanfile.txt                   # Conan dependencies
└── .venv/                          # Python virtual environment
```

---

## Prerequisites

| Tool                                | Version | Purpose              |
| ----------------------------------- | ------- | -------------------- |
| C++17 compiler (MSVC / GCC / Clang) | —       | Build pricing engine |
| CMake                               | 3.15+   | Build system         |
| Node.js + npm                       | 16+     | React frontend       |
| Python                              | 3.8+    | Test scripts         |

---

## Quick Start

### 1 — Build the C++ server

```bash
mkdir build && cd build
cmake ..
cmake --build .

# Verify the model
./build/test_runner.exe          # Windows
./build/test_runner              # Linux / macOS
```

### 2 — Start the pricing server

```bash
./build/pricing_server.exe       # Windows
./build/pricing_server           # Linux / macOS
# Listening on http://localhost:8080
```

### 3 — Start the React dashboard

```bash
cd src/frontend
npm install      # first time only
npm start
# Dashboard at http://localhost:3000
```

### 4 — Run Python integration tests (server must be running)

```bash
python -m venv .venv
# Windows:  .venv\Scripts\activate
# Unix:     source .venv/bin/activate

pip install -r requirements.txt
pytest tests/ -v                 # 97 tests
```

---

## API Reference

All endpoints are served on `http://localhost:8080`.

### `GET /health`

Returns `{"status": "healthy", "version": "1.0.0"}`. Use to verify the server is running.

### `POST /api/price` — Single option pricing

```bash
curl -X POST http://localhost:8080/api/price \
  -H "Content-Type: application/json" \
  -d '{"type":"call","spot":100,"strike":100,"rate":0.05,"volatility":0.2,"time":1.0}'
```

```json
{
  "price": 10.45,
  "delta": 0.64,
  "gamma": 0.019,
  "vega": 39.45,
  "theta": -6.41,
  "rho": 53.23
}
```

### `POST /api/strategy/price` — Named strategy pricing

```bash
curl -X POST http://localhost:8080/api/strategy/price \
  -H "Content-Type: application/json" \
  -d '{"strategy":"straddle","spot":100,"strike":100,"is_long":true,"rate":0.05,"volatility":0.2,"time":1.0}'
```

```json
{
  "strategy": "straddle",
  "price": 16.02,
  "delta": 0.274,
  "gamma": 0.038,
  "vega": 75.04,
  "theta": -12.82
}
```

> **Note:** ATM straddle delta ≈ 2·N(d₁)−1 ≈ 0.274 when r = 5% — it equals zero only at the forward-ATM strike K = S·e^{rT}, not the spot-ATM strike.

### `POST /api/portfolio/price` — Multi-leg portfolio pricing

```bash
curl -X POST http://localhost:8080/api/portfolio/price \
  -H "Content-Type: application/json" \
  -d '{
    "spot": 100, "rate": 0.05,
    "legs": [
      {"type":"european","optionType":"call","strike":100,"volatility":0.2,"time":1.0,"quantity":1},
      {"type":"american","optionType":"put", "strike":100,"volatility":0.2,"time":1.0,"quantity":1}
    ],
    "payoff_steps": 100
  }'
```

Each leg's `type` field selects the pricing model (`"european"` or `"american"`; default `"european"`). The response echoes `model` on every leg.

Response includes `portfolio.totalPrice`, `portfolio.greeks` (Δ, Γ, ν, θ, ρ), `portfolio.legs` (per-leg price + Greeks + `model`), and `portfolio.payoff` (spot_prices + payoffs arrays). Payoff values are **net P&L** (intrinsic value minus premium paid).

### `GET /api/strategies`

Returns list of available named strategies.

### `GET /api/greeks/surface`

Returns a 2-D Greeks surface over spot and time ranges. Query params: `type`, `strike`, `rate`, `volatility`, `spot_range`, `time_range`, `steps`.

---

## Implemented Features

**Pricing models**

- Black-Scholes European option pricing (closed-form)
- American option pricing via Cox-Ross-Rubinstein binomial tree (configurable steps)
- Full Greeks for both models: Δ, Γ, ν, θ, ρ
- Normal CDF via `std::erfc()` — no external math library required
- Mixed-model portfolios (European and American legs in the same request)

**Strategies**

- Straddle — long/short call + put at same strike
- Strangle — OTM call + OTM put at different strikes
- Bull Call Spread — long lower call + short higher call
- Iron Condor — short strangle + long wider protective strangle

**Frontend**

- Multi-leg portfolio builder (add/remove legs, set per-leg expiry)
- As-of date selector — legs with expiry ≤ as-of date are dimmed and excluded from pricing
- Real-time portfolio Greeks (Δ, Γ, ν, θ, ρ)
- Payoff diagram showing net P&L across spot range (Plotly)
- Greeks sensitivity charts (Δ, Γ, ν, θ, ρ vs. spot price)
- Common σ / r toggles for quick scenario analysis

---

## Formulas

$$d_1 = \frac{\ln(S/K) + (r + \tfrac{1}{2}\sigma^2)\,T}{\sigma\sqrt{T}}, \quad d_2 = d_1 - \sigma\sqrt{T}$$

$$C = S\,N(d_1) - K\,e^{-rT}\,N(d_2), \quad P = K\,e^{-rT}\,N(-d_2) - S\,N(-d_1)$$

$$\Delta_C = N(d_1),\quad \Gamma = \frac{\phi(d_1)}{S\,\sigma\sqrt{T}},\quad \mathcal{V} = S\,\phi(d_1)\sqrt{T}$$
