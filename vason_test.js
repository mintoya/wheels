const Module = require("./vason_parser.js");
function vasonToAST(vHandle) {
  if (!vHandle || !vHandle.isValid()) {
    if (vHandle) vHandle.delete();
    return null;
  }
  const tagVal = vHandle.tag().value;
  let result;
  if (tagVal === Module.vason_tag.vason_STRING.value) {
    result = vHandle.asString();
  } else if (tagVal === Module.vason_tag.vason_PAIR.value) {
    result = {
      _pair: [vasonToAST(vHandle.getIdx(0)), vasonToAST(vHandle.getIdx(1))],
    };
  } else if (tagVal === Module.vason_tag.vason_TABLE.value) {
    const count = vHandle.countChildren();
    const items = [];
    for (let i = 0; i < count; i++) {
      items.push(vasonToAST(vHandle.getIdx(i)));
    }
    result = { _array: items };
  }
  vHandle.delete();
  return result;
}
function astToVason(node) {
  if (typeof node === "string") {
    return /[:{},[\]\\]/.test(node)
      ? `"${node.replaceAll("\\", "\\\\").replaceAll('"', '\\"')}"`
      : node.replaceAll('"', '\\"');
  }
  if (node && node._pair) {
    return `${astToVason(node._pair[0])}:${astToVason(node._pair[1])}`;
  }
  if (node && node._array) {
    const items = node._array.map(astToVason);
    return `{${items.join(",")}}`;
  }
  return "";
}
Module.onRuntimeInitialized = () => {
  const vason =
    "{keyboard:{tapdances:{{taps:{K:A},holds:{M:la,M:ls}}},layers:{{K:ESC,K:Q,K:W,K:E,K:R,K:T,K:Y,K:U,K:I,K:O,K:P,K:\\[,M:ls,K:A,K:S,K:D,K:F,K:G,K:H,K:J,K:K,K:L,K:;,K:',M:lc,K:Z,K:X,K:C,K:V,K:B,K:N,K:M,K:\\,,K:.,K:\\/,K:ENT,,,M:la,L:1,K:SPC,,,L:2,L:1,M:lm,},{K:TAB,K:1,K:2,K:3,K:4,K:5,K:6,K:7,K:8,K:9,K:0,K:\\],,,,,,,,,,,,K:`,,,,,,,,K:-,K:=,,K:\\\\,K:BKS},{,K:F1,K:F2,K:F3,K:F4,K:F5,K:F6,K:F7,K:F8,K:F9,K:F10,K:F11,,,,,,,,,K:UP,,,,,,,,,,,K:LEFT,K:DOWN,K:RIGHT,}}}}";
  const rootHandle = Module.parseString(vason);
  const ast2 = vasonToAST(rootHandle);
  console.dir(ast2, { depth: null, colors: true });
  const out = astToVason(ast2);
  console.log(out.length);
  console.log(vason.length);
  console.log(out);
};
