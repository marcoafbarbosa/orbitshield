#!/usr/bin/env python3
"""
render-isl-worldmap.py

Reads the Graphviz DOT file produced by the isl-visualizer tool, extracts
satellite/ground-station positions and ISL edges, and renders them over an
equirectangular world map using matplotlib.

The output is a publication-quality PNG where every satellite dot is placed
exactly at its ground-track lat/lon — no distortion introduced by Graphviz's
node-overlap avoidance.

Dependencies (install with apt or pip):
    python3-matplotlib  (or pip install matplotlib)

Natural Earth 110m land polygons are downloaded automatically on first run
and cached next to this script as ".ne_110m_land.geojson".

Usage:
    python3 render-isl-worldmap.py <input.dot> <output.png> [OPTIONS]

Options:
    --width   Figure width in inches  (default: 20)
    --height  Figure height in inches (default: 10)
    --dpi     Output DPI              (default: 150)
    --no-labels  Hide satellite name labels on the map
"""

import argparse
import json
import os
import re
import sys
import urllib.request

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.collections import PatchCollection
from matplotlib.patches import Polygon

# ── Natural Earth data ────────────────────────────────────────────────────────
_NE_URL = (
    "https://raw.githubusercontent.com/nvkelso/natural-earth-vector"
    "/master/geojson/ne_110m_land.geojson"
)
_NE_CACHE = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".ne_110m_land.geojson")


def _load_land_geojson() -> dict:
    """Return the Natural Earth 110m land GeoJSON, downloading it if needed."""
    if not os.path.exists(_NE_CACHE):
        print(f"Downloading Natural Earth land data…", file=sys.stderr)
        try:
            urllib.request.urlretrieve(_NE_URL, _NE_CACHE)
        except Exception as exc:
            print(f"Warning: could not download world map data: {exc}", file=sys.stderr)
            print("The map will be rendered without land polygons.", file=sys.stderr)
            return {"features": []}
    with open(_NE_CACHE, encoding="utf-8") as fh:
        return json.load(fh)


def _draw_land(ax, geojson: dict) -> None:
    """Add land polygon patches to *ax*."""
    patches = []
    for feature in geojson.get("features", []):
        geom = feature.get("geometry", {})
        gtype = geom.get("type", "")
        if gtype == "Polygon":
            rings = [geom["coordinates"][0]]
        elif gtype == "MultiPolygon":
            rings = [poly[0] for poly in geom["coordinates"]]
        else:
            continue
        for ring in rings:
            patches.append(Polygon(ring, closed=True))

    if patches:
        col = PatchCollection(
            patches,
            facecolor="#d4e8c2",
            edgecolor="#7a9a6a",
            linewidth=0.35,
            zorder=2,
        )
        ax.add_collection(col)


# ── DOT file parsing ──────────────────────────────────────────────────────────
_NODE_RE = re.compile(r'"([^"]+)"\s*\[([^\]]+)\]')
_SAT_TOOLTIP_RE = re.compile(r'tooltip="lat=([-\d.]+)\s+lon=([-\d.]+)\s+alt=([-\d.]+)m"')
_GS_TOOLTIP_RE = re.compile(r'tooltip="type=ground-station\s+lat=([-\d.]+)\s+lon=([-\d.]+)"')
_FILLCOLOR_RE = re.compile(r'fillcolor=(\w+)')
_EDGE_RE = re.compile(r'"([^"]+)"\s*--\s*"([^"]+)"\s*\[label="([^"]+)"\]')
_META_CONSTELLATION_RE = re.compile(r'^\s*//\s*orbitshield\.constellation=(.+?)\s*$', re.MULTILINE)
_META_UTC_RE = re.compile(r'^\s*//\s*orbitshield\.utc=(.+?)\s*$', re.MULTILINE)

# Graphviz color names that differ from matplotlib's X11/CSS colour names.
_GRAPHVIZ_TO_MPL: dict[str, str] = {
    "lightgoldenrod": "lightgoldenrodyellow",
}


def _normalize_lon(lon: float) -> float:
    """Normalize longitude to [-180, 180)."""
    return ((lon + 180.0) % 360.0) - 180.0


