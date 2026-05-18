import React, { useState } from 'react';

const Card = ({ color, value, symbol, style }) => {
  return (
    <div className={`playing-card ${color}`} style={style}>
      <span className="card-value top-left">{value}</span>
      <div className="center-symbol">{symbol}</div>
      <span className="card-value bottom-right">{value}</span>
    </div>
  );
};

const PlayerSprite = ({ name, cardsCount, className }) => (
  <div className={`opponent ${className}`}>
    <div className="player-sprite-placeholder" title="Add custom sprite here later!">
      {name}
    </div>
    <div className="opponent-stats">
      <div className="opponent-name">{name}</div>
      <div className="opponent-cards">Cards: {cardsCount}</div>
    </div>
  </div>
);

function App() {
  const [hand, setHand] = useState([
    { id: 1, color: 'red', value: '7', symbol: '7' },
    { id: 2, color: 'black', value: '+4', symbol: '+4' },
    { id: 3, color: 'green', value: 'A', symbol: 'A' },
    { id: 4, color: 'yellow', value: 'K', symbol: 'K' },
    { id: 5, color: 'red', value: 'Draw 2', symbol: '+2' }
  ]);
  
  const [discard, setDiscard] = useState({ color: 'blue', value: '5', symbol: '5' });
  const [discardRotation] = useState(Math.floor(Math.random() * 20) - 10);

  const drawCard = () => {
    const colors = ['red', 'blue', 'green', 'yellow', 'black'];
    const values = ['2', '5', '9', 'J', 'Q', 'K', 'A', 'Skip', 'Rev'];
    
    const randomCard = {
      id: Date.now(),
      color: colors[Math.floor(Math.random() * colors.length)],
      value: values[Math.floor(Math.random() * values.length)],
      symbol: ''
    };
    randomCard.symbol = randomCard.value === 'Skip' ? '⊘' : randomCard.value === 'Rev' ? '↺' : randomCard.value;
    
    setHand([...hand, randomCard]);
  };

  const playCard = (cardId) => {
    const cardToPlay = hand.find(c => c.id === cardId);
    setDiscard(cardToPlay);
    setHand(hand.filter(c => c.id !== cardId));
  };

  return (
    <div className="app-container">
      <div className="ambient-light"></div>
      
      <header className="header">
        <h1>UNDERGROUND UNO</h1>
      </header>

      <main className="game-area">
        
        {/* The 3D Table */}
        <div className="poker-table">
          <div className="table-ring"></div>
        </div>

        {/* Opponents seated around the table */}
        <div className="opponents-container">
          <PlayerSprite name="Bad Dog" cardsCount={4} className="left" />
          <PlayerSprite name="Butcher Pig" cardsCount={6} className="center" />
          <PlayerSprite name="Raging Bull" cardsCount={3} className="right" />
        </div>

        {/* Center of the table */}
        <div className="center-board">
          <div className="draw-pile" onClick={drawCard} title="Click to Draw"></div>
          <div className="discard-pile">
            <Card 
              color={discard.color} 
              value={discard.value} 
              symbol={discard.symbol} 
              style={{ transform: `rotate(${discardRotation}deg)` }}
            />
          </div>
        </div>

        {/* Player's perspective hand at the bottom */}
        <div className="player-hand-container">
          <div className="hand">
            {hand.map((card, index) => (
              <div key={card.id} onClick={() => playCard(card.id)}>
                <Card 
                  color={card.color} 
                  value={card.value} 
                  symbol={card.symbol}
                  style={{ zIndex: index + 1 }}
                />
              </div>
            ))}
          </div>
        </div>
        
      </main>
    </div>
  );
}

export default App;
