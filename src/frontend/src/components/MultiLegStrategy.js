import React, { useState, useEffect } from 'react';
import axios from 'axios';
import Plot from 'react-plotly.js';

const API_BASE = 'http://localhost:8080/api';

// ---------------------------------------------------------------------------
// Date / time helpers (module-level — stable across renders)
// ---------------------------------------------------------------------------
function todayISO() {
  return new Date().toISOString().split('T')[0];
}

function plusOneYear(dateStr) {
  const d = new Date(dateStr);
  d.setFullYear(d.getFullYear() + 1);
  return d.toISOString().split('T')[0];
}

/** Returns time-to-expiry in years (minimum 1 calendar day). */
function calcTimeYears(asOf, expiry) {
  const diff = new Date(expiry) - new Date(asOf);
  return Math.max(diff / (365 * 24 * 60 * 60 * 1000), 1 / 365);
}

/** Returns true when the option has expired (expiry on or before asOf). */
function isLegExpired(leg, asOf) {
  return leg.expiryDate <= asOf;
}

export function MultiLegStrategy() {
  const [asOfDate, setAsOfDate] = useState(todayISO);

  const [marketParams, setMarketParams] = useState({
    spot: 100,
    rate: 0.05
  });

  const [useCommonVolatility, setUseCommonVolatility] = useState(true);
  const [commonVolatility, setCommonVolatility] = useState(0.2);
  const [useCommonRate, setUseCommonRate] = useState(true);
  const [commonRate, setCommonRate] = useState(0.05);

  const [legs, setLegs] = useState(() => {
    const today = todayISO();
    const expiry = plusOneYear(today);
    return [
      {
        id: 1,
        type: 'european',
        optionType: 'call',
        strike: 100,
        volatility: 0.2,
        time: 1.0,
        expiryDate: expiry,
        quantity: 1,
        direction: 'long',
        rate: 0.05
      },
      {
        id: 2,
        type: 'european',
        optionType: 'put',
        strike: 100,
        volatility: 0.2,
        time: 1.0,
        expiryDate: expiry,
        quantity: 1,
        direction: 'long',
        rate: 0.05
      }
    ];
  });

  const [portfolio, setPortfolio] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [nextLegId, setNextLegId] = useState(3);
  const [greeksSensitivity, setGreeksSensitivity] = useState(null);
  const [greeksLoading, setGreeksLoading] = useState(false);
  const [activeTab, setActiveTab] = useState('greeks'); // 'greeks' | 'legs' | 'payoff' | 'delta' | 'gamma' | 'vega' | 'theta' | 'rho'

  // Adjust leg quantity based on direction
  const getEffectiveQuantity = (leg) => {
    return leg.direction === 'long' ? leg.quantity : -leg.quantity;
  };

  // Fetch portfolio pricing
  useEffect(() => {
    const fetchPortfolio = async () => {
      setLoading(true);
      setError(null);
      try {
        const activeLegsList = legs.filter(leg => !isLegExpired(leg, asOfDate));
        if (activeLegsList.length === 0) {
          setPortfolio(null);
          setLoading(false);
          return;
        }
        const legsWithEffectiveQty = activeLegsList.map(leg => ({
          ...leg,
          quantity: getEffectiveQuantity(leg),
          volatility: useCommonVolatility ? commonVolatility : leg.volatility
        }));

        const payload = {
          spot: marketParams.spot,
          rate: useCommonRate ? commonRate : marketParams.rate,
          legs: legsWithEffectiveQty,
          payoff_steps: 100
        };

        const response = await axios.post(`${API_BASE}/portfolio/price`, payload);
        setPortfolio(response.data.portfolio);
      } catch (err) {
        setError(`Error: ${err.message}`);
        console.error('Portfolio fetch error:', err);
      } finally {
        setLoading(false);
      }
    };

    const timer = setTimeout(fetchPortfolio, 300);
    return () => clearTimeout(timer);
  }, [marketParams, legs, useCommonVolatility, commonVolatility, useCommonRate, commonRate]);

  // Generate Greeks sensitivity curves
  useEffect(() => {
    const generateGreeksSensitivity = async () => {
      const activeLegsList = legs.filter(leg => !isLegExpired(leg, asOfDate));
      if (!portfolio || activeLegsList.length === 0) return;
      
      setGreeksLoading(true);
      try {
        const minSpot = marketParams.spot * 0.7;
        const maxSpot = marketParams.spot * 1.3;
        const steps = 30;
        const spotPrices = [];
        const deltas = [];
        const gammas = [];
        const vegas = [];
        const thetas = [];
        const rhos = [];

        for (let i = 0; i <= steps; i++) {
          const s = minSpot + (i / steps) * (maxSpot - minSpot);
          spotPrices.push(s);

          try {
            const legsWithEffectiveQty = activeLegsList.map(leg => ({
              ...leg,
              quantity: getEffectiveQuantity(leg),
              volatility: useCommonVolatility ? commonVolatility : leg.volatility
            }));

            const payload = {
              spot: s,
              rate: useCommonRate ? commonRate : marketParams.rate,
              legs: legsWithEffectiveQty,
              payoff_steps: 10
            };
            
            const response = await axios.post(`${API_BASE}/portfolio/price`, payload);
            const greeks = response.data.portfolio.greeks;
            
            deltas.push(greeks.delta);
            gammas.push(greeks.gamma);
            vegas.push(greeks.vega);
            thetas.push(greeks.theta);
            rhos.push(greeks.rho);
          } catch (err) {
            console.error('Error fetching Greeks for spot', s);
          }
        }

        setGreeksSensitivity({
          spotPrices,
          deltas,
          gammas,
          vegas,
          thetas,
          rhos
        });
      } catch (err) {
        console.error('Error generating Greeks sensitivity:', err);
      } finally {
        setGreeksLoading(false);
      }
    };

    const timer = setTimeout(generateGreeksSensitivity, 500);
    return () => clearTimeout(timer);
  }, [portfolio, marketParams.rate, legs, useCommonVolatility, commonVolatility, useCommonRate, commonRate]);

  const handleMarketParamChange = (key, value) => {
    const parsed = parseFloat(value);
    if (isNaN(parsed)) return;
    setMarketParams(prev => ({
      ...prev,
      [key]: parsed
    }));
  };

  const handleAsOfDateChange = (value) => {
    setAsOfDate(value);
    setLegs(prev => prev.map(leg => ({
      ...leg,
      time: calcTimeYears(value, leg.expiryDate)
    })));
  };

  const handleLegChange = (legId, key, value) => {
    setLegs(prev => prev.map(leg => {
      if (leg.id !== legId) return leg;
      if (key === 'expiryDate') {
        return { ...leg, expiryDate: value, time: calcTimeYears(asOfDate, value) };
      }
      return {
        ...leg,
        [key]: (key === 'optionType' || key === 'type' || key === 'direction') ? value : parseFloat(value)
      };
    }));
  };

  const addLeg = () => {
    const expiry = plusOneYear(asOfDate);
    const newLeg = {
      id: nextLegId,
      type: 'european',
      optionType: 'call',
      strike: marketParams.spot,
      volatility: useCommonVolatility ? commonVolatility : 0.2,
      time: calcTimeYears(asOfDate, expiry),
      expiryDate: expiry,
      quantity: 1,
      direction: 'long',
      rate: useCommonRate ? commonRate : marketParams.rate
    };
    setLegs(prev => [...prev, newLeg]);
    setNextLegId(nextLegId + 1);
  };

  const removeLeg = (legId) => {
    if (legs.length > 1) {
      setLegs(prev => prev.filter(leg => leg.id !== legId));
    }
  };

  const renderGreeksGrid = () => {
    if (!portfolio || !portfolio.greeks) return null;

    const greekItems = [
      { label: 'Delta (Δ)', value: portfolio.greeks.delta, description: '💱 Price sensitivity' },
      { label: 'Gamma (Γ)', value: portfolio.greeks.gamma, description: '⚡ Delta acceleration' },
      { label: 'Vega (ν)', value: portfolio.greeks.vega, description: '🌊 Vol sensitivity' },
      { label: 'Theta (Θ)', value: portfolio.greeks.theta, description: '⏳ Time decay/day' },
      { label: 'Rho (ρ)', value: portfolio.greeks.rho, description: '🏦 Rate sensitivity' }
    ];

    return (
      <div className="greeks-grid">
        {greekItems.map((greek, idx) => (
          <div key={idx} className="greek-card">
            <div className="greek-label">{greek.label}</div>
            <div className="greek-value">{greek.value.toFixed(4)}</div>
            <div className="greek-description">{greek.description}</div>
          </div>
        ))}
      </div>
    );
  };

  const renderPayoffChart = () => {
    if (!portfolio || !portfolio.payoff) return null;

    return (
      <div style={{ width: '100%', display: 'flex', justifyContent: 'center' }}>
        <Plot
          data={[
            {
              x: portfolio.payoff.spot_prices,
              y: portfolio.payoff.payoffs,
              type: 'scatter',
              mode: 'lines',
              name: 'Portfolio Payoff',
              line: { color: '#2563eb', width: 3 },
              fill: 'tozeroy'
            }
          ]}
          layout={{
            title: 'Multi-Leg Portfolio Payoff at Expiration',
            xaxis: { title: 'Spot Price at Expiration ($)' },
            yaxis: { title: 'Profit/Loss ($)' },
            hovermode: 'closest',
            autosize: true,
            height: 500,
            plot_bgcolor: '#f3f4f6',
            paper_bgcolor: '#ffffff',
            margin: { t: 50, b: 50, l: 60, r: 30 }
          }}
          config={{ responsive: true }}
          style={{ width: '100%' }}
          useResizeHandler={true}
        />
      </div>
    );
  };

  const renderIndividualGreekChart = (greekType) => {
    if (!greeksSensitivity) return null;

    const greekConfig = {
      delta: { label: 'Delta (Δ)', color: '#667eea', data: greeksSensitivity.deltas, ylabel: 'Portfolio Delta' },
      gamma: { label: 'Gamma (Γ)', color: '#764ba2', data: greeksSensitivity.gammas, ylabel: 'Portfolio Gamma' },
      vega: { label: 'Vega (ν)', color: '#ff922b', data: greeksSensitivity.vegas, ylabel: 'Portfolio Vega' },
      theta: { label: 'Theta (Θ)', color: '#ff6b6b', data: greeksSensitivity.thetas, ylabel: 'Portfolio Theta (per day)' },
      rho: { label: 'Rho (ρ)', color: '#20c997', data: greeksSensitivity.rhos, ylabel: 'Portfolio Rho' }
    };

    const config = greekConfig[greekType];
    if (!config) return null;

    return (
      <div style={{ width: '100%', display: 'flex', justifyContent: 'center' }}>
        <Plot
          data={[
            {
              x: greeksSensitivity.spotPrices,
              y: config.data,
              type: 'scatter',
              mode: 'lines+markers',
              name: config.label,
              line: { color: config.color, width: 3 },
              marker: { size: 5 }
            }
          ]}
          layout={{
            title: `${config.label} Sensitivity to Spot Price`,
            xaxis: { title: 'Spot Price ($)' },
            yaxis: { title: config.ylabel },
            hovermode: 'closest',
            autosize: true,
            height: 500,
            plot_bgcolor: '#f3f4f6',
            paper_bgcolor: '#ffffff',
            margin: { t: 50, b: 50, l: 60, r: 30 }
          }}
          config={{ responsive: true }}
          style={{ width: '100%' }}
          useResizeHandler={true}
        />
      </div>
    );
  };

  const renderGreeksSensitivityCharts = () => {
    if (!greeksSensitivity || greeksLoading) {
      return greeksLoading ? (
        <div className="loading">Generating Greeks sensitivity charts...</div>
      ) : null;
    }

    return (
      <div className="greek-charts-grid">
        <div className="greek-chart">
          <Plot
            data={[
              {
                x: greeksSensitivity.spotPrices,
                y: greeksSensitivity.deltas,
                type: 'scatter',
                mode: 'lines+markers',
                name: 'Delta',
                line: { color: '#667eea', width: 3 },
                marker: { size: 4 }
              }
            ]}
            layout={{
              title: 'Delta (Δ) Sensitivity',
              xaxis: { title: 'Spot Price ($)' },
              yaxis: { title: 'Portfolio Delta' },
              plot_bgcolor: '#f8f9fa',
              paper_bgcolor: 'white',
              height: 480,
              margin: { l: 60, r: 20, t: 40, b: 50 }
            }}
            config={{ responsive: true, displayModeBar: false }}
          />
        </div>

        <div className="greek-chart">
          <Plot
            data={[
              {
                x: greeksSensitivity.spotPrices,
                y: greeksSensitivity.gammas,
                type: 'scatter',
                mode: 'lines+markers',
                name: 'Gamma',
                line: { color: '#764ba2', width: 3 },
                marker: { size: 4 }
              }
            ]}
            layout={{
              title: 'Gamma (Γ) Sensitivity',
              xaxis: { title: 'Spot Price ($)' },
              yaxis: { title: 'Portfolio Gamma' },
              plot_bgcolor: '#f8f9fa',
              paper_bgcolor: 'white',
              height: 480,
              margin: { l: 60, r: 20, t: 40, b: 50 }
            }}
            config={{ responsive: true, displayModeBar: false }}
          />
        </div>

        <div className="greek-chart">
          <Plot
            data={[
              {
                x: greeksSensitivity.spotPrices,
                y: greeksSensitivity.vegas,
                type: 'scatter',
                mode: 'lines+markers',
                name: 'Vega',
                line: { color: '#ff922b', width: 3 },
                marker: { size: 4 }
              }
            ]}
            layout={{
              title: 'Vega (ν) Sensitivity',
              xaxis: { title: 'Spot Price ($)' },
              yaxis: { title: 'Portfolio Vega' },
              plot_bgcolor: '#f8f9fa',
              paper_bgcolor: 'white',
              height: 480,
              margin: { l: 60, r: 20, t: 40, b: 50 }
            }}
            config={{ responsive: true, displayModeBar: false }}
          />
        </div>

        <div className="greek-chart">
          <Plot
            data={[
              {
                x: greeksSensitivity.spotPrices,
                y: greeksSensitivity.thetas,
                type: 'scatter',
                mode: 'lines+markers',
                name: 'Theta',
                line: { color: '#ff6b6b', width: 3 },
                marker: { size: 4 }
              }
            ]}
            layout={{
              title: 'Theta (Θ) Sensitivity',
              xaxis: { title: 'Spot Price ($)' },
              yaxis: { title: 'Portfolio Theta (per day)' },
              plot_bgcolor: '#f8f9fa',
              paper_bgcolor: 'white',
              height: 480,
              margin: { l: 60, r: 20, t: 40, b: 50 }
            }}
            config={{ responsive: true, displayModeBar: false }}
          />
        </div>
      </div>
    );
  };

  return (
    <div className="multi-leg-strategy">

      {/* ── TOP BAR: Market Parameters ── */}
      <div className="ml-params-bar">
        <div className="params-bar-group">
          <label>📅 As of Date</label>
          <input
            type="date"
            value={asOfDate}
            onChange={(e) => handleAsOfDateChange(e.target.value)}
            className="as-of-date-input"
          />
        </div>
        <div className="params-bar-divider" />
        <div className="params-bar-group">
          <label>📈 Spot Price ($)</label>
          <input
            type="number"
            value={marketParams.spot}
            onChange={(e) => handleMarketParamChange('spot', e.target.value)}
            step="0.5"
            min="0.01"
            className="spot-price-input"
          />
        </div>
        <div className="params-bar-divider" />
        <div className="params-bar-group">
          <label>🏦 Interest Rate (r %)</label>
          <input
            type="number"
            value={useCommonRate ? (commonRate * 100).toFixed(3) : (marketParams.rate * 100).toFixed(3)}
            onChange={(e) => {
              const pct = parseFloat(e.target.value);
              if (isNaN(pct)) return;
              useCommonRate
                ? setCommonRate(pct / 100)
                : handleMarketParamChange('rate', pct / 100);
            }}
            step="0.1"
            min="0"
            max="50"
            className="spot-price-input"
          />
        </div>
        <div className="params-bar-group">
          <label>🌊 Volatility (σ %)</label>
          <input
            type="number"
            value={(commonVolatility * 100).toFixed(1)}
            onChange={(e) => {
              const pct = parseFloat(e.target.value);
              if (isNaN(pct)) return;
              setCommonVolatility(pct / 100);
            }}
            step="1"
            min="1"
            max="200"
            className="spot-price-input"
          />
        </div>
        <div className="params-bar-divider" />
        <div className="params-bar-toggles">
          <label>
            <input type="checkbox" checked={useCommonVolatility} onChange={(e) => setUseCommonVolatility(e.target.checked)} />
            🔗 Common σ
          </label>
          <label>
            <input type="checkbox" checked={useCommonRate} onChange={(e) => setUseCommonRate(e.target.checked)} />
            🔗 Common r
          </label>
        </div>
      </div>

      {/* ── MAIN: Input left | Output right ── */}
      <div className="ml-main-layout">

        {/* LEFT: Option Leg Input Cards */}
        <div className="ml-input-panel">
          <div className="section-header">
            <h3>📋 Option Legs</h3>
            <button className="btn-add-leg" onClick={addLeg}>➕ Add Leg</button>
          </div>
          {legs.some(leg => isLegExpired(leg, asOfDate)) && (
            <div className="expired-legs-warning">
              {legs.filter(leg => isLegExpired(leg, asOfDate)).length} leg(s) expired as of {asOfDate} — excluded from pricing.
            </div>
          )}
          <div className="leg-cards-list">
            {legs.map((leg, idx) => (
              <div key={leg.id} className={`leg-input-card${isLegExpired(leg, asOfDate) ? ' leg-input-card--expired' : ''}`}>
                <div className="leg-input-card-header">
                  <span className="leg-badge">Leg {idx + 1}</span>
                  <div className="leg-badge-tags">
                    <span className={`leg-tag leg-tag-${leg.direction}`}>{leg.direction}</span>
                    <span className={`leg-tag leg-tag-${leg.optionType}`}>{leg.optionType}</span>
                    {isLegExpired(leg, asOfDate) && (
                      <span className="leg-tag leg-tag-expired">Expired</span>
                    )}
                  </div>
                  {legs.length > 1 && (
                    <button className="btn-remove-row" onClick={() => removeLeg(leg.id)}>✕</button>
                  )}
                </div>
                <div className="leg-input-grid">
                  <div className="leg-input-field">
                    <label>Style</label>
                    <select value={leg.type} onChange={(e) => handleLegChange(leg.id, 'type', e.target.value)}>
                      <option value="european">European</option>
                      <option value="american">American</option>
                    </select>
                  </div>
                  <div className="leg-input-field">
                    <label>Direction</label>
                    <select value={leg.direction} onChange={(e) => handleLegChange(leg.id, 'direction', e.target.value)}>
                      <option value="long">Long</option>
                      <option value="short">Short</option>
                    </select>
                  </div>
                  <div className="leg-input-field">
                    <label>Type</label>
                    <select value={leg.optionType} onChange={(e) => handleLegChange(leg.id, 'optionType', e.target.value)}>
                      <option value="call">Call</option>
                      <option value="put">Put</option>
                    </select>
                  </div>
                  <div className="leg-input-field">
                    <label>Strike ($)</label>
                    <input type="number" value={leg.strike} onChange={(e) => handleLegChange(leg.id, 'strike', e.target.value)} step="0.5" />
                  </div>
                  <div className="leg-input-field">
                    <label>Quantity</label>
                    <input type="number" value={leg.quantity} onChange={(e) => handleLegChange(leg.id, 'quantity', e.target.value)} step="1" min="1" />
                  </div>
                  <div className="leg-input-field">
                    <label>Expiry Date</label>
                    <input type="date" value={leg.expiryDate} min={asOfDate} onChange={(e) => handleLegChange(leg.id, 'expiryDate', e.target.value)} />
                    <span className="time-calc-label">T = {leg.time.toFixed(4)}y</span>
                  </div>
                  {!useCommonVolatility && (
                    <div className="leg-input-field">
                      <label>Vol (σ)</label>
                      <input type="number" value={leg.volatility} onChange={(e) => handleLegChange(leg.id, 'volatility', e.target.value)} step="0.01" min="0.01" max="2" />
                    </div>
                  )}
                  {!useCommonRate && (
                    <div className="leg-input-field">
                      <label>Rate (r)</label>
                      <input type="number" value={leg.rate} onChange={(e) => handleLegChange(leg.id, 'rate', e.target.value)} step="0.001" min="0" max="0.5" />
                    </div>
                  )}
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* RIGHT: Output Panel */}
        <div className="ml-output-panel">

          {error && <div className="ml-section error-box">⚠️ {error}</div>}
          {loading && <div className="ml-section"><div className="loading">⏳ Loading portfolio...</div></div>}

          {portfolio && !loading && (
            <>
              {/* Summary bar */}
              <div className="ml-summary-bar">
                <div className="summary-bar-item">
                  <span className="summary-bar-label">💰 Total Cost</span>
                  <span className="summary-bar-value">${portfolio.totalPrice.toFixed(2)}</span>
                </div>
                <div className="summary-bar-divider" />
                <div className="summary-bar-item">
                  <span className="summary-bar-label">📊 Legs</span>
                  <span className="summary-bar-value">{portfolio.legs.length}</span>
                </div>
                <div className="summary-bar-divider" />
                <div className="summary-bar-item">
                  <span className="summary-bar-label">📈 Spot</span>
                  <span className="summary-bar-value">${marketParams.spot}</span>
                </div>
                <div className="summary-bar-divider" />
                <div className="summary-bar-item">
                  <span className="summary-bar-label">📅 As of</span>
                  <span className="summary-bar-value">{asOfDate}</span>
                </div>
                <div className="summary-bar-divider" />
                <div className="summary-bar-item">
                  <span className="summary-bar-label">Δ</span>
                  <span className="summary-bar-value">{portfolio.greeks.delta.toFixed(4)}</span>
                </div>
                <div className="summary-bar-divider" />
                <div className="summary-bar-item">
                  <span className="summary-bar-label">Γ</span>
                  <span className="summary-bar-value">{portfolio.greeks.gamma.toFixed(4)}</span>
                </div>
                <div className="summary-bar-divider" />
                <div className="summary-bar-item">
                  <span className="summary-bar-label">ν</span>
                  <span className="summary-bar-value">{portfolio.greeks.vega.toFixed(2)}</span>
                </div>
                <div className="summary-bar-divider" />
                <div className="summary-bar-item">
                  <span className="summary-bar-label">Θ</span>
                  <span className="summary-bar-value">{portfolio.greeks.theta.toFixed(2)}</span>
                </div>
                <div className="summary-bar-divider" />
                <div className="summary-bar-item">
                  <span className="summary-bar-label">ρ</span>
                  <span className="summary-bar-value">{portfolio.greeks.rho.toFixed(2)}</span>
                </div>
              </div>

              {/* Unified tabbed panel: Greeks | Leg Details | Charts */}
              <div className="ml-section ml-unified-panel">
                <div className="ml-tabs-wrapper">
                  <div className="ml-tabs">
                    {[
                      { key: 'greeks',  label: '📐 Greeks' },
                      { key: 'legs',    label: '📋 Leg Details' },
                      { key: 'payoff',  label: '💹 Payoff' },
                      { key: 'delta',   label: 'Δ' },
                      { key: 'gamma',   label: 'Γ' },
                      { key: 'vega',    label: 'ν' },
                      { key: 'theta',   label: 'Θ' },
                      { key: 'rho',     label: 'ρ' },
                    ].map(({ key, label }) => (
                      <button
                        key={key}
                        className={`ml-tab-button ${activeTab === key ? 'active' : ''}`}
                        onClick={() => setActiveTab(key)}
                      >
                        {label}
                      </button>
                    ))}
                  </div>
                </div>
                <div className="ml-tab-content">
                  {activeTab === 'greeks' && (
                    <div className="unified-tab-section">
                      {renderGreeksGrid()}
                    </div>
                  )}
                  {activeTab === 'legs' && (
                    <div className="unified-tab-section summary-leg-details">
                      <div className="legs-details">
                        {portfolio.legs.map((leg, idx) => {
                          const inputLeg = legs[idx] || {};
                          return (
                          <div key={idx} className="leg-detail-card">
                            <div className="leg-detail-header">
                              <span className="leg-detail-title">
                                {leg.optionType.toUpperCase()} @ ${leg.strike.toFixed(0)}
                              </span>
                              <div className="leg-detail-meta">
                                <span className={`leg-tag leg-tag-${inputLeg.direction || 'long'}`}>{inputLeg.direction || 'long'}</span>
                                <span className={`leg-tag leg-tag-${leg.optionType}`}>{leg.optionType}</span>
                                <span className="leg-detail-badge">{inputLeg.type ? (inputLeg.type.charAt(0).toUpperCase() + inputLeg.type.slice(1)) : 'European'}</span>
                                <span className="leg-detail-badge">Qty: {inputLeg.quantity ?? 1}</span>
                                <span className="leg-detail-badge">Exp: {inputLeg.expiryDate || '—'}</span>
                              </div>
                            </div>
                            <div className="leg-detail-grid">
                              <div className="detail-item"><span>Price:</span><span>${leg.price.toFixed(2)}</span></div>
                              <div className="detail-item"><span>Δ:</span><span>{leg.delta.toFixed(4)}</span></div>
                              <div className="detail-item"><span>Γ:</span><span>{leg.gamma.toFixed(4)}</span></div>
                              <div className="detail-item"><span>ν:</span><span>{leg.vega.toFixed(2)}</span></div>
                              <div className="detail-item"><span>Θ:</span><span>{leg.theta.toFixed(2)}</span></div>
                              <div className="detail-item"><span>ρ:</span><span>{leg.rho.toFixed(2)}</span></div>
                            </div>
                          </div>
                          );
                        })}
                      </div>
                    </div>
                  )}
                  {activeTab === 'payoff' && renderPayoffChart()}
                  {!['greeks', 'legs', 'payoff'].includes(activeTab) && (
                    greeksLoading
                      ? <div className="loading">⏳ Generating charts...</div>
                      : renderIndividualGreekChart(activeTab)
                  )}
                </div>
              </div>
            </>
          )}
        </div>
      </div>
    </div>
  );
}

