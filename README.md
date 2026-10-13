# Secrets of Ravenwood

A C++17, HTML, and CSS horror adventure. Gameplay is handled by the C++ server, and browser controls use standard POST forms without JavaScript.

## Project structure

```text
Haunted mansion/
├── index.html         # Start screen
├── style.css          # Cinematic room styling and responsive layout
├── src/main.cpp       # HTTP server, room views, puzzles, and progression
├── Makefile           # Build and run shortcuts
└── README.md
```

## Run

Requires a C++17 compiler and a POSIX system (macOS or Linux).

```sh
make
./build/ravenwood
```

Then open http://localhost:8080. To choose another port, pass it to the server, for example `./build/ravenwood 9090`.

## Puzzle answers

- Hall: `clock`, `footsteps`, `echo`
- Bedroom: `closet` or `wardrobe`
- Kitchen: `onion` or `garlic`
- Library: `1` or `one`
- Stone door: `1336` (moon 1, three hall riddles, three kitchen fires, founding year ending in 6)

Player state remains in the running server and is associated with a browser cookie. Keep the server running while playing.
