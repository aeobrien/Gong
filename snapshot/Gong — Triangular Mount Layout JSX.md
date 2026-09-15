import { useState } from "react";

export default function ClothesRailGongDiagram() {
  const [view, setView] = useState("front");

  return (
    <div style={{
      minHeight: "100vh",
      background: "#1a1a1e",
      color: "#e0e0e0",
      fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
      padding: "24px",
      boxSizing: "border-box"
    }}>
      <h2 style={{
        fontSize: "14px",
        fontWeight: 400,
        letterSpacing: "0.15em",
        textTransform: "uppercase",
        color: "#888",
        margin: "0 0 20px 0"
      }}>
        Digital Gong — Triangular Mount Layout
      </h2>

      <div style={{ display: "flex", gap: "8px", marginBottom: "24px" }}>
        {[
          { id: "front", label: "Front View" },
          { id: "side", label: "Side View (clearance)" },
          { id: "segment", label: "Single Segment Detail" },
        ].map(v => (
          <button
            key={v.id}
            onClick={() => setView(v.id)}
            style={{
              background: view === v.id ? "#e0e0e0" : "transparent",
              color: view === v.id ? "#1a1a1e" : "#888",
              border: `1px solid ${view === v.id ? "#e0e0e0" : "#444"}`,
              padding: "6px 14px",
              fontSize: "12px",
              fontFamily: "inherit",
              cursor: "pointer",
              borderRadius: "2px",
              transition: "all 0.15s ease"
            }}
          >
            {v.label}
          </button>
        ))}
      </div>

      <svg viewBox="0 0 700 750" style={{ width: "100%", maxWidth: "700px" }}>
        {view === "front" && <FrontView />}
        {view === "side" && <SideView />}
        {view === "segment" && <SegmentDetail />}
      </svg>

      <div style={{
        marginTop: "20px",
        fontSize: "12px",
        lineHeight: 1.7,
        color: "#999",
        maxWidth: "580px"
      }}>
        {view === "front" && (
          <p style={{ margin: 0 }}>
            Three mounting bolts per segment arranged in a triangle around the contact mic.
            CB1/CB4 each carry two bolts (one per segment in the pair, positioned inward).
            CB2/CB3 each carry four bolts (two per segment — one near the outer edge,
            one closer to the inner arc). Twelve bolts total.
            The mic sits roughly at the centroid of each triangle, so the supported area
            surrounds the primary pickup zone rather than being far from it.
          </p>
        )}
        {view === "side" && (
          <p style={{ margin: 0 }}>
            Side view showing depth stack. The inner bolts on CB2/CB3 are closest to the
            Pi zone — both vertically and in terms of where they sit on the MDF. Longer
            bolts (M6 × 80mm+) give more standoff. If the Pi has a HAT or tall headers,
            you may need M6 × 90mm or to adjust CB2/CB3 position.
          </p>
        )}
        {view === "segment" && (
          <p style={{ margin: 0 }}>
            Isolated view of segment A (top-left) showing the three bolt positions forming
            a triangle around the contact mic. Distances from the mic to each bolt are
            shown. The dashed triangle shows the supported zone —
            the segment is rigid within this area and only flexes outside it.
          </p>
        )}
      </div>
    </div>
  );
}

const S = 0.42;
const CX = 350;
const CY = 350;
const OUTER_R = 356 * S;
const INNER_R = 75 * S;

const CB1_MM = -230;
const CB2_MM = -70;
const CB3_MM = 70;
const CB4_MM = 230;

function toY(mmFromCentre) { return CY + mmFromCentre * S; }
function toX(mmFromCentre) { return CX + mmFromCentre * S; }

const SEG_A_BOLTS = [
  { x: -150, y: CB1_MM, cb: "CB1" },
  { x: -310, y: CB2_MM, cb: "CB2" },
  { x: -80,  y: CB2_MM, cb: "CB2" },
];
const SEG_A_MIC = { x: -180, y: -123 };

const SEG_B_BOLTS = [
  { x: 150,  y: CB1_MM, cb: "CB1" },
  { x: 310,  y: CB2_MM, cb: "CB2" },
  { x: 80,   y: CB2_MM, cb: "CB2" },
];
const SEG_B_MIC = { x: 180, y: -123 };

