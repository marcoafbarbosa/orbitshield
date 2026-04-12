#!/usr/bin/env python3
"""
Convert TLE (Two-Line Element) file to CSV format.
Extracts all available information from TLE format.
"""

import csv
import re
import sys
from pathlib import Path


def parse_tle_file(filepath):
    """
    Parse a TLE file and return a list of satellite records.
    TLE files have format:
    Line 0: Satellite Name
    Line 1: TLE Line 1 (catalog data)
    Line 2: TLE Line 2 (orbital elements)
    """
    satellites = []
    
    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    # Process every 3 lines as a complete TLE record
    for i in range(0, len(lines), 3):
        if i + 2 >= len(lines):
            break
        
        name_line = lines[i].strip()
        line1 = lines[i + 1].strip()
        line2 = lines[i + 2].strip()
        
        if not (line1.startswith('1') and line2.startswith('2')):
            continue
        
        satellite = parse_tle_record(name_line, line1, line2)
        if satellite:
            satellites.append(satellite)
    
    return satellites


def parse_exp_notation(value_str):
    """
    Parse TLE exponential notation where the format is like:
    "00000-0" means 0.00000e-0
    "16199-3" means 0.16199e-3
    "38599-3" means 0.38599e-3
    "-14189-5" means -0.14189e-5 (negative mantissa)
    """
    if not value_str or not value_str.strip():
        return 0.0
    
    value_str = value_str.strip()
    
    # Find the LAST occurrence of + or - (the exponent sign)
    # Everything before it is the mantissa, including any leading sign
    sign_pos = -1
    for i in range(len(value_str) - 1, -1, -1):
        if value_str[i] in '+-':
            # Make sure this isn't at position 0 (which would be a leading sign)
            if i > 0:
                sign_pos = i
                break
    
    if sign_pos == -1:
        # No exponent sign found, treat as regular number
        try:
            return float(value_str)
        except:
            return 0.0
    
    # Extract mantissa and exponent parts
    mantissa_str = value_str[:sign_pos].strip()
    exponent_part = value_str[sign_pos:]
    
    # Build the mantissa with decimal point
    # If mantissa is 5 digits like "16199" or "-14189", it becomes 0.16199 or -0.14189
    try:
        if mantissa_str.startswith('-'):
            # Negative mantissa
            mantissa = float('-0.' + mantissa_str[1:])
        else:
            # Positive mantissa
            mantissa = float('0.' + mantissa_str)
        
        exponent = int(exponent_part)
        result = mantissa * (10.0 ** exponent)
        return result
    except Exception as e:
        print(f"Failed to parse: mantissa_str='{mantissa_str}', exponent_part='{exponent_part}', error={e}")
        return 0.0


def parse_tle_record(name_line, line1, line2):
    """
    Parse a single TLE record (3 lines) into a dictionary.
    """
    try:
        # Parse name line
        name_parts = name_line.split()
        satellite_name = ' '.join(name_parts[1:]) if len(name_parts) > 1 else name_line
        
        # Parse Line 1
        catalog_number = line1[2:7].strip()
        classification = line1[7:8].strip()
        international_designator = line1[9:17].strip()
        epoch_year = int(line1[18:20])
        epoch_day = float(line1[20:32])
        
        # First derivative of mean motion 
        first_derivative = float(line1[33:43])
        
        # Second derivative - special format
        second_derivative = parse_exp_notation(line1[44:52])
        
        # BSTAR - special format
        bstar = parse_exp_notation(line1[53:61])
        
        ephemeris_type = line1[62:63].strip()
        element_set_number = line1[64:68].strip()
        checksum1 = line1[68:69].strip()
        
        # Parse Line 2
        catalog_number2 = line2[2:7].strip()
        inclination = float(line2[8:16])
        raan = float(line2[17:25])
        
        # Eccentricity format "NNNNNNN" where decimal is assumed "0.NNNNNNN"
        eccentricity_str = line2[26:33]
        eccentricity = float('0.' + eccentricity_str)
        
        argument_of_perigee = float(line2[34:42])
        mean_anomaly = float(line2[43:51])
        mean_motion = float(line2[52:63])
        revolution_number = line2[63:68].strip()
        checksum2 = line2[68:69].strip()
        
        return {
            'Satellite_Name': satellite_name,
            'Catalog_Number': catalog_number,
            'Classification': classification,
            'International_Designator': international_designator,
            'Epoch_Year': epoch_year,
            'Epoch_Day': epoch_day,
            'First_Derivative_Mean_Motion': first_derivative,
            'Second_Derivative_Mean_Motion': second_derivative,
            'BSTAR_Drag_Term': bstar,
            'Ephemeris_Type': ephemeris_type,
            'Element_Set_Number': element_set_number,
            'Checksum_Line1': checksum1,
            'Inclination_deg': inclination,
            'RAAN_deg': raan,
            'Eccentricity': eccentricity,
            'Argument_of_Perigee_deg': argument_of_perigee,
            'Mean_Anomaly_deg': mean_anomaly,
            'Mean_Motion_rev_per_day': mean_motion,
            'Revolution_Number': revolution_number,
            'Checksum_Line2': checksum2,
        }
    except Exception as e:
        print(f"Error parsing TLE record: {e}")
        print(f"Name: {name_line}")
        print(f"Line1: {line1}")
        print(f"Line2: {line2}")
        return None


def write_csv(satellites, output_file=None):
    """
    Write satellite records to CSV format.
    If output_file is None, writes to stdout.
    """
    if not satellites:
        print("No satellites to write", file=sys.stderr)
        return
    
    fieldnames = satellites[0].keys()
    
    output = output_file if output_file else sys.stdout
    writer = csv.DictWriter(output, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerows(satellites)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: convert_tle_to_csv.py <input_tle_file>", file=sys.stderr)
        print("Output is written to stdout", file=sys.stderr)
        sys.exit(1)
    
    input_file = Path(sys.argv[1])
    
    if not input_file.exists():
        print(f"Error: File not found: {input_file}", file=sys.stderr)
        sys.exit(1)
    
    satellites = parse_tle_file(input_file)
    write_csv(satellites)
