#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "driver/uart.h"
#include "string.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "esp_err.h"


static const char *TAG = "example";

#define MOUNT_POINT "/sdcard"

// Pin assignments can be set in menuconfig, see "SD SPI Example Configuration" menu.
// You can also change the pin assignments here by changing the following 4 lines.
#define PIN_NUM_MISO  19 //CONFIG_EXAMPLE_PIN_MISO
#define PIN_NUM_MOSI  23 //CONFIG_EXAMPLE_PIN_MOSI
#define PIN_NUM_CLK   18 //CONFIG_EXAMPLE_PIN_CLK
#define PIN_NUM_CS    5 //CONFIG_EXAMPLE_PIN_CS

#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define UART_PORT_NUM UART_NUM_2
#define UART_BAUD_RATE 115200
#define BUF_SIZE 1024

const uint8_t ADDR = 0x08;


int count_lines(FILE *file) {
    char buf[40];
    int counter = 0;

    // Ensure the file pointer starts at the beginning
    fseek(file, 0, SEEK_SET);

    for (;;) {
        size_t res = fread(buf, 1, sizeof(buf), file);
        if (ferror(file))
            return -1;

        for (int i = 0; i < res; i++)
            if (buf[i] == '\n')
                counter++;

        if (feof(file))
            break;
    }

    // Reset the file pointer to the end for appending
    fseek(file, 0, SEEK_END);
    return counter;
}

void appendToCSV(FILE *file, float voltage, float current) {
    if (file == NULL) {
        printf("Error: Could not open file in appendToCSV()\n");
        return;
    }

    int num_lines = count_lines(file);

    if (num_lines == 0) {
        // Write the header if the file is empty
        fprintf(file, "voltage,current\n");
    }
    fprintf(file, "%.3f,%.4f\n", voltage, current);         // Write the data to the file
    //printf("Data appended successfully.\n");
}


void init_uart() {
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,

    };
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, BUF_SIZE, 0, 0, NULL, 0);
}

float read_voltage(uint8_t data_addr){
    uint8_t data[2];
    uint16_t value = 0; 
    float temp = 0;

    i2c_cmd_handle_t cmd0 = i2c_cmd_link_create();
    i2c_master_start(cmd0); //start condition
    i2c_master_write_byte(cmd0, (ADDR << 1) | I2C_MASTER_WRITE ,true); //handle, dev addr ack en ///////use |
    i2c_master_write(cmd0, &data_addr, 1, true); //dummy write, &dtaddr handle, m //////ack use 0x01

    i2c_master_start(cmd0);
    i2c_master_write_byte(cmd0, ( ADDR << 1 ) | I2C_MASTER_READ, true);
    i2c_master_read(cmd0, data, 2,I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd0);

    esp_err_t ret = i2c_master_cmd_begin(0, cmd0, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd0);

    if (ret == ESP_OK){
        //printf("voltage read success\n");
        value = value | data[1];
        value = value << 8;
        value = value | data[0];
        temp = value * 0.001;
    }
    else{
        //printf("voltage read fail\n");
    }
    return temp;

}

float read_current(uint8_t data_addr){
    uint8_t data[2];
    uint16_t value = 0; 
    float temp = 0;

    i2c_cmd_handle_t cmd0 = i2c_cmd_link_create();
    i2c_master_start(cmd0); //start condition
    i2c_master_write_byte(cmd0, (ADDR << 1) | I2C_MASTER_WRITE ,true); //handle, dev addr ack en ///////use |
    i2c_master_write(cmd0, &data_addr, 1, true); //dummy write, &dtaddr handle, m //////ack use 0x01

    i2c_master_start(cmd0);
    i2c_master_write_byte(cmd0, ( ADDR << 1 ) | I2C_MASTER_READ, true);
    i2c_master_read(cmd0, data, 2,I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd0);

    esp_err_t ret = i2c_master_cmd_begin(0, cmd0, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd0);

    if (ret == ESP_OK){
        //printf("Current read success\n");
        value = value | data[1];
        value = value << 8;
        value = value | data[0];
        temp = value * 0.001;
    }
    else{
        //printf("Current read fail\n");
    }
    return temp;
}

void modify_fet(uint8_t address){
    uint8_t low_byte = 0x22;
    uint8_t high_byte = 0x00;

    i2c_cmd_handle_t cmd0 = i2c_cmd_link_create();
    i2c_master_start(cmd0); //start condition
    i2c_master_write_byte(cmd0, (ADDR << 1) | I2C_MASTER_WRITE ,true); //handle, dev addr ack en ///////use |
    i2c_master_write(cmd0, &address, 1, true); //0x3E
    i2c_master_write(cmd0, &low_byte, 1, true); //0x22
    i2c_master_write(cmd0, &high_byte, 1, true); //0x00
    i2c_master_stop(cmd0);

    esp_err_t ret = i2c_master_cmd_begin(0, cmd0, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd0);

    if (ret == ESP_OK){
        printf("FET modified success\n");
    }
    else{
        printf("FET modify failed.\n");
    }
}

void app_main()
{
    esp_log_level_set("gpio", ESP_LOG_WARN);
    esp_log_level_set("sdspi_transaction", ESP_LOG_WARN);

    init_uart();

    i2c_config_t config_i2c;
    config_i2c.mode = I2C_MODE_MASTER;
    config_i2c.sda_io_num = 21;
    config_i2c.scl_io_num = 22;
    config_i2c.sda_pullup_en = true;
    config_i2c.scl_pullup_en = true;
    config_i2c.master.clk_speed = 50000;
    config_i2c.clk_flags = 0;
    ESP_ERROR_CHECK(i2c_param_config( 0 , &config_i2c));
    ESP_ERROR_CHECK(i2c_driver_install(0 , I2C_MODE_MASTER, 0 ,0 ,0));
    modify_fet(0x3E);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    #ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
            .format_if_mount_failed = true,
    #else
            .format_if_mount_failed = false,
    #endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
        };
     

 while(true)
 {
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    //ESP_LOGI(TAG, "Initializing SD card");

    //ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    //ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    //ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    //sdmmc_card_print_info(stdout, card);

    const char *file_hello = "/sdcard/bat.csv";                     // First create a file.

    FILE *file = fopen(file_hello, "a+");                           // Open the file in append mode, allowing read and write
    if (file == NULL) {
        printf("Error: Could not open file %s\n", file_hello);
    }                                                                
   
    float voltage1 = read_voltage(0x1C);
    vTaskDelay(pdMS_TO_TICKS(10));

    //float voltage2 = read_voltage(0x14);
    //vTaskDelay(pdMS_TO_TICKS(1000));

    //float pack_volt = read_voltage(0x26);
    //vTaskDelay(pdMS_TO_TICKS(1000));

    float current = read_current(0x3A);
    vTaskDelay(pdMS_TO_TICKS(10));
    appendToCSV(file, voltage1, current);
    fclose(file);
    
    //printf("Cell 1: %f\n", voltage1);
    //printf("Cell 2: %f\n", voltage2);
    //printf("Pack : %f\n", pack_volt);
    //printf("Current: %f\n", current);

    char message[100];
    snprintf(message, sizeof(message), "%.4f,%.5f\n", voltage1, current);
    printf("%s", message);
    uart_write_bytes(UART_PORT_NUM, message, strlen(message));
    
    vTaskDelay(pdMS_TO_TICKS(1000)); // Send data every second
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    //ESP_LOGI(TAG, "Card unmounted");

    spi_bus_free(host.slot);                                         //deinitialize the bus after all devices are removed  
 }
}