const SEG_C_BOLTS = [
  { x: -150, y: CB4_MM, cb: "CB4" },
  { x: -310, y: CB3_MM, cb: "CB3" },
  { x: -80,  y: CB3_MM, cb: "CB3" },
];
const SEG_C_MIC = { x: -180, y: 123 };

const SEG_D_BOLTS = [
  { x: 150,  y: CB4_MM, cb: "CB4" },
  { x: 310,  y: CB3_MM, cb: "CB3" },
  { x: 80,   y: CB3_MM, cb: "CB3" },
];
const SEG_D_MIC = { x: 180, y: 123 };

function BoltMarker({ x, y }) {
  return (
    <g>
      <circle cx={x} cy={y} r="6" fill="#1a1a1e" stroke="#e8a735" strokeWidth="1.5" />
      <circle cx={x} cy={y} r="2" fill="#e8a735" />
    </g>
  );
}

function MicMarker({ x, y }) {
  return (
    <g>
      <circle cx={x} cy={y} r="7" fill="none" stroke="#5a8a5a" strokeWidth="1.5" />
      <circle cx={x} cy={y} r="2.5" fill="#5a8a5a" />
    </g>
  );
}

function arcPathSvg(cx, cy, r, startDeg, endDeg) {
  const sr = (startDeg * Math.PI) / 180;
  const er = (endDeg * Math.PI) / 180;
  const x1 = cx + r * Math.cos(sr);
  const y1 = cy - r * Math.sin(sr);
  const x2 = cx + r * Math.cos(er);
  const y2 = cy - r * Math.sin(er);
  const largeArc = (endDeg - startDeg) > 180 ? 1 : 0;
  return { x1, y1, x2, y2, largeArc };
}

function segmentPathSvg(cx, cy, outerR, innerR, startAngle, endAngle) {
  const outer = arcPathSvg(cx, cy, outerR, startAngle, endAngle);
  const inner = arcPathSvg(cx, cy, innerR, startAngle, endAngle);
  return `M ${outer.x1},${outer.y1}
          A ${outerR},${outerR} 0 ${outer.largeArc},0 ${outer.x2},${outer.y2}
          L ${inner.x2},${inner.y2}
          A ${innerR},${innerR} 0 ${inner.largeArc},1 ${inner.x1},${inner.y1}
          Z`;
}

