# Digital Gong — Mounting Assembly Technical Plan

## 1. Overview

This document specifies the mounting system for the Digital Gong: four MDF quarter-annulus segments suspended vertically on a repurposed clothes rail frame via vibration-isolated rubber grommet mounts. Each segment carries one contact microphone feeding a Raspberry Pi. The mounting system must hold the assembly rigid under striking force while maintaining complete vibration isolation between all four segments and between segments and frame.

---

## 2. Assembly Concept

A steel clothes rail provides two vertical uprights and a weighted, wheeled base. Four horizontal crossbars (10mm mild steel rod) are installed through holes drilled in the uprights. Each crossbar is locked against rotation by a through-bolt at each upright junction. The MDF segments are mounted to the crossbars via M6 bolts with rubber grommets on both sides of the MDF, ensuring no rigid contact between MDF and steel at any point.

Each segment has three mounting bolts arranged in a triangle around its contact mic position. This distributes the support around the primary pickup zone and prevents flex at the centre of the striking surface.

---

## 3. Key Dimensions

### 3.1 Gong Assembly

| Parameter | Value |
|---|---|
| Outer radius | 356mm |
| Outer diameter (assembled) | 712mm |
| Inner radius | 75mm |
| Inner diameter (screen cutout) | 150mm |
| Segment wall thickness (radial) | 281mm |
| MDF thickness | 25mm |
| Gap between segments | 2mm (1mm inset per side) |
| Estimated weight (4 segments) | ~7 kg |
| Estimated total weight (with screen, Pi, veneer, hardware) | 8–10 kg |

### 3.2 Clothes Rail Frame

| Parameter | Value |
|---|---|
| Height | 160cm |
| Depth (front to back) | 43cm |
| Width (adjustable) | 110–150cm |
| Upright tube OD | ~25mm (to be confirmed on arrival) |
| Upright tube wall thickness | ~1.5mm (typical for this product class) |
| Maximum load capacity | 113 kg static |
| Wheels | 4 swivel, 2 with brakes |

### 3.3 Crossbar Positions

All measurements are relative to the centre of the gong assembly. The gong centre height on the frame is user-determined; recommended range is 90–110cm from ground for comfortable playing.

| Crossbar | Position (from gong centre) | Role | Bolts carried |
|---|---|---|---|
| CB1 | 230mm above centre | Top pair apex points | 2 (one per seg A, B) |
| CB2 | 70mm above centre | Top pair base points | 4 (two per seg A, B) |
| CB3 | 70mm below centre | Bottom pair base points | 4 (two per seg C, D) |
| CB4 | 230mm below centre | Bottom pair apex points | 2 (one per seg C, D) |

Spacing between crossbars: CB1→CB2 = 160mm, CB2→CB3 = 140mm, CB3→CB4 = 160mm.

CB2 and CB3 are positioned 70mm from the horizontal centre line (not at the centre) to ensure bolt holes are drilled well clear of the segment straight edges (minimum ~25mm from any edge).

### 3.4 Bolt Positions on Crossbars

All positions measured as horizontal distance from the gong centre (which aligns with the midpoint of each crossbar). Negative = left, positive = right when viewed from the front.

**CB1 (top, 2 bolts):**

| Bolt | Segment | Horizontal offset | Purpose |
|---|---|---|---|
| CB1-L | A (top-left) | −150mm | Apex of segment A triangle |
| CB1-R | B (top-right) | +150mm | Apex of segment B triangle |

**CB2 (upper-middle, 4 bolts):**

| Bolt | Segment | Horizontal offset | Purpose |
|---|---|---|---|
| CB2-1 | A (top-left) | −310mm | Segment A outer base point |
| CB2-2 | A (top-left) | −80mm | Segment A inner base point |
| CB2-3 | B (top-right) | +80mm | Segment B inner base point |
| CB2-4 | B (top-right) | +310mm | Segment B outer base point |

**CB3 (lower-middle, 4 bolts):** Vertical mirror of CB2.

