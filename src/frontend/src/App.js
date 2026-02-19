import React from 'react';
import './App.css';
import { MultiLegStrategy } from './components/MultiLegStrategy';

function App() {
  return (
    <div className="app">
      <header className="app-header">
        <h1>📊 Multi-Leg Option Strategy Pricer</h1>
        <p>⚡ Real-time portfolio pricing with Greeks analysis and payoff diagrams</p>
      </header>

      <div className="container">
        <MultiLegStrategy />
      </div>

      <footer className="app-footer">
        <p>🖥️ Full Stack: React Frontend → C++ REST API → Black-Scholes Pricing Engine</p>
        <p>🏗️ Architecture: Layer 1 (Frontend) → Layer 2 (REST API) → Layer 3 (Pricing Engine)</p>
      </footer>
    </div>
  );
}

export default App;
