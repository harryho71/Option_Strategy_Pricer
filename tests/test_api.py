"""
Option Strategy Pricer — full test suite.

Sections
--------
1. Smoke          : health, frontend, strategies list
2. Single options : parametrized price/delta/gamma over 10 scenarios
3. ATM accuracy   : numerical checks against Black-Scholes reference values
4. Error handling : 400 responses for bad requests
5. Portfolios     : named strategy tests (straddle, spreads, condor, etc.)
6. Sensitivity    : Greeks across a range of spot prices
7. Greeks surface : /api/greeks/surface endpoint
8. Black-Scholes  : pure-math reference tests (no server) + API cross-checks
9. American opts  : binomial-tree pricing/Greeks, put/call parity checks, mixed portfolio

Pure-math tests in section 8 run without a server.
All other tests auto-skip when the pricing server is unavailable (see conftest.py).
"""
import math
import pytest
import requests

pytest.importorskip("scipy", reason="scipy required for Black-Scholes reference tests")
from scipy.stats import norm  # noqa: E402


# ===========================================================================
# Domain data
# ===========================================================================

OPTION_SCENARIOS = [
    {"id": "atm_call",   "type": "call", "spot": 100, "strike": 100, "rate": 0.05, "sigma": 0.20, "time": 1.0},
    {"id": "atm_put",    "type": "put",  "spot": 100, "strike": 100, "rate": 0.05, "sigma": 0.20, "time": 1.0},
    {"id": "otm_call",   "type": "call", "spot": 100, "strike": 110, "rate": 0.05, "sigma": 0.20, "time": 1.0},
    {"id": "otm_put",    "type": "put",  "spot": 100, "strike":  90, "rate": 0.05, "sigma": 0.20, "time": 1.0},
    {"id": "itm_call",   "type": "call", "spot": 100, "strike":  90, "rate": 0.05, "sigma": 0.20, "time": 1.0},
    {"id": "itm_put",    "type": "put",  "spot": 100, "strike": 110, "rate": 0.05, "sigma": 0.20, "time": 1.0},
    {"id": "low_vol",    "type": "call", "spot": 100, "strike": 100, "rate": 0.05, "sigma": 0.05, "time": 1.0},
    {"id": "high_vol",   "type": "call", "spot": 100, "strike": 100, "rate": 0.05, "sigma": 0.50, "time": 1.0},
    {"id": "short_term", "type": "call", "spot": 100, "strike": 100, "rate": 0.05, "sigma": 0.20, "time": 0.019},
    {"id": "long_term",  "type": "call", "spot": 100, "strike": 100, "rate": 0.05, "sigma": 0.20, "time": 2.0},
]

STRATEGY_SCENARIOS = [
    {"id": "bull_call",   "legs": [
        {"optionType": "call", "strike": 100, "quantity":  1},
        {"optionType": "call", "strike": 110, "quantity": -1},
    ]},
    {"id": "straddle",    "legs": [
        {"optionType": "call", "strike": 100, "quantity": 1},
        {"optionType": "put",  "strike": 100, "quantity": 1},
    ]},
    {"id": "strangle",    "legs": [
        {"optionType": "call", "strike": 105, "quantity": 1},
        {"optionType": "put",  "strike":  95, "quantity": 1},
    ]},
    {"id": "iron_condor", "legs": [
        {"optionType": "call", "strike": 110, "quantity": -1},
        {"optionType": "call", "strike": 120, "quantity":  1},
        {"optionType": "put",  "strike":  90, "quantity": -1},
        {"optionType": "put",  "strike":  80, "quantity":  1},
    ]},
]


# ===========================================================================
# Helpers
# ===========================================================================

def _single_payload(s: dict) -> dict:
    return {
        "type": s["type"], "spot": s["spot"], "strike": s["strike"],
        "rate": s["rate"], "volatility": s["sigma"], "time": s["time"],
    }


def _leg(option_type: str, strike: float, qty: int,
         vol: float = 0.20, time: float = 1.0) -> dict:
    return {
        "type": "european", "optionType": option_type, "strike": strike,
        "volatility": vol, "time": time, "quantity": qty,
    }


