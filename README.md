# devgraph
Linux hardware topology mapper

Builds a graph from:
- /sys
- device tree
- /dev
- driver information
- I²C/SPI/USB/PCI relationships
For example:
SoC
 ├── I2C-1
 │    ├── PMIC
 │    └── temperature sensor
 │
 ├── SPI-0
 │    └── touchscreen
 │
 ├── USB
 │    └── camera
 │
 └── MMC
      └── eMMC
