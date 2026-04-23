# Praktyczne wprowadzenie do tworzenia sterowników urządzeń w systemie Linux

Projekt dyplomowy realizowany na Wydziale Elektrotechniki, Automatyki, Informatyki i Inżynierii Biomedycznej AGH.

Poszczególne etapy projektu oraz odpowiadający im kod źródłowy zostały umieszczone na dedykowanych branchach (gałęziach) repozytorium.

## Opis Projektu (PL)

Niniejsze repozytorium zawiera materiały oraz implementacje będące częścią pracy dyplomowej pt. **"Praktyczne wprowadzenie do tworzenia sterowników urządzeń w systemie Linux na przykładzie urządzeń z magistralami SPI i I2C"**.

Celem projektu było stworzenie kompleksowego przewodnika oraz zestawu sterowników demonstrujących mechanizmy działania jądra systemu Linux w kontekście obsługi urządzeń zewnętrznych. Praca łączy teorię programowania niskopoziomowego z praktycznym zastosowaniem na platformie sprzętowej (Raspberry Pi).

Pełna treść pracy dyplomowej dostępna jest tutaj: [ProjektDyplomowy.pdf](./ProjektDyplomowy.pdf).

### Co znajduje się w repozytorium?
* **Moduły jądra (LKM):** Przykłady minimalnych modułów, mechanizmy ładowania i usuwania sterowników.
* **Sterowniki urządzeń znakowych:** Implementacja komunikacji między przestrzenią użytkownika (userspace) a jądrem.
* **Obsługa Device Tree:** Konfiguracja opisu sprzętu dla systemów wbudowanych.
* **Sterowniki magistrali SPI:**
    * Obsługa przetwornika cyfrowo-analogowego (DAC MCP4921/4922).
    * Komunikacja z ekspanderem portów I/O przy użyciu mechanizmu **REGMAP**.
* **Sterowniki magistrali I2C:** Implementacja obsługi zewnętrznych układów scalonych.
* **Zastosowania praktyczne:** Sterowniki dla diod LED oraz serwomechanizmów (urządzenia platformowe).

### Technologie
* **Język:** C (Kernel-side)
* **System:** Linux (Kernel v6.1)
* **Sprzęt:** Raspberry Pi, magistrale SPI, I2C

---

## Project Overview (EN)

This repository contains materials and implementations developed as part of the diploma thesis: **"A practical introduction to creating device drivers in Linux using the example of devices with SPI and I2C buses"**.

The project aims to provide a comprehensive guide and a set of functional drivers demonstrating Linux kernel mechanisms for hardware abstraction and peripheral communication. The work bridges the gap between low-level programming theory and hands-on application on embedded platforms.

The source code and subsequent development stages are organized into dedicated branches within this repository.

The full text of the thesis is available here (PL only): [ProjektDyplomowy.pdf](./ProjektDyplomowy.pdf).

### Key Features
* **Linux Kernel Modules (LKM):** Foundational examples of module lifecycle management.
* **Character Device Drivers:** Implementing standard interfaces for userspace-kernel communication.
* **Device Tree Integration:** Hardware description and configuration for embedded Linux.
* **SPI Bus Drivers:**
    * Driver for Digital-to-Analog Converter (DAC MCP4921/4922).
    * I/O Port Expander communication utilizing the **REGMAP** API.
* **I2C Bus Drivers:** Implementation of drivers for integrated circuits over the I2C protocol.
* **Hardware Control:** Practical drivers for LED signaling and Servo motor control (Platform Devices).

### Tech Stack
* **Language:** C (Kernel-side)
* **OS:** Linux (Kernel v6.1)
* **Hardware:** Raspberry Pi, SPI & I2C peripherals