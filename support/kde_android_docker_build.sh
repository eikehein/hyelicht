#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>

set -e

# Assumes the container was started with e.g.:
# docker run -ti --rm -v $HOME/devel/android/apks:/output:Z \
# -v $HOME/devel/android/src:/home/user/src:Z \
# -v $HOME/devel/android/build:/home/user/build:Z kdeorg/android-sdk \
# ./src/hyelicht/support/kde_android_docker_build.sh

# Make sure we're in $HOME. KDE's scripts assume so.
cd /home/user

# We only want an arm64-v8a build.
export ONLY_ARM64=1

# Fetch additional files needed by KDE's scripts:
git clone --depth 1 kde:sysadmin/ci-tooling || true

# Build our dependencies:
BUILDOPTS="-DBUILD_TESTING=OFF"

build-kde-project extra-cmake-modules Frameworks $BUILDOPTS
build-kde-project ki18n Frameworks $BUILDOPTS
build-kde-project kcoreaddons Frameworks $BUILDOPTS
build-kde-project kconfig Frameworks $BUILDOPTS

# `kde:kcoreaddons` here is just to fill this expected argument
# with a placeholder. At this point we've already mounted a
# `hyelicht` source folder into the container, so the script
# won't clone anything.
TARGET="hyelicht"
APK_ARGS=$(python /opt/helpers/get-apk-args.py /home/user/src/$TARGET)
OUTDIR="/output"

build-cmake $TARGET kde:kcoreaddons $APK_ARGS -DANDROID_APK_OUTPUT_DIR=$OUTDIR $BUILDOPTS

create-apk $TARGET