def _parse_dot(dot_path: str):
    """
    Parse a DOT file produced by isl-visualizer.

    Returns:
        sat_nodes: dict name -> {lat, lon, alt, color}
        gs_nodes: dict  name -> {lat, lon, color}
        edges: list  [(nameA, nameB, label), …]
        metadata: dict with optional keys {constellation, utc}
    """
    with open(dot_path, encoding="utf-8") as fh:
        content = fh.read()

    sat_nodes = {}
    gs_nodes = {}
    for m in _NODE_RE.finditer(content):
        name = m.group(1)
        attrs = m.group(2)
        tm = _SAT_TOOLTIP_RE.search(attrs)
        cm = _FILLCOLOR_RE.search(attrs)
        if tm:
            sat_nodes[name] = {
                "lat": float(tm.group(1)),
                "lon": _normalize_lon(float(tm.group(2))),
                "alt": float(tm.group(3)),
                "color": _GRAPHVIZ_TO_MPL.get(cm.group(1), cm.group(1)) if cm else "lightblue",
            }
            continue

        gm = _GS_TOOLTIP_RE.search(attrs)
        if gm:
            gs_nodes[name] = {
                "lat": float(gm.group(1)),
                "lon": _normalize_lon(float(gm.group(2))),
                "color": _GRAPHVIZ_TO_MPL.get(cm.group(1), cm.group(1)) if cm else "khaki",
            }

    edges = []
    seen = set()
    for m in _EDGE_RE.finditer(content):
        a, b, label = m.group(1), m.group(2), m.group(3)
        key = (min(a, b), max(a, b))
        if key not in seen:
            seen.add(key)
            edges.append((a, b, label))

    metadata = {}
    m_const = _META_CONSTELLATION_RE.search(content)
    if m_const:
        metadata["constellation"] = m_const.group(1).strip()
    m_utc = _META_UTC_RE.search(content)
    if m_utc:
        metadata["utc"] = m_utc.group(1).strip()

    return sat_nodes, gs_nodes, edges, metadata


def _infer_constellation_name(nodes: dict) -> str:
    """Best-effort constellation name fallback when metadata is missing."""
    if not nodes:
        return "Constellation"

    first_name = sorted(nodes.keys())[0]
    token = first_name.split()[0] if first_name.split() else first_name
    token = token.strip("-_").strip()
    return token.title() if token else "Constellation"


# ── Antimeridian-aware edge drawing ───────────────────────────────────────────
def _segments_antimeridian(lon0: float, lat0: float, lon1: float, lat1: float):
    """
    Return one or two short segments clipped at the +/-180 deg seam.

    The segment follows the shortest longitudinal path. If that path crosses the
    map seam, it is split into two pieces with explicit seam endpoints on
    opposite sides (+180 and -180). This avoids drawing a long line across
    the whole map.
    """
    lon0 = _normalize_lon(lon0)
    lon1 = _normalize_lon(lon1)

    # Unwrap lon1 around lon0 so we follow the shortest path.
    dlon = ((lon1 - lon0 + 180.0) % 360.0) - 180.0
    lon1_u = lon0 + dlon

    # No seam crossing in map domain.
    if -180.0 <= lon1_u <= 180.0:
        return [[(lon0, lat0), (lon1_u, lat1)]]

    if lon1_u > 180.0:
        # Crossing eastward: first segment ends at +180, second starts at -180.
        t = (180.0 - lon0) / (lon1_u - lon0)
        lat_mid = lat0 + t * (lat1 - lat0)
        return [
            [(lon0, lat0), (180.0, lat_mid)],
            [(-180.0, lat_mid), (lon1_u - 360.0, lat1)],
        ]

    # Crossing westward: first segment ends at -180, second starts at +180.
    t = (-180.0 - lon0) / (lon1_u - lon0)
    lat_mid = lat0 + t * (lat1 - lat0)
    return [
        [(lon0, lat0), (-180.0, lat_mid)],
        [(180.0, lat_mid), (lon1_u + 360.0, lat1)],
    ]