function FrontView() {
  const railSpacing = 800 * S;
  const railLeft = CX - railSpacing / 2;
  const railRight = CX + railSpacing / 2;
  const railTop = 50;
  const railBottom = 660;

  const cbYs = [
    { y: toY(CB1_MM), label: "CB1", note: "2 bolts" },
    { y: toY(CB2_MM), label: "CB2", note: "4 bolts" },
    { y: toY(CB3_MM), label: "CB3", note: "4 bolts" },
    { y: toY(CB4_MM), label: "CB4", note: "2 bolts" },
  ];

  const segments = [
    { start: 91, end: 179, color: "#3a3a42", label: "A", lx: -75, ly: -25 },
    { start: 1, end: 89, color: "#33333b", label: "B", lx: 65, ly: -25 },
    { start: 181, end: 269, color: "#33333b", label: "C", lx: -75, ly: 30 },
    { start: 271, end: 359, color: "#3a3a42", label: "D", lx: 65, ly: 30 },
  ];

  const allBolts = [...SEG_A_BOLTS, ...SEG_B_BOLTS, ...SEG_C_BOLTS, ...SEG_D_BOLTS];
  const allMics = [SEG_A_MIC, SEG_B_MIC, SEG_C_MIC, SEG_D_MIC];
  const segTriangles = [SEG_A_BOLTS, SEG_B_BOLTS, SEG_C_BOLTS, SEG_D_BOLTS];

  return (
    <g>
      <text x="30" y="30" fill="#888" fontSize="11" fontFamily="inherit">
        FRONT VIEW — triangular mount layout (3 bolts per segment)
      </text>

      <rect x={railLeft - 5} y={railTop} width="10" height={railBottom - railTop} fill="#555" rx="2" />
      <rect x={railRight - 5} y={railTop} width="10" height={railBottom - railTop} fill="#555" rx="2" />
      <text x={railLeft} y={railTop - 8} fill="#666" fontSize="8" fontFamily="inherit" textAnchor="middle">Upright</text>
      <text x={railRight} y={railTop - 8} fill="#666" fontSize="8" fontFamily="inherit" textAnchor="middle">Upright</text>

      {[railLeft - 8, railLeft + 8, railRight - 8, railRight + 8].map((wx, i) => (
        <circle key={i} cx={wx} cy={railBottom + 8} r="6" fill="none" stroke="#444" strokeWidth="1.5" />
      ))}

      {cbYs.map((cb, i) => (
        <g key={i}>
          <line x1={railLeft} y1={cb.y} x2={railRight} y2={cb.y} stroke="#888" strokeWidth="3" />
          <text x={railRight + 15} y={cb.y + 4} fill="#888" fontSize="9" fontFamily="inherit">{cb.label}</text>
          <text x={railRight + 15} y={cb.y + 14} fill="#555" fontSize="8" fontFamily="inherit">{cb.note}</text>
        </g>
      ))}

      {segments.map((seg, i) => (
        <g key={i}>
          <path d={segmentPathSvg(CX, CY, OUTER_R, INNER_R, seg.start, seg.end)}
            fill={seg.color} stroke="#555" strokeWidth="0.5" />
          <text x={CX + seg.lx} y={CY + seg.ly}
            fill="#999" fontSize="13" fontFamily="inherit" textAnchor="middle" dominantBaseline="central">
            {seg.label}
          </text>
        </g>
      ))}

      <circle cx={CX} cy={CY} r={INNER_R} fill="#1a1a1e" stroke="#555" strokeWidth="0.5" />
      <text x={CX} y={CY - 5} fill="#666" fontSize="7" fontFamily="inherit" textAnchor="middle">Screen</text>
      <text x={CX} y={CY + 5} fill="#666" fontSize="7" fontFamily="inherit" textAnchor="middle">+ Pi</text>

      {segTriangles.map((bolts, i) => (
        <polygon key={`tri-${i}`}
          points={bolts.map(b => `${toX(b.x)},${toY(b.y)}`).join(" ")}
          fill="#e8a735" fillOpacity="0.06"
          stroke="#e8a735" strokeWidth="0.5" strokeDasharray="3,3" />
      ))}

      {allBolts.map((b, i) => (
        <BoltMarker key={`bolt-${i}`} x={toX(b.x)} y={toY(b.y)} />
      ))}

      {allMics.map((m, i) => (
        <MicMarker key={`mic-${i}`} x={toX(m.x)} y={toY(m.y)} />
      ))}

      <g transform="translate(30, 690)">
        <circle cx="6" cy="0" r="6" fill="#1a1a1e" stroke="#e8a735" strokeWidth="1.5" />
        <circle cx="6" cy="0" r="2" fill="#e8a735" />
        <text x="18" y="4" fill="#999" fontSize="9" fontFamily="inherit">Mounting bolt (M6 + grommets)</text>
      </g>
      <g transform="translate(30, 710)">
        <circle cx="6" cy="0" r="7" fill="none" stroke="#5a8a5a" strokeWidth="1.5" />
        <circle cx="6" cy="0" r="2.5" fill="#5a8a5a" />
        <text x="18" y="4" fill="#999" fontSize="9" fontFamily="inherit">Contact mic</text>
      </g>
      <g transform="translate(30, 730)">
        <line x1="0" y1="0" x2="14" y2="0" stroke="#e8a735" strokeWidth="0.5" strokeDasharray="3,3" />
        <text x="18" y="4" fill="#999" fontSize="9" fontFamily="inherit">Supported triangle (rigid zone around mic)</text>
      </g>
      <g transform="translate(330, 690)">
        <line x1="0" y1="0" x2="16" y2="0" stroke="#888" strokeWidth="3" />
        <text x="22" y="4" fill="#999" fontSize="9" fontFamily="inherit">10mm steel crossbar</text>
      </g>
      <g transform="translate(330, 710)">
        <rect x="0" y="-5" width="6" height="10" fill="#555" rx="1" />
        <text x="22" y="4" fill="#999" fontSize="9" fontFamily="inherit">Clothes rail upright</text>
      </g>
    </g>
  );
}

