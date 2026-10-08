# DHAKA 2099: URBAN WARFARE

A third-person urban combat + open-world game set across a 2.4 km x 2.4 km procedural Dhaka (12 x 12 blocks, 144 city blocks), written in C with raylib.
No external assets: every model, texture and sound is generated in code.

## What's in the game
- **Living city (GTA-style)**: 60-100 citizens walk the pavements (men in panjabi/lungi/taqiyah, women in saree/salwar/headscarf), vendors stand at tea stalls, fruit carts, fuchka carts and newsstands, and they **panic and run** when shots or explosions happen nearby. Shooting or running over citizens costs score and stars.
- **Moving traffic** on every road: rickshaws, CNG autos, sedans, buses; they keep to the left lane, queue behind each other, stop for pedestrians and obey **traffic signals** at every junction.
- **Drive anything**: press **F** next to any moving vehicle to hijack it (the driver flees) or any parked car to take it. Handbrake, collisions, damage, smoke, explosions, run-overs, third-person chase camera. Speed limits per vehicle (bus 19 m/s, sedan 30 m/s).
- **FREE ROAM** mode from the main menu: no objectives, just the whole city, traffic and weapons. Dying respawns you ("WASTED").
- **Faster movement**: walk 7 m/s, sprint about 12 m/s (more with fast operators).
- **City details**: mosques with domes and minarets, shop awnings, rooftop billboards, stalls, rickshaw stands, a cricket stadium, Tejgaon industrial zone, Hatirjheel lake, signals and lamps everywhere.
- **Accounts**: first launch asks for name, username, password (+confirm) and a password hint. Every later launch is a sign-in.
  Forgot it? Tap *Forgot password* to see the hint (also shown automatically after 3 wrong tries; 10 s lock after 5).
  Passwords are stored salted + stretched-hashed, never in plain text. *Reset profile* erases the save.
- **30 missions in 3 chapters.** Every mission = reach the enemy base -> clear **2-4 rounds** of mercenaries (**10 to 24 per mission**, reinforcements storm the gates between rounds) -> destroy the weapons cache with C4 or grenades. Stars for speed and low damage.
- **10 playable operators**, unlocked by clearing missions (0, 2, 4, 7, 10, 13, 16, 20, 24, 28). Every 10 missions cleared, **all operators rank up** (Rank 1-4: more health and damage, gold insignia on the model).
- **14 weapons**: Glock 17, Desert Eagle, MP5, UZI, FN P90, AK-47, M4A1, FN SCAR-H, Remington 870, SPAS-12, SVD Dragunov, AWM, PKM and the M79 grenade launcher (impact explosive). Pick up weapons from the street and from fallen enemies; your old weapon drops so you can swap back.
- **More pickups**: body armor, adrenaline (speed + damage resistance), intel briefcases (+300), plus health, ammo, grenades, C4.
- **Detection system**: enemies fill a **?** gauge when they see you (faster at close range, sprinting or driving; slower when standing still), then flip to **!** and attack. A top-centre HIDDEN / SUSPICIOUS / DETECTED meter, off-screen arrows, minimap facing lines, and **stealth kills** (+100). **Grenades** (bounce, 2.4 s fuse) and **C4** (plant, remote detonate), both pick-up-able; **explosive barrels** chain-react; blast damage falls off with distance and cover.
- **Enemies**: riflemen, SMG rushers, shotgunners, snipers (tower-mounted at the base), heavy gunners, commanders. They see, hear gunfire, use cover geometry, flank via a flow-field pathfinder, strafe, and throw grenades from mission 7.
- **City**: Mirpur, Uttara, Gulshan, Dhanmondi (+ lake), Farmgate, Motijheel, Old Dhaka (rickshaws, CNGs, buses, tea stalls), Ramna Park, Shahbag (Shaheed Minar), Sher-e-Bangla Nagar (Parliament), Lalbagh Fort, Ahsan Manzil, Sadarghat docks and the Buriganga. The enemy base moves to a different district every mission.
- **HUD**: live rotating minimap with district name (top-left), gold objective arrow + distance (top-centre), world markers, full-city map (M / tap minimap), round/hostile counter, hit markers, damage direction, grenade warning, kill feed.
- **Menus**: splash, register/login, main menu with profile, mission select, briefing, operator select (3D preview), settings (volume, sensitivity, FOV, draw distance, difficulty, invert Y, aim assist, shadows, FPS, touch controls, fullscreen), how-to-play, credits, pause, results.