def _price_portfolio(api_base: str, legs: list, spot: float = 100,
                     rate: float = 0.05) -> dict:
    r = requests.post(
        f"{api_base}/portfolio/price",
        json={"spot": spot, "rate": rate, "legs": legs},
        timeout=5,
    )
    assert r.status_code == 200, f"HTTP {r.status_code}: {r.text}"
    data = r.json()
    assert "error" not in data, f"API error: {data.get('error')}"
    return data["portfolio"]


def _portfolio_legs(strat: dict) -> list:
    return [_leg(l["optionType"], l["strike"], l["quantity"]) for l in strat["legs"]]


# ===========================================================================
# 1. Smoke tests
# ===========================================================================

def test_health_endpoint(server_url):
    r = requests.get(f"{server_url}/health", timeout=3)
    assert r.status_code == 200
    assert "status" in r.json()


def test_frontend_reachable():
    """Frontend is optional; skip gracefully if not running."""
    try:
        r = requests.get("http://localhost:3000", timeout=2)
        assert r.status_code == 200
    except requests.exceptions.ConnectionError:
        pytest.skip("Frontend not running on port 3000")


def test_strategies_list_not_empty(api_base):
    r = requests.get(f"{api_base}/strategies", timeout=3)
    assert r.status_code == 200
    assert len(r.json()["strategies"]) > 0


# ===========================================================================
# 2. Single option — parametrized over 10 scenarios
# ===========================================================================

@pytest.mark.parametrize("s", OPTION_SCENARIOS, ids=[s["id"] for s in OPTION_SCENARIOS])
def test_option_price_positive(api_base, s):
    r = requests.post(f"{api_base}/price", json=_single_payload(s), timeout=5)
    assert r.status_code == 200
    assert r.json()["price"] > 0


@pytest.mark.parametrize("s", OPTION_SCENARIOS, ids=[s["id"] for s in OPTION_SCENARIOS])
def test_option_all_greeks_present(api_base, s):
    r = requests.post(f"{api_base}/price", json=_single_payload(s), timeout=5)
    assert r.status_code == 200
    data = r.json()
    for field in ("price", "delta", "gamma", "vega", "theta", "rho"):
        assert field in data, f"Missing field {field!r}"


@pytest.mark.parametrize("s", OPTION_SCENARIOS, ids=[s["id"] for s in OPTION_SCENARIOS])
def test_option_delta_in_bounds(api_base, s):
    r = requests.post(f"{api_base}/price", json=_single_payload(s), timeout=5)
    assert r.status_code == 200
    delta = r.json()["delta"]
    assert -1 < delta < 1, f"delta={delta} out of (-1, 1) for {s['id']}"


@pytest.mark.parametrize("s", OPTION_SCENARIOS, ids=[s["id"] for s in OPTION_SCENARIOS])
def test_option_gamma_non_negative(api_base, s):
    r = requests.post(f"{api_base}/price", json=_single_payload(s), timeout=5)
    assert r.status_code == 200
    assert r.json()["gamma"] >= 0


# ===========================================================================
# 3. ATM accuracy
# ===========================================================================

def test_atm_call_price_and_delta(api_base):
    """S=K=100, r=5%, sigma=20%, T=1 — standard Black-Scholes benchmark."""
    r = requests.post(f"{api_base}/price", json={
        "spot": 100, "strike": 100, "rate": 0.05,
        "volatility": 0.20, "time": 1.0, "type": "call",
    }, timeout=3)
    assert r.status_code == 200
    data = r.json()
    assert data["price"] == pytest.approx(10.45, abs=0.05)
    assert data["delta"] == pytest.approx(0.6368, abs=0.01)


# ===========================================================================
# 4. Error handling
# ===========================================================================

def test_api_rejects_missing_params(api_base):
    r = requests.post(f"{api_base}/price", json={}, timeout=2)
    assert r.status_code == 400


def test_portfolio_rejects_empty_legs(api_base):
    r = requests.post(f"{api_base}/portfolio/price",
                      json={"spot": 100, "legs": []}, timeout=2)
    assert r.status_code == 400