function SideView() {
  const mdfX = 180;
  const mdfThick = 25 * 1.6;
  const mdfTop = 100;
  const mdfBot = 560;
  const rearMdf = mdfX + mdfThick;
  const grometThick = 8 * 1.5;
  const washerThick = 3 * 1.5;
  const crossbarDia = 10 * 1.5;
  const grommetStart = rearMdf + 3;
  const grommetEnd = grommetStart + grometThick;
  const washerEnd = grommetEnd + 2 + washerThick;
  const crossbarCentre = washerEnd + crossbarDia / 2 + 2;

  const piThick = 22 * 1.5;
  const piHeight = 60 * 0.9;
  const piStartX = rearMdf + 6;
  const piCentreY = (mdfTop + mdfBot) / 2;

  const cbPositions = [
    { y: toY(CB1_MM), label: "CB1" },
    { y: toY(CB2_MM), label: "CB2" },
    { y: toY(CB3_MM), label: "CB3" },
    { y: toY(CB4_MM), label: "CB4" },
  ];

  return (
    <g>
      <text x="30" y="30" fill="#888" fontSize="11" fontFamily="inherit">
        SIDE VIEW — depth stack and Pi clearance
      </text>
      <text x="30" y="46" fill="#666" fontSize="9" fontFamily="inherit">
        (front of gong faces left)
      </text>
      <text x={mdfX - 50} y={85} fill="#555" fontSize="9" fontFamily="inherit" textAnchor="middle">← FRONT</text>
      <text x={crossbarCentre + 70} y={85} fill="#555" fontSize="9" fontFamily="inherit" textAnchor="middle">BACK →</text>

      <rect x={mdfX} y={mdfTop} width={mdfThick} height={mdfBot - mdfTop} fill="#3a3a42" stroke="#666" strokeWidth="1" />
      <text x={mdfX + mdfThick / 2} y={mdfTop - 10} fill="#ccc" fontSize="9" fontFamily="inherit" textAnchor="middle">MDF (25mm)</text>

      <rect x={piStartX} y={piCentreY - piHeight / 2} width={piThick} height={piHeight}
        fill="#2a4a2a" stroke="#5a8a5a" strokeWidth="1" rx="2" />
      <text x={piStartX + piThick / 2} y={piCentreY + 3}
        fill="#8aba8a" fontSize="8" fontFamily="inherit" textAnchor="middle">Pi</text>

      <line x1={piStartX - 2} y1={piCentreY - piHeight / 2 - 3}
        x2={piStartX + piThick + 2} y2={piCentreY - piHeight / 2 - 3} stroke="#5a8a5a" strokeWidth="0.5" />
      <line x1={piStartX - 2} y1={piCentreY + piHeight / 2 + 3}
        x2={piStartX + piThick + 2} y2={piCentreY + piHeight / 2 + 3} stroke="#5a8a5a" strokeWidth="0.5" />

      {cbPositions.map((cb, i) => {
        const isNearPi = Math.abs(cb.y - piCentreY) < piHeight / 2 + 20;
        return (
          <g key={i}>
            <rect x={grommetStart} y={cb.y - 8} width={grometThick} height={16} fill="#e8a735" rx="3" opacity="0.8" />
            <rect x={grommetEnd + 2} y={cb.y - 5} width={washerThick} height={10} fill="#aaa" />
            <circle cx={crossbarCentre} cy={cb.y} r={crossbarDia / 2} fill="#888" stroke="#aaa" strokeWidth="0.5" />
            <line x1={mdfX - 15} y1={cb.y} x2={crossbarCentre + crossbarDia / 2 + 10} y2={cb.y}
              stroke="#999" strokeWidth="1.5" strokeDasharray="2,2" />
            <rect x={mdfX - 20} y={cb.y - 5} width="8" height="10" fill="#bbb" rx="1" />
            <rect x={mdfX - 10} y={cb.y - 4} width="3" height="8" fill="#aaa" />
            <rect x={mdfX - 6} y={cb.y - 8} width="6" height="16" fill="#e8a735" rx="2" opacity="0.8" />
            <rect x={crossbarCentre + crossbarDia / 2 + 3} y={cb.y - 5} width="8" height="10" fill="#bbb" rx="1" />
            <text x={crossbarCentre + crossbarDia / 2 + 18} y={cb.y + 4} fill="#888" fontSize="9" fontFamily="inherit">{cb.label}</text>
            {isNearPi && (
              <text x={crossbarCentre + crossbarDia / 2 + 18} y={cb.y + 16}
                fill="#e85a5a" fontSize="8" fontFamily="inherit">⚠ Pi clearance</text>
            )}
          </g>
        );
      })}

      <line x1={rearMdf} y1={mdfBot + 20} x2={rearMdf} y2={mdfBot + 38} stroke="#e8a735" strokeWidth="0.6" />
      <line x1={crossbarCentre} y1={mdfBot + 20} x2={crossbarCentre} y2={mdfBot + 38} stroke="#e8a735" strokeWidth="0.6" />
      <line x1={rearMdf} y1={mdfBot + 33} x2={crossbarCentre} y2={mdfBot + 33} stroke="#e8a735" strokeWidth="0.8" />
      <text x={(rearMdf + crossbarCentre) / 2} y={mdfBot + 50} fill="#e8a735" fontSize="8" fontFamily="inherit" textAnchor="middle">
        Standoff ~15-20mm
      </text>

      <line x1={mdfX - 20} y1={mdfTop + 20} x2={mdfX - 20} y2={mdfTop + 8} stroke="#666" strokeWidth="0.5" />
      <line x1={crossbarCentre + crossbarDia / 2 + 11} y1={mdfTop + 20}
        x2={crossbarCentre + crossbarDia / 2 + 11} y2={mdfTop + 8} stroke="#666" strokeWidth="0.5" />
      <line x1={mdfX - 20} y1={mdfTop + 12}
        x2={crossbarCentre + crossbarDia / 2 + 11} y2={mdfTop + 12} stroke="#666" strokeWidth="0.8" />
      <text x={(mdfX - 20 + crossbarCentre + crossbarDia / 2 + 11) / 2} y={mdfTop + 6}
        fill="#666" fontSize="8" fontFamily="inherit" textAnchor="middle">Bolt length: M6 × 80mm</text>

      <text x={30} y={620} fill="#e8a735" fontSize="9" fontFamily="inherit">
        Inner bolts on CB2/CB3 are closest to Pi zone
      </text>
      <text x={30} y={636} fill="#888" fontSize="9" fontFamily="inherit">
        Options: longer bolts (more standoff), or offset CB2/CB3 further from centre
      </text>
      <text x={30} y={660} fill="#666" fontSize="9" fontFamily="inherit">
        Pi 4/5: 85 × 56 × 17mm + standoffs + cables
      </text>
    </g>
  );
}

