# OrbitShield Module

## Overview

**OrbitShield** is an ns-3 module designed to simulate satellite constellations and their inter-satellite links (ISLs). It provides a comprehensive framework for modeling and analyzing satellite communication networks focusing on Low Earth Orbit (LEO) constellations.

## Features

### Current Implementation
- **Models**
  - **Satellite Node**: Basic satellite node representation with orbital propagation capabilities
  - **Constellation**: Collection of satellites with TLE file loading support
- **Test Suite**: Unit tests for core satellite and constellation functionality
- **Example Applications**: Basic examples demonstrating module usage

### Future Enhancements
- Inter-satellite link (ISL) channel model
- Orbital mechanics and position calculation (partially implemented)
- Satellite handover mechanisms
- Ground station connections
- Coverage analysis tools
- Link budget calculations
- Latency models based on orbital position

## Module Structure

```
contrib/orbitshield/
├── model/
│   ├── satellite.h              # Satellite node class definition
│   ├── satellite.cc             # Satellite node implementation
│   ├── constellation.h          # Constellation class definition
│   ├── constellation.cc         # Constellation implementation
│   └── orbitshield-module.h     # Module public interface
├── test/
│   ├── test-satellite.cc        # Satellite unit tests
│   └── test-constellation.cc    # Constellation unit tests
├── examples/
│   ├── orbitshield-basic-example.cc      # Basic satellite usage
│   ├── orbitshield-load-from-tle.cc      # Constellation loading example
│   └── CMakeLists.txt                    # Examples build configuration
├── data/
│   └── iridium-20260312.txt    # Sample TLE data
├── CMakeLists.txt               # Build configuration
└── README.md                     # This file
```

## Dependencies

This module has the following dependencies:

- [perturb](https://github.com/gunvirranu/perturb): A modern C++11 wrapper for the SGP4 orbit propagator

## Installation

The OrbitShield module is part of the ns-3 contrib collection. It is automatically included in the build if located in the `contrib/` directory and CMake is configured.

### Building ns-3 with OrbitShield

```bash
cd ns-3-dev
./ns3 build
```

The build system will detect and compile the OrbitShield module along with other ns-3 modules.

## Basic Usage

### Creating a Single Satellite from TLE

The `Satellite` class is initialized from Two-Line Element (TLE) data. Provide a `name` plus the two TLE lines to create a satellite instance.

```cpp
#include "ns3/core-module.h"
#include "ns3/orbitshield-module.h"

using namespace ns3;

int main()
{
    std::string name = "ISS (ZARYA)";
    std::string tle1 = "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996";
    std::string tle2 = "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227";

    Ptr<Satellite> satellite = CreateObject<Satellite>(name, tle1, tle2);

    // Query satellite properties
    std::cout << "Satellite: " << satellite->GetName() << std::endl;
    std::cout << "Position (ECI): " << satellite->GetPosition() << std::endl;

    return 0;
}
```

### Creating a Constellation from a TLE File

`Constellation` provides a helper to load many satellites from a TLE file (e.g., `contrib/orbitshield/data/iridium-20260312.txt`).

```cpp
#include "ns3/core-module.h"
#include "ns3/orbitshield-module.h"

using namespace ns3;

int main()
{
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromTleFile("contrib/orbitshield/data/iridium-20260312.txt");

    const auto& sats = constellation->GetSatellites();
    for (const auto& sat : sats)
    {
        std::cout << "Satellite " << sat->GetName() << " position=" << sat->GetPosition() << "\n";
    }

    return 0;
}
```

### Running the Examples

To run the basic example:

```bash
cd ns-3-dev
./ns3 configure --enable-examples  # Enable examples during configuration
./ns3 build
./ns3 run orbitshield-basic-example
```

To run the constellation loading example:

```bash
./ns3 run orbitshield-load-from-tle
```

**Note**: Examples are not built by default in ns-3. You must configure with `--enable-examples` to include them in the build.

## API Reference

### Satellite Class

```cpp
class Satellite : public Node
{
public:
    // Type identification
    static TypeId GetTypeId();

    // Constructor and Destructor
    Satellite(std::string& name, std::string& tle1, std::string& tle2);
    ~Satellite();

    // Satellite properties
    std::string GetName() const;
    Vector3D GetPosition();
};
```

### Constellation Class

```cpp
class Constellation : public Object
{
public:
    static TypeId GetTypeId();

    Constellation();
    ~Constellation();

    void LoadFromTleFile(const std::string& filename);
    const std::vector<Ptr<Satellite>>& GetSatellites() const;
};
```

## Testing

Unit tests are included in `test/test-satellite.cc` and `test/test-constellation.cc` and verify:
- Satellite object creation from TLE
- Satellite name access
- Position propagation via perturb
- Constellation loading from TLE files
- Satellite collection management

Run the test suite:

```bash
./ns3 configure --enable-tests
./ns3 run "test-runner --suite=orbitshield"
```

## Design Considerations

### Coordinate Systems
Currently uses absolute orbital parameters. Future versions may support:
- Earth Centered Inertial (ECI) coordinates
- Earth Centered Earth Fixed (ECEF) coordinates
- Geographic (latitude/longitude/altitude) coordinates

### Orbital Mechanics
The current implementation provides basic orbital mechanics with:
- TLE-based orbit propagation using the SGP4 model
- Position calculations in Earth-Centered Inertial (ECI) coordinates
- Future versions will include:
  - Two-body orbit propagation
  - Kepler elements
  - Velocity calculations
  - Orbital decay and atmospheric drag models

### Inter-Satellite Links
The framework is designed to support ISL modeling with planned features:
- Line-of-sight determination
- Link availability calculations
- Doppler shift effects
- Antenna pointing models

## Contributing

Contributions to the OrbitShield module are welcome! When contributing:

1. Follow ns-3 coding standards
2. Include unit tests for new features
3. Update documentation
4. Ensure compatibility with the current ns-3 architecture
5. Test with the full ns-3 build system

## Dependencies

- **Core**: ns-3 core module
- **Network**: ns-3 network module
- **C++ Standard**: C++14 or higher

## License

OrbitShield is part of the ns-3 project and is distributed under the GNU General Public License (GPL) version 2. See the LICENSE file in the root ns-3 directory for details.

## References

### Satellite Orbits
- ISS Orbit: ~400 km altitude, 51.6° inclination
- Starlink LEO: ~550 km altitude, 53° inclination
- Kuiper LEO: ~630 km altitude, 51.9° inclination
- MEO Constellations: ~20,000-35,000 km altitude
- GEO: ~35,786 km altitude, 0° inclination

### Related Standards and Specifications
- ITU-R Recommendations on satellite systems
- 3GPP technical specifications for non-terrestrial networks (NTN)
- CCSDS protocols for spacecraft communications

## Authors

OrbitShield module developed as part of the ns-3 project.

## Version

**Version 0.1** - Initial release with satellite node, constellation management, and TLE data loading capabilities.

## Support

For issues, questions, or suggestions regarding the OrbitShield module, please:
1. Check existing ns-3 documentation
2. Consult the ns-3 wiki and tutorials
3. Post in ns-3 community forums
4. Open issues in the project repository (when available)
