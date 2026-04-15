#!/usr/bin/env bash
# generate-constellation-image.sh — Run isl-visualizer and render the ISL topology as a PNG.
#
# By default the output is rendered over an equirectangular world map using the
# accompanying render-isl-worldmap.py script (requires python3-matplotlib).
# If Python/matplotlib are unavailable the script falls back to plain Graphviz
# (dot -Tpng), which still produces a correct topology diagram without a map.
#
# Usage:
#   ./contrib/orbitshield/tools/generate-constellation-image.sh [OPTIONS]
#
# Options:
#   --ringFile=<path>     Path to the YAML metadata file   (default: contrib/orbitshield/data/iridium-20260312.yaml)
#   --islMaxRange=<km>    Maximum ISL range in km   (default: 5000)
#   --groundMaxRange=<km> Maximum satellite-ground range in km (default: 3000)
#   --outputFile=<path>   DOT output file            (default: out.dot)
#   --pngFile=<path>      PNG output file            (default: derived from --outputFile, e.g. out.png)
#   --frames=<int>        Number of frames to render (default: 1)
#   --timeStep=<seconds>  Simulation step between frames (default: 60)
#   --startTime=<seconds> Simulation time of frame 0 (default: 0)
#   --gifFile=<path>      Optional animated GIF output path
#   --gifFps=<int>        GIF frame rate (default: 8)
#   --gifLoop=<int>       GIF loop count: 0=infinite (default: 0)
#   --width=<inches>      Map figure width           (default: 20, worldmap renderer only)
#   --height=<inches>     Map figure height          (default: 10, worldmap renderer only)
#   --dpi=<int>           Output DPI                 (default: 150, worldmap renderer only)
#   --labels              Annotate each satellite with its name (worldmap renderer only)
#   --no-worldmap         Use plain Graphviz instead of the worldmap renderer
#   -h, --help            Show this help message
#
# Must be run from the ns-3 root directory, e.g.:
#   cd /path/to/ns-3-dev
#   ./contrib/orbitshield/tools/generate-constellation-image.sh --maxRange=5000

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ── defaults ────────────────────────────────────────────────────────────────
RING_FILE="contrib/orbitshield/data/iridium-20260312.yaml"
ISL_MAX_RANGE="5000"  # km (5000 km default for ISL at LEO ~700 km altitude)
GROUND_MAX_RANGE="3000"  # km (3000 km typical for satellite-ground visibility in LEO)
DOT_FILE="out.dot"
PNG_FILE=""
FRAMES="1"
TIME_STEP="60"
START_TIME="0"
GIF_FILE=""
GIF_FPS="8"
GIF_LOOP="0"
GIF_MAX_WIDTH="1200"
WIDTH="20"
HEIGHT="10"
DPI="150"
LABELS=""
USE_WORLDMAP=1   # 1 = worldmap renderer; 0 = plain graphviz

# ── argument parsing ─────────────────────────────────────────────────────────
for arg in "$@"; do
    case "$arg" in
        --ringFile=*)    RING_FILE="${arg#*=}" ;;
        --islMaxRange=*)    ISL_MAX_RANGE="${arg#*=}" ;;
        --groundMaxRange=*) GROUND_MAX_RANGE="${arg#*=}" ;;
        --outputFile=*)  DOT_FILE="${arg#*=}" ;;
        --pngFile=*)     PNG_FILE="${arg#*=}" ;;
        --frames=*)      FRAMES="${arg#*=}" ;;
        --timeStep=*)    TIME_STEP="${arg#*=}" ;;
        --startTime=*)   START_TIME="${arg#*=}" ;;
        --gifFile=*)     GIF_FILE="${arg#*=}" ;;
        --gifFps=*)      GIF_FPS="${arg#*=}" ;;
        --gifLoop=*)     GIF_LOOP="${arg#*=}" ;;
        --width=*)       WIDTH="${arg#*=}" ;;
        --height=*)      HEIGHT="${arg#*=}" ;;
        --dpi=*)         DPI="${arg#*=}" ;;
        --labels)        LABELS="--labels" ;;
        --no-worldmap)   USE_WORLDMAP=0 ;;
        -h|--help)
            sed -n '2,/^[^#]/{ /^#/{ s/^# \?//; p }; /^[^#]/q }' "$0"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            exit 1
            ;;
    esac