# ===========================================================================
# 5. Portfolio strategy tests
# ===========================================================================

def test_long_straddle(api_base):
    """ATM straddle: gamma > 0, vega > 0, theta < 0.
    Net delta = 2*N(d1)-1 ≈ 0.27 when r=0.05 (not zero: straddle is delta-neutral
    only at the forward-ATM strike K=F=S*e^{rT}, not at spot-ATM K=S)."""
    p = _price_portfolio(api_base, [_leg("call", 100, 1), _leg("put", 100, 1)])
    g = p["greeks"]
    assert abs(g["delta"]) < 0.35          # bounded, but not zero when r > 0
    assert g["gamma"] > 0
    assert g["vega"] > 0
    assert g["theta"] < 0
    assert p["totalPrice"] == pytest.approx(16.02, abs=0.50)


def test_bull_call_spread(api_base):
    """Bull call spread: positive delta, net P&L capped at spread_width - net_debit.
    The payoff diagram shows P&L (intrinsic - premium), so max < spread width (10)."""
    p = _price_portfolio(api_base, [_leg("call", 100, 1), _leg("call", 110, -1)])
    assert p["greeks"]["delta"] > 0
    max_pnl = max(p["payoff"]["payoffs"])
    assert max_pnl > 0, "Bull spread should be profitable at high spot"
    assert max_pnl < 10.0, "Net P&L must be below gross spread width"


def test_long_strangle(api_base):
    """Strangle cheaper than straddle (wider strikes).
    Net delta is non-zero when r > 0 (same reason as straddle), but bounded."""
    straddle = _price_portfolio(api_base, [_leg("call", 100, 1), _leg("put", 100, 1)])
    strangle = _price_portfolio(api_base, [_leg("call", 105, 1), _leg("put",  95, 1)])
    assert strangle["totalPrice"] < straddle["totalPrice"]
    assert abs(strangle["greeks"]["delta"]) < 0.35  # bounded, not strictly zero


def test_iron_condor(api_base):
    """Iron condor: near-zero delta, negative vega (short volatility)."""
    p = _price_portfolio(api_base, [
        _leg("call", 110, -1), _leg("call", 120,  1),
        _leg("put",   90, -1), _leg("put",   80,  1),
    ])
    assert abs(p["greeks"]["delta"]) < 0.05
    assert p["greeks"]["vega"] < 0


def test_butterfly_spread(api_base):
    """Symmetric butterfly: near-zero delta, non-negative cost."""
    p = _price_portfolio(api_base, [
        _leg("call", 95, 1), _leg("call", 100, -2), _leg("call", 105, 1),
    ])
    assert p["totalPrice"] >= 0
    assert p["greeks"]["delta"] == pytest.approx(0.0, abs=0.10)


def test_collar(api_base):
    """Collar (long put + short call): net negative delta."""
    p = _price_portfolio(api_base, [_leg("put", 95, 1), _leg("call", 105, -1)])
    assert p["greeks"]["delta"] < 0


def test_large_position_scales_linearly(api_base):
    """10-lot cost must be exactly 10x 1-lot cost (linear pricing)."""
    single = _price_portfolio(api_base, [_leg("call", 100, 1),  _leg("put", 100, 1)])
    ten    = _price_portfolio(api_base, [_leg("call", 100, 10), _leg("put", 100, 10)])
    assert ten["totalPrice"] == pytest.approx(single["totalPrice"] * 10, rel=0.01)


def test_payoff_arrays_are_parallel(api_base):
    """spot_prices and payoffs must have equal length (> 0)."""
    p = _price_portfolio(api_base, [_leg("call", 100, 1), _leg("put", 100, 1)])
    payoff = p["payoff"]
    assert len(payoff["spot_prices"]) == len(payoff["payoffs"]) > 0


@pytest.mark.parametrize("strat", STRATEGY_SCENARIOS, ids=[s["id"] for s in STRATEGY_SCENARIOS])
def test_portfolio_required_fields(api_base, strat):
    """All named strategies must return totalPrice, greeks, legs, and payoff."""
    p = _price_portfolio(api_base, _portfolio_legs(strat))
    for field in ("totalPrice", "greeks", "legs", "payoff"):
        assert field in p, f"Missing field {field!r}"
    for greek in ("delta", "gamma", "vega", "theta", "rho"):
        assert greek in p["greeks"], f"Missing greek {greek!r}"


