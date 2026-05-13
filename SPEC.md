# External Merge Sort — Web Demo (Redesigned)

## 1. Concept & Vision

Interactive visualization of the **External Merge Sort** algorithm — an educational tool for understanding how large datasets that don't fit in memory are sorted using a K-way merge strategy. Users generate random data, watch step-by-step animations, and see the algorithm's multi-pass nature unfold.

**Feel**: Clean, focused, algorithm-first. No distractions — just clear visual feedback on every step. Dark theme with vibrant data colors.

---

## 2. Design Language

### Aesthetic
Minimal dark UI with glassmorphism panels. Data is the star — colors encode information, not decoration.

### Colors
| Token | Hex | Usage |
|-------|-----|-------|
| `--bg` | `#0a0e1a` | Page background |
| `--surface` | `rgba(15,23,42,0.85)` | Cards/panels |
| `--primary` | `#3b82f6` | Actions |
| `--success` | `#22c55e` | Completed / output |
| `--warning` | `#f59e0b` | Highlighted / active |
| `--text` | `#f1f5f9` | Body text |
| `--text-muted` | `#94a3b8` | Secondary text |

### Run Colors (cycled)
`#fcd34d` `#60a5fa` `#34d399` `#f472b6` `#a78bfa` `#fb923c`

### Typography
- **UI**: Inter (400–800 weight)
- **Data/Numbers**: JetBrains Mono

### Spacing
Base unit 4px. Padding: 8/12/16/20/24px.

### Motion
- Transitions: 150–250ms ease-out
- Data highlights: 250ms cubic-bezier(0.34, 1.56, 0.64, 1) (bounce)
- Cursor blink: 0.8s ease-in-out infinite alternate

---

## 3. Layout & Structure

```
┌─────────────────────────────────────────────────────┐
│  Header: Logo + Title + Subtitle                     │
├─────────────────────────────────────────────────────┤
│  Controls: [Generate] [Count] [K] [Speed] [Playback] │
├─────────────────────────────────────────────────────┤
│  Info Bar: Total / Runs / K / Pass / Step            │
├─────────────────────────────────────────────────────┤
│  Algorithm Panel: Phase # + Title + Description      │
├─────────────────────────────────────────────────────┤
│  ┌──────────────────┐ ┌──────────────────────────┐  │
│  │  Main Memory     │ │  Secondary Storage       │  │
│  │  - Chunks        │ │  - Runs list (R1..Rn)    │  │
│  │  - Output frame  │ │  - Cursor indicators     │  │
│  └──────────────────┘ └──────────────────────────┘  │
├─────────────────────────────────────────────────────┤
│  Progress Bar + Step counter                        │
├─────────────────────────────────────────────────────┤
│  Final Output (shown when done)                     │
├─────────────────────────────────────────────────────┤
│  Log Console (collapsible)                          │
└─────────────────────────────────────────────────────┘
```

### Responsive
- ≥900px: 2-column visualization
- <900px: single column stack

---

## 4. Features & Interactions

### 4.1 Data Generation
- Seeded random doubles (seed=42, range -1000..1000)
- Count options: 8, 12, 16, 20, 24
- Auto-enables Start button

### 4.2 K-way Merge Control
- User-selectable K: 2, 3, 4, 5
- Affects chunk size, run count, and merge groups

### 4.3 Algorithm Phases

| Phase | Name | Description |
|-------|------|-------------|
| 0 | Initial | Shows raw input array |
| 1 | Chunking | Splits data into K-sized chunks |
| 2 | Sorting | Sorts each chunk into initial runs |
| 3 | K-way Merge | Multi-pass merge (main algorithm) |
| 4 | Complete | Shows final sorted result |

### 4.4 Playback Controls
- **Start**: Reset and run from beginning
- **Auto**: Play/pause step-by-step with adjustable speed (200–2000ms)
- **Prev/Next**: Manual step navigation
- **Reset**: Return to step 0
- **Keyboard**: Arrow Left/Right, Space for auto

### 4.5 Speed Control
- Range slider: 200ms to 2000ms
- Changes take effect immediately in auto mode

### 4.6 Step Information
- Algorithm panel shows: phase number, step title, detailed description
- Descriptions use `<strong>` for key values and `<em>` for results

### 4.7 Visualization
- **Main Memory**: Shows input chunks (Phase 1) and output frame during merge
- **Secondary Storage**: Runs list with cursor indicators
  - Green dot: active cursor position
  - Dim: consumed values
  - Highlight: run with current minimum
