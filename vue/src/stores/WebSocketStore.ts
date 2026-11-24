import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';
import { KEY } from '@/generated/protocol';
import type { TKey } from '@/generated/protocol';
import { useAppStore } from './AppStore.ts';

import type { TMessage } from '@/types';
import type { IWebSocketStore, IPort, IDallas, IProgress, IMessage, IDiscovery, IInfo, IDevice, IBuffer, IEvent, IModbus, IModbusProfile, IModbusRaw, IModbusVarView, ISettings } from '@/types';

const initialState = (): IWebSocketStore => ({
  main: {
    ports: {},
    info: {
      id: 0,
      firmware: [],
      totalBytes: 0,
      usedBytes: 0,
      uptime: 0,
      name: '',
      board: 0,
      freeHeap: 0,
      minFreeHeap: 0,
      maxBlock: 0,
      stackFree: 0,
      fragmentation: 0,
      loopLastUs: 0,
      loopMaxUs: 0,
      loopP95Us: 0,
      loopCount: 0,
      clients: 0,
      scripts: 0,
      peers: 0,
      wifi: 0,
      diagnostics: 0,
      ethernet: 0,
      ethSpeed: 0,
      ethIp: [0, 0, 0, 0],
    },
    device: { command: 0, input: 0, output: 0, analog: 0, adc1: 0, adc2: 0, adc3: 0, adc4: 0, now: 0, list: 0, message: '' },
    buffer: { empty: 0, head: 0, tail: 0, count: 0, data: [] },
    event: { command: 0, subCommand: 0, value1: 0, value2: 0, name: '' },
    modbus: {
      command: 0, role: 0, address: 1, baud: 9600, dataBits: 8, parity: 0, stopBits: 1,
      status: 0, lastFunction: 0, lastAddress: 0, lastError: 0, requests: 0, errors: 0,
      varCount: 0, reserved: [0, 0, 0], updated: 0, point: 0, ok: 0, pad: [0, 0],
      value: 0, text: '', vars: [],
    },
    modbusProfile: { command: 0, version: 1, deviceCount: 0, pointCount: 0, varCount: 0, reserved: 0, devices: [], points: [], vars: [] },
    modbusRaw: { command: 0, slave: 1, function: 3, address: 0, length: 1, autoCrc: 1, request: [], requestLen: 0, response: [], responseLen: 0, status: 0, reserved: 0, elapsed: 0 },
    vars: {},
    dallas: {},
    discovery: {},
    slots: {},
  },
  settings: {
    wifiDhcp: 1,
    wifiMode: 1,
    authMode: 1,
    version: 1,
    device: 0,
    wifiIp: [],
    wifiSubnet: [],
    wifiGateway: [],
    wifiDns: [],
    wifiSsid: '',
    wifiPass: '',
    authLogin: '',
    authPass: '',
    ethEnabled: 1,
    ethDhcp: 1,
    ethIp: [192, 168, 23, 135],
    ethSubnet: [255, 255, 255, 0],
    ethGateway: [192, 168, 23, 1],
    ethDns: [192, 168, 23, 1],
    discovery: 0,
    discoveryInterval: 5,
    discoveryPort: 0,
  },
  progress: { status: 0, empty: 0, size: 0, length: 0 },
  message: { type: 0, active: 0, handler: 0, date: 0, index: 0, id: 0, len: 0, text: '' },
});

export const useWebSocketStore = defineStore('webSocketStore', {
  state: initialState,
  actions: {
    SET_SETTINGS(value: ISettings) {
      this.settings = value;
    },
    SET_PROGRESS(value: IProgress) {
      this.progress = value;
    },
    CLEAR_PROGRESS() {
      this.progress = { status: 0, empty: 0, size: 0, length: 0 };
    },
    SET_PORT(port: IPort) {
      const gpio = port.gpio.toString();
      this.main.ports[gpio] = port;
    },
    SET_DALLAS(data: IDallas) {
      const name = (data.address || []).map((i) => (i < 16 ? `0${i.toString(16)}` : i.toString(16))).join('');
      this.main.dallas[name] = data;
    },
    SET_DISCOVERY(data: IDiscovery) {
      const id = data.id.toString(16);
      this.main.discovery[id] = data;
    },
    SET_INFO(value: IInfo) {
      this.main.info = value;
    },
    SET_DEVICE(value: IDevice) {
      this.main.device = value;
    },
    SET_MODBUS(value: IModbus) {
      this.main.modbus = value;
      const profile = this.main.modbusProfile;
      if (!profile.varCount) return;
      const vars = { ...this.main.vars };
      for (let i = 0; i < value.varCount && i < profile.varCount; i++) {
        const def = profile.vars[i];
        const live = value.vars[i];
        if (!def?.id || !live) continue;
        vars[def.id] = { id: def.id, name: def.name, type: def.type, unit: def.unit, decimals: def.decimals, value: live.value, text: live.text, quality: live.quality, updated: value.updated };
      }
      this.main.vars = vars;
    },
    SET_MODBUSRAW(value: IModbusRaw) {
      this.main.modbusRaw = value;
    },
    SET_MODBUSPROFILE(value: IModbusProfile) {
      this.main.modbusProfile = value;
      const vars: Record<string, IModbusVarView> = {};
      for (let i = 0; i < value.varCount; i++) {
        const def = value.vars[i];
        const live = this.main.modbus.vars[i];
        if (!def?.id) continue;
        vars[def.id] = { id: def.id, name: def.name, type: def.type, unit: def.unit, decimals: def.decimals, value: live?.value ?? 0, text: live?.text ?? '', quality: live?.quality ?? 1, updated: this.main.modbus.updated };
      }
      this.main.vars = vars;
    },
    SET_BUFFER(value: IBuffer) {
      this.main.buffer = value;
    },
    SET_EVENT(value: IEvent) {
      this.main.event = value;
    },
    SET_MESSAGE(message: IMessage) {
      this.message = message;
      const { type } = message;
      if (type === 1) {
        const appStore = useAppStore();
        appStore.setNotification(message)
      }
      if (type === 2) {
        const index = message.index;
        this.main.slots[index || 0] = message;
      }
    },
    onMessage({ key, object }: TMessage) {
      switch (key) {
        case KEY.SETTINGS:
          this.SET_SETTINGS(object);
          return;
        case KEY.PROGRESS:
          this.SET_PROGRESS(object);
          return;
        case KEY.PORT:
          this.SET_PORT(object);
          return;
        case KEY.DALLAS:
          this.SET_DALLAS(object);
          return;
        case KEY.DISCOVERY:
          this.SET_DISCOVERY(object);
          return;
        case KEY.MESSAGE:
          this.SET_MESSAGE(object);
          return;
        case KEY.INFO:
          this.SET_INFO(object);
          return;
        case KEY.DEVICE:
          this.SET_DEVICE(object);
          return;
        case KEY.MODBUS:
          this.SET_MODBUS(object);
          return;
        case KEY.MODBUSPROFILE:
          this.SET_MODBUSPROFILE(object);
          return;
        case KEY.MODBUSRAW:
          this.SET_MODBUSRAW(object);
          return;
        case KEY.BUFFER:
          this.SET_BUFFER(object);
          return;
        case KEY.EVENT:
          this.SET_EVENT(object);
          return;
        default:
          return;
      }
    },
    onSend(key: TKey | number, object?: object) {
      const store = useWebSocket();
      store.onSend(key, object);
    },
  },
});