# ===========================================================================
# 6. Greeks sensitivity across spot range
# ===========================================================================

@pytest.mark.parametrize("spot", [90, 95, 100, 105, 110])
def test_straddle_greeks_valid_at_spot(api_base, spot):
    """All Greeks must be finite floats for an ATM straddle at each spot."""
    p = _price_portfolio(api_base, [
        _leg("call", 100, 1), _leg("put", 100, 1),
    ], spot=spot)
    for greek in ("delta", "gamma", "vega", "theta", "rho"):
        val = p["greeks"][greek]
        assert isinstance(val, (int, float)), f"{greek} not numeric at spot={spot}"
        assert math.isfinite(val), f"{greek}={val} is not finite at spot={spot}"


# ===========================================================================
# 7. Greeks surface endpoint
# ===========================================================================

def test_greeks_surface_endpoint(api_base):
    r = requests.get(f"{api_base}/greeks/surface", params={
        "type": "call", "strike": 100, "rate": 0.05, "volatility": 0.20,
        "spot_range": "[80, 120]", "time_range": "[0.1, 2.0]", "steps": 10,
    }, timeout=20)
    assert r.status_code in (200, 501)


# ===========================================================================
# 8. Black-Scholes analytical reference (pure math — no server required)
# ===========================================================================

def _d1(S, K, r, sigma, T):
    return (math.log(S / K) + (r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))


def _d2(S, K, r, sigma, T):
    return _d1(S, K, r, sigma, T) - sigma * math.sqrt(T)


def ref_call(S, K, r, sigma, T):
    d1, d2 = _d1(S, K, r, sigma, T), _d2(S, K, r, sigma, T)
    return S * norm.cdf(d1) - K * math.exp(-r * T) * norm.cdf(d2)


def ref_put(S, K, r, sigma, T):
    d1, d2 = _d1(S, K, r, sigma, T), _d2(S, K, r, sigma, T)
    return K * math.exp(-r * T) * norm.cdf(-d2) - S * norm.cdf(-d1)


def ref_delta(S, K, r, sigma, T, otype):
    return norm.cdf(_d1(S, K, r, sigma, T)) if otype == "call" else norm.cdf(_d1(S, K, r, sigma, T)) - 1


def ref_gamma(S, K, r, sigma, T):
    return norm.pdf(_d1(S, K, r, sigma, T)) / (S * sigma * math.sqrt(T))


def ref_vega(S, K, r, sigma, T):
    return S * norm.pdf(_d1(S, K, r, sigma, T)) * math.sqrt(T)


