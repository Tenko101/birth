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
  const isActionCard = value === 'Skip' || value === 'Rev' || value === '+2';
  return (
    <div className={`playing-card ${color} ${disabled ? 'disabled' : ''} ${isActionCard ? 'holographic' : ''}`} style={style}>
      {isActionCard && <div className="holo-overlay"></div>}
      <span className="card-value top-left">{value}</span>
      <div className="center-symbol">{symbol}</div>
      <span className="card-value bottom-right">{value}</span>
    </div>
  );
};

const ConfettiCanvas = () => {
  const canvasRef = React.useRef(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    let animationFrameId;

    let width = (canvas.width = window.innerWidth);
    let height = (canvas.height = window.innerHeight);

    const handleResize = () => {
      width = canvas.width = window.innerWidth;
      height = canvas.height = window.innerHeight;
    };
    window.addEventListener('resize', handleResize);

    const colors = [
      '#f43f5e', '#ec4899', '#d946ef', '#a855f7', '#8b5cf6', 
      '#6366f1', '#3b82f6', '#0ea5e9', '#06b6d4', '#14b8a6', 
      '#10b981', '#22c55e', '#84cc16', '#eab308', '#f97316'
    ];

    const pieces = Array.from({ length: 150 }).map(() => ({
      x: Math.random() * width,
      y: Math.random() * -height - 20,
      r: Math.random() * 6 + 4,
      d: Math.random() * height,
      color: colors[Math.floor(Math.random() * colors.length)],
      tilt: Math.random() * 10 - 5,
      tiltAngleIncremental: Math.random() * 0.07 + 0.02,
      tiltAngle: 0,
      w: Math.random() * 10 + 5,
      h: Math.random() * 18 + 8,
      speed: Math.random() * 3 + 2,
      active: true,
    }));

    let activeCount = pieces.length;

    const draw = () => {
      ctx.clearRect(0, 0, width, height);

      pieces.forEach(p => {
        if (!p.active) return;

        p.tiltAngle += p.tiltAngleIncremental;
        p.y += p.speed;
        p.tilt = Math.sin(p.tiltAngle - p.y / 20) * 12;

        if (p.y > height) {
          p.active = false;
          activeCount--;
          return;
        }

        ctx.beginPath();
        ctx.lineWidth = p.r;
        ctx.strokeStyle = p.color;
        ctx.moveTo(p.x + p.tilt + p.w / 2, p.y);
        ctx.lineTo(p.x + p.tilt, p.y + p.tilt + p.h / 2);
        ctx.stroke();
      });

      if (activeCount > 0) {
        animationFrameId = requestAnimationFrame(draw);
      }
    };

    draw();

    return () => {
      window.removeEventListener('resize', handleResize);
      cancelAnimationFrame(animationFrameId);
    };
  }, []);

  return (
    <canvas
      ref={canvasRef}
      style={{
        position: 'fixed',
        top: 0,
        left: 0,
        width: '100vw',
        height: '100vh',
        pointerEvents: 'none',
        zIndex: 10005,
      }}
    />
  );
};

const ConfettiShower = ({ active }) => {
  if (!active) return null;
  return <ConfettiCanvas />;
};

const SealSleeperOverlay = ({ active, onClose }) => {
  if (!active) return null;

  const [seals, setSeals] = useState([]);
  const [count, setCount] = useState(0);
  const basePath = import.meta.env.BASE_URL || '/';

  useEffect(() => {
    const spawnSeal = (id) => {
      const randomImg = ['sealwalk1.png', 'sealwalk2.png', 'sealwalk3.png', 'sealwalk4.png'][Math.floor(Math.random() * 4)];
      setSeals(prev => [...prev, { id, img: randomImg }]);

      setTimeout(() => {
        setCount(c => c + 1);
      }, 4000);

      setTimeout(() => {
        setSeals(prev => prev.filter(s => s.id !== id));
      }, 8000);
    };

    spawnSeal(Date.now());

    const interval = setInterval(() => {
      spawnSeal(Date.now() + Math.random());
    }, 5000);

    return () => clearInterval(interval);
  }, []);

  return (
    <div className="seal-sleeper-overlay">
      <div className="sky-bg">
        {Array.from({ length: 45 }).map((_, i) => (
          <div 
            key={i} 
            className="twinkle-star" 
            style={{
              top: `${Math.random() * 70}%`,
              left: `${Math.random() * 100}%`,
              animationDelay: `${Math.random() * 4}s`,
              transform: `scale(${Math.random() * 0.7 + 0.3})`
            }}
          />
        ))}
        <div className="crescent-moon"></div>
      </div>

      <div className="sleepy-hills">
        <div className="hill-back"></div>
        <div className="hill-front"></div>
      </div>

      <div className="counting-fence-container">
        <div className="wooden-fence">
          <div className="fence-post post-left"></div>
          <div className="fence-rail rail-top"></div>
          <div className="fence-rail rail-bottom"></div>
          <div className="fence-post post-right"></div>
        </div>
      </div>

      {seals.map(seal => (
        <div key={seal.id} className="sleeper-seal-wrapper">
          <img src={`${basePath}${seal.img}`} alt="Sleepy Seal" className="sleeper-seal-img" />
        </div>
      ))}

      <div className="sleepy-counter">
        <span className="count-number">{count}</span> {count === 1 ? 'Seal' : 'Seals'} jumping over the fence...
      </div>

      <button className="close-sleeper" onClick={onClose}>Wake Up</button>
    </div>
  );
};

