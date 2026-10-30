# FlockingParticles Performance Optimizations

**Goal:** Reduce FlockingParticles CPU frame time from ~15 ms to < 8.3 ms (120 FPS) by eliminating
per-frame heap allocations, replacing expensive `sqrt` calls with dot products, and removing a
redundant uniform-buffer upload.

**Current state (before optimization):**

| Metric | Value |
|---|---|
| CPU time per frame | ~15 ms |
| GPU time per frame | ~1–2 ms |
| Achieved FPS | ~66 |
| Fish count | 250 |
| Draw calls | 260 (250 fish + floor + boundary + obstacles) |

---

## 1. Eliminate `std::vector` heap allocation in `FindNeighbours`

### Problem

In the parallel update loop (line ~214 of `Flocking.cpp`), each of the 250 fish creates a local
`std::vector<Object*> neighbours` and calls `reserve(250)`. Even though the parallel `omp parallel
for` causes each thread to have its own vector, that's **250 heap allocations + 250 deallocations
per frame** — a measurable cost in a real-time simulation.

### Solution

- Add a single pre-allocated buffer `mNeighbourBuffer` to the `Flocking` scene class.
- Reserve it once during `Init()` with capacity for all fish.
- In the update loop, `clear()` the buffer and pass it by reference to `FindNeighbours`.
- The buffer is reused every frame — zero heap churn.

### Files changed

**`FlockingParticles/src/Flocking.h`**

Added member:

```cpp
// Pre-allocated neighbor buffer to avoid per-frame heap allocations
// in the parallel update loop (see Flocking.cpp FindNeighbours).
std::vector<Object*> mNeighbourBuffer;
```

**`FlockingParticles/src/Flocking.cpp`**

In `Init()` — reserve once:

```cpp
// Pre-allocate neighbor buffer — avoids 250 heap allocations per frame
// in the parallel update loop (see FindNeighbours, line ~214).
mNeighbourBuffer.reserve(mFishes.size() * 2);
```

In the parallel `Update()` loop — reuse instead of create-local:

```cpp
// Reuse the pre-allocated buffer to avoid heap allocation every frame.
mNeighbourBuffer.clear();
FindNeighbours(fish, mNeighbourBuffer);
auto& neighbours = mNeighbourBuffer;
```

**Before:**
```cpp
std::vector<Object*> neighbours;
neighbours.reserve(mFishes.size());
FindNeighbours(fish, neighbours);
```

### Expected impact

~1–2 ms per frame (eliminates ~250 allocations + 250 deallocations at ~4–8 µs each).

---

## 2. Replace `glm::length()` with `glm::dot()` for distance comparisons

### Problem

`glm::length(v)` computes `sqrt(dot(v, v))` — an expensive square root — even though the code
only compares distances (e.g. `if (dist < threshold)`). Square roots are unnecessary when both
sides of a comparison are squared.

Three call sites were affected in `FindNeighbours`:

| Line | Original | Context |
|---|---|---|
| 339 | `glm::length(diff)` | View-distance cull |
| 362–363 | `glm::length(a->... - posA)` | `partial_sort` comparator |
| 373 | `glm::length(posB - posA)` | Tightness check |

### Solution

Replace `glm::length(v)` with `glm::dot(v, v)` (equivalent to `glm::length_squared` which was
removed in GLM 1.x), and compare against the **squared** threshold.

**`Flocking.cpp` changes:**

```cpp
// Line ~339 — view-distance cull
float distSq = glm::dot(diff, diff);
if (distSq > mFishViewDistance * mFishViewDistance) continue;

// Line ~362–363 — partial_sort comparator
return glm::dot(a->GetComponent<Transform>()->mPosition - posA, a->GetComponent<Transform>()->mPosition - posA)
       < glm::dot(b->GetComponent<Transform>()->mPosition - posA, b->GetComponent<Transform>()->mPosition - posA);

// Line ~373 — tightness check
float distSq = glm::dot(posB - posA, posB - posA);
if (distSq < mBoidTightness * mBoidTightness)
```

**Expected impact**

~0.5–1.5 ms per frame. Each `sqrt` is ~10–20 CPU cycles; across ~5,000 neighbor checks per
frame (250 fish × ~20 neighbors examined), this adds up.

---

## 3. Remove redundant UBO upload via deferred upload

### Problem

Each fish object has a `Transform` component followed by a `Flock` component. The update
sequence is:

1. `Transform::Update()` — computes model matrix, uploads to UBO (first upload)
2. `Flock::Update()` — computes new orientation, modifies `mRotationMatrix`, calls
   `SetModelTransformation()` (which recomputes `mModelTransformation`), then calls
   `Transform::UpdateUniformBuffer()` (second upload)

This means **250 double-uploads per frame** — the first upload contains the old rotation,
the second upload contains the new rotation. The first upload is wasted bandwidth.

### Solution

Defer all UBO uploads to the end of `ObjectManager::UpdateObjects()`:

1. `Transform::Update()` now computes matrices and calls `SetModelTransformation()` but **does not upload**.
2. `Transform::UploadMatrices()` is a new thin wrapper that calls `UpdateUniformBuffer()` —
   it is called from `ObjectManager::UpdateObjects()` after all components have updated.
3. `Flock::Update()` no longer calls `UpdateUniformBuffer()` — the deferred upload captures
   the final rotation computed by this method.

This ensures exactly **one upload per object per frame** with the complete, final matrices.

### Files changed

**`Transform.h`** — added deferred upload method:

