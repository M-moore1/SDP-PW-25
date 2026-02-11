//Author: Sai Raparla
//Reviewed by: Krish Shah
import { useEffect, useRef, useState, useCallback } from 'react';
import { loadEncryptionModule, encryptJson } from '../utils/encryption';

export type ConnectionStatus = 'connecting' | 'connected' | 'reconnecting' | 'disconnected';

interface UseWebSocketOptions {
  /** 32-byte key as 64 hex chars. When set, messages are encrypted with AES-256-GCM before sending. */
  encryptionKey?: string;
  /** Called with the actual payload sent over the wire (encrypted or plain JSON string). */
  onMessageSent?: (payload: string) => void;
}

interface UseWebSocketReturn {
  status: ConnectionStatus;
  sendMessage: (data: unknown) => void;
  lastError: string | null;
}

const INITIAL_RECONNECT_DELAY = 1000; // 1s
const MAX_RECONNECT_DELAY = 30000; // 30s
const HEARTBEAT_INTERVAL = 25000; // 25s

/**
 * Robust WebSocket hook with:
 * - Auto-reconnect with exponential backoff
 * - Message queue (sends when connection opens)
 * - Heartbeat ping to keep connection alive
 * - Optional AES-256-GCM encryption via WASM
 * - Automatic cleanup on visibility change and unmount
 */
