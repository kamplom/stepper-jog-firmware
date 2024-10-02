#include "config.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "system.h"
#include <string.h>

static const char *TAG = "Serial";

static void uart_event_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Created UART event task");
    uart_event_t event;
    size_t buffered_size;
    char* dtmp = (char*) malloc(UART_RX_BUF_SIZE);
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(uart_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, UART_RX_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", UART_SEL_NUM);
            switch (event.type) {
            //Event of UART receiving data
            /*We'd better handler data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.*/
            case UART_DATA:
                // ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                // uart_read_bytes(UART_SEL_NUM, dtmp, event.size, portMAX_DELAY);
                // ESP_LOGI(TAG, "[DATA EVT]:");
                // uart_write_bytes(UART_SEL_NUM, (const char*) dtmp, event.size);
                break;
            //Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(UART_SEL_NUM);
                xQueueReset(uart_queue);
                break;
            //Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                // If buffer full happened, you should consider increasing your buffer size
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(UART_SEL_NUM);
                xQueueReset(uart_queue);
                break;
            //Event of UART RX break detected
            case UART_BREAK:
                ESP_LOGI(TAG, "uart rx break");
                break;
            //Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGI(TAG, "uart parity error");
                break;
            //Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
            //UART_PATTERN_DET
            case UART_PATTERN_DET:
                uart_get_buffered_data_len(UART_SEL_NUM, &buffered_size);
                int pos = uart_pattern_pop_pos(UART_SEL_NUM);
                ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1) {
                    // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                    // record the position. We should set a larger queue size.
                    // As an example, we directly flush the rx buffer here.
                    uart_flush_input(UART_SEL_NUM);
                } else {
                    uart_read_bytes(UART_SEL_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
                    char* pat = (char*)malloc(2);
                    bzero(pat, 2);
                    uart_read_bytes(UART_SEL_NUM, pat, 1, 100 / portTICK_PERIOD_MS);
                    execute_line(dtmp, pat);
                }
                break;
            //Others
            default:
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void init_uart(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_LOGI(TAG, "Installing uart driver");
    uart_driver_install(UART_SEL_NUM, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 20, &uart_queue, 0);
    ESP_LOGI(TAG, "Configuring uart driver");
    uart_param_config(UART_SEL_NUM, &uart_config);
    uart_set_pin(UART_SEL_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_LOGI(TAG, "Setting up patten detection");
    uart_enable_pattern_det_baud_intr(UART_SEL_NUM, 's', 1, 9, 0, 0);
    uart_pattern_queue_reset(UART_SEL_NUM, 20);

    //Create buffer for read lines. Filled on ISR
    // line_buff = xRingbufferCreate(1028, RINGBUF_TYPE_NOSPLIT);
    // if (line_buff == NULL) {
    //     printf("Failed to create ring buffer\n");
    // }
    xTaskCreate(uart_event_task, "uart_event_task", 3072, NULL, 12, NULL);
}