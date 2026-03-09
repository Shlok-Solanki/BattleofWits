🎮 Battle of Wits

A C++17 quiz game project that fetches trivia questions from an online API and allows players to answer them using lifelines similar to quiz shows like Who Wants to Be a Millionaire.

The project is built using CMake and supports both console mode and an optional SFML GUI start screen.

📌 Features

🎯 Trivia questions fetched from the OpenTDB API

👤 Player system with scoring

🎮 Multiple quiz genres/categories

🧠 Lifelines:

50-50

Audience Poll

🏆 Local Leaderboard system

🔀 Randomized answer options

🖥 Optional SFML GUI start menu

🛠 Tech Stack

Language: C++17

Build System: CMake

Libraries:

nlohmann/json

cpp-httplib

SFML (optional)

OpenSSL (optional for HTTPS)

📂 Project Structure

battle_of_wits/
│
├── include/
│   ├── ApiClient.h
│   ├── Game.h
│   ├── Genre.h
│   ├── Question.h
│   ├── Player.h
│   ├── Lifeline.h
│   ├── Leaderboard.h
│   └── Utils.h
│
├── src/
│   ├── main.cpp
│   ├── ApiClient.cpp
│   ├── Game.cpp
│   ├── Genre.cpp
│   ├── Question.cpp
│   ├── Player.cpp
│   ├── Lifeline.cpp
│   ├── Leaderboard.cpp
│   └── Utils.cpp
│
├── data/
│   └── leaderboard.txt
│
├── CMakeLists.txt
└── README.md

⚙️ Build Instructions
1️⃣ Clone the repository
git clone https://github.com/Shlok-Solanki/Python_DSA.git
cd battle_of_wits
2️⃣ Configure the project

Debug build:

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

Release build:

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
3️⃣ Build

For single-config generators:

cmake --build build

For multi-config generators (MSVC):

cmake --build build --config Debug
cmake --build build --config Release
▶️ Run the Program
Windows (MSVC)
build\Debug\battle_of_wits.exe

or

build\Release\battle_of_wits.exe
Linux / MinGW / Ninja
build/battle_of_wits
🧩 Game Architecture
Main

Handles the program entry point and optionally displays an SFML start screen before starting the game.

Game

Controls the overall gameplay flow including:

Player state

Genre selection

Question fetching

Score updates

API Client

Fetches trivia questions from OpenTDB API and parses them into structured Question objects.

Domain Models

Genre

Handles quiz categories.

Question

Stores:

Question text

Options

Correct answer

Hint

Player

Stores player name and score.

Lifeline

Implements:

50-50 elimination

Audience poll

Leaderboard

Stores and retrieves player scores.

Utils

Utility functions like:

HTML decoding

Fisher–Yates option shuffling

These modules together form the core structure of the project. 

WARP

🏆 Leaderboard

Scores are stored locally in:

data/leaderboard.txt

Make sure the data folder exists before running the game if you want scores to persist.

🔌 Dependencies

The project automatically downloads dependencies using CMake FetchContent:

nlohmann/json

cpp-httplib

Optional dependencies:

SFML → GUI start screen

OpenSSL → HTTPS support for API requests

🚀 Future Improvements

Multiplayer mode

Difficulty levels

Timer system

Full GUI interface

Online leaderboard

Unit tests

👨‍💻 Author

Shlok Solanki
B.Tech CSE — Noida Institute of Engineering & Technology
Interested in Cloud Computing, Distributed Systems, and Backend Development
