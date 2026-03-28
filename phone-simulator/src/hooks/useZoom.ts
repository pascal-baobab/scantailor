import { useState, useCallback, useEffect, useRef } from 'react';

const ZOOM_PRESETS = [50, 75, 100, 125, 150];

/** Hook for managing viewport zoom level */
export function useZoom() {
  const [zoom, setZoom] = useState(75);
  const [fitToWindow, setFitToWindow] = useState(false);
  const containerRef = useRef<HTMLDivElement>(null);

  const setZoomLevel = useCallback((level: number) => {
    setFitToWindow(false);
    setZoom(Math.max(25, Math.min(200, level)));
  }, []);

  const calculateFitZoom = useCallback(
    (containerWidth: number, containerHeight: number, deviceWidth: number, deviceHeight: number) => {
      const padding = 80;
      const availW = containerWidth - padding;
      const availH = containerHeight - padding;
      const scaleW = availW / deviceWidth;
      const scaleH = availH / deviceHeight;
      return Math.floor(Math.min(scaleW, scaleH) * 100);
    },
    []
  );

  const toggleFitToWindow = useCallback(() => {
    setFitToWindow((prev) => !prev);
  }, []);

  useEffect(() => {
    if (!fitToWindow || !containerRef.current) return;
    const el = containerRef.current;
    const obs = new ResizeObserver(() => {
      if (containerRef.current) {
        const { width, height } = containerRef.current.getBoundingClientRect();
        const fit = calculateFitZoom(width, height, 500, 1000);
        setZoom(fit);
      }
    });
    obs.observe(el);
    return () => obs.disconnect();
  }, [fitToWindow, calculateFitZoom]);

  return { zoom, setZoomLevel, fitToWindow, toggleFitToWindow, containerRef, zoomPresets: ZOOM_PRESETS };
}
