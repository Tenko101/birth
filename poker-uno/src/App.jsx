import React, { useState, useEffect } from 'react';

const COLORS = ['red', 'blue', 'green', 'yellow'];
const VALUES = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'Skip', 'Rev', '+2'];
const PLAYERS = ['You', 'Seal One', 'Seal Two', 'Seal Three'];

const createDeck = () => {
  let newDeck = [];
  let id = 1;
  COLORS.forEach(color => {
    VALUES.forEach(value => {
      let symbol = value;
      if (value === 'Skip') symbol = '⊘';
      if (value === 'Rev') symbol = '↺';
      
      newDeck.push({ id: id++, color, value, symbol });
      if (value !== '0') {
        newDeck.push({ id: id++, color, value, symbol });
      }
    });
  });
  // Shuffle
  return newDeck.sort(() => Math.random() - 0.5);
};

const Card = ({ color, value, symbol, style, disabled }) => {
  return (
    <div className={`playing-card ${color} ${disabled ? 'disabled' : ''}`} style={style}>
      <span className="card-value top-left">{value}</span>
      <div className="center-symbol">{symbol}</div>
      <span className="card-value bottom-right">{value}</span>
    </div>
  );
};

const OpponentHand = ({ count }) => {
  return (
    <div className="opponent-hand">
      {Array.from({ length: count }).map((_, i) => {
        const offset = i - (count - 1) / 2;
        const rot = offset * 8;
        const y = Math.abs(offset) * 3;
        return <div key={i} className="opponent-card-back" style={{ transform: `rotate(${rot}deg) translateY(${y}px)` }} />
      })}
    </div>
  );
};

const PlayerSprite = ({ name, cardsCount, className, isActive, expression, spriteId }) => {
  let currentExpression = expression;
  if (!currentExpression && cardsCount === 1) currentExpression = 'uno';

  let imageSuffix = 'default';
  if (currentExpression === 'uno') imageSuffix = 'uno';
  else if (currentExpression === 'got-skipped') imageSuffix = 'skipped';
  else if (currentExpression === 'played-action') imageSuffix = 'action';
  
  const basePath = import.meta.env.BASE_URL || '/';
  const imagePath = `${basePath}${spriteId}_${imageSuffix}.png`;

  return (
    <div className={`opponent ${className} ${isActive ? 'active-turn' : ''}`}>
      <div className={`player-sprite-container ${currentExpression}`} title={`Drop ${imagePath} in public folder`}>
        <img 
          src={imagePath} 
          alt={name}
          className="player-sprite-image" 
        />
      </div>
      <OpponentHand count={cardsCount} />
    </div>
  );
};

