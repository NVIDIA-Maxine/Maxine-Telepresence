# Copyright 2020-2023 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

project(Applications)

add_subdirectory(MagicMirrorLocalApplication)
add_subdirectory(LocalVideoConferencingApplication)
add_subdirectory(PlaybackApplication)

install(DIRECTORY . DESTINATION bin FILES_MATCHING
    PATTERN "MagicMirrorLocalApplication/*" EXCLUDE
    PATTERN "LocalVideoConferencingApplication/*" EXCLUDE
    PATTERN "PlaybackApplication/*" EXCLUDE
    PATTERN "*.json")

install(DIRECTORY . DESTINATION bin FILES_MATCHING
    PATTERN "MagicMirrorLocalApplication/*" EXCLUDE
    PATTERN "LocalVideoConferencingApplication/*" EXCLUDE
    PATTERN "PlaybackApplication/*" EXCLUDE
    PATTERN "*.bat")
