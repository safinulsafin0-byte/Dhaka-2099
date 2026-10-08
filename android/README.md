# Dhaka 2099 — Android APK build

This Android layer does **not** change gameplay or game features. It packages the existing C/raylib game for Android and builds an installable APK through GitHub Actions.

- Package: `com.dhaka2099.urbanwarfare`
- Orientation: landscape
- ABIs: `arm64-v8a` + `armeabi-v7a`
- Raylib: 5.5
- APK build: GitHub Actions
- Signing: temporary CI keystore by default; use repository secrets for a permanent release keystore later.

The current game already has Android touch input and `PLATFORM_ANDROID` code paths. Save data remains handled by the existing `data/save.dat` path; raylib's Android file wrapper is enabled at the final native link so writable app storage is used for file I/O.
