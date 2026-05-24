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

const generateRandomSeal = () => {
  const imgs = ['sealwalk1.png', 'sealwalk2.png', 'sealwalk3.png', 'sealwalk4.png'];
  const dirs = ['left-to-right', 'right-to-left'];
  return {
    id: Date.now() + Math.random(),
    img: imgs[Math.floor(Math.random() * imgs.length)],
    dir: dirs[Math.floor(Math.random() * dirs.length)],
    duration: `${Math.floor(Math.random() * 15) + 10}s`,
    delay: '0s',
    size: Math.floor(Math.random() * 100) + 100,
    bounceDuration: `${(Math.random() * 0.3 + 0.2).toFixed(2)}s`,
    baseBottom: Math.floor(Math.random() * 80) + 5
  };
};

const BouncingSeals = () => {
  const basePath = import.meta.env.BASE_URL || '/';
  const [exploded, setExploded] = useState({});
  const [seals, setSeals] = useState([
    { id: 1, img: 'sealwalk1.png', dir: 'left-to-right', duration: '15s', delay: '0s', size: 150, bounceDuration: '0.4s', baseBottom: 15 },
    { id: 2, img: 'sealwalk2.png', dir: 'right-to-left', duration: '20s', delay: '2s', size: 120, bounceDuration: '0.35s', baseBottom: 65 },
    { id: 3, img: 'sealwalk3.png', dir: 'left-to-right', duration: '12s', delay: '7s', size: 180, bounceDuration: '0.5s', baseBottom: 40 },
    { id: 4, img: 'sealwalk4.png', dir: 'right-to-left', duration: '25s', delay: '1s', size: 140, bounceDuration: '0.45s', baseBottom: 25 },
    { id: 5, img: 'sealwalk1.png', dir: 'left-to-right', duration: '18s', delay: '12s', size: 160, bounceDuration: '0.4s', baseBottom: 55 },
    { id: 6, img: 'sealwalk3.png', dir: 'right-to-left', duration: '14s', delay: '5s', size: 130, bounceDuration: '0.3s', baseBottom: 75 },
  ]);

  const handleExplode = (id) => {
    if (!exploded[id]) {
      setExploded(prev => ({ ...prev, [id]: true }));
      
      // Spawn two new ones!
      setSeals(prev => [...prev, generateRandomSeal(), generateRandomSeal()]);
      
      setTimeout(() => {
        setExploded(prev => ({ ...prev, [id]: 'hidden' }));
      }, 800); // Hides the explosion after 800ms
    }
  };

  return (
    <div className="bouncing-seals-container">
      {seals.map(seal => {
        const state = exploded[seal.id];
        if (state === 'hidden') return null;

        return (
          <div 
            key={seal.id}
            className={`seal-walker-wrapper ${seal.dir}`}
            style={{
              animationDuration: seal.duration,
              animationDelay: seal.delay,
              bottom: `${seal.baseBottom}%`
            }}
          >
            <img 
              src={state === true ? `${basePath}explosion.gif` : `${basePath}${seal.img}`} 
              alt="Bouncing Seal" 
              className="bouncing-seal-img"
              onClick={() => handleExplode(seal.id)}
              style={{
                animationDuration: state === true ? '0s' : seal.bounceDuration,
                width: `${seal.size}px`,
                cursor: 'crosshair',
                filter: state === true ? 'none' : undefined
              }}
            />
          </div>
        );
      })}
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
  const [hasStarted, setHasStarted] = useState(false);
  const [showExtras, setShowExtras] = useState(false);
  const [isCheater, setIsCheater] = useState(false);
  const [showBigPresent, setShowBigPresent] = useState(false);

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
    setShowExtras(false);
    setIsCheater(false);
    setShowBigPresent(false);
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

  const basePath = import.meta.env.BASE_URL || '/';

  if (!hasStarted) {
    return (
      <div className="start-screen">
        <BouncingSeals />
        <img src={`${basePath}GAME%20TITLE.png`} alt="Game Title" className="title-sprite" />
        <img src={`${basePath}startbutton.png`} alt="Start Game" className="start-button-sprite" onClick={() => setHasStarted(true)} />
      </div>
    );
  }

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
      
      {!winner && (
        <button 
          className="debug-win-btn" 
          onClick={() => {
            setIsCheater(true);
            setWinner('You');
          }}
        >
          Instant Win
        </button>
      )}
      
      {winner && !showExtras && (
        <div className="winner-overlay">
          <h2>{isCheater ? 'you fucking cheater' : (winner === 'You' ? 'You Win!' : `${winner} Wins!`)}</h2>
          <div className="winner-buttons">
            <button onClick={initGame}>Play Again</button>
            <button onClick={() => setShowExtras(true)}>Other gifts</button>
          </div>
        </div>
      )}

      {showExtras && (
        <div className="extras-overlay">
          <h2>Other gifts</h2>
          <div className="presents-container">
            <img 
              src={`${basePath}present3.png`} 
              alt="Present 3" 
              className="present-sprite" 
              onClick={() => setShowBigPresent(true)} 
            />
            <img src={`${basePath}present2.png`} alt="Present 2" className="present-sprite" />
            <img src={`${basePath}present1.jpg`} alt="Present 1" className="present-sprite" />
          </div>
          <div className="extras-buttons">
            <button onClick={() => setShowExtras(false)}>Go back to game</button>
            <button className="unfinished-button" disabled>Unfinished</button>
          </div>
        </div>
      )}

      {showBigPresent && (
        <div className="big-present-overlay" onClick={() => setShowBigPresent(false)}>
          <img 
            src={`${basePath}card.png`} 
            alt="Big Present" 
            className="big-present-img" 
            onClick={e => e.stopPropagation()} 
          />
          <button className="close-big-present" onClick={() => setShowBigPresent(false)}>Close</button>
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
