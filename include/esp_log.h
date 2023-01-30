#pragma once

#define ESP_LOGI(tag, format, ...) { printf("[I][%s] ", tag); printf(format, ##__VA_ARGS__); printf("\n"); } 
#define ESP_LOGE(tag, format, ...) { printf("[E][%s] ", tag); printf(format, ##__VA_ARGS__); printf("\n"); } 
#define ESP_LOGW(tag, format, ...) { printf("[W][%s] ", tag); printf(format, ##__VA_ARGS__); printf("\n"); } 
