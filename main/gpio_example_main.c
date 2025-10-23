/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)
   jrm 9/22/2025 
   The example code has been changed to something more simpler. This program
   uses two inputs gpio pin 4 and 5 as inputs which are connected to buttons.
   Any btn press fires a interrupt that sends the gpio # to the gpio task.
   The task actually performs the work keeping the interrupt routine as short
   as possible. This is the recommended approach.
   One thing that was strange is function esp_err_t gpio_pullup_en(gpio_num_t gpio_num)
   did not enable pullup on pin 4 as that pin should be pulled up to vcc but wasn't.
 
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_log.h>

//Generate pulses on input pins 4,5 from output pins 18,19 which causes an interrupt
/**
 * Brief:
 *
 * GPIO status:
 * GPIO18: output, GPIO19: output that drives led's
 * GPIO4, GPIO5  input, pulled up, interrupt from rising edge
 */

#define ESP_INTR_FLAG_DEFAULT 0
static const char *TAG = "GPIO";
static QueueHandle_t gpio_evt_queue = NULL;

//interrupt handler
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

//gpio task communicates with isr
static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Pin %lu has been pushed", io_num);
           // printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

void app_main(void)
{

    gpio_set_intr_type(GPIO_NUM_5, GPIO_INTR_POSEDGE);
    gpio_set_intr_type(GPIO_NUM_4, GPIO_INTR_POSEDGE);
   
    gpio_evt_queue = xQueueCreate(20, sizeof(uint32_t));
   
    xTaskCreate(gpio_task_example, "gpio_task_example",2048, NULL, 3, NULL);
    
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_19, GPIO_MODE_OUTPUT);
    
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);  //install gpio isr service, allows seperate single pin interrupts

    gpio_isr_handler_add(GPIO_NUM_5, gpio_isr_handler, (void*) GPIO_NUM_5); //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_NUM_4, gpio_isr_handler, (void*) GPIO_NUM_4); //hook isr handler for specific gpio pin

    gpio_intr_enable(GPIO_NUM_4);
    gpio_intr_enable(GPIO_NUM_5);
   
   // gpio_pullup_en(GPIO_NUM_4); //didnt work
    gpio_set_pull_mode(GPIO_NUM_5,GPIO_PULLUP_ONLY);
   // gpio_pullup_en(GPIO_NUM_5);
    gpio_set_pull_mode(GPIO_NUM_4,GPIO_PULLUP_ONLY);
  
    //we can also remove isr's
    //gpio_isr_handler_remove(GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin again
    //gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);

    //printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());
    ESP_LOGI(TAG, "Minimum free heap size: %lu bytes", esp_get_minimum_free_heap_size());

    int cnt = 0;
    while (1) {
       // printf("cnt: %d\n", cnt++);
       cnt++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_18, cnt % 2);
        gpio_set_level(GPIO_NUM_19, cnt % 2);
    }
}
