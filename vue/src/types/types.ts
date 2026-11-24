import type { IDashboardItem, IDialog, IMenuItem, INotification, IPackage, ISuggestion } from 'vuesp-components/types';
import type { IDallas, IDevice, IDiscovery, IEvent, IInfo, IBuffer, IMessage, IModbus, IModbusProfile, IModbusRaw, IPort, IProgress, ISettings, TKey } from '@/generated/protocol';

export type { TMessage } from '@/generated/protocol';

export type TSend = (key: TKey | number, object?: object) => void;


export interface IAppStore {
  menu: IMenuItem[];
  dashboard: IDashboardItem[];
  suggestions: ISuggestion[];
  theme: string;
  dialog: IDialog;
  dialogInfo: boolean;
  pkg: IPackage;
  notifications: INotification[];
}


export interface IWebSocket {
  socket: WebSocket | null;
  pingClient: number;
  pingDevice: number;
  isInit: boolean;
  infoLoaded: boolean;
}


export interface IWebSocketStore {
  settings: ISettings;
  main: IStateMain;
  progress: IProgress;
  message: IMessage;
}

export interface IModbusVarView {
  id: string;
  name: string;
  type: number;
  unit: string;
  decimals: number;
  value: number;
  text: string;
  quality: number;
  updated: number;
}

export interface IStateMain {
  dallas: Record<string, IDallas>;
  discovery: Record<string, IDiscovery>;
  info: IInfo;
  ports: Record<string, IPort>;
  device: IDevice;
  buffer: IBuffer;
  event: IEvent;
  modbus: IModbus;
  modbusProfile: IModbusProfile;
  modbusRaw: IModbusRaw;
  vars: Record<string, IModbusVarView>;
  slots: Record<number, IMessage>;
}