# ── Main renderer ─────────────────────────────────────────────────────────────
def render(dot_path: str, out_path: str, width: float, height: float, dpi: int, show_labels: bool) -> None:
    sat_nodes, gs_nodes, edges, metadata = _parse_dot(dot_path)
    if not sat_nodes and not gs_nodes:
        print("Error: no plottable nodes found in DOT file.", file=sys.stderr)
        sys.exit(1)

    print(
        f"  {len(sat_nodes)} satellites, {len(gs_nodes)} ground stations, {len(edges)} ISL edges",
        file=sys.stderr,
    )

    land = _load_land_geojson()

    fig, ax = plt.subplots(figsize=(width, height), dpi=dpi)

    # Ocean background
    ax.set_facecolor("#b0cfe0")

    # Land polygons
    _draw_land(ax, land)

    # Graticule
    for lon in range(-180, 181, 30):
        ax.axvline(lon, color="#c0c0c0", linewidth=0.3, zorder=1)
    for lat in range(-90, 91, 30):
        ax.axhline(lat, color="#c0c0c0", linewidth=0.3, zorder=1)
    ax.axvline(0, color="#aaaaaa", linewidth=0.5, zorder=1)
    ax.axhline(0, color="#aaaaaa", linewidth=0.5, zorder=1)

    # ISL edges
    all_nodes = {**sat_nodes, **gs_nodes}
    for a, b, label in edges:
        if a not in all_nodes or b not in all_nodes:
            continue
        lon0, lat0 = all_nodes[a]["lon"], all_nodes[a]["lat"]
        lon1, lat1 = all_nodes[b]["lon"], all_nodes[b]["lat"]
        for seg in _segments_antimeridian(lon0, lat0, lon1, lat1):
            lons = [p[0] for p in seg]
            lats = [p[1] for p in seg]
            ax.plot(lons, lats, color="#e05000", linewidth=0.9, alpha=0.75, zorder=3,
                    solid_capstyle="round")

    # Satellite nodes, grouped by ring color for a single scatter call each
    color_groups: dict[str, list] = {}
    for name, info in sat_nodes.items():
        color_groups.setdefault(info["color"], []).append((info["lon"], info["lat"], name))

    for color, entries in color_groups.items():
        lons = [e[0] for e in entries]
        lats = [e[1] for e in entries]
        ax.scatter(
            lons, lats,
            s=100, color=color, edgecolors="#333333", linewidths=0.5,
            zorder=4, label=color,
        )
        if show_labels:
            for lon, lat, name in entries:
                ax.annotate(
                    name, (lon, lat),
                    fontsize=7.5, ha="center", va="bottom",
                    xytext=(0, 3), textcoords="offset points",
                    clip_on=True, annotation_clip=True,
                    zorder=5,
                )

    # Ground-station nodes with a distinct marker style.
    if gs_nodes:
        gs_lons = [info["lon"] for info in gs_nodes.values()]
        gs_lats = [info["lat"] for info in gs_nodes.values()]
        ax.scatter(
            gs_lons,
            gs_lats,
            s=85,
            marker="s",
            color="#f4d35e",
            edgecolors="#5b4a00",
            linewidths=0.7,
            zorder=6,
        )
        if show_labels:
            for name, info in gs_nodes.items():
                label = name[3:] if name.startswith("GS:") else name
                ax.annotate(
                    label,
                    (info["lon"], info["lat"]),
                    fontsize=8,
                    ha="center",
                    va="bottom",
                    xytext=(0, 4),
                    textcoords="offset points",
                    clip_on=True,
                    annotation_clip=True,
                    zorder=7,
                )

    # Axes cosmetics
    ax.set_xlim(-180, 180)
    ax.set_ylim(-90, 90)
    ax.set_xlabel("Longitude (°)", fontsize=9)
    ax.set_ylabel("Latitude (°)", fontsize=9)
    ax.set_aspect("equal")

    # Degree tick labels
    ax.set_xticks(range(-180, 181, 30))
    ax.set_yticks(range(-90, 91, 30))
    ax.tick_params(labelsize=7)

    constellation_name = metadata.get("constellation", _infer_constellation_name(sat_nodes or gs_nodes))
    utc_value = metadata.get("utc")
    if utc_value:
        title = f"{constellation_name} - {utc_value}"
    else:
        title = constellation_name

    ax.set_title(title, fontsize=11, pad=8)

    # Keep a fixed canvas across frames: no tight bounding-box resizing.
    fig.subplots_adjust(left=0.05, right=0.995, bottom=0.075, top=0.92)
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)
    print(f"Saved: {out_path}", file=sys.stderr)


# ── CLI ───────────────────────────────────────────────────────────────────────
def main() -> None:
    p = argparse.ArgumentParser(
        description="Render an ISL DOT file over an equirectangular world map."
    )
    p.add_argument("dot_file", help="Input .dot file from isl-visualizer")
    p.add_argument("output_png", help="Output PNG path")
    p.add_argument("--width", type=float, default=20.0, help="Figure width in inches (default: 20)")
    p.add_argument("--height", type=float, default=10.0, help="Figure height in inches (default: 10)")
    p.add_argument("--dpi", type=int, default=150, help="Output DPI (default: 150)")
    p.set_defaults(labels=True)
    p.add_argument("--labels",
                   dest="labels",
                   action="store_true",
                   help="Annotate each satellite with its name (default: enabled)")
    p.add_argument("--no-labels",
                   dest="labels",
                   action="store_false",
                   help="Disable satellite name labels")
    args = p.parse_args()

    render(args.dot_file, args.output_png, args.width, args.height, args.dpi, args.labels)


if __name__ == "__main__":
    main()
