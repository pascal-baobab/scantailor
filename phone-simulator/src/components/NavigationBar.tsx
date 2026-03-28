import React, { useState } from 'react';

interface NavigationBarProps {
  height: number;
}

/** Android navigation bar with button or gesture bar modes */
export const NavigationBar: React.FC<NavigationBarProps> = ({ height }) => {
  const [gestureMode, setGestureMode] = useState(true);

  if (height === 0) return null;

  if (gestureMode) {
    return (
      <div
        className="device-gesture-bar"
        style={{ cursor: 'pointer' }}
        onClick={() => setGestureMode(false)}
        role="button"
        aria-label="Switch to button navigation"
        tabIndex={0}
      />
    );
  }

  return (
    <div
      className="device-nav-bar device-nav-bar--buttons"
      style={{ height }}
      onClick={() => setGestureMode(true)}
      role="button"
      aria-label="Switch to gesture navigation"
      tabIndex={0}
    >
      <div className="device-nav-button" aria-label="Back">◁</div>
      <div className="device-nav-button" aria-label="Home">○</div>
      <div className="device-nav-button" aria-label="Recents">□</div>
    </div>
  );
};
