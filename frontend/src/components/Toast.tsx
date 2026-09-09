// src/components/Toast.tsx
import { useCallback, useState } from "react";

export interface ToastItem {
  id: number;
  message: string;
}

let toastIdCounter = 0;

// hook à instancier une fois par écran (ou à remonter dans un contexte global
// plus tard si plusieurs écrans doivent partager les mêmes toasts)
export function useToasts() {
  const [toasts, setToasts] = useState<ToastItem[]>([]);

  const pushToast = useCallback((message: string) => {
    const id = ++toastIdCounter;
    setToasts((prev) => [...prev, { id, message }]);
    // disparition automatique après 5s
    setTimeout(() => {
      setToasts((prev) => prev.filter((t) => t.id !== id));
    }, 5000);
  }, []);

  const dismissToast = useCallback((id: number) => {
    setToasts((prev) => prev.filter((t) => t.id !== id));
  }, []);

  return { toasts, pushToast, dismissToast };
}

export function ToastContainer({
  toasts,
  onDismiss,
}: {
  toasts: ToastItem[];
  onDismiss: (id: number) => void;
}) {
  if (toasts.length === 0) return null;

  return (
    <div
      className="fixed top-6 left-1/2 -translate-x-1/2 z-[100] flex flex-col gap-3 w-full max-w-md px-4"
      aria-live="assertive"
    >
      {toasts.map((toast) => (
        <div
          key={toast.id}
          role="alert"
          className="flex items-start gap-3 rounded-xl border px-4 py-3 shadow-lg animate-toast-in"
          style={{
            background: "linear-gradient(135deg, #fef2f2, #fee2e2)",
            borderColor: "rgba(220,38,38,0.35)",
            boxShadow: "0 15px 35px rgba(220,38,38,0.25)",
          }}
        >
          <span className="text-red-500 text-lg leading-none mt-0.5" aria-hidden="true">⚠</span>
          <p className="flex-1 text-sm text-[#7F1D1D] leading-relaxed">{toast.message}</p>
          <button
            type="button"
            aria-label="Fermer"
            onClick={() => onDismiss(toast.id)}
            className="text-[#B91C1C] text-sm leading-none hover:opacity-70 transition-opacity"
          >
            ✕
          </button>
        </div>
      ))}
    </div>
  );
}