| Bolt | Segment | Horizontal offset | Purpose |
|---|---|---|---|
| CB3-1 | C (bottom-left) | −310mm | Segment C outer base point |
| CB3-2 | C (bottom-left) | −80mm | Segment C inner base point |
| CB3-3 | D (bottom-right) | +80mm | Segment D inner base point |
| CB3-4 | D (bottom-right) | +310mm | Segment D outer base point |

**CB4 (bottom, 2 bolts):** Vertical mirror of CB1.

| Bolt | Segment | Horizontal offset | Purpose |
|---|---|---|---|
| CB4-L | C (bottom-left) | −150mm | Apex of segment C triangle |
| CB4-R | D (bottom-right) | +150mm | Apex of segment D triangle |

### 3.5 Contact Mic Positions

Each mic sits approximately at the centroid of its segment's bolt triangle. Approximate positions (from gong centre):

| Segment | Mic X | Mic Y | Notes |
|---|---|---|---|
| A (top-left) | −180mm | −123mm | Roughly equidistant from all three bolts |
| B (top-right) | +180mm | −123mm | Mirror of A |
| C (bottom-left) | −180mm | +123mm | Mirror of A vertically |
| D (bottom-right) | +180mm | +123mm | Mirror of B vertically |

### 3.6 Depth Stack (Side View)

The bolt assembly from front to back at each mounting point:

| Component | Thickness | Running total from front face |
|---|---|---|
| Front nut (M6) | ~5mm | Protrudes 5mm in front of MDF |
| Front washer | ~2mm | — |
| Front rubber grommet | ~5mm | — |
| MDF segment | 25mm | 25mm |
| Rear rubber grommet | ~5mm | 30mm |
| Rear washer | ~2mm | 32mm |
| Air gap / standoff | variable | — |
| Crossbar (10mm rod) | 10mm | ~42–47mm from rear face |
| Rear washer | ~2mm | — |
| Rear nut (M6) | ~5mm | — |

Total bolt length required: approximately 70–80mm. M6 × 80mm recommended as standard; M6 × 90mm if additional Pi clearance is needed.

Standoff from rear MDF face to crossbar centre: approximately 15–20mm with standard grommets. This must clear the Raspberry Pi (17mm board height plus standoffs and cables). The inner bolts on CB2/CB3 (positions CB2-2, CB2-3, CB3-2, CB3-3) are closest to the Pi zone — if clearance is tight, either use longer bolts or adjust CB2/CB3 further from the centre line.

---

## 4. Bill of Materials

### 4.1 Frame and Crossbars

| Item | Specification | Qty | Source | Notes |
|---|---|---|---|---|
| Clothes rail | 160cm H × 43cm D, 110–150cm adjustable W, locking wheels | 1 | Amazon UK | Confirm upright tube OD on arrival |
| Steel round bar | 10mm OD, mild steel, cut to upright spacing + 30mm each end | ~4.5m total (4 pieces) | eBay / metal supplier | Exact length determined after measuring rail upright spacing |

### 4.2 Anti-Rotation Hardware (Crossbar-to-Upright Joints)

| Item | Specification | Qty | Notes |
|---|---|---|---|
| M5 × 30mm bolts | Hex head or socket cap | 8 | One per crossbar-upright junction (4 crossbars × 2 uprights) |
| M5 nuts | Standard hex | 8 | |
| M5 washers | Standard flat | 16 | One each side |

### 4.3 Segment Mounting Hardware

| Item | Specification | Qty | Notes |
|---|---|---|---|
| M6 × 80mm bolts | Hex head or socket cap | 12 | 3 per segment × 4 segments. Use 90mm if Pi clearance is tight. |
| M6 nuts | Standard hex, or nyloc for vibration resistance | 12 | |
| M6 flat washers | Standard | 24 | One each side of each grommet |
| M6 penny washers | Large OD (~25mm) | 24 | Optional: spreads load against grommets. Use in place of standard washers if grommets are soft. |
| Rubber grommets | ID to suit M6 (~6.5mm), suitable OD for MDF hole (~12–15mm) | 24 | 2 per bolt × 12 bolts. Source: RS Components, Screwfix, Amazon UK. Search "M6 rubber grommet" or "anti-vibration mount M6". |

