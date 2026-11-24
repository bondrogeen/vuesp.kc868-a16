export const examples = [
  { name: 'variable', value: "$u0=0;$u0=$u0+5;$u9=4561213;$i0=-1000;$s0='text:';$s0=$s0+$u0;" },
  { name: 'if', value: 'if:$u0==0;$u0=1;else;$u0=0;end;log($u0)' },
  { name: 'log()', value: "$u0=0;$u0=$u0+5;log($u0);$u9=4561213;$i0=-1000;log($u9,$i0);$s0='text:';$s0=$s0+$u0;$a0={1,2,3,4,5};log($s0, $a0)" },
  { name: 'wait()', value: "log('start');wait(1s);log('stop');wait(1s);" },
  { name: 'while', value: '$u0=0;while:$u0<3;log($u0);$u0=$u0+1;wait(1s);end;' },
  { name: 'array', value: '$a0={1,2,3};log(len($a0));set($a0,0,10);log(get($a0,0));' },
  { name: 'event()', value: "on('btn_c');log('button',$e0,$e1);end;" },
];

export const enum ScriptType {
  SCRIPT_START = 10,
  SCRIPT_STOP = 11,
  SCRIPT_STOP_ALL = 12,
  SCRIPT_REMOVE = 13,
  SCRIPT_GET_ALL_SLOT = 20,
}
