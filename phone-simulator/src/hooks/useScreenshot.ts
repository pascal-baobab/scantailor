import { useCallback } from 'react';
import html2canvas from 'html2canvas';

/** Hook for capturing screenshots of the device frame or screen */
export function useScreenshot() {
  const capture = useCallback(async (element: HTMLElement, deviceName: string, includeFrame: boolean) => {
    try {
      const canvas = await html2canvas(element, {
        backgroundColor: null,
        scale: 2,
        useCORS: true,
        logging: false,
      });
      const link = document.createElement('a');
      const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
      const suffix = includeFrame ? 'mockup' : 'screen';
      link.download = `${deviceName}_${suffix}_${timestamp}.png`;
      link.href = canvas.toDataURL('image/png');
      link.click();
    } catch (err) {
      console.error('Screenshot failed:', err);
    }
  }, []);

  return { capture };
}