### 4.4 Vibration Isolation Strip

| Item | Specification | Qty | Source | Notes |
|---|---|---|---|---|
| Neoprene rubber strip | 2mm thick, self-adhesive, solid (not sponge/foam) | ~2m | Amazon UK | Applied to the straight edges of each segment to fill the 2mm assembly gap |

### 4.5 Tools Required

| Tool | Specification | Notes |
|---|---|---|
| Centre punch | Standard | Essential for marking hole positions on steel. Prevents drill bit wandering. |
| HSS drill bit set | Must include 5mm, 6.5mm, 10mm | HSS (High Speed Steel) rated for metal. Do not use wood bits on steel. |
| Cordless drill | Standard | Any decent cordless drill will handle 10mm through 1.5mm tube wall. Use slow speed, moderate pressure. |
| Cutting oil or WD-40 | Any | Apply a drop to each hole before drilling. Reduces heat, extends bit life, cleaner cut. |
| Hacksaw | Standard | For cutting 10mm round bar to length. A pipe cutter also works but may not grip 10mm rod. |
| Metal file | Flat, medium cut | Deburr cut ends of crossbars and drilled holes. |
| Tape measure + marker | Steel tape, fine-tip marker or scriber | For marking hole positions on uprights and crossbars. |
| Spirit level | Small | For aligning crossbar heights. |
| Spanners / socket set | 8mm (for M5), 10mm (for M6) | For tightening bolts. Two spanners needed per bolt (hold nut, turn bolt). |

---

## 5. Assembly Procedure

### 5.1 Preparation (Before Assembly)

1. Receive clothes rail. Assemble per instructions. Measure and record the upright tube outer diameter and wall thickness. Measure the internal spacing between uprights at the intended gong centre height.
2. Cut four crossbars from 10mm round bar. Length = internal upright spacing + 30mm overhang each end (15mm protruding past outer wall of each upright). Deburr all cut ends with a file.
3. Determine the gong centre height on the frame (recommended: 90–110cm from ground). Mark this point on both uprights.

### 5.2 Drilling the Uprights

4. From the gong centre mark, measure up 230mm and mark both uprights: this is the CB1 height. Mark CB2 at 70mm above centre, CB3 at 70mm below centre, CB4 at 230mm below centre. Use a spirit level across both uprights to ensure marks are at identical heights.
5. Centre-punch each mark. Drill 10mm holes through both walls of each upright at all four crossbar heights (8 holes total per upright, front and back wall = 16 drill operations, but each pass-through goes through both walls in one operation if aligned). Apply cutting oil before each hole.

### 5.3 Installing Crossbars

6. Slide each crossbar through the holes in both uprights. Centre it so equal lengths protrude on each side.
7. For each crossbar-upright junction: drill a 5mm hole through the upright wall AND through the crossbar at the intersection point (perpendicular to the crossbar axis). This is the anti-rotation through-bolt. Centre-punch the upright wall first, then drill through into the crossbar in a single operation. Insert an M5 bolt, add washer and nut, tighten.
8. Repeat for all 8 junctions (4 crossbars × 2 uprights). Each crossbar should now be completely locked against rotation.

### 5.4 Drilling the Crossbars for Segment Mounting

9. Mark bolt positions on each crossbar using the horizontal offsets from Section 3.4. Measure from the gong centre point (midpoint of crossbar) outward in each direction.
10. Centre-punch each mark. Drill 6.5mm holes at each position (12 holes total across all four crossbars). Deburr all holes.

### 5.5 Drilling the MDF Segments

11. Mark bolt hole positions on each MDF segment. These are the same positions specified in Section 3.4, but measured relative to the segment geometry. Transfer measurements carefully using the segment's arc edges and straight edges as reference.
12. Drill holes in the MDF at approximately 10mm diameter (oversized relative to the M6 bolt shaft). This oversize is critical — the bolt must not contact the MDF. The only things touching the MDF should be the rubber grommets.

### 5.6 Mounting the Segments

