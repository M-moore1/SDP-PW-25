#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "arm.h"

static const char* TAG = "ARM_TEST";

// ─── Test points (inches) ─────────────────────────────────────────────────────
// Cycles through these in order forever — watch the arm and check the logs.
// Adjust values to cover your expected working range.

static const struct { float x; float y; float z; const char* label; } test_points[] = {
    { ARM_HOME_X, ARM_HOME_Y, ARM_HOME_Z, "Home"        },
    { -4.0f,      7.5f,       5.5f,       "X in"        },
    { -6.0f,      7.5f,       5.5f,       "X out"       },
    { ARM_HOME_X, ARM_HOME_Y, ARM_HOME_Z, "Home"        },
    { -5.0f,      8.5f,       5.5f,       "Y right"     },
    { -5.0f,      6.5f,       5.5f,       "Y left"      },
    { ARM_HOME_X, ARM_HOME_Y, ARM_HOME_Z, "Home"        },
    { -5.0f,      7.5f,       6.5f,       "Z up"        },
    { -5.0f,      7.5f,       4.5f,       "Z down"      },
    { ARM_HOME_X, ARM_HOME_Y, ARM_HOME_Z, "Home return" },
};

#define NUM_POINTS  (sizeof(test_points) / sizeof(test_points[0]))

// Time to hold each position before moving to the next.
// 500 ms gives the servo plenty of time to physically arrive.
// Lower to 200 ms once you confirm the arm is reaching each point cleanly.
#define HOLD_MS  500

void app_main(void) {
    arm_init();
    ESP_LOGI(TAG, "Arm test ready — %d points, %d ms hold", NUM_POINTS, HOLD_MS);

    while (1) {
        for (int i = 0; i < NUM_POINTS; i++) {
            float x = test_points[i].x;
            float y = test_points[i].y;
            float z = test_points[i].z;

            ESP_LOGI(TAG, "[%d/%d] %-12s (%.2f, %.2f, %.2f)",
                     i + 1, NUM_POINTS, test_points[i].label, x, y, z);

            if (arm_move_to(x, y, z) != 0) {
                ESP_LOGW(TAG, "  rejected — outside joint limits, skipping");
            }

            vTaskDelay(pdMS_TO_TICKS(HOLD_MS));
        }

        ESP_LOGI(TAG, "── cycle complete, repeating ──");
    }
}