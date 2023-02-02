/*
 * Copyright (C) 2020-2022 Tilt Five, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/// \file
/// \brief Sample code using the Tilt Five™ C API

/// \privatesection

#ifdef _WIN32
#include <windows.h>
#define SLEEP(ms) Sleep(ms)
#elif __unix
//  For linux, OSX, and other unixes
#define _POSIX_C_SOURCE 199309L  // or greater
#include <time.h>
#define SLEEP(ms)                           \
    do {                                    \
        struct timespec ts;                 \
        ts.tv_sec  = (ms) / 1000;           \
        ts.tv_nsec = (ms) % 1000 * 1000000; \
        nanosleep(&ts, NULL);               \
    } while (0)
#else
#warning Unsupported platform - sleep unavailable, code may run too fast
#define SLEEP(ms) \
    {}
#endif

#include "include/TiltFiveNative.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IDENTIFIER_BUFFER_SIZE 1024
#define GLASSES_BUFFER_SIZE    1024
#define PARAM_BUFFER_SIZE      1024
#define WAND_BUFFER_SIZE       4

bool checkConnection(T5_Glasses glassesHandle, const char* id) {
    T5_ConnectionState connectionState;
    T5_Result err = t5GetGlassesConnectionState(glassesHandle, &connectionState);
    if (err) {
        printf("Glasses connection : '%s' -> Error: %s\n", id, t5GetResultMessage(err));
        return false;
    }

    switch (connectionState) {
        case kT5_ConnectionState_ExclusiveConnection:
            printf("Glasses connection : '%s' -> Connected\n", id);
            return true;

        case kT5_ConnectionState_ExclusiveReservation:
            printf("Glasses connection : '%s' -> Reserved\n", id);
            return true;

        case kT5_ConnectionState_NotExclusivelyConnected:
            printf("Glasses connection : '%s' -> Not Connected\n", id);
            return true;

        case kT5_ConnectionState_Disconnected:
            printf("Glasses connection : '%s' -> Device Lost\n", id);
            return true;

        default:
            printf("Glasses connection : '%s' -> Unexpected state!\n", id);
            return false;
    }
}

void findWands(T5_Glasses glassesHandle) {
    uint8_t count = WAND_BUFFER_SIZE;
    T5_WandHandle wandListBuffer[WAND_BUFFER_SIZE];

    T5_Result err = t5ListWandsForGlasses(glassesHandle, wandListBuffer, &count);
    if (err) {
        printf("Wand list      : Error : %s\n", t5GetResultMessage(err));
        return;
    }

    printf("Wand list      : %u wand(s)\n", count);
    for (int i = 0; i < count; i++) {
        printf(" Wand found    : ID=%d\n", wandListBuffer[i]);
    }

    /// [ReadWand]
    if (count > 0) {
        // Enable wand stream
        T5_WandStreamConfig config;
        config.enabled = true;
        err            = t5ConfigureWandStreamForGlasses(glassesHandle, &config);
        if (err) {
            printf("Failed to enable stream : %s\n", t5GetResultMessage(err));
            return;
        }

        T5_WandStreamEvent event;
        for (int i = 0; i < 100; i++) {
            err = t5ReadWandStreamForGlasses(glassesHandle, &event, 100);
            if (err) {
                if (err == T5_TIMEOUT) {
                    continue;
                }

                printf("Failed to read stream : %s\n", t5GetResultMessage(err));
                return;
            }

            switch (event.type) {
                case kT5_WandStreamEventType_Connect:
                    printf("WAND EVENT : CONNECT [%u]\n", event.wandId);
                    break;

                case kT5_WandStreamEventType_Disconnect:
                    printf("WAND EVENT : DISCONNECT [%u]\n", event.wandId);
                    break;

                case kT5_WandStreamEventType_Desync:
                    printf("WAND EVENT : DESYNC [%u]\n", event.wandId);
                    break;

                case kT5_WandStreamEventType_Report:
                    printf("WAND EVENT : REPORT [%u] %fx%f\n",
                           event.wandId,
                           event.report.stick.x,
                           event.report.stick.y);
                    break;
            }
        }

        // Disable wand stream
        config.enabled = false;
        err            = t5ConfigureWandStreamForGlasses(glassesHandle, &config);
        if (err) {
            printf("Failed to disable stream : %s\n", t5GetResultMessage(err));
            return;
        }
    }
    /// [ReadWand]
}

const char* gameboardTypeString(T5_GameboardType gameboardType) {
    switch (gameboardType) {
        case kT5_GameboardType_None:
            return "None";
        case kT5_GameboardType_LE:
            return "LE";
        case kT5_GameboardType_XE:
            return "XE";
        case kT5_GameboardType_XE_Raised:
            return "XE Raised";
        default:
            return "<Unknown>";
    }
}

/// [ExclusiveOps]
void getPoses(T5_Glasses glassesHandle) {
    T5_Result err;
    T5_GlassesPose pose;

    printf("Getting some poses");

    bool waiting = false;
    for (int i = 0; i < 1000; i++) {
        err = t5GetGlassesPose(glassesHandle, kT5_GlassesPoseUsage_GlassesPresentation, &pose);
        if (err) {
            if (err == T5_ERROR_TRY_AGAIN) {
                if (!waiting) {
                    printf("\nWaiting...");
                    fflush(stdout);
                } else {
                    printf(".");
                    fflush(stdout);
                }
                SLEEP(100);
                i--;
                waiting = true;
                continue;
            } else {
                printf("Failed to get pose : %s\n", t5GetResultMessage(err));
                return;
            }
        }

        waiting = false;

        printf("\nGLASSES POSE : [%lu] (Board:%s) "
               "[%6.4f, %6.4f, %6.4f] [%6.4f, %6.4f, %6.4f, %6.4f]",
               pose.timestampNanos,
               gameboardTypeString(pose.gameboardType),
               pose.posGLS_GBD.x,
               pose.posGLS_GBD.y,
               pose.posGLS_GBD.z,
               pose.rotToGLS_GBD.w,
               pose.rotToGLS_GBD.x,
               pose.rotToGLS_GBD.y,
               pose.rotToGLS_GBD.z);

        SLEEP(16);
    }

    printf("\n");
}
/// [ExclusiveOps]

/// [NonExclusiveOps]
T5_Result endlesslyWatchSettings(T5_Glasses glassesHandle, const char* id) {
    T5_Result err;

    printf("Watching for changes to settings... (forever)\n");
    for (;;) {
        uint16_t count = PARAM_BUFFER_SIZE;
        T5_ParamGlasses paramBuffer[PARAM_BUFFER_SIZE];
        err = t5GetChangedGlassesParams(glassesHandle, paramBuffer, &count);
        if (err) {
            printf("Error getting changed params for '%s' : %s\n", id, t5GetResultMessage(err));
            return err;
        }

        // Get the values of the changed params
        for (int i = 0; i < count; i++) {
            switch (paramBuffer[i]) {
                case kT5_ParamGlasses_Float_IPD: {
                    double value = 0.0;
                    err          = t5GetGlassesFloatParam(glassesHandle, 0, paramBuffer[i], &value);
                    if (err) {
                        printf("Error getting changed IPD for '%s' : %s\n",
                               id,
                               t5GetResultMessage(err));
                        return err;
                    }

                    printf("IPD changed for '%s' : %f\n", id, value);
                } break;

                case kT5_ParamGlasses_UTF8_FriendlyName: {
                    char buffer[T5_MAX_STRING_PARAM_LEN];
                    size_t bufferSize = T5_MAX_STRING_PARAM_LEN;
                    err               = t5GetGlassesUtf8Param(
                        glassesHandle, paramBuffer[i], 0, buffer, &bufferSize);
                    if (err) {
                        printf("Error getting changed friendly name for '%s' : %s\n",
                               id,
                               t5GetResultMessage(err));
                        return err;
                    }

                    printf("Friendly name changed for '%s' : '%s'\n", id, buffer);
                } break;

                default:
                    // Ignore other parameters
                    break;
            }
        }
    }
}
/// [NonExclusiveOps]

void connectGlasses(T5_Context t5ctx, const char* id) {
    /// [WaitForGlasses2]

    // CREATE GLASSES
    T5_Glasses glassesHandle;
    T5_Result err = t5CreateGlasses(t5ctx, id, &glassesHandle);
    if (err) {
        printf("Error creating glasses '%s' : %s\n", id, t5GetResultMessage(err));
        return;
    } else {
        printf("Created glasses   : '%s'\n", id);
    }
    /// [WaitForGlasses2]

    // CHECK CONNECTION STATE
    if (!checkConnection(glassesHandle, id)) {
        return;
    }

    /// [Reserve]
    err = t5ReserveGlasses(glassesHandle, "My Awesome Application");
    if (err) {
        printf("Error acquiring glasses '%s': %s\n", id, t5GetResultMessage(err));
        return;
    }
    /// [Reserve]

    printf("Glasses reserved      : '%s'\n", id);

    /// CHECK CONNECTION STATE
    if (!checkConnection(glassesHandle, id)) {
        return;
    }

    /// [EnsureReady]
    // ENSURE GLASSES ARE READY
    for (;;) {
        err = t5EnsureGlassesReady(glassesHandle);
        if (err) {
            if (err == T5_ERROR_TRY_AGAIN) {
                // A non-sample program would probably sleep here or retry on another frame pass.
                continue;
            }
            printf("Error ensure glasses ready '%s': %s\n", id, t5GetResultMessage(err));
            return;
        }

        printf("Glasses ready     : '%s'\n", id);
        break;
    }
    /// [EnsureReady]

    // CHECK CONNECTION STATE
    if (!checkConnection(glassesHandle, id)) {
        return;
    }

    // RECALL THE IDENTIFIER FOR GLASSES
    size_t bufferSize = IDENTIFIER_BUFFER_SIZE;
    char recalledId[IDENTIFIER_BUFFER_SIZE];
    err = t5GetGlassesIdentifier(glassesHandle, recalledId, &bufferSize);
    if (err) {
        printf("Error getting ID for glasses '%s' : %s\n", id, t5GetResultMessage(err));
        return;
    } else if (strcmp(id, recalledId) != 0) {
        printf("Mismatch getting ID for glasses : '%s' -> '%s'\n", id, recalledId);
    } else {
        printf("Got ID for glasses : '%s' -> '%s'\n", id, recalledId);
    }

    // BRIEFLY PERFORM AN EXCLUSIVE OPERATION ON THE GLASSES
    if (true) {
        getPoses(glassesHandle);
    }

    // FIND AND BRIEFLY WATCH CONNECTED WANDS
    if (true) {
        findWands(glassesHandle);
    }

    // CHECK CONNECTION STATE
    if (!checkConnection(glassesHandle, id)) {
        return;
    }

    printf("Glasses ready     : '%s'\n", id);

    // Enable this to watch for changes to the glasses settings
    if (false) {
        err = endlesslyWatchSettings(glassesHandle, id);
        if (err) {
            printf("Error watching settings for glasses '%s' : %s\n", id, t5GetResultMessage(err));
            return;
        }
    }

    /// [Release]
    // RELEASE THE GLASSES
    t5ReleaseGlasses(glassesHandle);
    /// [Release]

    // CHECK CONNECTION STATE
    if (!checkConnection(glassesHandle, id)) {
        return;
    }

    // FIND AND BRIEFLY WATCH CONNECTED WANDS
    // Note that the glasses are not reserved here!
    if (true) {
        findWands(glassesHandle);
    }

    // DESTROY THE GLASSES
    t5DestroyGlasses(&glassesHandle);
    printf("Destroyed glasses : '%s'\n", id);
}

size_t strnlen(const char* str, size_t maxLen) {
    size_t len = 0;
    while ((len < maxLen) && (str[len] != '\0')) {
        len++;
    }
    return len;
}

int main(int argc, char** argv) {
    /// [CreateContext]
    T5_ClientInfo clientInfo = {
        .applicationId      = "com.MyCompany.MyApplication",
        .applicationVersion = "1.0.0",
        .sdkType            = 0,
        .reserved           = 0,
    };

    T5_Context t5ctx;
    T5_Result err = t5CreateContext(&t5ctx, &clientInfo, 0);
    if (err) {
        printf("Failed to create context\n");
    }
    /// [CreateContext]
    printf("Init complete\n");

    {
        /// [SystemWideQuery]
        T5_GameboardSize gameboardSize;
        err = t5GetGameboardSize(t5ctx, kT5_GameboardType_LE, &gameboardSize);
        if (err) {
            printf("Failed to get gameboard size\n");
        } else {
            printf("LE Gameboard size : %fm x %fm x %fm\n",
                   gameboardSize.viewableExtentPositiveX + gameboardSize.viewableExtentNegativeX,
                   gameboardSize.viewableExtentPositiveY + gameboardSize.viewableExtentNegativeY,
                   gameboardSize.viewableExtentPositiveZ);
        }
        /// [SystemWideQuery]
    }

    /// [WaitForService]
    bool waiting = false;
    for (;;) {
        char serviceVersion[T5_MAX_STRING_PARAM_LEN];
        size_t bufferSize = T5_MAX_STRING_PARAM_LEN;
        err               = t5GetSystemUtf8Param(
            t5ctx, kT5_ParamSys_UTF8_Service_Version, serviceVersion, &bufferSize);
        if (!err) {
            printf("Service version : %s\n", serviceVersion);
            break;
        }

        if (err == T5_ERROR_NO_SERVICE) {
            if (!waiting) {
                printf("Waiting for service...\n");
                waiting = true;
            }
        } else {
            printf("Error getting service version : %s\n", t5GetResultMessage(err));
            exit(EXIT_FAILURE);
        }
    }
    /// [WaitForService]

    /// [WaitForGlasses1]
    for (;;) {
        size_t bufferSize = GLASSES_BUFFER_SIZE;
        char glassesListBuffer[GLASSES_BUFFER_SIZE];
        err = t5ListGlasses(t5ctx, glassesListBuffer, &bufferSize);
        if (!err) {
            size_t glassesCount = 0;

            const char* buffPtr = glassesListBuffer;
            for (;;) {
                // Get the length of the string and exit if we've read the
                // terminal string (Zero length)
                size_t stringLength = strnlen(buffPtr, GLASSES_BUFFER_SIZE);
                if (stringLength == 0) {
                    break;
                }

                fprintf(stderr, "Glasses : %s\n", buffPtr);
                glassesCount++;

                /// [WaitForGlasses1]
                connectGlasses(t5ctx, buffPtr);
                /// [WaitForGlasses3]

                // Advance through the returned values
                buffPtr += stringLength;
                if (buffPtr > (glassesListBuffer + GLASSES_BUFFER_SIZE)) {
                    printf("Warning: list may be missing null terminator");
                    break;
                }
            }

            printf("Listed glasses [%zu found]\n", glassesCount);
            break;
        }

        if (err == T5_ERROR_NO_SERVICE) {
            if (!waiting) {
                printf("Waiting for service...\n");
                waiting = true;
            }
        } else {
            printf("Error listing glasses : %s\n", t5GetResultMessage(err));
            exit(EXIT_FAILURE);
        }
    }
    /// [WaitForGlasses3]

    /// [ReleaseContext]
    t5DestroyContext(&t5ctx);
    /// [ReleaseContext]
    printf("Shutdown complete\n");
}
