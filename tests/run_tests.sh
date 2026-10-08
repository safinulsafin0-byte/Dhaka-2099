#!/bin/sh
# Headless logic test (no GPU, no raylib needed): worldgen, all mission/round flow, C4/cache, AI, balance.
# Rendering/audio/input are stubbed, so this checks game LOGIC and memory safety, not visuals.
cd "$(dirname "$0")" && mkdir -p data && gcc -std=gnu99 -g -O1 -fsanitize=address,undefined -Wno-misleading-indentation \
  -Istub -I../src harness.c stub_impl.c ../src/chars.c ../src/audio.c ../src/save.c ../src/world.c ../src/city.c ../src/ui.c ../src/hud.c -o harness -lm && ./harness
