import type { IWebSocket, TMessage } from '../types/types.ts';
import { KEY } from '@/generated/protocol';
import type { TKey } from '@/generated/protocol';

import { defineStore } from 'pinia';
import { useWebSocketStore } from './WebSocketStore.ts';

import { struct, event } from '@/assets/js/';

const log = (...arg: any) => (process.env.NODE_ENV === 'development' ? console.log(arg) : null);

const initialState = (): IWebSocket => ({
  socket: null,
  pingClient: 5000,
  pingDevice: 0,
  isInit: false,
  infoLoaded: false,
});

export const useWebSocket = defineStore('webSocket', {
  state: initialState,
  actions: {
    init() {
      this.infoLoaded = false;
      this.onSend(KEY.INFO);
      event.emit('init');
      this.isInit = true;
    },
    requestInfo() {
      if (!this.socket || this.socket.readyState !== WebSocket.OPEN) return;
      const buffer = struct.set(KEY.INFO);
      if (buffer) this.socket.send(buffer);
    },
    onopen() {
      this.pingDevice = Date.now();
      this.pingClient = Date.now();
      this.init();
      event.emit('connected', true);
    },
    onmessage(message: MessageEvent) {
      this.pingDevice = Date.now();
      if (message.data instanceof ArrayBuffer) {
        const data = struct.get(message.data);
        event.emit('messages', data as TMessage);
        if (data) {
          const { object, key } = data;
          if (key !== KEY.PING) log(object, key);
          if (key === KEY.INFO) this.infoLoaded = true;
          useWebSocketStore().onMessage(data as TMessage);
        }
      }
    },
    onclose(data: any) {
      event.emit('connected', false);
      log(data);
    },
    onerror(data: any) {
      event.emit('connected', false);
      log(data);
    },
    onSend(key: TKey | number, object?: object) {
      log(key, object);
      if (this.socket && this.socket.readyState === WebSocket.OPEN && this.isConnect) {
        const buffer = struct.set(key, object as Record<string, unknown> | undefined);
        if (buffer) this.socket.send(buffer);
      }
    },
    onPing() {
      this.pingClient = Date.now();
    },
  },
  getters: {
    isConnect: (state) => state.pingClient - state.pingDevice < 30000,
  },
});
