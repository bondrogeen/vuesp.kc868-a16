export interface IMapItem {
  key: string;
  type: string;
  address: string;
  target: string;
  fc: number;
  start: number;
  count: number;
  access: string;
  child?: boolean;
  pointIndex?: number;
  notApplied?: boolean;
}

export interface IMapDetails {
  title: string;
  subtitle?: string;
  type: string;
  address: string;
  fc: number;
  count: number;
  countLabel: string;
  access: string;
  hex?: string;
  source?: string;
  live?: string;
  readFrame: string;
  responseFrame?: string;
  note?: string;
  notApplied?: boolean;
}