function App() {
  const [deck, setDeck] = useState([]);
  const [discardPile, setDiscardPile] = useState([]);
  const [hands, setHands] = useState([[], [], [], []]);
  const [turn, setTurn] = useState(0);
  const [direction, setDirection] = useState(1);
  const [winner, setWinner] = useState(null);
  const [discardRotation, setDiscardRotation] = useState(0);
  const [expressions, setExpressions] = useState(['', '', '', '']);

  const initGame = () => {
    const freshDeck = createDeck();
    const initialHands = [[], [], [], []];
    
    // Deal 7 cards to 4 players
    for (let i = 0; i < 7; i++) {
      for (let p = 0; p < 4; p++) {
        initialHands[p].push(freshDeck.pop());
      }
    }
    
    const firstDiscard = freshDeck.pop();
    setDeck(freshDeck);
    setHands(initialHands);
    setDiscardPile([firstDiscard]);
    setTurn(0);
    setDirection(1);
    setWinner(null);
    setExpressions(['', '', '', '']);
    setDiscardRotation(Math.floor(Math.random() * 20) - 10);
  };

  useEffect(() => {
    initGame();
  }, []);

  const isValidPlay = (card, topCard) => {
    return card.color === topCard.color || card.value === topCard.value;
  };

  const nextTurn = (skip = false, currentDir = direction, currentTurn = turn) => {
    let steps = skip ? 2 : 1;
    let next = (currentTurn + (currentDir * steps)) % 4;
    if (next < 0) next += 4;
    setTurn(next);
  };

  const processPlay = (playerIndex, card) => {
    // Remove card from hand
    const newHands = [...hands];
    newHands[playerIndex] = newHands[playerIndex].filter(c => c.id !== card.id);
    setHands(newHands);
    
    // Add to discard
    setDiscardPile([...discardPile, card]);
    setDiscardRotation(Math.floor(Math.random() * 30) - 15);

    // Check Win
    if (newHands[playerIndex].length === 0) {
      setWinner(PLAYERS[playerIndex]);
      return;
    }

    // Process effects
    let newDir = direction;
    let skipNext = false;
    let target = (playerIndex + newDir) % 4;
    if (target < 0) target += 4;
    
    const newExpressions = [...expressions];

    if (card.value === 'Rev') {
      newDir = direction * -1;
      setDirection(newDir);
      newExpressions[playerIndex] = 'played-action';
    } else if (card.value === 'Skip') {
      skipNext = true;
      newExpressions[playerIndex] = 'played-action';
      newExpressions[target] = 'got-skipped';
    } else if (card.value === '+2') {
      skipNext = true;
      newExpressions[playerIndex] = 'played-action';
      newExpressions[target] = 'got-skipped';
      
      const newDeck = [...deck];
      const cardsToDraw = [];
      if (newDeck.length > 0) cardsToDraw.push(newDeck.pop());
      if (newDeck.length > 0) cardsToDraw.push(newDeck.pop());
      setDeck(newDeck);
      
      newHands[target] = [...newHands[target], ...cardsToDraw];
      setHands(newHands);
    } else {
      // Clear expression if they played a normal card
      newExpressions[playerIndex] = '';
    }
    
    setExpressions(newExpressions);

    nextTurn(skipNext, newDir, playerIndex);
  };

  const handlePlayerPlay = (card) => {
    if (turn !== 0 || winner) return;
    const topCard = discardPile[discardPile.length - 1];
    
    if (isValidPlay(card, topCard)) {
      processPlay(0, card);
    }
  };

  const handleDrawCard = () => {
    if (turn !== 0 || winner) return;
    
    const newDeck = [...deck];
    if (newDeck.length === 0) return; // In real uno, we'd reshuffle discard
    
    const card = newDeck.pop();
    setDeck(newDeck);
    
    const newHands = [...hands];
    newHands[0].push(card);
    setHands(newHands);
    
    nextTurn(false, direction, 0);
  };

  // Bot Logic and Turn Management
  useEffect(() => {
    if (winner) return;
    
    // Clear expression for the player whose turn just started
    setExpressions(prev => {
      if (prev[turn] !== '') {
        const nextExp = [...prev];
        nextExp[turn] = '';
        return nextExp;
      }
      return prev;
    });

    if (turn === 0 || hands[turn].length === 0) return;

    const timer = setTimeout(() => {
      const topCard = discardPile[discardPile.length - 1];
      const botHand = hands[turn];
      
      // Find valid card
      const validCard = botHand.find(c => isValidPlay(c, topCard));
      
      if (validCard) {
        processPlay(turn, validCard);
      } else {
        // Draw card
        const newDeck = [...deck];
        if (newDeck.length > 0) {
          const card = newDeck.pop();
          setDeck(newDeck);
          
          const newHands = [...hands];
          newHands[turn].push(card);
          setHands(newHands);
        }
        nextTurn(false, direction, turn);
      }
    }, 1500);

    return () => clearTimeout(timer);
  }, [turn, winner]);

  if (deck.length === 0 && discardPile.length === 0) return null; // loading

  const topCard = discardPile[discardPile.length - 1];

  // Calculate card rotations for fanning out the player's hand
  const getCardStyle = (index, total) => {
    const middle = (total - 1) / 2;
    const offset = index - middle;
    const rotation = offset * 5;
    const yTransform = Math.abs(offset) * 5;
    return {
      transform: `rotate(${rotation}deg) translateY(${yTransform}px)`,
      zIndex: index + 1
    };
  };

  return (
    <div className="app-container">
      <div className="ambient-light"></div>
      
      {winner && (
        <div className="winner-overlay">
          <h2>{winner} Wins!</h2>
          <button onClick={initGame}>Play Again</button>
        </div>
      )}


      <div className="turn-indicator">
        Current Turn: <strong>{PLAYERS[turn]}</strong> 
        {direction === 1 ? ' (Clockwise)' : ' (Counter-Clockwise)'}
      </div>

      <main className="game-area">
        <div className="opponents-container">
          <PlayerSprite name="Seal One" spriteId="seal_one" cardsCount={hands[1].length} className="left" isActive={turn === 1} expression={expressions[1]} />
          <PlayerSprite name="Seal Two" spriteId="seal_two" cardsCount={hands[2].length} className="center" isActive={turn === 2} expression={expressions[2]} />
          <PlayerSprite name="Seal Three" spriteId="seal_three" cardsCount={hands[3].length} className="right" isActive={turn === 3} expression={expressions[3]} />
        </div>

        <div className="poker-table">
          <div className="table-ring"></div>
          <div className="center-board">
            <div className="draw-pile" onClick={handleDrawCard} title="Click to Draw"></div>
            {topCard && (
              <div className="discard-pile">
                <Card 
                  color={topCard.color} 
                  value={topCard.value} 
                  symbol={topCard.symbol} 
                  style={{ transform: `rotate(${discardRotation}deg)` }}
                />
              </div>
            )}
          </div>
        </div>

        <div className={`player-hand-container ${turn === 0 ? 'active-turn' : ''}`}>
          <div className="hand">
            {hands[0].map((card, index) => {
              const isValid = topCard ? isValidPlay(card, topCard) : false;
              return (
                <div key={card.id} onClick={() => handlePlayerPlay(card)}>
                  <Card 
                    color={card.color} 
                    value={card.value} 
                    symbol={card.symbol}
                    style={getCardStyle(index, hands[0].length)}
                    disabled={turn !== 0 || !isValid}
                  />
                </div>
              );
            })}
          </div>
        </div>
        
      </main>
    </div>
  );
}

export default App;
