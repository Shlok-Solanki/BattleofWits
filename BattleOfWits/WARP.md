# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.
``

Project overview
- C++17 project built with CMake. Produces a library target battle_of_wits_lib and an executable battle_of_wits.
- Dependencies via CMake FetchContent: nlohmann/json and cpp-httplib. Optional: SFML (graphics/audio/window/system) and OpenSSL (for HTTPS in httplib).
- If SFML is found, the executable is built with HAS_SFML defined for a simple start menu window; otherwise it runs in console.

Commands
Build (out-of-source)
- Configure (Debug):
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
- Configure (Release):
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
- Build (multi-config generators like MSVC):
  cmake --build build --config Debug
  cmake --build build --config Release
- Build (single-config generators like Ninja/MinGW):
  cmake --build build

Run
- MSVC-style output:
  build\\Debug\\battle_of_wits.exe
  build\\Release\\battle_of_wits.exe
- Single-config (e.g., Ninja/MinGW):
  build/battle_of_wits

Clean
- Remove build artifacts:
  cmake --build build --target clean
  # or delete the build directory

Lint/format
- No linters/formatters are configured in-repo (no .clang-format/.clang-tidy present).

Tests
- No tests are currently defined. If CTest is added later: ctest --test-dir build -C Debug and ctest -R <name> to run a single test.

High-level architecture
- main (src/main.cpp): Entry point. Optionally shows an SFML start screen (guarded by HAS_SFML), then runs a sample round via Game.
- Game (include/Game.h, src/Game.cpp): Orchestrates a round. Manages Player state, selected GenreType, triggers fetching a question and presenting it.
- ApiClient (include/ApiClient.h, src/ApiClient.cpp): Fetches questions from OpenTDB using cpp-httplib; parses JSON with nlohmann/json; normalizes to Question. If OpenSSL is available, CPPHTTPLIB_OPENSSL_SUPPORT is defined to enable HTTPS.
- Domain models:
  - Genre (include/Genre.h, src/Genre.cpp): GenreType enum and helpers to map to OpenTDB category ids and strings.
  - Question (include/Question.h, src/Question.cpp): Holds category, text, 4 options, correct index, optional hint.
  - Player (include/Player.h, src/Player.cpp): Name, score, and LifelineStatus flags.
  - Lifeline (include/Lifeline.h, src/Lifeline.cpp): Implements 50-50 (indices to hide) and audience poll (A/B/C/D distribution summing to 100).
  - Leaderboard (include/Leaderboard.h, src/Leaderboard.cpp): Appends scores to data/leaderboard.txt and reads top N entries. Ensure the data directory exists at runtime.
  - Utils (include/Utils.h, src/Utils.cpp): htmlDecode for OpenTDB entities; shuffleOptions to Fisher–Yates shuffle while tracking the correct answer index.

Build configuration notes
- CMake collects all headers and sources into battle_of_wits_lib and then links that into the battle_of_wits executable.
- Optional dependencies are discovered with find_package; behavior toggles via compile definitions and link libraries accordingly.

Operational notes
- Networking depends on cpp-httplib. For HTTPS endpoints, make sure OpenSSL is found by CMake or use HTTP endpoints.
- If adding GUI assets (fonts/textures), load them in main/SFML paths and include them in your run-time working directory.
- Leaderboard writes to data/leaderboard.txt relative to the working directory. Create the data folder before running if you want scores persisted.