done

# Derive PNG path from DOT path if not explicitly set.
if [[ -z "$PNG_FILE" ]]; then
    PNG_FILE="${DOT_FILE%.dot}.png"
fi

if ! [[ "$FRAMES" =~ ^[0-9]+$ ]] || [[ "$FRAMES" -lt 1 ]]; then
    echo "Error: --frames must be an integer >= 1" >&2
    exit 1
fi

if ! [[ "$TIME_STEP" =~ ^-?[0-9]+([.][0-9]+)?$ ]]; then
    echo "Error: --timeStep must be numeric" >&2
    exit 1
fi

if ! [[ "$START_TIME" =~ ^-?[0-9]+([.][0-9]+)?$ ]]; then
    echo "Error: --startTime must be numeric" >&2
    exit 1
fi

if ! [[ "$GIF_FPS" =~ ^[0-9]+$ ]] || [[ "$GIF_FPS" -lt 1 ]]; then
    echo "Error: --gifFps must be an integer >= 1" >&2
    exit 1
fi

if ! [[ "$GIF_LOOP" =~ ^[0-9]+$ ]]; then
    echo "Error: --gifLoop must be an integer >= 0" >&2
    exit 1
fi

# ── sanity checks ────────────────────────────────────────────────────────────
if [[ ! -f "./ns3" ]]; then
    echo "Error: must be run from the ns-3 root directory (no './ns3' found)." >&2
    exit 1
fi

# ── render setup ─────────────────────────────────────────────────────────────
RENDERER_PY="$SCRIPT_DIR/render-isl-worldmap.py"

CAN_USE_WORLDMAP=0
if [[ "$USE_WORLDMAP" -eq 1 ]] && command -v python3 &>/dev/null \
        && python3 -c "import matplotlib" 2>/dev/null \
        && [[ -f "$RENDERER_PY" ]]; then
    CAN_USE_WORLDMAP=1
elif [[ "$USE_WORLDMAP" -eq 1 ]]; then
    echo "Note: matplotlib not available; falling back to plain Graphviz rendering."
    echo "      Install with: sudo apt install python3-matplotlib"
fi

if [[ "$CAN_USE_WORLDMAP" -eq 0 ]] && ! command -v dot &>/dev/null; then
    echo "Error: 'dot' (Graphviz) not found. Install it with: sudo apt install graphviz" >&2
    exit 1
fi

# ── helpers ─────────────────────────────────────────────────────────────────
_frame_paths() {
    local base="$1"
    local idx="$2"
    local stem ext

    stem="${base%.*}"
    if [[ "$base" == "$stem" ]]; then
        ext=""
    else
        ext=".${base##*.}"
    fi

    if [[ "$FRAMES" -eq 1 ]]; then
        printf '%s\n' "$base"
    else
        printf '%s_%04d%s\n' "$stem" "$idx" "$ext"
    fi
}

_frame_time() {
    local idx="$1"
    awk -v s="$START_TIME" -v d="$TIME_STEP" -v i="$idx" 'BEGIN { printf "%.9g", s + (d * i) }'
}