class TestBlackScholesFormulas:
    """Pure-math Black-Scholes checks — run without a server."""
    S, K, r, sigma, T = 100.0, 100.0, 0.05, 0.2, 1.0

    def test_atm_call_price(self):
        assert ref_call(self.S, self.K, self.r, self.sigma, self.T) == pytest.approx(10.45, abs=0.05)

    def test_atm_put_price(self):
        assert ref_put(self.S, self.K, self.r, self.sigma, self.T) == pytest.approx(5.57, abs=0.05)

    def test_put_call_parity(self):
        call = ref_call(self.S, self.K, self.r, self.sigma, self.T)
        put  = ref_put(self.S, self.K, self.r, self.sigma, self.T)
        parity = call - put - (self.S - self.K * math.exp(-self.r * self.T))
        assert parity == pytest.approx(0.0, abs=1e-10)

    def test_call_delta_between_0_and_1(self):
        assert 0 < ref_delta(self.S, self.K, self.r, self.sigma, self.T, "call") < 1

    def test_put_delta_between_minus1_and_0(self):
        assert -1 < ref_delta(self.S, self.K, self.r, self.sigma, self.T, "put") < 0

    def test_gamma_positive(self):
        assert ref_gamma(self.S, self.K, self.r, self.sigma, self.T) > 0

    def test_vega_positive(self):
        assert ref_vega(self.S, self.K, self.r, self.sigma, self.T) > 0

    def test_straddle_delta_near_zero(self):
        """Put-call parity: delta_put = delta_call - 1.
        Straddle delta = 2*N(d1)-1 ≈ 0.274 for ATM-spot when r=0.05 (not zero)."""
        cd = ref_delta(self.S, self.K, self.r, self.sigma, self.T, "call")
        pd = ref_delta(self.S, self.K, self.r, self.sigma, self.T, "put")
        # Verify put-call delta parity exactly
        assert pd == pytest.approx(cd - 1, abs=1e-10)
        # Straddle delta = 2*N(d1)-1; must be in (-1, 1) and positive when r > 0
        straddle_delta = cd + pd
        d1 = _d1(self.S, self.K, self.r, self.sigma, self.T)
        assert straddle_delta == pytest.approx(2 * norm.cdf(d1) - 1, abs=0.01)

    def test_straddle_gamma_and_vega(self):
        assert 2 * ref_gamma(self.S, self.K, self.r, self.sigma, self.T) == pytest.approx(0.0376, abs=0.005)
        assert 2 * ref_vega(self.S, self.K, self.r, self.sigma, self.T)  == pytest.approx(75.05, abs=1.0)


class TestAPIvsBlackScholes:
    """Cross-check C++ API output against the analytical reference."""
    S, K, r, sigma, T = 100.0, 100.0, 0.05, 0.2, 1.0
    TOL = 0.01

    def _api(self, api_base, option_type):
        resp = requests.post(f"{api_base}/price", json={
            "spot": self.S, "strike": self.K, "rate": self.r,
            "volatility": self.sigma, "time": self.T, "type": option_type,
        }, timeout=3)
        assert resp.status_code == 200
        return resp.json()

    def test_call_price(self, api_base):
        assert self._api(api_base, "call")["price"] == pytest.approx(
            ref_call(self.S, self.K, self.r, self.sigma, self.T), abs=self.TOL)

    def test_call_delta(self, api_base):
        assert self._api(api_base, "call")["delta"] == pytest.approx(
            ref_delta(self.S, self.K, self.r, self.sigma, self.T, "call"), abs=self.TOL)

    def test_call_gamma(self, api_base):
        assert self._api(api_base, "call")["gamma"] == pytest.approx(
            ref_gamma(self.S, self.K, self.r, self.sigma, self.T), abs=self.TOL)

    def test_put_price(self, api_base):
        assert self._api(api_base, "put")["price"] == pytest.approx(
            ref_put(self.S, self.K, self.r, self.sigma, self.T), abs=self.TOL)

    def test_put_delta(self, api_base):
        assert self._api(api_base, "put")["delta"] == pytest.approx(
            ref_delta(self.S, self.K, self.r, self.sigma, self.T, "put"), abs=self.TOL)


# ===========================================================================
# 9. American options
# ===========================================================================

AMERICAN_SCENARIOS = [
    {"id": "am_atm_call", "type": "call", "spot": 100, "strike": 100, "rate": 0.05, "sigma": 0.20, "time": 1.0},
    {"id": "am_atm_put",  "type": "put",  "spot": 100, "strike": 100, "rate": 0.05, "sigma": 0.20, "time": 1.0},
    {"id": "am_itm_put",  "type": "put",  "spot": 100, "strike": 110, "rate": 0.05, "sigma": 0.20, "time": 1.0},
    {"id": "am_otm_put",  "type": "put",  "spot": 100, "strike":  90, "rate": 0.05, "sigma": 0.20, "time": 1.0},
    {"id": "am_itm_call", "type": "call", "spot": 100, "strike":  90, "rate": 0.05, "sigma": 0.20, "time": 1.0},
]


def _american_payload(s: dict) -> dict:
    return {**_single_payload(s), "model": "american"}


@pytest.mark.parametrize("s", AMERICAN_SCENARIOS, ids=[s["id"] for s in AMERICAN_SCENARIOS])
def test_american_price_positive(api_base, s):
    r = requests.post(f"{api_base}/price", json=_american_payload(s), timeout=15)
    assert r.status_code == 200
    data = r.json()
    assert data["price"] > 0
    assert data.get("model") == "american"


