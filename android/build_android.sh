#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
RAYLIB_VERSION="5.5"
API="29"
PKG="com.dhaka2099.urbanwarfare"
APP_NAME="Dhaka 2099"
VERSION_NAME="1.0.0"
VERSION_CODE="1"
ANDROID_HOME="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
NDK_VERSION="${ANDROID_NDK_VERSION:-25.2.9519653}"
NDK="${ANDROID_HOME}/ndk/${NDK_VERSION}"
BUILD_TOOLS="${ANDROID_BUILD_TOOLS:-35.0.0}"
PLATFORM_DIR="${ANDROID_HOME}/platforms/android-${API}"
BT="${ANDROID_HOME}/build-tools/${BUILD_TOOLS}"

if [[ -z "$ANDROID_HOME" ]]; then echo "ANDROID_HOME/ANDROID_SDK_ROOT is not set"; exit 1; fi
if [[ ! -d "$NDK" ]]; then echo "NDK not found: $NDK"; exit 1; fi
if [[ ! -d "$PLATFORM_DIR" ]]; then echo "Android platform not found: $PLATFORM_DIR"; exit 1; fi
if [[ ! -x "$BT/aapt" ]]; then echo "aapt not found: $BT/aapt"; exit 1; fi

WORK="$ROOT_DIR/android/.build"
RAYLIB="$WORK/raylib"
rm -rf "$WORK"
mkdir -p "$WORK" "$ROOT_DIR/android/output"

if [[ ! -d "$RAYLIB" ]]; then
  git clone --depth 1 --branch "$RAYLIB_VERSION" https://github.com/raysan5/raylib.git "$RAYLIB"
fi

# Build raylib once for each ABI using the official raylib Android backend.
build_raylib() {
  local arch="$1"
  local out="$WORK/raylib-$arch"
  rm -rf "$out"
  mkdir -p "$out"
  make -C "$RAYLIB/src" clean >/dev/null 2>&1 || true
  make -C "$RAYLIB/src" \
    PLATFORM=PLATFORM_ANDROID \
    ANDROID_ARCH="$arch" \
    ANDROID_API_VERSION="$API" \
    ANDROID_NDK="$NDK" \
    RAYLIB_LIBTYPE=STATIC \
    RAYLIB_RELEASE_PATH="$out" \
    -j2
  cp "$out/libraylib.a" "$out/libraylib.a"
}

build_app_abi() {
  local arch="$1"
  local abi="$2"
  local compiler="$3"
  local out="$WORK/app-$arch"
  local rlib="$WORK/raylib-$arch/libraylib.a"
  local sysroot="$NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
  local glue="$NDK/sources/android/native_app_glue"
  rm -rf "$out"
  mkdir -p "$out/obj" "$out/lib/$abi"

  local cflags=( -std=gnu11 -O3 -fPIC -ffunction-sections -funwind-tables -fstack-protector-strong
    -D__ANDROID__ -DPLATFORM_ANDROID -D__ANDROID_API__="$API"
    -I"$ROOT_DIR/src" -I"$RAYLIB/src" -I"$glue" )
  if [[ "$arch" == "arm" ]]; then cflags+=( -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 ); else cflags+=( -mfix-cortex-a53-835769 ); fi

  local sources=(main.c ui.c hud.c game.c city.c world.c chars.c audio.c save.c)
  for src in "${sources[@]}"; do
    "$compiler" "${cflags[@]}" --sysroot="$sysroot" -c "$ROOT_DIR/src/$src" -o "$out/obj/${src%.c}.o"
  done

  "$compiler" -shared -fPIC -Wl,-soname,libmain.so \
    -Wl,-u,ANativeActivity_onCreate -Wl,--no-undefined \
    -Wl,--wrap=fopen -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now \
    -L"$WORK/raylib-$arch" \
    -o "$out/lib/$abi/libmain.so" \
    "$out/obj/"*.o "$rlib" \
    -llog -landroid -lEGL -lGLESv2 -lOpenSLES -ldl -lm -lc
}

build_raylib arm64
build_raylib arm

TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"
build_app_abi arm64 arm64-v8a "$TOOLCHAIN/aarch64-linux-android${API}-clang"
build_app_abi arm armeabi-v7a "$TOOLCHAIN/armv7a-linux-androideabi${API}-clang"

PKGDIR="$WORK/apk"
rm -rf "$PKGDIR"
mkdir -p "$PKGDIR/res/drawable-ldpi" "$PKGDIR/res/drawable-mdpi" "$PKGDIR/res/drawable-hdpi" "$PKGDIR/res/values" "$PKGDIR/assets" "$PKGDIR/lib/arm64-v8a" "$PKGDIR/lib/armeabi-v7a" "$PKGDIR/bin" "$PKGDIR/src"
cp "$ROOT_DIR/android/icon/icon-36.png" "$PKGDIR/res/drawable-ldpi/icon.png"
cp "$ROOT_DIR/android/icon/icon-48.png" "$PKGDIR/res/drawable-mdpi/icon.png"
cp "$ROOT_DIR/android/icon/icon-72.png" "$PKGDIR/res/drawable-hdpi/icon.png"
printf '<resources><string name="app_name">%s</string></resources>\n' "$APP_NAME" > "$PKGDIR/res/values/strings.xml"