function SegmentDetail() {
  const detailCX = 350;
  const detailCY = 380;
  const ds = 0.7;
  const outerR = 356 * ds;
  const innerR = 75 * ds;

  const bolts = SEG_A_BOLTS.map(b => ({
    x: detailCX + b.x * ds,
    y: detailCY + b.y * ds,
    label: b.cb
  }));
  const mic = {
    x: detailCX + SEG_A_MIC.x * ds,
    y: detailCY + SEG_A_MIC.y * ds
  };

  const dists = SEG_A_BOLTS.map(b => {
    const dx = b.x - SEG_A_MIC.x;
    const dy = b.y - SEG_A_MIC.y;
    return Math.round(Math.sqrt(dx * dx + dy * dy));
  });

  return (
    <g>
      <text x="30" y="30" fill="#888" fontSize="11" fontFamily="inherit">
        SEGMENT A DETAIL — triangular mount with mic at centre
      </text>

      <path d={segmentPathSvg(detailCX, detailCY, outerR, innerR, 91, 179)}
        fill="#3a3a42" stroke="#666" strokeWidth="1" />

      <circle cx={detailCX} cy={detailCY} r={innerR} fill="none" stroke="#444" strokeWidth="0.5" />

      {/* Triangle fill */}
      <polygon
        points={bolts.map(b => `${b.x},${b.y}`).join(" ")}
        fill="#e8a735" fillOpacity="0.08"
        stroke="#e8a735" strokeWidth="1" strokeDasharray="4,4" />

      {/* Lines from mic to bolts with distance labels */}
      {bolts.map((b, i) => {
        const midX = (mic.x + b.x) / 2;
        const midY = (mic.y + b.y) / 2;
        const offsetX = i === 0 ? 12 : i === 1 ? -14 : 14;
        const offsetY = i === 0 ? -4 : 10;
        return (
          <g key={`line-${i}`}>
            <line x1={mic.x} y1={mic.y} x2={b.x} y2={b.y}
              stroke="#e8a735" strokeWidth="0.5" strokeDasharray="2,3" opacity="0.5" />
            <text x={midX + offsetX} y={midY + offsetY}
              fill="#e8a735" fontSize="8" fontFamily="inherit" textAnchor="middle" opacity="0.8">
              {dists[i]}mm
            </text>
          </g>
        );
      })}

      {/* Bolt markers with labels */}
      {bolts.map((b, i) => {
        const labelX = i === 0 ? b.x : i === 1 ? b.x - 16 : b.x + 16;
        const labelY = b.y - 12;
        const labels = ["P1 (CB1)", "P2 (CB2 outer)", "P3 (CB2 inner)"];
        return (
          <g key={`bolt-${i}`}>
            <BoltMarker x={b.x} y={b.y} />
            <text x={labelX} y={labelY}
              fill="#e8a735" fontSize="8" fontFamily="inherit" textAnchor="middle">
              {labels[i]}
            </text>
          </g>
        );
      })}

      <MicMarker x={mic.x} y={mic.y} />
      <text x={mic.x} y={mic.y + 16}
        fill="#5a8a5a" fontSize="9" fontFamily="inherit" textAnchor="middle">
        Contact mic
      </text>

      {/* Edge clearance from P3 to straight edge */}
      <line x1={bolts[2].x} y1={bolts[2].y + 12}
        x2={detailCX - 1 * ds} y2={bolts[2].y + 12}
        stroke="#666" strokeWidth="0.5" strokeDasharray="2,2" />
      <text x={(bolts[2].x + detailCX) / 2} y={bolts[2].y + 24}
        fill="#666" fontSize="7" fontFamily="inherit" textAnchor="middle">
        ~80mm to straight edge
      </text>

      <text x={detailCX - 120} y={detailCY - 90}
        fill="#999" fontSize="18" fontFamily="inherit" textAnchor="middle">A</text>

      {/* Crossbar height indicators */}
      <line x1={detailCX + outerR + 15} y1={bolts[0].y}
        x2={detailCX + outerR + 35} y2={bolts[0].y} stroke="#888" strokeWidth="0.5" />
      <text x={detailCX + outerR + 40} y={bolts[0].y + 4}
        fill="#888" fontSize="8" fontFamily="inherit">CB1 height</text>

      <line x1={detailCX + outerR + 15} y1={bolts[1].y}
        x2={detailCX + outerR + 35} y2={bolts[1].y} stroke="#888" strokeWidth="0.5" />
      <text x={detailCX + outerR + 40} y={bolts[1].y + 4}
        fill="#888" fontSize="8" fontFamily="inherit">CB2 height</text>

      <g transform="translate(30, 700)">
        <circle cx="6" cy="0" r="6" fill="#1a1a1e" stroke="#e8a735" strokeWidth="1.5" />
        <circle cx="6" cy="0" r="2" fill="#e8a735" />
        <text x="18" y="4" fill="#999" fontSize="9" fontFamily="inherit">Mounting bolt position</text>
        <circle cx="220" cy="0" r="7" fill="none" stroke="#5a8a5a" strokeWidth="1.5" />
        <circle cx="220" cy="0" r="2.5" fill="#5a8a5a" />
        <text x="232" y="4" fill="#999" fontSize="9" fontFamily="inherit">Contact mic position</text>
      </g>
    </g>
  );
}