13. For each mounting bolt position: thread the M6 × 80mm bolt from the front. The assembly order from front to back is: bolt head → washer → rubber grommet → MDF (through oversized hole) → rubber grommet → washer → crossbar (through 6.5mm hole) → washer → nut.
14. Tighten the nut until the grommets are lightly compressed but not crushed. The MDF should feel secure but not rigidly clamped — some compliance in the grommets is desirable for vibration isolation.
15. Repeat for all 12 bolt positions.

### 5.7 Finishing

16. Apply self-adhesive neoprene strip to the straight edges of each segment where they face adjacent segments. The strip fills the 2mm gap and prevents accidental rigid contact between segments.
17. Mount the screen and Raspberry Pi in the central 150mm cutout (mounting method TBD — see Open Questions).
18. Attach contact mics at the positions specified in Section 3.5.

---

## 6. Critical Design Constraints

1. **No rigid contact between any segment and the frame.** Every mechanical connection passes through rubber grommets. The bolt shaft passes through an oversized hole in the MDF and never touches the MDF directly.

2. **No rigid contact between adjacent segments.** The 2mm gap is filled with compressible neoprene, not a rigid material.

3. **Crossbars CB2 and CB3 must not foul the Raspberry Pi.** The Pi is mounted behind the centre of the gong assembly. The inner bolts (CB2-2, CB2-3, CB3-2, CB3-3 at ±80mm horizontal offset) are closest to the Pi zone. Verify clearance after initial dry-fit before final tightening.

4. **Bolt holes in MDF must be oversized.** A 10mm hole for an M6 bolt (6mm shaft) provides ~2mm clearance all around. If the bolt contacts the MDF wall, vibration isolation is defeated at that point.

5. **Triangular bolt layout must surround the contact mic.** The mic should sit approximately at the centroid of each bolt triangle. This prevents the striking zone from being cantilevered away from the support points.

---

## 7. Open Questions

| # | Question | Impact | Resolution needed before |
|---|---|---|---|
| 1 | Confirm upright tube diameter (19mm, 25mm, or 32mm?) | Determines crossbar diameter and hole sizes. If uprights are 19mm, a 10mm crossbar hole may be too large relative to the tube. | After rail arrives |
| 2 | How to mount screen and Pi in central cutout | Must not create a rigid vibration path. Needs its own isolation or bracket from the crossbars. | Before final assembly |
| 3 | Exact crossbar lengths | Depends on upright spacing at the gong centre height. Adjustable rail means spacing could vary. | After rail arrives and width is set |
| 4 | Pi clearance at CB2/CB3 inner bolts | May need longer bolts (90mm) or position adjustment. Depends on Pi HAT configuration and cable routing. | During dry-fit |
| 5 | Surface veneer material and application | Affects final weight and may require pre-application before drilling MDF. | Before drilling MDF |
| 6 | Grommet stiffness / durometer | Softer grommets = better isolation but more wobble under striking. May need to test options. | Before ordering grommets in bulk |

---

## 8. Notes on Technical Drawing Standards

For reference, the standard format for this kind of assembly plan in a formal engineering context would be a **General Arrangement (GA) drawing** conforming to **BS 8888** (UK) or **ISO 128 / ISO 5457** (international). A GA drawing includes:

- A **title block** (drawing number, revision, date, scale, author, material, tolerances)
- **Orthographic projection views** (typically front, side, and plan) with full dimensioning conforming to ISO 129
- A **Bill of Materials (BOM)** table with item numbers cross-referenced to the drawing via balloon callouts
- **Detail views** at enlarged scale for complex areas (e.g., the grommet sandwich)
- **Section views** showing internal features (e.g., crossbar passing through upright)
- Standard line types: solid for visible edges, dashed for hidden edges, chain for centre lines
- Dimensions using leader lines with arrowheads, tolerances where critical

For this project, the interactive diagrams (gong-clothes-rail-layout.jsx) and this document serve the same purpose in a less formal but more practical format. If a formal drawing were ever needed (e.g., for a fabrication shop), the dimensions in this document could be transferred to a CAD tool like FreeCAD or LibreCAD and exported as a proper GA drawing to BS 8888.