cat > "$PKGDIR/AndroidManifest.xml" <<MANIFEST
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="$PKG"
    android:versionCode="$VERSION_CODE"
    android:versionName="$VERSION_NAME">
    <uses-sdk android:minSdkVersion="29" android:targetSdkVersion="29" />
    <uses-feature android:glEsVersion="0x00020000" android:required="true" />
    <application android:allowBackup="false" android:label="@string/app_name" android:icon="@drawable/icon" android:hasCode="true">
        <activity android:name="com.dhaka2099.urbanwarfare.NativeLoader"
            android:theme="@android:style/Theme.NoTitleBar.Fullscreen"
            android:configChanges="orientation|keyboard|keyboardHidden|screenSize"
            android:screenOrientation="landscape"
            android:launchMode="singleTask"
            android:exported="true">
            <meta-data android:name="android.app.lib_name" android:value="main" />
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
MANIFEST

mkdir -p "$PKGDIR/src/com/dhaka2099/urbanwarfare"
cat > "$PKGDIR/src/com/dhaka2099/urbanwarfare/NativeLoader.java" <<'JAVA'
package com.dhaka2099.urbanwarfare;

public class NativeLoader extends android.app.NativeActivity {
    static {
        System.loadLibrary("main");
    }
}
JAVA

"$BT/aapt" package -f -m -S "$PKGDIR/res" -J "$PKGDIR/src" -M "$PKGDIR/AndroidManifest.xml" -I "$PLATFORM_DIR/android.jar"
"$JAVA_HOME/bin/javac" -source 8 -target 8 -d "$PKGDIR/bin" -cp "$PLATFORM_DIR/android.jar" "$PKGDIR/src/com/dhaka2099/urbanwarfare/R.java" "$PKGDIR/src/com/dhaka2099/urbanwarfare/NativeLoader.java"
"$(command -v d8)" --release --output "$PKGDIR/bin" --lib "$PLATFORM_DIR/android.jar" "$PKGDIR/bin/com/dhaka2099/urbanwarfare/R.class" "$PKGDIR/bin/com/dhaka2099/urbanwarfare/NativeLoader.class"

cp "$WORK/app-arm64/lib/arm64-v8a/libmain.so" "$PKGDIR/lib/arm64-v8a/"
cp "$WORK/app-arm/lib/armeabi-v7a/libmain.so" "$PKGDIR/lib/armeabi-v7a/"

UNALIGNED="$WORK/Dhaka2099-unaligned.apk"
ALIGNED="$WORK/Dhaka2099-aligned.apk"
UNSIGNED="$WORK/Dhaka2099-unsigned.apk"
SIGNED="$ROOT_DIR/android/output/Dhaka2099-v${VERSION_NAME}.apk"

"$BT/aapt" package -f -M "$PKGDIR/AndroidManifest.xml" -S "$PKGDIR/res" -A "$PKGDIR/assets" -I "$PLATFORM_DIR/android.jar" -F "$UNSIGNED" "$PKGDIR/bin"
(cd "$PKGDIR" && "$BT/aapt" add "$UNSIGNED" lib/arm64-v8a/libmain.so lib/armeabi-v7a/libmain.so)
"$BT/zipalign" -p -f 4 "$UNSIGNED" "$ALIGNED"

KEYSTORE="$WORK/dhaka2099-test.keystore"
KEYPASS="android"
if [[ -n "${ANDROID_KEYSTORE_B64:-}" ]]; then
  echo "$ANDROID_KEYSTORE_B64" | base64 --decode > "$KEYSTORE"
  KEYPASS="${ANDROID_KEY_PASSWORD:?ANDROID_KEY_PASSWORD secret required with ANDROID_KEYSTORE_B64}"
  ALIAS="${ANDROID_KEY_ALIAS:?ANDROID_KEY_ALIAS secret required with ANDROID_KEYSTORE_B64}"
else
  "$JAVA_HOME/bin/keytool" -genkeypair -noprompt -storetype PKCS12 -keystore "$KEYSTORE" \
    -storepass "$KEYPASS" -keypass "$KEYPASS" -alias dhaka2099 \
    -keyalg RSA -keysize 2048 -validity 10000 \
    -dname "CN=Dhaka 2099, O=Dhaka 2099, C=BD"
  ALIAS="dhaka2099"
fi

"$BT/apksigner" sign --ks "$KEYSTORE" --ks-pass "pass:$KEYPASS" --key-pass "pass:$KEYPASS" --ks-key-alias "$ALIAS" --out "$SIGNED" "$ALIGNED"
"$BT/apksigner" verify --verbose "$SIGNED"

echo "APK_READY=$SIGNED"
