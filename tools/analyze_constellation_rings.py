#!/usr/bin/env python3
"""
Analyze satellite constellation CSV data and generate rings configuration file.
Groups satellites into orbital planes (rings) based on RAAN clustering.
"""

import csv
import math
import sys
from pathlib import Path


def load_satellite_data(csv_filepath):
    """
    Load satellite data from CSV file.
    Returns list of satellite dictionaries.
    """
    satellites = []
    
    with open(csv_filepath, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                satellites.append({
                    'name': row['Satellite_Name'],
                    'raan': float(row['RAAN_deg']) % 360.0,
                    'inclination': float(row['Inclination_deg']),
                    'mean_motion': float(row['Mean_Motion_rev_per_day']),
                    'catalog_number': row['Catalog_Number']
                })
            except (KeyError, ValueError) as e:
                print(f"Warning: Skipping invalid row: {e}", file=sys.stderr)
                continue
    
    return satellites


def identify_constellation_rings(satellites, min_inclination, max_inclination, num_rings):
    """
    Identify orbital planes (rings) by clustering satellites based on RAAN.
    Returns a list of rings, where each ring is a list of satellite names.
    """
    # Focus on main constellation (satellites within specified inclination range)
    main_constellation = [sat for sat in satellites 
                         if min_inclination <= sat['inclination'] <= max_inclination]
    
    if not main_constellation:
        print(f"Warning: No satellites found in inclination range {min_inclination}°-{max_inclination}°", file=sys.stderr)
        return []
    
    print(f"Analyzing {len(main_constellation)} satellites in inclination range {min_inclination}°-{max_inclination}°", file=sys.stderr)
    
    if num_rings == 'auto':
        # Automatic ring detection based on RAAN clustering
        rings = identify_rings_auto(main_constellation)
    else:
        # Fixed number of rings with predefined centers
        rings = identify_rings_fixed(main_constellation, int(num_rings))
    
    return rings


def identify_rings_auto(satellites):
    """
    Automatically identify rings by finding natural clusters in RAAN values.
    """
    sorted_satellites = sorted(satellites, key=lambda sat: sat['raan'])

    # Find natural breaks by looking for gaps larger than a threshold
    gap_threshold = 0.5  # degrees - gaps larger than this indicate plane boundaries

    clusters = []
    current_cluster = [sorted_satellites[0]]

    for sat in sorted_satellites[1:]:
        gap = sat['raan'] - current_cluster[-1]['raan']
        if gap > gap_threshold:
            clusters.append(current_cluster)
            current_cluster = [sat]
        else:
            current_cluster.append(sat)

    if current_cluster:
        clusters.append(current_cluster)

    print(f"Auto-detected {len(clusters)} rings based on RAAN gaps > {gap_threshold}°", file=sys.stderr)
    return summarize_ring_clusters(clusters)


def circular_difference(angle, reference):
    """Return the signed shortest angular difference from reference to angle."""
    diff = (angle - reference + 180.0) % 360.0 - 180.0
    return diff


def summarize_ring_clusters(clusters):
    """Print ring summary information and return the resulting ring assignments."""
    ring_infos = []
    for cluster in clusters:
        ring_names = [sat['name'] for sat in cluster]
        ring_raans = [sat['raan'] for sat in cluster]
        ring_mean_motions = [sat['mean_motion'] for sat in cluster]
        center_raan = circular_mean(ring_raans) if ring_raans else None
        ring_infos.append((center_raan, ring_names, ring_raans, ring_mean_motions))

    ring_infos.sort(key=lambda item: (item[0] is None, item[0] if item[0] is not None else 0.0))

    rings = []
    for i, (center_raan, ring_names, ring_raans, ring_mean_motions) in enumerate(ring_infos):
        ring_names.sort()
        rings.append(ring_names)

        if not ring_names:
            print(f"Ring {i}: 0 satellites (empty cluster)", file=sys.stderr)
            continue

        raan_half_range = max(abs(circular_difference(raan, center_raan)) for raan in ring_raans)
        raan_summary = f"RAAN {center_raan:.1f}° ± {raan_half_range:.1f}°"

        if ring_mean_motions:
            mm_min = min(ring_mean_motions)
            mm_max = max(ring_mean_motions)
            print(
                f"Ring {i}: {len(ring_names)} satellites ({raan_summary}, Mean Motion {mm_min:.5f}-{mm_max:.5f} rev/day)",
                file=sys.stderr
            )
        else:
            print(
                f"Ring {i}: {len(ring_names)} satellites ({raan_summary})",
                file=sys.stderr
            )

    return rings


def circular_mean(angles):
    """Return the circular mean of a list of angles in degrees."""
    if not angles:
        return 0.0
    x = sum(math.cos(math.radians(angle)) for angle in angles)
    y = sum(math.sin(math.radians(angle)) for angle in angles)
    mean_angle = math.degrees(math.atan2(y, x))
    return mean_angle % 360.0


def _initialize_cluster_centers(points, num_rings):
    """Initialize cluster centers for circular k-means using furthest-point selection."""
    centers = [(points[0]['x'], points[0]['y'])]
    while len(centers) < num_rings:
        farthest = None
        max_distance = -1.0
        for point in points:
            distance = min((point['x'] - cx) ** 2 + (point['y'] - cy) ** 2 for cx, cy in centers)
            if distance > max_distance:
                max_distance = distance
                farthest = point
        if farthest is None:
            break
        centers.append((farthest['x'], farthest['y']))
    return centers


def identify_rings_fixed(satellites, num_rings):
    """
    Identify rings using fixed number and circular clustering on RAAN values.
    """
    if num_rings <= 0:
        return []

    # Represent RAAN as points on the unit circle for circular clustering.
    points = [
        {
            'name': sat['name'],
            'raan': sat['raan'],
            'mean_motion': sat['mean_motion'],
            'x': math.cos(math.radians(sat['raan'])),
            'y': math.sin(math.radians(sat['raan']))
        }
        for sat in satellites
    ]

    if not points:
        return [[] for _ in range(num_rings)]

    # Initialize centers based on the data distribution rather than fixed values.
    centers = _initialize_cluster_centers(points, min(num_rings, len(points)))
    if len(centers) < num_rings:
        centers.extend([(0.0, 0.0)] * (num_rings - len(centers)))

    for _ in range(50):
        clusters = [[] for _ in range(num_rings)]
        for point in points:
            best_ring = None
            best_distance = float('inf')
            for i, (cx, cy) in enumerate(centers):
                dx = point['x'] - cx
                dy = point['y'] - cy
                distance = dx * dx + dy * dy
                if distance < best_distance:
                    best_distance = distance
                    best_ring = i
            clusters[best_ring].append(point)

        new_centers = []
        converged = True
        for i, cluster in enumerate(clusters):
            if not cluster:
                new_centers.append(centers[i])
                continue
            avg_x = sum(point['x'] for point in cluster) / len(cluster)
            avg_y = sum(point['y'] for point in cluster) / len(cluster)
            norm = math.hypot(avg_x, avg_y)
            if norm < 1e-12:
                new_centers.append(centers[i])
                continue
            new_center = (avg_x / norm, avg_y / norm)
            if abs(new_center[0] - centers[i][0]) > 1e-6 or abs(new_center[1] - centers[i][1]) > 1e-6:
                converged = False
            new_centers.append(new_center)

        centers = new_centers
        if converged:
            break

    # Sort clusters by circular mean RAAN so ring ordering is consistent.
    return summarize_ring_clusters(clusters)


def write_rings_file(rings, constellation_name=None):
    """
    Write rings configuration to stdout in orbitshield format.
    """
    if constellation_name is None:
        constellation_name = "constellation"
    
    print(f"constellationName={constellation_name}")
    print(f"ringCount={len(rings)}")
    
    for i, ring in enumerate(rings):
        satellite_list = ','.join(ring)
        print(f"ring.{i}={satellite_list}")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Analyze satellite constellation CSV data and generate rings configuration.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s data.csv --min-inc 86.0 --max-inc 87.0 --rings 6
  %(prog)s data.csv --min-inc 80.0 --max-inc 90.0 --rings auto
  %(prog)s data.csv --rings auto  # Use default inclination range
        '''
    )
    
    parser.add_argument('input_csv', help='Input CSV file with satellite data')
    parser.add_argument('--min-inc', type=float, default=86.35, 
                       help='Minimum inclination to consider (degrees, default: 86.35)')
    parser.add_argument('--max-inc', type=float, default=86.45,
                       help='Maximum inclination to consider (degrees, default: 86.45)')
    parser.add_argument('--rings', default='auto',
                       help='Number of rings or "auto" for automatic detection (default: auto)')
    parser.add_argument('--min-sats', type=int, default=2,
                       help='Minimum number of satellites per ring (default: 2)')
    parser.add_argument('--name', default=None,
                       help='Constellation name (default: auto-generated from filename)')
    
    args = parser.parse_args()
    
    # Validate arguments
    if args.rings != 'auto':
        try:
            num_rings = int(args.rings)
            if num_rings < 1:
                parser.error("Number of rings must be positive")
        except ValueError:
            parser.error("Number of rings must be a positive integer or 'auto'")
    
    if args.min_sats < 1:
        parser.error("Minimum satellites per ring must be positive")
    
    if not Path(args.input_csv).exists():
        parser.error(f"Input CSV file not found: {args.input_csv}")
    
    # Load and analyze data
    satellites = load_satellite_data(args.input_csv)
    rings = identify_constellation_rings(satellites, args.min_inc, args.max_inc, args.rings)
    
    # Filter rings by minimum satellite count
    filtered_rings = [ring for ring in rings if len(ring) >= args.min_sats]
    
    if len(filtered_rings) < len(rings):
        removed_count = len(rings) - len(filtered_rings)
        print(f"Filtered out {removed_count} ring(s) with fewer than {args.min_sats} satellites", file=sys.stderr)
    
    if not filtered_rings:
        print("Error: No rings identified after filtering", file=sys.stderr)
        sys.exit(1)
    
    # Determine constellation name
    constellation_name = args.name
    if constellation_name is None:
        # Auto-generate from filename
        csv_name = Path(args.input_csv).stem
        if '-20' in csv_name:
            parts = csv_name.split('-')
            if len(parts) >= 2 and len(parts[1]) >= 4:
                year_short = parts[1][:4]
                constellation_name = f"{parts[0]}-{year_short}"
            else:
                constellation_name = csv_name
        else:
            constellation_name = csv_name
    
    # Output to stdout
    write_rings_file(filtered_rings, constellation_name)


if __name__ == '__main__':
    main()