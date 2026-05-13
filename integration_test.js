const fs = require('fs');
const code = fs.readFileSync('c:\\Users\\ACER\\Desktop\\CS523\\External Merge Sort Demo\\index.html', 'utf8');
const scriptMatch = code.match(/<script>([\s\S]*?)<\/script>/);
const script = scriptMatch[1];

// Test harness
function fmt(v) {
    const a = Math.abs(v);
    if (a >= 1000) return Math.round(v).toString();
    if (a >= 100) return v.toFixed(1);
    return v.toFixed(2);
}
function seededRand(seed) {
    let s = seed >>> 0;
    return () => { s = Math.imul(s, 1664525) + 1013904223 >>> 0; return s / 0xFFFFFFFF; };
}
function genData(n, seed = 7) {
    const rng = seededRand(seed);
    return Array.from({ length: n }, () => Math.round((rng() * 2000 - 1000) * 100) / 100);
}

// Mock DOM
const dom = {};
const mockEl = (id) => {
    if (!dom[id]) dom[id] = { innerHTML: '', textContent: '', style: {}, className: '', disabled: false };
    return dom[id];
};
global.document = {
    getElementById: mockEl,
    addEventListener: () => {},
    createElement: () => ({ style: {}, appendChild: () => {} })
};

// Extract and eval just the buildSteps + helper functions (not App class)
const buildStepsMatch = script.match(/function buildSteps\([\s\S]*?^}/m);
const makeStepMatch = script.match(/function makeStep\([\s\S]*?^\}/m);
const buildActiveMatch = script.match(/function buildActiveRunsState\([\s\S]*?^}/m);

if (buildStepsMatch) {
    eval(buildStepsMatch[0]);
    console.log("buildSteps function found and evaluated");
}
if (makeStepMatch) {
    eval(makeStepMatch[0]);
    console.log("makeStep function found and evaluated");
}
if (buildActiveMatch) {
    eval(buildActiveMatch[0]);
    console.log("buildActiveRunsState function found and evaluated");
}

// Test
try {
    const input = genData(12, 1234);
    const steps = buildSteps(input, 3, 3);
    console.log("\n=== RESULTS ===");
    console.log("Total steps:", steps.length);
    console.log("\nStep by step:");
    steps.forEach((step, i) => {
        console.log("Step " + i + ": phase=" + step.phase + ", title=\"" + step.title + "\", runsState.length=" + ((step.state.runsState || []).length));
    });
    
    // Simulate the App logic
    let stepIdx = -1;
    let sorted = null;
    
    // Simulate next() 3 times
    for (let sim = 0; sim < 3; sim++) {
        stepIdx++;
        const step = steps[stepIdx];
        console.log("\nAfter next() #" + (sim+1) + ": stepIdx=" + stepIdx + ", phase=" + step.phase + ", title=\"" + step.title + "\"");
        if (step.state.final) { sorted = step.state.final; console.log("SORTED!", sorted); }
    }
    
    console.log("\ncanNext test: stepIdx " + stepIdx + " steps.length " + steps.length + " canNext: " + (stepIdx + 1 < steps.length));
    
} catch(e) {
    console.error("ERROR:", e.message);
    console.error(e.stack);
}