- **Final Output**: Only shown at phase 4

### 4.8 Progress Tracking
- Progress bar with percentage
- Step counter: "Bước X / Y"
- Stats bar: total elements, run count, K value, current pass, step

### 4.9 Log Console
- Timestamped entries
- Color-coded: info, success, warning, error
- Clearable
- Collapsible

---

## 5. Component Inventory

### Header
- Logo (gradient box with grid icon)
- H1 title + subtitle

### Control Panel
- Generate button (primary style)
- Count dropdown (8–24)
- K dropdown (2–5)
- Speed slider + label
- Playback buttons: Start, Auto, Prev, Next, Reset

### Info Bar (stats row)
- 5 stats: Total, Runs, K, Pass, Step
- Each has icon + label + monospace value

### Algorithm Panel
- Phase number badge (circle)
- Title + HTML description

### Visualization Panel (2 columns)
- **Left**: Main Memory section
  - Data chips for chunks (Phase 1)
  - Output box (Phase 3)
- **Right**: Secondary Storage section
  - Runs list with per-run chips
  - Cursor indicators
  - Run tags with color coding

### Progress Bar
- Thin gradient fill (primary → success)
- Step counter + percentage

### Final Output
- Green border box
- Centered data chips
- Only shown at completion

### Log Console
- Collapsible panel
- Monospace text
- Clear button

---

## 6. Technical Approach

### Architecture
Single HTML file. No build step. All CSS/JS embedded.

### JavaScript Modules

```
// Utilities
generateData(count, seed)     → number[]
formatVal(v)                  → string

// Algorithm
buildSteps(input, K)          → Step[]

// Rendering
renderSnapshot(snapshot)       → DOM updates

// Controller
DemoApp class
  generate()   — create data + steps
  start()      — reset + first step
  prev/next()  — navigation
  startAuto()  — interval loop
  stopAuto()
  render()     — update UI
  log(msg, type)
```

### State / Snapshots

```javascript
Step = {
  phase: 0|1|2|3|4,
  title: string,
  desc: string,           // HTML
  snapshot: Snapshot
}

Snapshot = {
  input: number[],
  runs: Run[],            // current level runs
  output: number[],       // current output frame
  newRuns: Run[]|null,   // for pass transitions
  final: number[]|null,   // for phase 4
  phase: number,
  pass: number,
  group: number,
  action: string,
  cursorRuns: Cursor[],
  highlightIdx: number,
  writeOutput: number[]
}

Run = {
  chips: number[],
  sorted: boolean,
  originalIdx: number
}

Cursor = {
  idx: number,            // run index
  done: boolean,
  current: number,        // position in run chips
  highlight?: boolean
}
```

### ExternalSorter (C++ Backend)

Unchanged from original — works as-is.

```cpp
void sortDoublesBinary(
    const std::string& inputPath,
    const std::string& outputPath,
    std::size_t chunkMB,
    LogFn log,
    ProgressFn progress
);
```

---

## 7. Algorithm Steps Breakdown

### Phase 0: Initial Data
Single step showing raw input array.

### Phase 1: Chunking
For each K-sized chunk: show chunk label + elements.
- Input: `[5,2,8,1,6,3,9,4]`, K=3
- Steps: C1:[5,2,8], C2:[1,6,3], C3:[9,4]

### Phase 2: Sorting
For each chunk: show before → after sort.
- Steps: sort C1, sort C2, sort C3
- Final: show all sorted runs ready

### Phase 3: K-way Merge (Multi-pass)
For each pass:
1. Pass start summary
2. For each group:
   - Load: show cursor positions
   - For each merge step: select min → write output
   - Group done: show new run

### Phase 4: Complete
Show final sorted array.

---

## 8. Key Design Decisions

1. **No page abstraction** — simplified to chips. Each run displays all its values as chips, with cursor positions marked by green dots and consumed values dimmed.

2. **K is configurable** — unlike the original which hardcoded K=3. This lets users see how different K values affect run count.

3. **Phase-based organization** — algorithm panel clearly shows which phase the current step belongs to, making the multi-pass nature explicit.

4. **Clean state machine** — each step is a snapshot, not a transformation. Going back rebuilds from initial state.

5. **HTML descriptions** — step descriptions use HTML markup for key value highlighting, making technical descriptions readable.

6. **No over-engineering** — no pages, no frame counters, no complex animations. Just: chunks → sorted runs → K-way merge → result.
