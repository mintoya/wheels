import VASONmoduleFactory from "./vason_wasm/vason_parser.js";
const VASONmodule = await VASONmoduleFactory();
function vasonToAST(vHandle) {
  if (!vHandle || !vHandle.isValid()) {
    if (vHandle) vHandle.delete();
    return null;
  }
  const tagVal = vHandle.tag().value;
  let result;
  if (tagVal === VASONmodule.vason_tag.vason_STRING.value) {
    result = vHandle.asString();
  } else if (tagVal === VASONmodule.vason_tag.vason_PAIR.value) {
    result = {
      _pair: [vasonToAST(vHandle.getIdx(0)), vasonToAST(vHandle.getIdx(1))],
    };
  } else if (tagVal === VASONmodule.vason_tag.vason_TABLE.value) {
    const count = vHandle.countChildren();
    const items = [];
    for (let i = 0; i < count; i++) {
      items.push(vasonToAST(vHandle.getIdx(i)));
    }
    result = { _array: items };
  }
  if (vHandle.istop()) {
    vHandle.free();
  }
  vHandle.delete();
  return result;
}
function astToVason(node) {
  if (typeof node === "string") {
    return /[:{},[\]\\]/.test(node)
      ? `"${node.replaceAll('"', '\\"')}"`
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
/**
 * @typedef {string | { _pair: [VasonNode, VasonNode] } | { _array: VasonNode[] }} VasonNode
 */
class Vason {
  constructor(data) {
    this.data = data;
  }
  /**
   * @param {string} vasonStr
   * @returns {VasonNode}
   */
  static parse(vasonStr) {
    return new Vason(vasonToAST(VASONmodule.parseString(vasonStr)));
  }
  _isValidNode(val) {
    if (typeof val === "string") return true;
    if (
      val && typeof val === "object" &&
      (Array.isArray(val._array) || Array.isArray(val._pair))
    ) return true;
    return false;
  }
  /**
   * @param {string | number} key
   * @returns {VasonNode}
   */
  get(key) {
    if (typeof key === "string") {
      if (this.data && this.data._array) {
        for (let i = 0; i < this.data._array.length; i++) {
          const child = this.data._array[i];
          if (child && child._pair && child._pair[0] === key) {
            return new Vason(child._pair[1]);
          }
        }
      }
      return null;
    }
    if (typeof key === "number") {
      if (
        this.data && this.data._array && key >= 0 &&
        key < this.data._array.length
      ) {
        return new Vason(this.data._array[key]);
      } else if (this.data && this.data._pair && (key === 0 || key === 1)) {
        return new Vason(this.data._pair[key]);
      }
      return null;
    }
    return null;
  }
  /**
   * @param { number} index
   * @param {any} val
   */
  set(index, val) {
    if (typeof index !== "number") {
      throw new Error("Set requires a numeric index.");
    }
    if (!this._isValidNode(val)) {
      throw new Error(
        "Value must be a string, { _array: [...] }, or { _pair: [...] }",
      );
    }
    if (this.data && this.data._array) {
      if (index >= this.data._array.length) {
        for (let i = this.data._array.length - 1; i < index; i++) {
          this.set(i, "");
        }
      }
      this.data._array[index] = val;
    } else if (this.data && this.data._pair) {
      if (index !== 0 && index !== 1) {
        throw new Error("Pair index must be 0 (key) or 1 (value).");
      }
      this.data._pair[index] = val;
    } else {
      throw new Error("Cannot set on a string node.");
    }
  }
  /**
   * @param {VasonNode} val
   */
  append(val) {
    if (!this._isValidNode(val)) {
      throw new Error(
        "Value must be a string, { _array: [...] }, or { _pair: [...] }",
      );
    }
    if (this.data && this.data._array) {
      this.data._array.push(val);
    } else {
      throw new Error("Can only append to a table (_array).");
    }
  }
  /**
   * @returns {string}
   */
  stringify() {
    return astToVason(this.data);
  }
}
export default Vason;
