#!/bin/bash
# ========== SWAP SETUP ==========
set -euo pipefail

SWAPDIR="/swap_partition"
SWAPFILE="$SWAPDIR/swapfile"
SIZE_GB=1

if swapon --show | grep -q "$SWAPFILE"; then
  echo "Swap already enabled at $SWAPFILE"
  exit 0
fi

echo "Creating swap directory at $SWAPDIR..."
sudo mkdir -p "$SWAPDIR"
sudo chmod 700 "$SWAPDIR"

echo "Allocating ${SIZE_GB}G swap file..."
sudo fallocate -l ${SIZE_GB}G "$SWAPFILE" || sudo dd if=/dev/zero of="$SWAPFILE" bs=1G count=$SIZE_GB
sudo chmod 600 "$SWAPFILE"

sudo mkswap "$SWAPFILE"
sudo swapon "$SWAPFILE"

if ! grep -q "$SWAPFILE" /etc/fstab; then
  echo "$SWAPFILE none swap sw 0 0" | sudo tee -a /etc/fstab
fi

echo "Setting swappiness to 20..."
sudo sysctl vm.swappiness=20
echo "vm.swappiness=20" | sudo tee /etc/sysctl.d/99-swappiness.conf

echo ""
echo "Data disk swap setup complete!"
swapon --show
free -h

# ============================================

set -e

BASE=~/libs
SRC=$BASE/src
BIN=$BASE/bin
LIB=$BASE/lib
INCLUDE=$BASE/include
mkdir -p "$SRC" "$BIN" "$LIB" "$INCLUDE"

echo "[*] Installing system dependencies..."
sudo apt update && sudo apt install -y \
  build-essential cmake git wget curl unzip \
  libssl-dev libgmp-dev libboost-all-dev \
  libzstd-dev libz-dev libreadline-dev cargo python3-pip python3-venv

# ========== Install EMP Toolkit ==========
echo "[*] Installing emp-tool..."
cd "$SRC"
git clone https://github.com/emp-toolkit/emp-tool.git || true
cd emp-tool
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$BASE ..
make -j && make install

cd "$SRC"
git clone https://github.com/emp-toolkit/emp-ot.git || true
cd emp-ot
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$BASE -DCMAKE_PREFIX_PATH=$BASE ..
make -j && make install

cd "$SRC"
git clone https://github.com/emp-toolkit/emp-zk.git || true
cd emp-zk
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$BASE -DCMAKE_PREFIX_PATH=$BASE ..
make -j && make install

# ========== Install NTL ==========
echo "[*] Installing NTL..."
cd "$SRC"
wget https://www.shoup.net/ntl/ntl-11.5.1.tar.gz
 tar xzf ntl-11.5.1.tar.gz
cd ntl-11.5.1/src
./configure PREFIX=$BASE
make -j && make install


# ========== Setup Python packages ==========
echo "[*] Installing Python requirements..."
pip3 install -r "$BASE/requirements.txt" 2>/dev/null || true




# COMMENT THE FOLLOWING BLOCK IF NOT RUNNING THE ENTIRE PROCEDURE

# ------------------------------------------------------
      # ========== Install ABC ==========
      echo "[*] Installing ABC..."
      cd "$SRC"
      git clone https://github.com/berkeley-abc/abc.git || true
      cd abc
      make -j
      cp abc "$BIN/"

      # # ========== Install AIGER ==========
      echo "[*] Installing AIGER..."
      cd "$SRC"
      git clone https://github.com/arminbiere/aiger.git || true
      cd aiger
      ./configure.sh
      make -j
      find . -maxdepth 1 -type f -executable -name "aig*" -exec cp {} "$BIN/" \;

      # ========== Install DepQBF v1.0 ==========
      echo "[*] Installing DepQBF v1.0..."
      cd "$SRC"
      if [ ! -f version-1.0.tar.gz ]; then
      wget https://github.com/lonsing/depqbf/archive/version-1.0.tar.gz
      else
      echo "[*] version-1.0.tar.gz already exists, skipping download"
      fi
      tar -xzf version-1.0.tar.gz
      cd depqbf-version-1.0
      make -j
      cp depqbf "$BIN/"
      cd "$SRC"

      # ========== Install QRPcheck v1.0.3 ==========
      echo "[*] Installing QRPcheck v1.0.3..."
      cd "$SRC"
      if [ ! -f qrpcheck-1.0.3.tar.gz ]; then
	      wget https://fmv.jku.at/qrpcheck/qrpcheck-1.0.3.tar.gz
      else
	      echo "[*] qrpcheck-1.0.3.tar.gz already exists, skipping download"
      fi
      tar -xvzf qrpcheck-1.0.3.tar.gz
      cd qrpcheck-1.0.3
      mkdir libs
      cd libs
      wget -c http://fmv.jku.at/picosat/picosat-936.tar.gz
      tar xvf picosat-936.tar.gz
      mv picosat-936 picosat
      cd picosat
      ./configure
      make          # make PicoSAT
      cd ../../     # change back to $QRPcheckROOTDIR
      make -j
      mkdir -p "$BIN"
      cp qrpcheck "$BIN/"
      echo "[*] QRPcheck installed to $BIN/qrpcheck"

      cd "$SRC"
      # ========== Install PicoSAT v965 ==========
      wget https://fmv.jku.at/picosat/picosat-965.tar.gz
      tar -xvzf picosat-965.tar.gz
      mv picosat-965 picosat
      cd picosat
      ./configure.sh --trace
      make -j
      cp picosat "$BIN/"
      cd ../../

      # ========== Install CAQE v2 ==========
      echo "[*] Installing CAQE v2..."
      cd "$SRC"
      wget --no-check-certificate https://finkbeiner.groups.cispa.de/tools/caqe/downloads/caqe-2.tar.gz -O caqe-2.tar.gz
      tar -xzf caqe-2.tar.gz
      cd caqe-2
      chmod +x configure
      ./configure
      make -j
      cp caqe bcaqe certcheck "$BIN/" 2>/dev/null || true

# ------------------------------------------------------

# ========== Final Env Setup ==========
echo "[*] Setting environment variables..."
PROFILE=~/.bashrc
if ! grep -q "# ZKQBF ENV" "$PROFILE"; then
  echo "# ZKQBF ENV" >> "$PROFILE"
  echo "export PATH=\"$BIN:\$PATH\"" >> "$PROFILE"
  echo "export LD_LIBRARY_PATH=\"$LIB:\$LD_LIBRARY_PATH\"" >> "$PROFILE"
fi
source "$PROFILE"

echo "[*] Environment setup complete. Tools installed in $BASE/bin"
ls -lh "$BIN"

# ========== Clone zkqbf-suite ==========
echo "[*] Cloning zkqbf-suite..."
cd ~
git clone https://github.com/PP-FM/zkqbf-suite.git || true
cd zkqbf-suite
./prepare_for_aws.sh
cd src/zkqres
cmake .
make
mkdir -p data
cd ../zkqcube
cmake .
make
mkdir -p data
cd ../zkws-herbrand 
cmake .
make
mkdir -p data
cd ../zkws-skolem
cmake .
make
mkdir -p data
cd 
echo "[*] zkqbf-suite setup complete."