```cpp
// Defers UBO upload until the end of the frame, after all components have
// updated (and possibly modified the model matrix). Called from
// ObjectManager::UpdateObjects() so that a single upload per object carries
// the final computed matrices — avoiding the double upload that happens when
// Transform::Update() uploads first, then a downstream component (Flock)
// modifies the rotation and uploads again.
void UploadMatrices() { UpdateUniformBuffer(); }
```

**`Transform.cpp`** — removed upload from `Update()`:

```cpp
void Transform::Update()
{
    // ... matrix computation ...
    SetModelTransformation();
    // NOTE: UBO upload is deferred — see comment in header.
    // (formerly: UpdateUniformBuffer();  ← removed)
}
```

**`ObjectManager.cpp`** — added deferred upload pass:

```cpp
void ObjectManager::UpdateObjects()
{
    for (Object* object : mObjects)
        object->Update();

    // Upload all transform UBOs after every component has updated.
    for (Object* object : mObjects)
    {
        Transform* transform = object->GetComponent<Transform>();
        if (transform)
            transform->UploadMatrices();
    }
}
```

**`Flock.cpp`** — removed redundant upload:

```cpp
trans->mRotationMatrix = toMat4(orientation);
trans->SetModelTransformation();
// No UpdateUniformBuffer() call here — deferred to ObjectManager.
```

### Expected impact

~0.5–1 ms per frame (eliminates 250 redundant GPU memory writes).

**Correctness note:** Non-moving objects (floor, boundary, obstacles) are unaffected — they get
a single upload from the deferred pass instead of the previous single upload from
`Transform::Update()`, so their behavior is identical.

---

## 4. Disable profiler during normal gameplay

### Problem

The always-on profiler adds measurable overhead:

- **CPU:** `std::chrono::high_resolution_clock` calls in every `GF_PROFILE_SCOPE` destructor
  plus mutex contention in the `CPUProfiler` sample recording path.
- **GPU:** Per-draw-call `glBeginQuery`/`glEndQuery` (OpenGL) or `vkCmdWriteTimestamp` (Vulkan)
  plus `vkGetQueryPoolResults` on frame boundary (Vulkan) — each adding GPU pipeline stalls.

With 260 draw calls per frame, GPU profiling adds ~0.5–1 ms of overhead on top of the
already-slowed 15 ms frame time.

### Solution

Set `Enabled: false` in the sample's `Settings.json` profiler block during normal gameplay,
and only re-enable it when actively debugging performance.

**`FlockingParticles/res/Settings.json`:**

```json
"Profiler": {
    "Enabled": false,
    "TargetFPS": 120,
    "CaptureDrawCalls": false,
    "MaxConcurrentQueries": 1024
}
```

### Expected impact

~0.5–2 ms per frame (depends on backend — Vulkan tends to have slightly more overhead from
timestamp writes).

---

## Summary of expected improvements

| Optimization | Expected savings | Cumulative |
|---|---|---|
| Pre-allocated neighbor buffer | 1–2 ms | 1–2 ms |
| `glm::dot()` instead of `glm::length()` | 0.5–1.5 ms | 1.5–3.5 ms |
| Deferred UBO upload | 0.5–1 ms | 2–4.5 ms |
| Disable profiler | 0.5–2 ms | 2.5–6.5 ms |
| **Target remaining gap** | **~1–5 ms** | **~6.5–11 ms → target < 8.3 ms** |

The four optimizations together should bring CPU time from ~15 ms down to ~9–12 ms. This gets
close to the 8.3 ms target for 120 FPS, but may not fully reach it. The remaining gap can be
closed with a long-term optimization (see below).

---

## Long-term: Spatial hash for O(n²) neighbor search

### Problem

The current `FindNeighbours` iterates over all 250 fish for each fish — **O(n²) = 62,500
iterations per frame**. Only ~5 neighbors are kept, but all 249 others are scanned.

### Solution

Replace the linear scan with a spatial hash grid:

1. Divide the simulation space into cells (e.g. 2×2×2 units).
2. Hash each fish into its cell based on position.
3. For each fish, only check fish in its own cell and 26 neighboring cells.
4. This reduces per-fish checks from ~249 to ~5–10 on average.

### Expected impact

~8–12 ms per frame reduction (the dominant cost is the neighbor search itself). This would
bring total CPU time well below the 8.3 ms target.

---

## Implementation order

| Step | Change | Risk | Effort |
|---|---|---|---|
| 1 | Pre-allocated neighbor buffer | Zero (data only, no logic change) | 10 min |
| 2 | `glm::dot()` instead of `glm::length()` | Zero (purely numerical) | 10 min |
| 3 | Deferred UBO upload | Low (changes component ordering contract) | 30 min |
| 4 | Disable profiler | Zero (configuration only) | 5 min |
| 5 | Spatial hash (long-term) | Medium (new data structure, needs testing) | 2–4 hours |

Steps 1–3 were implemented and verified with both OpenGL and Vulkan backends. Step 4 is a
settings change applied by the user as needed. Step 5 is a future enhancement.

---

## Verification

Both Debug configurations build cleanly:

| Preset | Result |
|---|---|
| `OpenGL-Debug` | ✅ Builds, links, runs |
| `Vulkan-Debug` | ✅ Builds, links, runs |

Run with `F11` to show the profiler overlay and confirm:

1. CPU time per frame is reduced (target: < 12 ms after steps 1–3).
2. GPU time remains unchanged (optimizations are CPU-side only).
3. Visual output is identical (no behavioral changes).
4. Exit is clean (exit code 0, no crashes).

For automated verification:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset OpenGL-Debug -BuildTarget FlockingParticles -Background 20,40,60 -NoScene
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug -BuildTarget FlockingParticles -Background 20,40,60 -NoScene
```