@pytest.mark.parametrize("s", AMERICAN_SCENARIOS, ids=[s["id"] for s in AMERICAN_SCENARIOS])
def test_american_all_greeks_present(api_base, s):
    r = requests.post(f"{api_base}/price", json=_american_payload(s), timeout=15)
    assert r.status_code == 200
    data = r.json()
    for field in ("price", "delta", "gamma", "vega", "theta", "rho"):
        assert field in data, f"Missing field {field!r} for {s['id']}"


@pytest.mark.parametrize("s", AMERICAN_SCENARIOS, ids=[s["id"] for s in AMERICAN_SCENARIOS])
def test_american_delta_in_bounds(api_base, s):
    r = requests.post(f"{api_base}/price", json=_american_payload(s), timeout=15)
    assert r.status_code == 200
    delta = r.json()["delta"]
    assert -1 < delta < 1, f"delta={delta} out of (-1, 1) for {s['id']}"


def test_american_put_ge_european_put(api_base):
    """American put >= European put due to early exercise premium."""
    params = {
        "spot": 100, "strike": 110, "rate": 0.05,
        "volatility": 0.20, "time": 1.0, "type": "put",
    }
    eu = requests.post(f"{api_base}/price", json={**params, "model": "european"}, timeout=5).json()
    am = requests.post(f"{api_base}/price", json={**params, "model": "american"}, timeout=15).json()
    assert am["price"] >= eu["price"] - 0.001, (
        f"American put {am['price']:.4f} < European put {eu['price']:.4f}"
    )


def test_american_call_equals_european_call_no_dividends(api_base):
    """American call == European call when there are no dividends (Black 1975)."""
    params = {
        "spot": 100, "strike": 100, "rate": 0.05,
        "volatility": 0.20, "time": 1.0, "type": "call",
    }
    eu = requests.post(f"{api_base}/price", json={**params, "model": "european"}, timeout=5).json()
    am = requests.post(f"{api_base}/price", json={**params, "model": "american"}, timeout=15).json()
    assert am["price"] == pytest.approx(eu["price"], rel=0.02), (
        f"American call {am['price']:.4f} != European call {eu['price']:.4f} (no-dividend parity violated)"
    )


def test_american_portfolio_model_echoed(api_base):
    """model field must be echoed back correctly for each leg in the portfolio response."""
    payload = {
        "spot": 100,
        "rate": 0.05,
        "legs": [
            {"type": "american", "optionType": "put",  "strike": 100, "volatility": 0.2, "time": 1.0, "quantity": 1},
            {"type": "european", "optionType": "call", "strike": 100, "volatility": 0.2, "time": 1.0, "quantity": 1},
        ],
    }
    r = requests.post(f"{api_base}/portfolio/price", json=payload, timeout=20)
    assert r.status_code == 200, f"HTTP {r.status_code}: {r.text}"
    legs = r.json()["portfolio"]["legs"]
    assert len(legs) == 2
    assert legs[0].get("model") == "american", f"leg[0] model={legs[0].get('model')!r}"
    assert legs[1].get("model") == "european", f"leg[1] model={legs[1].get('model')!r}"


def test_american_portfolio_mixed_price_positive(api_base):
    """A mixed American/European portfolio should price all legs without error."""
    payload = {
        "spot": 100,
        "rate": 0.05,
        "legs": [
            {"type": "american", "optionType": "put",  "strike": 105, "volatility": 0.25, "time": 0.5,  "quantity":  1},
            {"type": "european", "optionType": "call", "strike":  95, "volatility": 0.20, "time": 1.0,  "quantity": -1},
        ],
    }
    r = requests.post(f"{api_base}/portfolio/price", json=payload, timeout=20)
    assert r.status_code == 200, f"HTTP {r.status_code}: {r.text}"
    data = r.json()
    assert "error" not in data
    for leg in data["portfolio"]["legs"]:
        assert leg.get("price", 0) > 0
