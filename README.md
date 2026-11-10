# Klangbild Bass

Bass-Kanalzug im Klangbild-Vintage-Look – AU, VST3 und Standalone für macOS (Logic Pro).

## Signalweg

```
KOMPRESSOR → WEICHE ─┬─ Tiefen (sauber) ─────────────┬→ KLANG → BOX → AUSGANG
                     └─ Mitten/Höhen → ZERRE (Mix) ──┘
```

Die Frequenzweiche trennt den Bass: Unterhalb der **Weiche** (80–400 Hz) bleibt das Fundament
immer sauber und druckvoll, nur darüber wird angezerrt. So bekommt der Bass Biss und setzt sich
im Mix durch, ohne dass der Tiefbass matschig wird.

| Regler | Aufgabe |
|---|---|
| **Kompression** | Gleichmäßiger Pegel, 4:1 mit weichem Knie; lange Töne regeln ruhiger aus |
| **Zerre** | Wie stark die Mitten/Höhen angezerrt werden (bei 0 völlig sauber) |
| **Mischung** | Anteil der Zerre gegenüber dem sauberen Mittenanteil |
| **Weiche** | Ab welcher Frequenz die Zerre greift |
| **Charakter** | Röhre (warm, gerade Obertöne) oder Transistor (härter, knurriger) |
| **Bass / Mitten / Höhen** | Klangregelung; Mitten mit wählbarer Frequenz 250 Hz – 3 kHz |
| **Box-Simulation** | DI (direkt) oder Bassbox: Tiefbass-Bauch, Mitten-Senke, früher Höhenabfall |
| **Ausgang** | Ausgangspegel |

Zwei VU-Meter zeigen die Pegelreduktion des Kompressors und den Ausgangspegel.

## Presets

| Preset | Gedacht für |
|---|---|
| Standard | Neutraler Startpunkt, leichte Kompression |
| Motown – Flatwound | Runde 60er-Soul-Bässe, Höhen weg, Box an |
| Funk – Slap | Mitten ausgehöhlt, kräftige Kompression, viel Attack |
| Soul – warm | Warm, leicht angeraut, sitzt unter der Gitarre |
| Blues – rau | Mehr Mitten und Röhrenzerre |
| Rock – Plektrum | Bissig und präsent für Plektrum-Spiel |
| Modern – Knurren | Transistor-Zerre ab 250 Hz, sauberer Tiefbass |
| Dub – Tiefbass | Viel Fundament, Höhen stark gedämpft |
| Fingerstyle – natürlich | Dezent, natürlicher Fingeranschlag |

Alle Presets sind auf etwa gleiche Lautstärke eingepegelt.

## Oberfläche

Das Fenster lässt sich an der rechten unteren Ecke stufenlos von 75 % bis 150 % vergrößern;
die Größe wird gemerkt.

## Installation

Am einfachsten über den [Klangbild Installer](https://github.com/Guitoli1/KlangbildInstaller):

```
curl -fsSL https://raw.githubusercontent.com/Guitoli1/KlangbildInstaller/main/install.sh | bash
```

## Build

JUCE 8 / CMake. GitHub Actions baut auf macOS 15 ein Universal Binary (arm64 + x86_64), signiert ad-hoc und prüft mit `auval`.