_build_gif() {
    local gif_path="$1"
    local png_pattern="$2"
    shift 2
    local frame_files=("$@")

    if command -v ffmpeg &>/dev/null; then
        # Palette-based GIF generation gives much better quality than direct conversion.
        local palette
        palette="$(mktemp /tmp/orbitshield-palette-XXXXXX.png)"
        ffmpeg -y -hide_banner -loglevel error -thread_queue_size 1024 \
            -framerate "$GIF_FPS" -i "$png_pattern" \
            -vf "scale='min(${GIF_MAX_WIDTH},iw)':-1:flags=lanczos,palettegen" \
            -frames:v 1 -update 1 "$palette"
        ffmpeg -y -hide_banner -loglevel error -thread_queue_size 1024 \
            -framerate "$GIF_FPS" -i "$png_pattern" -i "$palette" \
            -lavfi "scale='min(${GIF_MAX_WIDTH},iw)':-1:flags=lanczos[x];[x][1:v]paletteuse" \
            -loop "$GIF_LOOP" "$gif_path"
        rm -f "$palette"
        return 0
    fi

    if command -v magick &>/dev/null; then
        local delay
        delay=$((100 / GIF_FPS))
        [[ "$delay" -lt 1 ]] && delay=1
        magick -delay "$delay" -loop "$GIF_LOOP" "${frame_files[@]}" \
            -resize "${GIF_MAX_WIDTH}x>" "$gif_path"
        return 0
    fi

    if command -v convert &>/dev/null; then
        local delay
        delay=$((100 / GIF_FPS))
        [[ "$delay" -lt 1 ]] && delay=1
        convert -delay "$delay" -loop "$GIF_LOOP" "${frame_files[@]}" \
            -resize "${GIF_MAX_WIDTH}x>" "$gif_path"
        return 0
    fi

    echo "Error: no GIF tool found. Install ffmpeg or ImageMagick (magick/convert)." >&2
    return 1
}

# ── generate frames ─────────────────────────────────────────────────────────
echo "Generating ISL frames..."
echo "  ringFile   : $RING_FILE"
echo "  islMaxRange   : $ISL_MAX_RANGE km"
echo "  groundMaxRange: $GROUND_MAX_RANGE km"
echo "  frames     : $FRAMES"
echo "  startTime  : $START_TIME s"
echo "  timeStep   : $TIME_STEP s"
if [[ -n "$GIF_FILE" ]]; then
    echo "  gifFile    : $GIF_FILE"
    echo "  gifFps     : $GIF_FPS"
    echo "  gifLoop    : $GIF_LOOP"
    echo "  gifMaxW    : ${GIF_MAX_WIDTH}px"
fi

GENERATED_PNGS=()

for ((i = 0; i < FRAMES; ++i)); do
    FRAME_DOT="$(_frame_paths "$DOT_FILE" "$i")"
    FRAME_PNG="$(_frame_paths "$PNG_FILE" "$i")"
    FRAME_TIME="$(_frame_time "$i")"

    echo "[Frame $((i + 1))/$FRAMES] t=${FRAME_TIME}s"
    echo "  DOT: $FRAME_DOT"
    echo "  PNG: $FRAME_PNG"

    ./ns3 run orbitshield-isl-visualizer -- \
        --ringFile="$RING_FILE" \
        --islMaxRange="$ISL_MAX_RANGE" \
        --groundMaxRange="$GROUND_MAX_RANGE" \
        --simTime="$FRAME_TIME" \
        --outputFile="$FRAME_DOT"

    if [[ ! -f "$FRAME_DOT" ]]; then
        echo "Error: DOT file '$FRAME_DOT' was not created." >&2
        exit 1
    fi

    if [[ "$CAN_USE_WORLDMAP" -eq 1 ]]; then
        python3 "$RENDERER_PY" "$FRAME_DOT" "$FRAME_PNG" \
            --width="$WIDTH" --height="$HEIGHT" --dpi="$DPI" $LABELS
    else
        dot -Tpng "$FRAME_DOT" -o "$FRAME_PNG"
    fi

    GENERATED_PNGS+=("$FRAME_PNG")
done

if [[ -n "$GIF_FILE" ]]; then
    if [[ "$FRAMES" -eq 1 ]]; then
        FRAME_PATTERN="$PNG_FILE"
    else
        FRAME_PATTERN="${PNG_FILE%.*}_%04d.${PNG_FILE##*.}"
    fi

    echo "Building animated GIF: $GIF_FILE"
    _build_gif "$GIF_FILE" "$FRAME_PATTERN" "${GENERATED_PNGS[@]}"
fi

if [[ "$FRAMES" -eq 1 ]]; then
    echo "Done. PNG saved to: $PNG_FILE"
else
    echo "Done. Generated $FRAMES PNG frames (pattern: ${PNG_FILE%.*}_0000.${PNG_FILE##*.})"
fi

if [[ -n "$GIF_FILE" ]]; then
    echo "Done. GIF saved to: $GIF_FILE"
fi
