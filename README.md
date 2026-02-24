# OrbitShield Module

## Overview

**OrbitShield** is an ns-3 module designed to simulate satellite constellations and their inter-satellite links (ISLs). It provides a comprehensive framework for modeling and analyzing satellite communication networks, including Low Earth Orbit (LEO), Medium Earth Orbit (MEO), and Geostationary Earth Orbit (GEO) constellations.

## Features

### Current Implementation
- **Satellite Node Model**: Basic satellite node representation with orbital parameters
- **Orbital Parameters**:
  - Altitude configuration (in meters)
  - Orbital inclination configuration (in degrees)
- **Test Suite**: Unit tests for core satellite functionality
- **Example Applications**: Basic examples demonstrating module usage

### Future Enhancements
- Inter-satellite link (ISL) channel model
- Orbital mechanics and position calculation
- Satellite handover mechanisms
- Ground station connections
- Coverage analysis tools
- Constellation management helpers
- Link budget calculations
- Latency models based on orbital position

## Module Structure

```
contrib/orbitshield/
├── model/
│   ├── satellite.h              # Satellite node class definition
│   ├── satellite.cc             # Satellite node implementation
│   └── orbitshield-module.h     # Module public interface
├── test/
│   └── test-satellite.cc        # Unit tests
├── examples/
│   └── orbitshield-basic-example.cc  # Basic usage example
├── CMakeLists.txt               # Build configuration
└── README.md                     # This file
```

## Installation

The OrbitShield module is part of the ns-3 contrib collection. It is automatically included in the build if located in the `contrib/` directory and CMake is configured.

### Building ns-3 with OrbitShield

```bash
cd ns-3-dev
./ns3 build
```

The build system will detect and compile the OrbitShield module along with other ns-3 modules.

## Basic Usage

### Creating a Single Satellite

```cpp
#include "ns3/core-module.h"
#include "ns3/orbitshield-module.h"

using namespace ns3;

int main()
{
    // Create a satellite
    Ptr<Satellite> satellite = CreateObject<Satellite>();
    
    // Configure orbital parameters
    satellite->SetAltitude(400000.0);      // 400 km altitude (ISS-like)
    satellite->SetInclination(51.6);       // ISS-like inclination
    
    // Query satellite properties
    std::cout << "Altitude: " << satellite->GetAltitude() << " m" << std::endl;
    std::cout << "Inclination: " << satellite->GetInclination() << "°" << std::endl;
    
    return 0;
}
```

### Creating a Constellation

```cpp
#include "ns3/core-module.h"
#include "ns3/orbitshield-module.h"

using namespace ns3;

int main()
{
    // Create a constellation with multiple satellites
    std::vector<Ptr<Satellite>> constellation;
    
    for (int i = 0; i < 36; ++i)
    {
        Ptr<Satellite> sat = CreateObject<Satellite>();
        sat->SetAltitude(550000.0);        // 550 km (typical LEO)
        sat->SetInclination(53.0);          // Preferred inclination
        constellation.push_back(sat);
    }
    
    NS_LOG_INFO("Created constellation with " << constellation.size() 
                << " satellites");
    
    return 0;
}
```

### Running the Examples

To run the basic example:

```bash
cd ns-3-dev
./ns3 run orbitshield-basic-example
```

## API Reference

### Satellite Class

```cpp
class Satellite : public Node
{
public:
    // Type identification
    static TypeId GetTypeId();
    
    // Constructor and Destructor
    Satellite();
    ~Satellite();
    
    // Orbital Parameters
    void SetAltitude(double altitude);
    double GetAltitude() const;
    
    void SetInclination(double inclination);
    double GetInclination() const;
};
```

#### Attributes

- **Altitude** (double, default: 400000.0 m)
  - Orbital altitude above Earth's surface in meters
  - Valid range: [0, ∞)
  
- **Inclination** (double, default: 45.0°)
  - Orbital inclination relative to Earth's equator in degrees
  - Valid range: [0°, 180°]

## Testing

Unit tests are included in `test/test-satellite.cc` and verify:
- Satellite object creation
- Default orbital parameter values
- Setting and getting altitude
- Setting and getting inclination

Run the test suite:

```bash
./ns3 run "test-runner --suite=orbitshield"
```

## Design Considerations

### Coordinate Systems
Currently uses absolute orbital parameters. Future versions may support:
- Earth Centered Inertial (ECI) coordinates
- Earth Centered Earth Fixed (ECEF) coordinates
- Geographic (latitude/longitude/altitude) coordinates

### Orbital Mechanics
The current implementation provides a foundation for orbital mechanics. Future versions will include:
- Two-body orbit propagation
- Kepler elements
- Position/velocity calculations
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

**Version 0.1** - Initial bare-bones release with satellite node and orbital parameters.

## Support

For issues, questions, or suggestions regarding the OrbitShield module, please:
1. Check existing ns-3 documentation
2. Consult the ns-3 wiki and tutorials
3. Post in ns-3 community forums
4. Open issues in the project repository (when available)
