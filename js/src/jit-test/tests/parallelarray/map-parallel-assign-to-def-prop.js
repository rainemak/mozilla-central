load(libdir + "parallelarray-helpers.js");

function wrapInObject(v) {
    var obj = {f: 2};
    obj.f += v;
    return obj;
}

compareAgainstArray(range(0, 64), "map", wrapInObject);