const UnfinishedSpritesOverlay = ({ active, onClose, onOpenBigImage, onOpenBigAssets }) => {
  if (!active) return null;

  const basePath = import.meta.env.BASE_URL || '/';

  const draftItems = [
    {
      id: 'seal_oc',
      name: 'Seal',
      type: 'image',
      icon: '🦭',
      description: 'Wanted all the sprites to be custom and not just erith seal but the website itself took so much fucking time to code itself so this just went unused :(. PS you can click on the icon on the left to make them bigger',
      imageSrc: 'seal oc.png'
    },
    {
      id: 'unused_assets',
      name: 'Assets',
      type: 'image',
      icon: '🎨',
      description: 'I wanted to make an animatic for you like you did for me but WOAW SUPRISE SUPRISE ITS VERY HARD AND ALOT OF TIME and exams and procrastinaion so i will finish this evnetually but yeah here some shit.',
      imageSrc: 'unfished stuff.png'
    }
  ];

  const [selectedItem, setSelectedItem] = useState(draftItems[0]); // Default to first item (Seal OC)
  
  // Placeholders/Fallback states
  const [sealImgError, setSealImgError] = useState(false);
  const [assetsImgError, setAssetsImgError] = useState(false);

  return (
    <div className="unfinished-overlay">
      <div className="unfinished-content glass-panel">
        <button className="close-unfinished-top" onClick={onClose}>×</button>
        
        <h2>things i didn't get to finish</h2>
        <p className="unfinished-subtitle">click on them to see what they were gonna be</p>
        
        {/* Showcase drafts grid */}
        <div className="drafts-container">
          {draftItems.map(item => {
            const isSelected = selectedItem?.id === item.id;
            const isSeal = item.id === 'seal_oc';
            const hasError = isSeal ? sealImgError : assetsImgError;
            const setError = isSeal ? setSealImgError : setAssetsImgError;
            return (
              <div 
                key={item.id} 
                className={`draft-card ${isSelected ? 'selected' : ''}`}
                onClick={() => setSelectedItem(item)}
              >
                <div className="draft-thumbnail-frame">
                  {!hasError ? (
                    <img 
                      src={`${basePath}${isSeal ? 'seal%20oc.png' : 'unfished%20stuff.png'}`} 
                      alt={item.name} 
                      className="draft-mini-img"
                      onError={() => setError(true)} 
                    />
                  ) : (
                    <span className="draft-mini-icon">{item.icon}</span>
                  )}
                </div>
                <h3>{item.name}</h3>
              </div>
            );
          })}
        </div>

        {/* Details panel */}
        <div className="draft-details-panel">
          {selectedItem ? (
            <div className="selected-draft-detail-card">
              <div className="draft-detail-body">

                {selectedItem.id === 'seal_oc' && (
                  <div className="media-detail-content">
                    <div 
                      className="detail-media-container interactive-zoom" 
                      onClick={onOpenBigImage}
                      style={{ cursor: 'zoom-in' }}
                      title="click to zoom"
                    >
                      {!sealImgError ? (
                        <>
                          <img 
                            src={`${basePath}seal%20oc.png`} 
                            alt="Seal OC" 
                            className="large-detail-img"
                            onError={() => setSealImgError(true)}
                          />
                          <div className="media-hover-overlay">
                            <span>🔍 click to zoom</span>
                          </div>
                        </>
                      ) : (
                        <div className="mini-missing-alert" onClick={e => e.stopPropagation()}>
                          <span className="alert-icon">📷</span>
                          <h4>seal oc.png is missing</h4>
                          <p>Drop your file inside the <strong>public</strong> folder to view it here!</p>
                          <button className="retry-mini-btn" onClick={() => setSealImgError(false)}>Retry Load</button>
                        </div>
                      )}
                    </div>
                    <div className="detail-text-container">
                      <h3>{selectedItem.name}</h3>
                      <p className="detail-description">{selectedItem.description}</p>
                    </div>
                  </div>
                )}

                {selectedItem.id === 'unused_assets' && (
                  <div className="media-detail-content">
                    <div 
                      className="detail-media-container interactive-zoom" 
                      onClick={onOpenBigAssets}
                      style={{ cursor: 'zoom-in' }}
                      title="click to zoom"
                    >
                      {!assetsImgError ? (
                        <>
                          <img 
                            src={`${basePath}unfished%20stuff.png`} 
                            alt="Assets" 
                            className="large-detail-img"
                            onError={() => setAssetsImgError(true)}
                          />
                          <div className="media-hover-overlay">
                            <span>🔍 click to zoom</span>
                          </div>
                        </>
                      ) : (
                        <div className="mini-missing-alert" onClick={e => e.stopPropagation()}>
                          <span className="alert-icon">🎨</span>
                          <h4>unfished stuff.png is missing</h4>
                          <p>Drop your file inside the <strong>public</strong> folder to view it here!</p>
                          <button className="retry-mini-btn" onClick={() => setAssetsImgError(false)}>Retry Load</button>
                        </div>
                      )}
                    </div>
                    <div className="detail-text-container">
                      <h3>{selectedItem.name}</h3>
                      <p className="detail-description">{selectedItem.description}</p>
                    </div>
                  </div>
                )}

              </div>
            </div>
          ) : (
            <div className="detail-placeholder-box">
              <p>🎁 click on them to check out his drawings!</p>
            </div>
          )}
        </div>

        <div className="unfinished-footer">
          <button className="btn-close-unfinished" onClick={onClose}>close</button>
        </div>
      </div>
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

  const [bubbleText, setBubbleText] = useState('');
  const [showBubble, setShowBubble] = useState(false);
  const [badge, setBadge] = useState(null);

  useEffect(() => {
    if (currentExpression) {
      let quotes = [];
      let badgeText = '';
      let badgeClass = '';

      if (currentExpression === 'uno') {
        quotes = ["UNO!", "🦭🦭🦭🦭🦭🦭🦭🦭🦭🦭", "uno", "ez"];
        badgeText = 'UNO!';
        badgeClass = 'badge-uno';
      } else if (currentExpression === 'got-skipped') {
        quotes = ["KILL YOURSELF", "WHORE", "wisconsin...", "What the fuck"];
        badgeText = 'SKIPPED!';
        badgeClass = 'badge-skipped';
      } else if (currentExpression === 'got-plus-two') {
        quotes = ["fucking +2!", "fucking +2", "fucking +2", "fucking +2"];
        badgeText = '+2!';
        badgeClass = 'badge-skipped';
      } else if (
        currentExpression === 'played-action' ||
        currentExpression === 'played-skip' ||
        currentExpression === 'played-reverse' ||
        currentExpression === 'played-plus-two'
      ) {
        quotes = ["uehuehueuehuehueh", "mauw", ";3", "aaaaaaaaaaaaaaaaaaaa"];
        badgeClass = 'badge-action';
        if (currentExpression === 'played-skip') badgeText = 'SKIP!';
        else if (currentExpression === 'played-plus-two') badgeText = '+2!';
        else badgeText = 'REVERSE!';
      }
      
      const randomQuote = quotes[Math.floor(Math.random() * quotes.length)];
      setBubbleText(randomQuote);
      setShowBubble(true);

      const timer = setTimeout(() => {
        setShowBubble(false);
      }, 3000);

      if (badgeText) {
        setBadge({ text: badgeText, className: badgeClass, id: Date.now() });
        const badgeTimer = setTimeout(() => {
          setBadge(null);
        }, 2000);
        return () => {
          clearTimeout(timer);
          clearTimeout(badgeTimer);
        };
      }

      return () => clearTimeout(timer);
    } else {
      setShowBubble(false);
    }
  }, [currentExpression]);

  let imageSuffix = 'default';
  if (currentExpression === 'uno') imageSuffix = 'uno';
  else if (currentExpression === 'got-skipped' || currentExpression === 'got-plus-two') imageSuffix = 'skipped';
  else if (
    currentExpression === 'played-action' ||
    currentExpression === 'played-skip' ||
    currentExpression === 'played-reverse' ||
    currentExpression === 'played-plus-two'
  ) {
    imageSuffix = 'action';
  }
  
  const basePath = import.meta.env.BASE_URL || '/';
  const imagePath = `${basePath}${spriteId}_${imageSuffix}.png`;

  return (
    <div className={`opponent ${className} ${isActive ? 'active-turn' : ''}`}>
      {showBubble && (
        <div className="speech-bubble-wrapper">
          <div className="speech-bubble">{bubbleText}</div>
        </div>
      )}
      {badge && (
        <div key={badge.id} className={`floating-badge ${badge.className}`}>
          {badge.text}
        </div>
      )}
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
  const expressionsRef = React.useRef(expressions);
  expressionsRef.current = expressions;
  const [hasStarted, setHasStarted] = useState(false);
  const [showExtras, setShowExtras] = useState(false);
  const [isCheater, setIsCheater] = useState(false);
  const [showBigPresent, setShowBigPresent] = useState(false);
  const [showSealSleeper, setShowSealSleeper] = useState(false);
  const [showUnfinished, setShowUnfinished] = useState(false);
  const [showBigSealImage, setShowBigSealImage] = useState(false);
  const [showBigAssetsImage, setShowBigAssetsImage] = useState(false);
  const [showFoxy, setShowFoxy] = useState(false);

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
    setShowSealSleeper(false);
    setShowUnfinished(false);
    setShowBigSealImage(false);
    setShowBigAssetsImage(false);
  };



  useEffect(() => {
    initGame();
  }, []);

  useEffect(() => {
    const handleClick = () => {
      if (!showFoxy && Math.random() < 0.0001) {
        setShowFoxy(true);
      }
    };
    document.addEventListener('click', handleClick);
    return () => document.removeEventListener('click', handleClick);
  }, [showFoxy]);

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
      newExpressions[playerIndex] = 'played-reverse';
    } else if (card.value === 'Skip') {
      skipNext = true;
      newExpressions[playerIndex] = 'played-skip';
      newExpressions[target] = 'got-skipped';
    } else if (card.value === '+2') {
      skipNext = true;
      newExpressions[playerIndex] = 'played-plus-two';
      newExpressions[target] = 'got-plus-two';
      
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

    const hasActiveAnimation = expressionsRef.current.some(exp => exp !== '');
    const delay = hasActiveAnimation ? 2500 : 1200;

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
    }, delay);

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
  const isPlayerTurn = turn === 0 && !winner;
  const hasPlayableCard = topCard ? hands[0].some(card => isValidPlay(card, topCard)) : false;
  const shouldDraw = isPlayerTurn && !hasPlayableCard;

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
      <ConfettiShower active={winner !== null} />
      
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
            <img 
              src={`${basePath}present2.png`} 
              alt="Present 2" 
              className="present-sprite" 
              onClick={() => setShowSealSleeper(true)} 
            />
            <img 
              src={`${basePath}present1.jpg`} 
              alt="Present 1" 
              className="present-sprite" 
              onClick={() => setShowUnfinished(true)}
            />
          </div>
          <div className="extras-buttons">
            <button onClick={() => setShowExtras(false)}>Go back to game</button>
            <button className="unfinished-button" onClick={() => setShowUnfinished(true)}>things i didn't get to finish</button>
          </div>
        </div>
      )}

      <SealSleeperOverlay active={showSealSleeper} onClose={() => setShowSealSleeper(false)} />
      <UnfinishedSpritesOverlay 
        active={showUnfinished} 
        onClose={() => setShowUnfinished(false)} 
        onOpenBigImage={() => setShowBigSealImage(true)}
        onOpenBigAssets={() => setShowBigAssetsImage(true)}
      />

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

      {showBigSealImage && (
        <div className="big-present-overlay" onClick={() => setShowBigSealImage(false)}>
          <img 
            src={`${basePath}seal%20oc.png`} 
            alt="Seal OC Large" 
            className="big-present-img" 
            onClick={e => e.stopPropagation()} 
          />
          <button className="close-big-present" onClick={() => setShowBigSealImage(false)}>close</button>
        </div>
      )}

      {showBigAssetsImage && (
        <div className="big-present-overlay" onClick={() => setShowBigAssetsImage(false)}>
          <img 
            src={`${basePath}unfished%20stuff.png`} 
            alt="Unfinished Assets Large" 
            className="big-present-img" 
            onClick={e => e.stopPropagation()} 
          />
          <button className="close-big-present" onClick={() => setShowBigAssetsImage(false)}>close</button>
        </div>
      )}

      {showFoxy && (
        <div className="foxy-overlay" onClick={() => setShowFoxy(false)}>
          <img 
            src={`${basePath}foxy-jumpscare.gif`} 
            alt="BOO!" 
            className="foxy-img"
            onClick={e => e.stopPropagation()}
          />
          <button className="close-big-present" onClick={() => setShowFoxy(false)}>Close</button>
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
            <div 
              className={`draw-pile ${shouldDraw ? 'glow-prompt' : ''}`} 
              onClick={handleDrawCard} 
              title={shouldDraw ? "No playable cards! Click to Draw" : "Click to Draw"}
            ></div>
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
