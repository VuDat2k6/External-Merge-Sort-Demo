
// ═══════════════════════════════════════════════════════════════
//  EXTERNAL MERGE SORT — FULL APPLICATION (FIXED)
// ═══════════════════════════════════════════════════════════════

const $ = id => document.getElementById(id);

// ── Utilities ─────────────────────────────────────────────────
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

function isSorted(arr) {
    for (let i = 1; i < arr.length; i++) if (arr[i - 1] > arr[i]) return false;
    return true;
}

function readBinaryFile(file) {
    return new Promise((res, rej) => {
        const r = new FileReader();
        r.onload = () => { try { res(Array.from(new Float64Array(r.result))); } catch (e) { rej(e); } };
        r.onerror = rej;
        r.readAsArrayBuffer(file);
    });
}

function downloadBinary(arr) {
    const buf = new Float64Array(arr).buffer;
    const blob = new Blob([buf], { type: 'application/octet-stream' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url; a.download = 'sorted_output.bin'; a.click();
    URL.revokeObjectURL(url);
}

// ── State Structure ───────────────────────────────────────────
// Each state snapshot contains:
// {
//   input: [...],        // original input
//   phase: 0|1|2|3|4,   // algorithm phase
//   pass: number,
//   group: number,
//   runsState: [        // runs for visualization
//     {
//       idx,            // run index
//       chips,          // values in this run
//       sorted,         // is this run internally sorted
//       cursor,         // current cursor position (-1 = not set)
//       consumed,       // [bool] per chip — has been merged out
//       done,           // run fully consumed
//       isNew,          // this is a newly created run
//       highlight       // this run has the current min
//     }
//   ],
//   output: [...],       // current output frame (in-memory)
//   outputWritten: [...],// runs that have been flushed to storage this step
//   newRunIdx: number,   // index of run being created in current group
//   comparison: [...],   // [{val, runIdx, isWinner}] — values being compared
//   winner: number,     // winning value in comparison
//   winnerRun: number,  // run index of winner
//   final: [...]        // final sorted array (phase 4 only)
// }

// ── Build Steps ──────────────────────────────────────────────
function buildSteps(input, K, chunkSize) {
    const steps = [];
    const N = input.length;

    // ── Phase 0: Initial data ──────────────────────────────────
    steps.push(makeStep({
        phase: 0, title: 'Dữ liệu ban đầu',
        desc: `Có <strong>${N}</strong> phần tử Float64 (8 bytes). File quá lớn không chứa trong RAM → dùng External Merge Sort.`,
        runsState: [], output: [], outputWritten: [],
        comparison: null, winner: null, winnerRun: -1
    }));

    // ── Phase 1: Chunking ────────────────────────────────────
    const chunks = [];
    for (let i = 0; i < N; i += chunkSize) {
        chunks.push(input.slice(i, Math.min(i + chunkSize, N)));
    }

    for (let c = 0; c < chunks.length; c++) {
        const rs = chunks.map((ch, idx) => ({
            idx, chips: [...ch], sorted: false,
            cursor: -1, consumed: new Array(ch.length).fill(false),
            done: false, isNew: false, highlight: false
        }));
        rs[c].cursor = 0; // show chunking pointer

        steps.push(makeStep({
            phase: 1, title: `Chia chunk #${c + 1}`,
            desc: `Chunk <strong>${c + 1}</strong> gồm <strong>${chunks[c].length}</strong> phần tử chưa sắp xếp.`,
            runsState: rs, output: [], outputWritten: [],
            comparison: null, winner: null, winnerRun: -1
        }));
    }

    // ── Phase 2: Sort each chunk into a run ──────────────────
    const sortedChunks = chunks.map(ch => [...ch].sort((a, b) => a - b));

    for (let r = 0; r < sortedChunks.length; r++) {
        const rs = sortedChunks.map((ch, idx) => ({
            idx, chips: [...ch], sorted: true,
            cursor: idx === r ? 0 : -1, consumed: new Array(ch.length).fill(false),
            done: false, isNew: idx === r, highlight: false
        }));

        steps.push(makeStep({
            phase: 2, title: `Sắp xếp Run ${r + 1}`,
            desc: `Sắp xếp nội bộ Run ${r + 1}: <strong>[${chunks[r].map(fmt).join(', ')}]</strong> → <em>[${sortedChunks[r].map(fmt).join(', ')}]</em>`,
            runsState: rs, output: [], outputWritten: [],
            comparison: null, winner: null, winnerRun: -1
        }));
    }

    // ── Run overview ──────────────────────────────────────────
    steps.push(makeStep({
        phase: 2, title: 'Tất cả Run đã sắp xếp',
        desc: `Có <strong>${sortedChunks.length}</strong> run đã sắp xếp. Mỗi run chứa dữ liệu có thứ tự nội bộ.`,
        runsState: sortedChunks.map((ch, idx) => ({
            idx, chips: [...ch], sorted: true,
            cursor: -1, consumed: new Array(ch.length).fill(false),
            done: false, isNew: false, highlight: false
        })),
        output: [], outputWritten: [],
        comparison: null, winner: null, winnerRun: -1
    }));

    // ── Phase 3: K-way Merge ──────────────────────────────────
    let currentRuns = sortedChunks.map(ch => [...ch]);
    let passNum = 1;

    while (currentRuns.length > 1) {
        const numGroups = Math.ceil(currentRuns.length / K);

        // Pass start
        steps.push(makeStep({
            phase: 3, pass: passNum, group: 0,
            title: `Pass ${passNum} — Bắt đầu`,
            desc: `Pass <strong>${passNum}</strong>: Gộp <strong>${currentRuns.length}</strong> run (K=${K}) → <strong>${numGroups}</strong> run mới.`,
            runsState: currentRuns.map((ch, idx) => ({
                idx, chips: [...ch], sorted: true,
                cursor: -1, consumed: new Array(ch.length).fill(false),
                done: false, isNew: false, highlight: false
            })),
            output: [], outputWritten: [],
            comparison: null, winner: null, winnerRun: -1
        }));

        for (let g = 0; g < numGroups; g++) {
            const gStart = g * K;
            const gEnd = Math.min(gStart + K, currentRuns.length);
            const groupRuns = currentRuns.slice(gStart, gEnd);
            const newRunIdx = gStart + Math.floor(gStart / K); // where new run goes

            if (groupRuns.length === 1) {
                // Single run passes through
                steps.push(makeStep({
                    phase: 3, pass: passNum, group: g + 1,
                    title: `Pass ${passNum}, Nhóm ${g + 1}`,
                    desc: `Nhóm <strong>${g + 1}</strong> chỉ có <strong>1 run</strong> → chuyển trực tiếp.`,
                    runsState: currentRuns.map((ch, idx) => ({
                        idx, chips: [...ch], sorted: true,
                        cursor: -1, consumed: new Array(ch.length).fill(false),
                        done: idx === gStart, isNew: false, highlight: false
                    })),
                    output: [], outputWritten: [],
                    comparison: null, winner: null, winnerRun: -1
                }));
                continue;
            }

            // Load phase
            steps.push(makeStep({
                phase: 3, pass: passNum, group: g + 1,
                title: `Pass ${passNum}, Nhóm ${g + 1} — Nạp`,
                desc: `Nạp <strong>${groupRuns.length}</strong> run vào bộ nhớ chính. Pointer ở đầu mỗi run.`,
                runsState: buildActiveRunsState(currentRuns, groupRuns, gStart, gEnd, [], -1, -1),
                output: [], outputWritten: [],
                comparison: null, winner: null, winnerRun: -1
            }));

            // K-way merge
            // cursors[i] = position in groupRuns[i]
            const cursors = groupRuns.map(() => 0);
            const outputFrame = [];
            let newRunChipIdx = 0;

            while (cursors.some((pos, i) => pos < groupRuns[i].length)) {
                // Find minimum
                let minVal = Infinity, minLocalIdx = -1;
                for (let i = 0; i < cursors.length; i++) {
                    if (cursors[i] >= groupRuns[i].length) continue;
                    const val = groupRuns[i][cursors[i]];
                    if (val < minVal) { minVal = val; minLocalIdx = i; }
                }
                if (minLocalIdx === -1) break;

                const globalWinnerRun = gStart + minLocalIdx;

                // Build comparison list
                const comparison = cursors.map((pos, i) => {
                    if (pos < groupRuns[i].length) return { val: groupRuns[i][pos], runIdx: gStart + i, isWinner: i === minLocalIdx };
                    return null;
                });

                // Step: compare
                steps.push(makeStep({
                    phase: 3, pass: passNum, group: g + 1,
                    title: `Pass ${passNum}, Nhóm ${g + 1} — Tìm min`,
                    desc: `So sánh <strong>${comparison.filter(c => c).length}</strong> giá trị → min = <strong>${fmt(minVal)}</strong> (Run ${minLocalIdx + 1}).`,
                    runsState: buildActiveRunsState(currentRuns, groupRuns, gStart, gEnd, cursors, minLocalIdx, gStart + minLocalIdx),
                    output: [...outputFrame], outputWritten: [],
                    comparison, winner: minVal, winnerRun: globalWinnerRun
                }));

                // Advance cursor BEFORE writing to output (so render shows cursor AFTER the taken value)
                cursors[minLocalIdx]++;

                // Write to output
                outputFrame.push(minVal);

                // Check if output frame is full (flush)
                if (outputFrame.length === chunkSize) {
                    steps.push(makeStep({
                        phase: 3, pass: passNum, group: g + 1,
                        title: `Pass ${passNum}, Nhóm ${g + 1} — Flush`,
                        desc: `Output frame đầy (<strong>${chunkSize}</strong> phần tử). ⚡ Ghi ra secondary storage.`,
                        runsState: buildActiveRunsState(currentRuns, groupRuns, gStart, gEnd, cursors, -1, -1),
                        output: [...outputFrame], outputWritten: [...outputFrame],
                        comparison: null, winner: null, winnerRun: -1
                    }));
                    outputFrame.length = 0;
                }

                // Step: show cursor advanced
                steps.push(makeStep({
                    phase: 3, pass: passNum, group: g + 1,
                    title: `Pass ${passNum}, Nhóm ${g + 1} — Ghi output`,
                    desc: `Ghi <strong>${fmt(minVal)}</strong> vào output frame.${outputFrame.length > 0 ? ` (${outputFrame.length}/${chunkSize})` : ''}`,
                    runsState: buildActiveRunsState(currentRuns, groupRuns, gStart, gEnd, cursors, -1, -1),
                    output: [...outputFrame], outputWritten: [],
                    comparison: null, winner: null, winnerRun: -1
                }));
            }

            // Flush remaining output
            if (outputFrame.length > 0) {
                steps.push(makeStep({
                    phase: 3, pass: passNum, group: g + 1,
                    title: `Pass ${passNum}, Nhóm ${g + 1} — Flush cuối`,
                    desc: `Kết thúc nhóm. Ghi <strong>${outputFrame.length}</strong> phần tử còn lại ra storage.`,
                    runsState: buildActiveRunsState(currentRuns, groupRuns, gStart, gEnd, cursors, -1, -1),
                    output: [], outputWritten: [...outputFrame],
                    comparison: null, winner: null, winnerRun: -1
                }));
                outputFrame.length = 0;
            }

            // Group done
            steps.push(makeStep({
                phase: 3, pass: passNum, group: g + 1,
                title: `Pass ${passNum}, Nhóm ${g + 1} — Hoàn tất`,
                desc: `Nhóm <strong>${g + 1}</strong> hoàn tất. Tạo run mới từ K-way merge.`,
                runsState: currentRuns.map((ch, idx) => ({
                    idx, chips: [...ch], sorted: true,
                    cursor: -1, consumed: new Array(ch.length).fill(false),
                    done: idx >= gStart && idx < gEnd, isNew: false, highlight: false
                })),
                output: [], outputWritten: [],
                comparison: null, winner: null, winnerRun: -1
            }));
        }

        // Build next level
        const nextRuns = [];
        for (let g = 0; g < numGroups; g++) {
            const gStart = g * K;
            const gEnd = Math.min(gStart + K, currentRuns.length);
            const grp = currentRuns.slice(gStart, gEnd);
            if (grp.length === 1) {
                nextRuns.push(grp[0]);
            } else {
                const merged = [];
                const cs = grp.map(() => 0);
                while (cs.some((pos, i) => pos < grp[i].length)) {
                    let mv = Infinity, mi = -1;
                    for (let i = 0; i < cs.length; i++) {
                        if (cs[i] >= grp[i].length) continue;
                        if (grp[i][cs[i]] < mv) { mv = grp[i][cs[i]]; mi = i; }
                    }
                    if (mi === -1) break;
                    merged.push(mv);
                    cs[mi]++;
                }
                nextRuns.push(merged);
            }
        }

        currentRuns = nextRuns;
        passNum++;
    }

    // ── Phase 4: Complete ────────────────────────────────────
    steps.push(makeStep({
        phase: 4, title: 'Hoàn tất!',
        desc: `Đã sắp xếp xong! Mảng kết quả: <em>[${currentRuns[0].map(fmt).join(', ')}]</em>`,
        runsState: currentRuns.map((ch, idx) => ({
            idx, chips: [...ch], sorted: true,
            cursor: -1, consumed: new Array(ch.length).fill(false),
            done: false, isNew: false, highlight: false
        })),
        output: [], outputWritten: [],
        comparison: null, winner: null, winnerRun: -1,
        final: [...currentRuns[0]]
    }));

    return steps;
}

function makeStep({ phase = 0, pass = 0, group = 0, title = '', desc = '', runsState = [], output = [], outputWritten = [], comparison = null, winner = null, winnerRun = -1, final = null }) {
    return {
        phase, pass, group, title, desc,
        state: {
            input: [], phase, pass, group,
            runsState, output, outputWritten,
            comparison, winner, winnerRun,
            final
        }
    };
}

// Build runsState for the active merge group
// cursors: array of positions per local run in groupRuns
// minLocalIdx: index in groupRuns that has the current min (-1 = none)
// highlightRun: global run index of highlighted run
function buildActiveRunsState(currentRuns, groupRuns, gStart, gEnd, cursors, minLocalIdx, highlightRun) {
    // For runs NOT in this group: show as-is
    // For runs IN this group: show with cursor positions
    const mergedOut = new Map(); // runIdx -> count consumed from it

    return currentRuns.map((chips, globalIdx) => {
        const inGroup = globalIdx >= gStart && globalIdx < gEnd;

        if (!inGroup) {
            return {
                idx: globalIdx, chips: [...chips], sorted: true,
                cursor: -1, consumed: new Array(chips.length).fill(false),
                done: false, isNew: false, highlight: false
            };
        }

        const localIdx = globalIdx - gStart;
        const pos = cursors.length > localIdx ? cursors[localIdx] : 0;
        const runLen = chips.length;

        // Determine consumed count
        let consumedCount = 0;
        if (pos > 0) consumedCount = pos;

        const consumed = chips.map((_, i) => i < consumedCount);

        return {
            idx: globalIdx,
            chips: [...chips],
            sorted: true,
            cursor: pos,
            consumed,
            done: pos >= runLen,
            isNew: false,
            highlight: globalIdx === highlightRun
        };
    });
}

// ── Renderer ─────────────────────────────────────────────────
const RUN_COLORS = ['#fcd34d', '#60a5fa', '#34d399', '#f472b6', '#a78bfa', '#fb923c'];

function chipHTML(val, extraClass = '') {
    const consumed = extraClass.includes('consumed');
    const highlight = extraClass.includes('highlight');
    const written = extraClass.includes('written');
    const atCursor = extraClass.includes('at-cursor');
    return `<div class="chip ${extraClass}${highlight ? ' highlight' : ''}${consumed ? ' consumed' : ''}${written ? ' written' : ''}${atCursor ? ' at-cursor' : ''}">${fmt(val)}</div>`;
}

function renderState(step) {
    const s = step.state;

    // Badge
    const badgeEl = $('stepBadge');
    badgeEl.textContent = step.phase;
    badgeEl.className = `algo-step-badge p${step.phase}`;
    $('stepTitle').textContent = step.title;
    $('stepDesc').innerHTML = step.desc;

    // Input preview
    if (s.input.length > 0) {
        $('inputChips').innerHTML = s.input.map((v, i) => chipHTML(v, `r${i % 6}`)).join('');
        $('inputCount').textContent = `${s.input.length} phần tử`;
    }

    // Runs area
    const runsArea = $('runsArea');
    if (!s.runsState || s.runsState.length === 0) {
        runsArea.innerHTML = `<div class="empty-state"><div class="icon">🔀</div><p>Các run sẽ hiển thị ở đây.</p></div>`;
    } else {
        runsArea.innerHTML = s.runsState.map((rs) => {
            const color = RUN_COLORS[rs.idx % RUN_COLORS.length];
            const baseCls = `r${rs.idx % 6}`;

            const chips = rs.chips.map((v, vi) => {
                let cls = baseCls;
                if (rs.consumed[vi]) cls += ' consumed';
                if (rs.cursor === vi) cls += ' highlight';
                return chipHTML(v, cls);
            }).join('');

            const cursorLabel = rs.cursor >= 0 && !rs.done
                ? `<span class="cursor-dot" style="background:${color};box-shadow:0 0 8px ${color}"></span>`
                : '';

            return `<div class="run-card${rs.highlight ? ' active' : ''}">
                <div class="run-header">
                    <div class="run-tag" style="background:${rs.done ? 'rgba(34,197,94,0.15)' : 'rgba(255,255,255,0.06)'};color:${rs.done ? '#22c55e' : color}">
                        ${rs.done ? '✓' : 'R' + (rs.idx + 1)}
                    </div>
                    <span class="run-info">
                        ${rs.chips.length} phần tử${rs.sorted ? ' · đã sắp' : ''}
                        ${rs.cursor >= 0 && !rs.done ? ` · cursor: ${rs.cursor + 1}/${rs.chips.length}` : ''}
                        ${rs.isNew ? ' · MỚI' : ''}
                    </span>
                </div>
                <div class="run-chips" style="position:relative">${chips}${cursorLabel}</div>
            </div>`;
        }).join('');
    }

    // Merge comparison panel
    const mergePanel = $('mergePanel');
    const mergeValues = $('mergeValues');
    if (s.comparison && s.comparison.length > 0) {
        mergePanel.style.display = 'block';
        mergeValues.innerHTML = s.comparison.map(c => {
            if (!c) return `<span class="vs" style="color:var(--text-3);padding:0 4px">—</span>`;
            const cls = `r${c.runIdx % 6}${c.isWinner ? ' highlight' : ''}`;
            return `<span style="display:inline-flex;flex-direction:column;align-items:center;gap:4px">
                <span class="chip ${cls}">${fmt(c.val)}</span>
                <span style="font-size:0.6rem;color:var(--text-3)">R${c.runIdx + 1}</span>
            </span>`;
        }).join('<span class="vs" style="color:var(--text-3);padding:0 4px;font-size:0.7rem">vs</span>');
    } else {
        mergePanel.style.display = 'none';
    }

    // Output frame
    const outSection = $('outputSection');
    const outChips = $('outputChips');
    if (s.output && s.output.length > 0) {
        outSection.style.display = 'block';
        outChips.innerHTML = s.output.map(v => chipHTML(v, 'written')).join('');
    } else {
        outSection.style.display = 'none';
    }

    // Final result
    const finalEl = $('finalResult');
    const finalChips = $('finalChips');
    if (step.phase === 4 && s.final && s.final.length > 0) {
        finalEl.style.display = 'block';
        finalChips.innerHTML = s.final.map(v => chipHTML(v, 'written')).join('');
    } else {
        finalEl.style.display = 'none';
    }
}

// ── App Controller ───────────────────────────────────────────
class App {
    constructor() {
        this.steps = [];
        this.stepIdx = -1;
        this.data = [];
        this.sorted = null;
        this.autoTimer = null;
        this.autoRunning = false;
        this.speed = 700;
        this.logOpen = true;

        this.bindUI();
        this.log('Sẵn sàng. Tải file .bin hoặc sinh dữ liệu ngẫu nhiên.', 'info');
    }

    bindUI() {
        const dropZone = $('dropZone');
        const fileInput = $('fileInput');
        fileInput.addEventListener('change', e => this.loadFile(e.target.files[0]));
        dropZone.addEventListener('dragover', e => { e.preventDefault(); dropZone.classList.add('drag-over'); });
        dropZone.addEventListener('dragleave', () => dropZone.classList.remove('drag-over'));
        dropZone.addEventListener('drop', e => {
            e.preventDefault();
            dropZone.classList.remove('drag-over');
            if (e.dataTransfer.files[0]) this.loadFile(e.dataTransfer.files[0]);
        });

        $('btnGenRandom').onclick = () => this.generate();

        $('speedSlider').oninput = () => {
            this.speed = parseInt($('speedSlider').value);
            $('speedLabel').textContent = this.speed + 'ms';
            if (this.autoRunning) { this.stopAuto(); this.startAuto(); }
        };

        $('btnStart').onclick = () => this.start();
        $('btnAuto').onclick = () => this.toggleAuto();
        $('btnPrev').onclick = () => this.prev();
        $('btnNext').onclick = () => this.next();
        $('btnReset').onclick = () => this.reset();
        $('btnDownload').onclick = () => this.download();
        $('logToggle').onclick = () => {
            this.logOpen = !this.logOpen;
            $('logBody').classList.toggle('collapsed', !this.logOpen);
        };

        document.addEventListener('keydown', e => {
            if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;
            if (e.key === 'ArrowRight') this.next();
            if (e.key === 'ArrowLeft') this.prev();
            if (e.key === ' ') { e.preventDefault(); this.toggleAuto(); }
        });
    }

    async loadFile(file) {
        if (!file) return;
        try {
            this.data = await readBinaryFile(file);
            $('fiName').textContent = file.name;
            $('fiSize').textContent = (file.size / 1024).toFixed(2) + ' KB';
            $('fiCount').textContent = this.data.length;
            const sorted = isSorted(this.data);
            $('fiSorted').textContent = sorted ? '✓ Có' : '✗ Chưa';
            $('fiSorted').className = `val ${sorted ? 'good' : ''}`;
            $('fileInfo').style.display = 'block';
            this.enableStart();
            this.log(`Đã đọc: ${file.name} (${this.data.length} phần tử, ${(file.size / 1024).toFixed(1)} KB)`, 'ok');
            if (sorted) this.log('Dữ liệu đã được sắp xếp từ trước!', 'warn');
            this.renderInputPreview();
        } catch (e) {
            this.log('Lỗi đọc file: ' + e.message, 'err');
        }
    }

    generate() {
        const n = parseInt($('genCount').value);
        const seed = Math.floor(Math.random() * 9999);
        this.data = genData(n, seed);
        $('fileInfo').style.display = 'none';
        this.enableStart();
        this.log(`Đã sinh ${n} số ngẫu nhiên (seed=${seed}).`, 'ok');
        this.log(`Input: [${this.data.map(fmt).join(', ')}]`, 'info');
        this.renderInputPreview();
    }

    enableStart() {
        $('btnStart').disabled = false;
        $('btnAuto').disabled = false;
        $('btnPrev').disabled = false;
        $('btnNext').disabled = false;
    }

    start() {
        const K = Math.max(2, Math.min(6, parseInt($('paramK').value) || 3));
        const chunkSize = Math.max(2, Math.min(10, parseInt($('paramChunk').value) || 3));
        $('paramK').value = K;
        $('paramChunk').value = chunkSize;

        this.stopAuto();
        this.sorted = null;
        $('btnDownload').disabled = true;
        this.steps = buildSteps(this.data, K, chunkSize);
        this.stepIdx = -1;
        this.log(`Bắt đầu: K=${K}, chunk=${chunkSize}, ${this.steps.length} bước.`, 'info');
        this.next();
    }

    reset() {
        this.stopAuto();
        this.stepIdx = -1;
        if (this.steps.length > 0) this.renderStep();
    }

    toggleAuto() {
        if (this.autoRunning) this.stopAuto();
        else this.startAuto();
    }

    startAuto() {
        if (!this.canNext()) return;
        this.autoRunning = true;
        $('icoPlay').style.display = 'none';
        $('icoPause').style.display = 'block';
        this.autoTimer = setInterval(() => {
            if (this.canNext()) this.next();
            else this.stopAuto();
        }, this.speed);
    }

    stopAuto() {
        this.autoRunning = false;
        $('icoPlay').style.display = 'block';
        $('icoPause').style.display = 'none';
        if (this.autoTimer) { clearInterval(this.autoTimer); this.autoTimer = null; }
    }

    canNext() { return this.stepIdx + 1 < this.steps.length; }
    canPrev() { return this.stepIdx > 0; }

    next() {
        if (!this.canNext()) { this.stopAuto(); return; }
        this.stepIdx++;
        this.renderStep();
    }

    prev() {
        if (!this.canPrev()) return;
        this.stepIdx--;
        this.renderStep();
    }

    renderInputPreview() {
        if (this.data.length > 0) {
            $('inputChips').innerHTML = this.data.map((v, i) => chipHTML(v, `r${i % 6}`)).join('');
            $('inputCount').textContent = `${this.data.length} phần tử`;
        }
    }

    renderStep() {
        const step = this.steps[this.stepIdx];
        if (!step) return;

        $('btnPrev').disabled = !this.canPrev();
        $('btnNext').disabled = !this.canNext();
        $('btnAuto').disabled = !this.canNext();

        const pct = Math.round(((this.stepIdx + 1) / this.steps.length) * 100);
        $('progFill').style.width = pct + '%';
        $('progText').textContent = `Bước ${this.stepIdx + 1} / ${this.steps.length}`;
        $('progPct').textContent = pct + '%';

        if (step.phase === 4 && step.state.final) {
            this.sorted = step.state.final;
            $('btnDownload').disabled = false;
        }

        // Selective logging
        if (step.phase === 2 && step.state.runsState?.some(r => r.isNew)) {
            const r = step.state.runsState.find(r => r.isNew);
            this.log(`✓ Sắp xếp Run ${r.idx + 1} thành công`, 'ok');
        } else if (step.phase === 3 && step.phase !== this.steps[Math.max(0, this.stepIdx - 1)]?.phase) {
            this.log(`→ Pass ${step.state.pass} bắt đầu`, 'info');
        } else if (step.phase === 4) {
            this.log(`Hoàn tất sắp xếp! ✓ (${this.sorted.length} phần tử)`, 'ok');
        }

        renderState(step);
    }

    download() {
        if (!this.sorted) return;
        downloadBinary(this.sorted);
        this.log('Đã tải sorted_output.bin', 'ok');
    }

    log(msg, type = '') {
        const now = new Date();
        const t = now.toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        const el = document.createElement('div');
        el.className = 'log-entry';
        el.innerHTML = `<span class="log-t">[${t}]</span><span class="log-m ${type}">${msg}</span>`;
        $('logBody').appendChild(el);
        $('logBody').scrollTop = $('logBody').scrollHeight;
    }
}

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

// Test
const input = genData(12, 1234);
console.log("Input:", input.join(", "));
const steps = buildSteps(input, 3, 3);
console.log("Total steps:", steps.length);
console.log("Step 0:", JSON.stringify({phase: steps[0].phase, title: steps[0].title}));
console.log("Step 1:", JSON.stringify({phase: steps[1].phase, title: steps[1].title}));
console.log("Step 2:", JSON.stringify({phase: steps[2].phase, title: steps[2].title}));
console.log("Last step:", JSON.stringify({phase: steps[steps.length-1].phase, title: steps[steps.length-1].title}));

// Test canNext logic
let stepIdx = -1;
console.log("Initial stepIdx:", stepIdx);
console.log("canNext (before first next):", stepIdx + 1 < steps.length);
stepIdx++;
console.log("After next, stepIdx:", stepIdx, "canNext:", stepIdx + 1 < steps.length);
stepIdx++;
console.log("After 2nd next, stepIdx:", stepIdx, "canNext:", stepIdx + 1 < steps.length);