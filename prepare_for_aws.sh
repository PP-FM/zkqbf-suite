#!/usr/bin/env bash
set -euo pipefail

ROOT="./src"
timestamp="$(date +%Y%m%d-%H%M%S)"

# Iterate immediate subdirectories of ./src
find "$ROOT" -mindepth 1 -maxdepth 1 -type d -print0 |
while IFS= read -r -d '' dir; do
  cmake_file="$dir/CMakeLists.txt"

  if [[ -f "$cmake_file" ]]; then
    cp -f -- "$cmake_file" "$cmake_file.bak-$timestamp"
    echo "Backed up $cmake_file -> $cmake_file.bak-$timestamp"
  else
    echo "Creating $cmake_file"
  fi

  # Write the required contents (quoted heredoc prevents $ expansion)
  cat > "$cmake_file" <<'EOF'
cmake_minimum_required(VERSION 3.10)
project(emp_private_refutation)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1 -mrdseed -maes -mpclmul -O3 -pthread -w")
set(LIBS_DIR "$ENV{HOME}/libs")
include_directories(${LIBS_DIR}/include)
link_directories(${LIBS_DIR}/lib)
add_executable(test main.cpp polynomial.cpp clause.cpp clauseRAM.cpp)
target_link_libraries(test PUBLIC emp-tool emp-zk gmp ssl crypto ntl gmp)
EOF

  echo "Wrote $cmake_file"
done

echo "Done."

if [ ! -d ".venv" ]; then
  echo "Creating virtual environment..."
  python3 -m venv .venv
else
  echo ".venv already exists, skipping creation."
fi
source .venv/bin/activate
pip install python-sat