## Controls
| | Keyboard + mouse | Touch |
|---|---|---|
| Move / look | WASD / mouse | left thumb / drag right side |
| Fire / aim | LMB / RMB | FIRE / AIM buttons |
| Sprint | Shift | push stick to the edge |
| Reload / swap | R / Q or wheel | RLD / SWAP |
| Grenade / C4 / detonate | G / B / V | GREN / C4 / BOOM |
| Jump / vehicle handbrake | Space | JUMP (BRAKE in vehicle) |
| Enter / exit vehicle | F | USE |
| Map / pause | M / P or Esc | tap minimap / II |

## Build (Windows, same toolchain as before)
```bat
build.bat            :: debug build, runs it
build.bat release    :: optimised, no console window
```
Uses `RAYLIB_DIR` (default `C:\raylib\raylib`) and `W64_DIR` (default `C:\raylib\w64devkit`). Needs raylib 5.x (4.5+ should work).

## Phones and tablets
- **Fastest route - browser build**: `build_web.bat` (needs emsdk + a raylib web library, see the comments inside). Serve `web\` over HTTPS and "Add to Home Screen". Profile and progress persist through IndexedDB.
- **Native Android**: drop `src/*.c` into raylib's Android template and define `PLATFORM_ANDROID`; touch controls then default to ON. Mind where the template lets you write files (`SAVE_FILE` in `src/save.c`).
- Touch mode also works on a desktop with a mouse (Settings -> Touch Controls) which is how to test the layout.
- UI scales to any resolution/aspect (720-unit virtual height), and an on-screen keyboard appears for the register/login forms.

## Project layout
```
src/game.h      shared types / prototypes     src/world.c   city, base, collision, rays, nav, rendering
src/game.c      missions, rounds, AI, weapons  src/city.c    citizens, traffic, signals, driving
src/chars.c   operators, weapon table, humanoid renderer
src/ui.c        widgets, auth, menus, input    src/hud.c     HUD, minimap, touch controls, big map
src/save.c      profile + progress             src/audio.c   synthesised SFX
tests/          headless logic tests (tests/run_tests.sh, Linux/WSL, no GPU needed)
legacy/         the original top-down prototype
```
Balance knobs live at the top of `src/game.c` (`MissionEnemyTotal`, `MissionRounds`, `DiffDmg/DiffHp/DiffAcc`, enemy tables in `SpawnEnemy`) and in `CHARS[]` / `WEAPONS[]` in `src/chars.c`.

## Update: speed, accidents, crime
- On-foot speed is 3x (FOOT_SPEED_MUL in src/game.c).
- Accidents: crash damage to the driver, being hit by moving vehicles on foot, hard-landing fall damage, limping below 25% HP. Vehicles are solid for pedestrians.
- Crime: killing/running over a civilian gives a wanted star (max 5). Police cars chase you, ram your vehicle, and officers get out and shoot. Hide out of sight to lose the stars.

## Police system update (GTA-style)
- **Siren**: looping wail + yelp, audible from ~200 m so you hear cruisers coming; louder as they close in. Radio squelch on dispatch / arrival.
- **Cruisers**: black-and-white livery, push bar, double-flash red/blue light bar, front/rear strobes, flashing light pool on the road.
- **Officers**: dark navy uniform, black vest, cap and shades.
- **HUD**: gold wanted stars (blink while you're evading), pursuit/evading status, heat bar, red/blue screen-edge flash when a cruiser is near, flashing police blips on the minimap.

## More medkits
- 40 medkits (+55 HP) now spawn across the map (was 18), plus 10 large **first-aid kits** (white case, full heal) and extra ones at the enemy base.
- Enemy drops give medkits more often, and much more often the lower your health is (low HP can drop a first-aid kit).
