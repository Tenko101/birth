import React, { useState } from 'react';

const Card = ({ color, value, symbol, style }) => {
  return (
    <div className={`playing-card ${color} animated-enter`} style={style}>
      <span className="card-value top-left">{value}</span>
      <div className="center-symbol">{symbol}</div>
      <span className="card-value bottom-right">{value}</span>
    </div>
  );
};

const PlayerSprite = ({ name }) => (
  <div className="player-sprite-placeholder" title="Add custom sprite here later!">
    <div>{name} <br/> (Sprite)</div>
  </div>
);

function App() {
  const [hand, setHand] = useState([
    { id: 1, color: 'red', value: '7', symbol: '♠' },
    { id: 2, color: 'blue', value: 'Skip', symbol: '⊘' },
    { id: 3, color: 'green', value: 'A', symbol: '♥' },
    { id: 4, color: 'yellow', value: 'K', symbol: '♣' },
    { id: 5, color: 'red', value: 'Draw 2', symbol: '+2' }
  ]);
  
  const [discard, setDiscard] = useState({ color: 'blue', value: '5', symbol: '♦' });

  const drawCard = () => {
    const colors = ['red', 'blue', 'green', 'yellow'];
    const values = ['2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K', 'A', 'Skip', 'Rev'];
    const symbols = ['♠', '♥', '♦', '♣', '⊘', '↺'];
    
    const randomCard = {
      id: Date.now(),
      color: colors[Math.floor(Math.random() * colors.length)],
      value: values[Math.floor(Math.random() * values.length)],
      symbol: symbols[Math.floor(Math.random() * symbols.length)]
    };
    
    setHand([...hand, randomCard]);
  };

  const playCard = (cardId) => {
    const cardToPlay = hand.find(c => c.id === cardId);
    setDiscard(cardToPlay);
    setHand(hand.filter(c => c.id !== cardId));
  };

  return (
    <div className="app-container">
      <header className="header">
        <h1>Poker Uno</h1>
        <div className="game-controls">
          <button className="btn btn-secondary">Settings</button>
          <button className="btn">New Game</button>
        </div>
      </header>

      <main className="game-area">
        {/* Opponents Area */}
        <div className="opponent-area glass-panel">
          <PlayerSprite name="Bot 1" />
          <PlayerSprite name="Bot 2" />
          <PlayerSprite name="Bot 3" />
        </div>

        {/* Board Area */}
        <div className="board">
          <div className="deck-area">
            <div className="draw-pile" onClick={drawCard} title="Click to Draw">
              {/* Back of card pattern */}
            </div>
            
            <div className="discard-pile">
              <Card 
                color={discard.color} 
                value={discard.value} 
                symbol={discard.symbol} 
                style={{ margin: 0 }}
              />
            </div>
          </div>
        </div>

        {/* Player Area */}
        <div className="player-area">
          <div className="hand">
            {hand.map((card, index) => (
              <div key={card.id} onClick={() => playCard(card.id)}>
                <Card 
                  color={card.color} 
                  value={card.value} 
                  symbol={card.symbol}
                  style={{ zIndex: index }}
                />
              </div>
            ))}
          </div>
          
          <div style={{ display: 'flex', gap: '2rem', alignItems: 'center', marginTop: '1rem' }}>
            <PlayerSprite name="You" />
            <h2 style={{ opacity: 0.8 }}>Your Turn</h2>
          </div>
        </div>
      </main>
    </div>
  );
}

export default App;
