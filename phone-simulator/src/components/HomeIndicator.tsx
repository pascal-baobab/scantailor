import React from 'react';

interface HomeIndicatorProps {
  height: number;
}

/** iOS home indicator bar at the bottom of the screen */
export const HomeIndicator: React.FC<HomeIndicatorProps> = ({ height }) => {
  if (height === 0) return null;

  return <div className="device-home-indicator" aria-hidden="true" />;
};
