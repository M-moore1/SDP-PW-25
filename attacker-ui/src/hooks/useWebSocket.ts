import { useEffect, useRef, useState, useCallback } from 'react';

export type ConnectionStatus = 'connecting' | 'connected' | 'reconnecting' | 'disconnected';

interface UseWebSocketOptions {
  onMessage?: (data: string) => void;
}

interface UseWebSocketReturn {
  status: ConnectionStatus;
  sendRaw: (data: string) => void;
  lastError: string | null;
}

const INITIAL_RECONNECT_DELAY = 1000;
const MAX_RECONNECT_DELAY = 30000;
const HEARTBEAT_INTERVAL = 25000;

export function useWebSocket(url: string, options: UseWebSocketOptions = {}): UseWebSocketReturn {
  const [status, setStatus] = useState<ConnectionStatus>('connecting');
  const [lastError, setLastError] = useState<string | null>(null);

  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<number | null>(null);
  const heartbeatIntervalRef = useRef<number | null>(null);
  const reconnectDelayRef = useRef<number>(INITIAL_RECONNECT_DELAY);
  const messageQueueRef = useRef<string[]>([]);
  const shouldConnectRef = useRef<boolean>(true);
  const onMessageRef = useRef(options.onMessage);
  onMessageRef.current = options.onMessage;

  const startHeartbeat = useCallback(() => {
    if (heartbeatIntervalRef.current !== null) {
      clearInterval(heartbeatIntervalRef.current);
    }
    heartbeatIntervalRef.current = window.setInterval(() => {
      if (wsRef.current?.readyState === WebSocket.OPEN) {
        wsRef.current.send(JSON.stringify({ type: 'ping' }));
      }
    }, HEARTBEAT_INTERVAL);
  }, []);

  const stopHeartbeat = useCallback(() => {
    if (heartbeatIntervalRef.current !== null) {
      clearInterval(heartbeatIntervalRef.current);
      heartbeatIntervalRef.current = null;
    }
  }, []);

  const sendRaw = useCallback((data: string) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(data);
      setLastError(null);
    } else {
      messageQueueRef.current.push(data);
    }
  }, []);

  const flushQueue = useCallback(() => {
    while (messageQueueRef.current.length > 0) {
      const data = messageQueueRef.current.shift();
      if (data && wsRef.current?.readyState === WebSocket.OPEN) {
        wsRef.current.send(data);
      }
    }
  }, []);

  const connect = useCallback(() => {
    if (!shouldConnectRef.current) return;

    if (reconnectTimeoutRef.current !== null) {
      clearTimeout(reconnectTimeoutRef.current);
      reconnectTimeoutRef.current = null;
    }

    try {
      setStatus('connecting');
      const ws = new WebSocket(url);

      ws.onopen = () => {
        setStatus('connected');
        setLastError(null);
        reconnectDelayRef.current = INITIAL_RECONNECT_DELAY;
        startHeartbeat();
        flushQueue();
      };

      ws.onclose = (event) => {
        stopHeartbeat();
        if (shouldConnectRef.current) {
          setStatus('reconnecting');
          const delay = Math.min(reconnectDelayRef.current, MAX_RECONNECT_DELAY);
          reconnectTimeoutRef.current = window.setTimeout(() => {
            reconnectDelayRef.current = Math.min(reconnectDelayRef.current * 2, MAX_RECONNECT_DELAY);
            connect();
          }, delay);
        } else {
          setStatus('disconnected');
        }
        if (!event.wasClean) {
          setLastError(`Connection closed (code ${event.code})`);
        }
      };

      ws.onerror = () => {
        setLastError('Connection error occurred');
      };

      ws.onmessage = (event: MessageEvent) => {
        const data = typeof event.data === 'string' ? event.data : JSON.stringify(event.data);
        onMessageRef.current?.(data);
      };

      wsRef.current = ws;
    } catch (error) {
      setLastError('Failed to create connection');
      setStatus('reconnecting');
      reconnectTimeoutRef.current = window.setTimeout(() => {
        reconnectDelayRef.current = Math.min(reconnectDelayRef.current * 2, MAX_RECONNECT_DELAY);
        connect();
      }, reconnectDelayRef.current);
    }
  }, [url, startHeartbeat, stopHeartbeat, flushQueue]);

  // Handle page visibility changes
  useEffect(() => {
    const handleVisibilityChange = () => {
      if (document.hidden) {
        stopHeartbeat();
      } else {
        if (wsRef.current?.readyState === WebSocket.OPEN) {
          startHeartbeat();
        } else if (shouldConnectRef.current) {
          connect();
        }
      }
    };
    document.addEventListener('visibilitychange', handleVisibilityChange);
    return () => document.removeEventListener('visibilitychange', handleVisibilityChange);
  }, [connect, startHeartbeat, stopHeartbeat]);

  // Initial connection and cleanup
  useEffect(() => {
    shouldConnectRef.current = true;
    connect();
    return () => {
      shouldConnectRef.current = false;
      if (reconnectTimeoutRef.current !== null) clearTimeout(reconnectTimeoutRef.current);
      stopHeartbeat();
      if (wsRef.current) {
        wsRef.current.close();
        wsRef.current = null;
      }
      setStatus('disconnected');
    };
  }, [connect, stopHeartbeat]);

  return { status, sendRaw, lastError };
}
