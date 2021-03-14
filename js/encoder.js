function AKEncoder () {
}

/* send bytes, not number of chars */
AKEncoder.prototype.len = function (str) {
    return (new TextEncoder().encode(str)).length.toString(36).padStart(8, "0")
}

AKEncoder.prototype.encodeBool = function (name, value) {
    return `BOO${this.len(name)}00000001${name}${value ? '1' : '0'}`
}

AKEncoder.prototype.encodeInt = function (name, intval) {
    let str = intval.toString();
    return `INT${this.len(name)}${this.len(str)}${name}${str}`;
}

AKEncoder.prototype.encodeFloat = function (name, floatval) {
    let str = floatval.toString();
    return `FLO${this.len(name)}${this.len(str)}${name}${str}`;
}

AKEncoder.prototype.encodeString = function (name, str) {
    return `STR${this.len(name)}${this.len(str)}${name}${str}`;
}

AKEncoder.prototype.encodeNul = function (name, str) {
    return `NUL${this.len(name)}00000000${name}`
}

AKEncoder.prototype.encodeObject = function (name, object) {
    let encoded = '';
    console.log(object)
    for (let k of Object.keys(object)) {
        switch (typeof(object[k])) {
            // array are encoded as object as they can't hold multiple type
            case 'object':
                if (object[k] === null) {
                    encoded += this.encodeNul(k, object[k])
                } else {
                    encoded += this.encodeObject(k, object[k])
                }
                continue;
            case 'number':
                if (Number.isInteger(object[k])) { encoded += this.encodeInt(k, object[k]); continue}
                encoded += this.encodeFloat(k, object[k]);
                continue;
            case 'string':
                encoded += this.encodeString(k, object[k]);
                continue;
            case 'boolean':
                encoded += this.encodeBool(k, object[k]);
                continue;
        }
    }
    return `OBJ${this.len(name)}${this.len(encoded)}${name}${encoded}`
}
