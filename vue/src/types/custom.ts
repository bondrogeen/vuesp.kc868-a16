import type { IMessageDevice, IMessageSettings, TypeConstMessage, TypeKEYS } from 'vuesp-components/types';

export interface IA16Device extends IMessageDevice {
  portIn1: number;
  portIn2: number;
  portOut1: number;
  portOut2: number;
  adc1: number;
  adc2: number;
  adc3: number;
  adc4: number;
}

export type MyTypeKEYS = (TypeKEYS & {}) | 'DEVICE';

interface IKeyMessageDevice {
  key: Extract<MyTypeKEYS, 'DEVICE'>;
  object?: IMessageDevice | Partial<IMessageDevice>;
}

export type TypeMessage = (TypeConstMessage & {}) | IKeyMessageDevice;

export interface IMyMessageSettings extends IMessageSettings {
  EthIp?: [];
  EthSubnet?: [];
  EthGateway?: [];
  EthDns?: [];
}
