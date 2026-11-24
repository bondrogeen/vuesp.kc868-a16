export interface IModbusFramePoint {
  type: number;
  flags: number;
  registers?: number;
  scale?: number;
  offset?: number;
}

const TYPE_U16 = 0;
const TYPE_S16 = 1;
const TYPE_U32 = 2;
const TYPE_S32 = 3;
const TYPE_U64 = 4;
const TYPE_S64 = 5;
const TYPE_FLOAT32 = 6;
const TYPE_FLOAT64 = 7;
const TYPE_BOOL = 8;
const TYPE_STRING = 9;
const TYPE_BIT = 10;
const TYPE_ENUM = 11;
const TYPE_BCD = 12;

const FLAG_WORD_LITTLE = 1;
const FLAG_BYTE_LITTLE = 2;

export const registersForType = (type: number, stringRegisters = 1): number => {
  if (type === TYPE_STRING) return Math.min(16, Math.max(1, stringRegisters || 1));
  if (type === TYPE_U32 || type === TYPE_S32 || type === TYPE_FLOAT32) return 2;
  if (type === TYPE_U64 || type === TYPE_S64 || type === TYPE_FLOAT64) return 4;
  return 1;
};

const uint16 = (value: number): number => Math.trunc(value) & 0xffff;

const encodeWords = (point: IModbusFramePoint, value: number): number[] => {
  const count = registersForType(point.type, point.registers ?? 1);
  const scale = point.scale && point.scale !== 0 ? point.scale : 1;
  const raw = (value - (point.offset ?? 0)) / scale;
  const words = new Array<number>(count).fill(0);

  switch (point.type) {
    case TYPE_U16:
    case TYPE_S16:
    case TYPE_BOOL:
    case TYPE_BIT:
    case TYPE_ENUM:
    case TYPE_BCD:
      words[0] = uint16(raw);
      break;
    case TYPE_U32:
    case TYPE_S32: {
      const packed = Math.trunc(raw) >>> 0;
      words[0] = (packed >>> 16) & 0xffff;
      words[1] = packed & 0xffff;
      break;
    }
    case TYPE_FLOAT32: {
      const buffer = new ArrayBuffer(4);
      const view = new DataView(buffer);
      view.setFloat32(0, raw, false);
      words[0] = view.getUint16(0, false);
      words[1] = view.getUint16(2, false);
      break;
    }
    case TYPE_U64:
    case TYPE_S64: {
      const packed = BigInt.asUintN(64, BigInt(Math.trunc(raw)));
      words[0] = Number((packed >> 48n) & 0xffffn);
      words[1] = Number((packed >> 32n) & 0xffffn);
      words[2] = Number((packed >> 16n) & 0xffffn);
      words[3] = Number(packed & 0xffffn);
      break;
    }
    case TYPE_FLOAT64: {
      const buffer = new ArrayBuffer(8);
      const view = new DataView(buffer);
      view.setFloat64(0, raw, false);
      words[0] = view.getUint16(0, false);
      words[1] = view.getUint16(2, false);
      words[2] = view.getUint16(4, false);
      words[3] = view.getUint16(6, false);
      break;
    }
    default:
      words[0] = uint16(raw);
  }

  if (point.flags & FLAG_WORD_LITTLE) {
    if (count === 2) {
      [words[0], words[1]] = [words[1], words[0]];
    } else if (count === 4) {
      [words[0], words[3]] = [words[3], words[0]];
      [words[1], words[2]] = [words[2], words[1]];
    }
  }

  const bytes: number[] = [];
  for (const word of words) {
    const hi = (word >>> 8) & 0xff;
    const lo = word & 0xff;
    if (point.flags & FLAG_BYTE_LITTLE) bytes.push(lo, hi);
    else bytes.push(hi, lo);
  }

  return bytes;
};

export const encodePointValue = (point: IModbusFramePoint, value: number): number[] => encodeWords(point, value);

export const crc16 = (bytes: number[]): number => {
  let crc = 0xffff;
  for (const byte of bytes) {
    crc ^= byte & 0xff;
    for (let bit = 0; bit < 8; bit++) {
      crc = crc & 1 ? (crc >>> 1) ^ 0xa001 : crc >>> 1;
    }
  }
  return crc & 0xffff;
};

export const withCrc = (bytes: number[]): number[] => {
  const crc = crc16(bytes);
  return [...bytes, crc & 0xff, (crc >>> 8) & 0xff];
};

export const buildReadFrame = (slave: number, fc: number, start: number, count: number): number[] =>
  withCrc([slave & 0xff, fc & 0xff, (start >>> 8) & 0xff, start & 0xff, (count >>> 8) & 0xff, count & 0xff]);

export const buildReadResponse = (slave: number, fc: number, data: number[]): number[] => withCrc([slave & 0xff, fc & 0xff, data.length & 0xff, ...data]);

export const formatHex = (bytes: number[]): string => bytes.map((byte) => (byte & 0xff).toString(16).padStart(2, '0').toUpperCase()).join(' ');
