# PHASE 1 — GEOMETRY & EDITING (MONTH 2–3)

This document breaks **Phase 1** into **big tasks → small executable tasks**.
You should be able to work **top to bottom**, ticking items daily.

---

## 0. ENTRY CRITERIA (DO NOT START WITHOUT THIS)

* Phase 0 DXF import/export stable
* Internal geometry model exists (Line, Arc)
* Pan / Zoom / Snap / Grid working

---

## 1. CORE GEOMETRY STABILITY (WEEK 1)

### 1.1 Geometry primitives hardening

* Define `Line2D` (start, end)
* Define `Arc2D` (center, radius, startAngle, endAngle)
* Add bounding box calculation for each
* Add tolerance-based equality helpers

### 1.2 Geometry math utilities

* Distance point–point
* Distance point–line
* Angle normalization (0–2π)
* Arc length validation

### 1.3 Geometry safety checks

* Zero-length line detection
* Zero-radius arc detection
* Invalid arc angle detection

Deliverable: **No geometry object can exist in invalid state**

---

## 2. SELECTION SYSTEM (WEEK 1–2)

### 2.1 Basic selection

* Single click select
* Clear selection on empty click
* Selection highlight rendering

### 2.2 Multi-selection

* Shift + click toggle
* Selection list management

### 2.3 Box selection

* Left-drag → inside only
* Right-drag → crossing
* Bounding box intersection logic

### 2.4 Selection visuals

* Blue highlight color
* Bounding box draw
* Minimal grip points (corners only)

Deliverable: **User can reliably select exactly what they expect**

---

## 3. DRAWING TOOLS (WEEK 2)

### 3.1 Line tool

* Click–click creation
* Snap-aware endpoints
* Cancel on Esc

### 3.2 Arc tool

* Center–start–end workflow
* Snap support
* Direction consistency

### 3.3 Rectangle helper

* Two-corner input
* Internally creates 4 lines

### 3.4 Tool lifecycle

* Tool activate
* Tool preview
* Tool commit
* Tool cancel

Deliverable: **Only minimal creation tools, zero surprises**

---

## 4. TRANSFORMATION TOOLS (WEEK 3)

### 4.1 Move tool

* Drag move
* Numeric input (dx, dy)
* Snap respected

### 4.2 Rotate tool

* Pick rotation center
* Angle snap (15°, 30°, 45°)
* Preview before commit

### 4.3 Mirror tool

* X-axis mirror
* Y-axis mirror
* Picked line mirror
* Preview overlay

### 4.4 Transform validation

* Precision preserved
* No cumulative drift

Deliverable: **Exact, repeatable transformations**

---

## 5. UNDO / REDO SYSTEM (WEEK 3–4)

### 5.1 Command architecture

* Base `Command` interface
* `execute()` / `undo()` methods

### 5.2 Commands to implement

* Create entity
* Delete entity
* Move entities
* Rotate entities
* Mirror entities

### 5.3 Stack management

* Undo stack
* Redo stack
* Clear redo on new command

### 5.4 Stress testing

* 100+ undo/redo cycles
* Random operation sequences

Deliverable: **Ctrl+Z is unbreakable**

---

## 6. DUPLICATE & ZERO-LENGTH DETECTION (WEEK 4)

### 6.1 Detection logic

* Overlapping lines (tolerance-based)
* Coincident arcs
* Zero-length lines
* Zero-radius arcs

### 6.2 Background scan

* Run after import
* Run after edit

### 6.3 UI feedback

* Muted yellow highlight
* Issue counter display
* Select-on-click issue

### 6.4 Manual fix

* "Remove duplicates"
* "Remove zero-length"

Deliverable: **Clean geometry without auto-guessing**

---

## 7. UI STRUCTURE (PARALLEL TASK)

### 7.1 Main layout

* Fixed menu bar
* Left vertical toolbar
* Central canvas
* Status bar

### 7.2 Toolbar

* Select
* Line
* Arc
* Rectangle
* Move
* Rotate
* Mirror
* Delete

### 7.3 Status bar

* Units display
* Snap status
* Cursor coordinates
* Warning indicator

Deliverable: **Industrial, distraction-free UI**

---

## 8. DXF ROUND-TRIP SAFETY (FINAL WEEK)

### 8.1 Metadata preservation

* DXF handle tracking
* Layer retention

### 8.2 Export validation

* Entity count match
* Geometry precision check

### 8.3 Re-import verification

* Visual diff testing
* Bounding box consistency

Deliverable: **Import → Edit → Export → Re-import = identical**

---

## 9. EXIT CRITERIA (PHASE 1 COMPLETE)

You may proceed ONLY if:

* Real shop DXFs edited safely
* Undo never corrupts geometry
* Transform tools are trusted
* Duplicate cleanup works
* No DXF regression bugs

---

## DAILY RULE

> If a task touches geometry, write a test for it.

---

END OF PHASE 1

---