export function useWebSocket(url: string, options?: UseWebSocketOptions): UseWebSocketReturn {
  const encryptionKey = options?.encryptionKey;
  const onMessageSent = options?.onMessageSent;
  const [status, setStatus] = useState<ConnectionStatus>('connecting');
  const [lastError, setLastError] = useState<string | null>(null);

  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<number | null>(null);
  const heartbeatIntervalRef = useRef<number | null>(null);
  const reconnectDelayRef = useRef<number>(INITIAL_RECONNECT_DELAY);
  const messageQueueRef = useRef<unknown[]>([]);
  const shouldConnectRef = useRef<boolean>(true);

  // Send a message (queues if not connected, encrypts if encryptionKey is set)
  const sendMessage = useCallback((data: unknown) => {
    const doSend = async (payload: unknown) => {
      try {
        let message: string;
        if (encryptionKey) {
          await loadEncryptionModule();
          const encrypted = encryptJson(payload as object, encryptionKey);
          if (!encrypted) {
            setLastError('Encryption failed');
            return;
          }
          message = encrypted;
        } else {
          message = JSON.stringify(payload);
        }

        if (wsRef.current?.readyState === WebSocket.OPEN) {
          wsRef.current.send(message);
          setLastError(null);
        } else {
          messageQueueRef.current.push(payload);
        }
        onMessageSent?.(message);
      } catch (error) {
        const errorMsg = error instanceof Error ? error.message : 'Failed to send message';
        setLastError(errorMsg);
        console.error('WebSocket send error:', error);
      }
    };

    if (encryptionKey) {
      doSend(data);
    } else {
      try {
        const message = JSON.stringify(data);
        if (wsRef.current?.readyState === WebSocket.OPEN) {
          wsRef.current.send(message);
          setLastError(null);
        } else {
          messageQueueRef.current.push(data);
        }
        onMessageSent?.(message);
      } catch (error) {
        const errorMsg = error instanceof Error ? error.message : 'Failed to send message';
        setLastError(errorMsg);
        console.error('WebSocket send error:', error);
      }
    }
  }, [encryptionKey, onMessageSent]);

  // Flush queued messages
  const flushQueue = useCallback(() => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      while (messageQueueRef.current.length > 0) {
        const data = messageQueueRef.current.shift();
        if (data) {
          sendMessage(data);
        }
      }
    }
  }, [sendMessage]);

  // Start heartbeat ping
  const startHeartbeat = useCallback(() => {
    // Clear existing heartbeat
    if (heartbeatIntervalRef.current !== null) {
      clearInterval(heartbeatIntervalRef.current);
    }

    const key = encryptionKey;
    heartbeatIntervalRef.current = window.setInterval(() => {
      if (wsRef.current?.readyState === WebSocket.OPEN) {
        try {
          const ping = { type: 'ping' };
          if (key) {
            loadEncryptionModule().then(() => {
              const enc = encryptJson(ping, key);
              if (enc) wsRef.current?.send(enc);
            });
          } else {
            wsRef.current.send(JSON.stringify(ping));
          }
        } catch (error) {
          console.error('Heartbeat ping failed:', error);
        }
      }
    }, HEARTBEAT_INTERVAL);
  }, [encryptionKey]);

  // Stop heartbeat
  const stopHeartbeat = useCallback(() => {
    if (heartbeatIntervalRef.current !== null) {
      clearInterval(heartbeatIntervalRef.current);
      heartbeatIntervalRef.current = null;
    }
  }, []);

  // Connect to WebSocket
  const connect = useCallback(() => {
    if (!shouldConnectRef.current) return;

    // Clear any pending reconnect
    if (reconnectTimeoutRef.current !== null) {
      clearTimeout(reconnectTimeoutRef.current);
      reconnectTimeoutRef.current = null;
    }

    try {
      setStatus('connecting');
      const ws = new WebSocket(url);

      ws.onopen = () => {
        console.log('WebSocket connected');
        setStatus('connected');
        setLastError(null);
        reconnectDelayRef.current = INITIAL_RECONNECT_DELAY; // Reset backoff
        
        // Start heartbeat
        startHeartbeat();
        
        // Flush any queued messages
        flushQueue();
      };

      ws.onclose = (event) => {
        console.log('WebSocket closed:', event.code, event.reason);
        stopHeartbeat();
        
        if (shouldConnectRef.current) {
          setStatus('reconnecting');
          
          // Exponential backoff reconnect
          const delay = Math.min(reconnectDelayRef.current, MAX_RECONNECT_DELAY);
          console.log(`Reconnecting in ${delay}ms...`);
          
          reconnectTimeoutRef.current = window.setTimeout(() => {
            reconnectDelayRef.current = Math.min(reconnectDelayRef.current * 2, MAX_RECONNECT_DELAY);
            connect();
          }, delay);
        } else {
          setStatus('disconnected');
        }
      };

      ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        setLastError('Connection error occurred');
      };

      ws.onmessage = (event) => {
        // Handle incoming messages (e.g., pong responses)
        try {
          const data = JSON.parse(event.data);
          if (data.type === 'pong') {
            // Heartbeat response received
            console.debug('Heartbeat pong received');
          }
        } catch (error) {
          // Ignore parse errors for non-JSON messages
          console.debug('Received non-JSON message:', event.data);
        }
      };

      wsRef.current = ws;
    } catch (error) {
      console.error('Failed to create WebSocket:', error);
      setLastError('Failed to create connection');
      setStatus('reconnecting');
      
      // Retry with backoff
      reconnectTimeoutRef.current = window.setTimeout(() => {
        reconnectDelayRef.current = Math.min(reconnectDelayRef.current * 2, MAX_RECONNECT_DELAY);
        connect();
      }, reconnectDelayRef.current);
    }
  }, [url, startHeartbeat, stopHeartbeat, flushQueue]);

  // Disconnect WebSocket
  const disconnect = useCallback(() => {
    shouldConnectRef.current = false;
    
    if (reconnectTimeoutRef.current !== null) {
      clearTimeout(reconnectTimeoutRef.current);
      reconnectTimeoutRef.current = null;
    }

    stopHeartbeat();

    if (wsRef.current) {
      wsRef.current.close();
      wsRef.current = null;
    }

    setStatus('disconnected');
  }, [stopHeartbeat]);

  // Handle page visibility changes
  useEffect(() => {
    const handleVisibilityChange = () => {
      if (document.hidden) {
        console.log('Page hidden, pausing WebSocket');
        // Don't disconnect, just pause heartbeat and mark for potential reconnect
        stopHeartbeat();
      } else {
        console.log('Page visible, resuming WebSocket');
        // Resume heartbeat if connected, or reconnect if needed
        if (wsRef.current?.readyState === WebSocket.OPEN) {
          startHeartbeat();
        } else if (shouldConnectRef.current) {
          connect();
        }
      }
    };

    document.addEventListener('visibilitychange', handleVisibilityChange);

    return () => {
      document.removeEventListener('visibilitychange', handleVisibilityChange);
    };
  }, [connect, startHeartbeat, stopHeartbeat]);

  // Initial connection and cleanup on unmount
  useEffect(() => {
    shouldConnectRef.current = true;
    connect();

    return () => {
      disconnect();
    };
  }, [connect, disconnect]);

  return {
    status,
    sendMessage,
    lastError,
  };
}

