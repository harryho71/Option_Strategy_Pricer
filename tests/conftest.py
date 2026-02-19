"""
Shared pytest fixtures for the Option Strategy Pricer integration test suite.

All tests that use `api_base` (or `server_url`) are automatically skipped when
the pricing server is not reachable on http://localhost:8080. Start it first:

    cd build && ./pricing_server.exe   # Windows
    cd build && ./pricing_server               # Linux / macOS
"""

import pytest
import requests


def pytest_configure(config: pytest.Config) -> None:
    config.addinivalue_line(
        "markers",
        "integration: requires a running pricing server on port 8080",
    )


@pytest.fixture(scope="session")
def server_url() -> str:
    """Return the server root URL, or skip the session if the server is down."""
    url = "http://localhost:8080"
    try:
        resp = requests.get(f"{url}/health", timeout=3)
        resp.raise_for_status()
    except Exception as exc:
        pytest.skip(
            f"Pricing server unavailable at {url} — "
            f"start pricing_server.exe first. ({exc})"
        )
    return url


@pytest.fixture(scope="session")
def api_base(server_url: str) -> str:
    """Return the /api base URL."""
    return f"{server_url}/api"
