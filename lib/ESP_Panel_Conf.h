/*
 * Bronco Controls - ESP32 Display Panel Configuration
 * Waveshare ESP32-S3 Touch LCD variants (4.3" and 7")
 */

#ifndef ESP_PANEL_CONF_H
#define ESP_PANEL_CONF_H

#ifndef BRONCO_PANEL_VARIANT_4_3
#define BRONCO_PANEL_VARIANT_4_3    (0)
#endif

#ifndef BRONCO_PANEL_VARIANT_7_0
#define BRONCO_PANEL_VARIANT_7_0    (1)
#endif

#ifndef BRONCO_PANEL_VARIANT
#define BRONCO_PANEL_VARIANT        BRONCO_PANEL_VARIANT_4_3
#endif

// Always use custom board definitions
#define ESP_PANEL_USE_SUPPORTED_BOARD   (0)

#define BRONCO_LCD_RES_H            (800)
#define BRONCO_LCD_RES_V            (480)
#define BRONCO_RGB_CLK_HZ           (16 * 1000 * 1000)
#define BRONCO_RGB_PCLK_NEG         (1)
#define BRONCO_RGB_DATA_WIDTH       (16)

#define BRONCO_RGB_IO_HSYNC         (46)
#define BRONCO_RGB_IO_VSYNC         (3)
#define BRONCO_RGB_IO_DE            (5)
#define BRONCO_RGB_IO_PCLK          (7)
#define BRONCO_RGB_IO_DATA0         (14)
#define BRONCO_RGB_IO_DATA1         (38)
#define BRONCO_RGB_IO_DATA2         (18)
#define BRONCO_RGB_IO_DATA3         (17)
#define BRONCO_RGB_IO_DATA4         (10)
#define BRONCO_RGB_IO_DATA5         (39)
#define BRONCO_RGB_IO_DATA6         (0)
#define BRONCO_RGB_IO_DATA7         (45)
#define BRONCO_RGB_IO_DATA8         (48)
#define BRONCO_RGB_IO_DATA9         (47)
#define BRONCO_RGB_IO_DATA10        (21)
#define BRONCO_RGB_IO_DATA11        (1)
#define BRONCO_RGB_IO_DATA12        (2)
#define BRONCO_RGB_IO_DATA13        (42)
#define BRONCO_RGB_IO_DATA14        (41)
#define BRONCO_RGB_IO_DATA15        (40)

#define BRONCO_TOUCH_SCL_IO         (9)
#define BRONCO_TOUCH_SDA_IO         (8)

#define BRONCO_TOUCH_RST_IO         (-1)
#define BRONCO_TOUCH_INT_IO         (-1)

#if BRONCO_PANEL_VARIANT == BRONCO_PANEL_VARIANT_4_3
    #define BRONCO_BL_PWM_PIN       (6)
    #define BRONCO_BL_USES_PWM      (1)
#elif BRONCO_PANEL_VARIANT == BRONCO_PANEL_VARIANT_7_0
    #define BRONCO_BL_PWM_PIN       (6)
    #define BRONCO_BL_USES_PWM      (1)
#else
    #error "Unsupported BRONCO_PANEL_VARIANT value"
#endif

/*-------------------------------- LCD Related --------------------------------*/
#define ESP_PANEL_USE_LCD           (1)
#define ESP_PANEL_LCD_NAME          ST7262
#define ESP_PANEL_LCD_H_RES         (BRONCO_LCD_RES_H)
#define ESP_PANEL_LCD_V_RES         (BRONCO_LCD_RES_V)
#define ESP_PANEL_LCD_BUS_SKIP_INIT_HOST        (1)
#define ESP_PANEL_LCD_BUS_TYPE      (3)

#if ESP_PANEL_LCD_BUS_TYPE == 3
    #define ESP_PANEL_LCD_RGB_CLK_HZ            (BRONCO_RGB_CLK_HZ)
    #define ESP_PANEL_LCD_RGB_HPW               (10)
    #define ESP_PANEL_LCD_RGB_HBP               (10)
    #define ESP_PANEL_LCD_RGB_HFP               (20)
    #define ESP_PANEL_LCD_RGB_VPW               (10)
    #define ESP_PANEL_LCD_RGB_VBP               (10)
    #define ESP_PANEL_LCD_RGB_VFP               (10)
    #define ESP_PANEL_LCD_RGB_PCLK_ACTIVE_NEG   (BRONCO_RGB_PCLK_NEG)
    #define ESP_PANEL_LCD_RGB_DATA_WIDTH        (BRONCO_RGB_DATA_WIDTH)

    #define ESP_PANEL_LCD_RGB_IO_HSYNC          (BRONCO_RGB_IO_HSYNC)
    #define ESP_PANEL_LCD_RGB_IO_VSYNC          (BRONCO_RGB_IO_VSYNC)
    #define ESP_PANEL_LCD_RGB_IO_DE             (BRONCO_RGB_IO_DE)
    #define ESP_PANEL_LCD_RGB_IO_PCLK           (BRONCO_RGB_IO_PCLK)
    #define ESP_PANEL_LCD_RGB_IO_DATA0          (BRONCO_RGB_IO_DATA0)
    #define ESP_PANEL_LCD_RGB_IO_DATA1          (BRONCO_RGB_IO_DATA1)
    #define ESP_PANEL_LCD_RGB_IO_DATA2          (BRONCO_RGB_IO_DATA2)
    #define ESP_PANEL_LCD_RGB_IO_DATA3          (BRONCO_RGB_IO_DATA3)
    #define ESP_PANEL_LCD_RGB_IO_DATA4          (BRONCO_RGB_IO_DATA4)
    #define ESP_PANEL_LCD_RGB_IO_DATA5          (BRONCO_RGB_IO_DATA5)
    #define ESP_PANEL_LCD_RGB_IO_DATA6          (BRONCO_RGB_IO_DATA6)
    #define ESP_PANEL_LCD_RGB_IO_DATA7          (BRONCO_RGB_IO_DATA7)
    #define ESP_PANEL_LCD_RGB_IO_DATA8          (BRONCO_RGB_IO_DATA8)
    #define ESP_PANEL_LCD_RGB_IO_DATA9          (BRONCO_RGB_IO_DATA9)
    #define ESP_PANEL_LCD_RGB_IO_DATA10         (BRONCO_RGB_IO_DATA10)
    #define ESP_PANEL_LCD_RGB_IO_DATA11         (BRONCO_RGB_IO_DATA11)
    #define ESP_PANEL_LCD_RGB_IO_DATA12         (BRONCO_RGB_IO_DATA12)
    #define ESP_PANEL_LCD_RGB_IO_DATA13         (BRONCO_RGB_IO_DATA13)
    #define ESP_PANEL_LCD_RGB_IO_DATA14         (BRONCO_RGB_IO_DATA14)
    #define ESP_PANEL_LCD_RGB_IO_DATA15         (BRONCO_RGB_IO_DATA15)
    #define ESP_PANEL_LCD_RGB_IO_DISP           (-1)

    #if !ESP_PANEL_LCD_BUS_SKIP_INIT_HOST
        #define ESP_PANEL_LCD_SPI_CLK_HZ            (500 * 1000)
        #define ESP_PANEL_LCD_SPI_MODE              (0)
        #define ESP_PANEL_LCD_SPI_CMD_BYTES         (1)
        #define ESP_PANEL_LCD_SPI_PARAM_BITS        (8)
    #endif
    #define ESP_PANEL_LCD_SPI_IO_CS             (6)
    #define ESP_PANEL_LCD_SPI_IO_SCK            (7)
    #define ESP_PANEL_LCD_SPI_IO_MOSI           (6)
    #define ESP_PANEL_LCD_SPI_IO_MISO           (-1)
    #define ESP_PANEL_LCD_SPI_IO_DC             (-1)
#endif

#define ESP_PANEL_LCD_COLOR_BITS    (16)
#define ESP_PANEL_LCD_COLOR_SPACE   (0)
#define ESP_PANEL_LCD_INEVRT_COLOR  (0)
#define ESP_PANEL_LCD_SWAP_XY       (0)
#define ESP_PANEL_LCD_MIRROR_X      (0)
#define ESP_PANEL_LCD_MIRROR_Y      (0)
#define ESP_PANEL_LCD_IO_RST        (-1)
#define ESP_PANEL_LCD_RST_LEVEL     (0)

/*-------------------------------- LCD Touch Related --------------------------------*/
#define ESP_PANEL_USE_LCD_TOUCH     (1)
#define ESP_PANEL_LCD_TOUCH_NAME            GT911
#define ESP_PANEL_TOUCH_MAX_POINTS          (5)
#define ESP_PANEL_TOUCH_MAX_BUTTONS         (0)
#define ESP_PANEL_LCD_TOUCH_H_RES           (ESP_PANEL_LCD_H_RES)
#define ESP_PANEL_LCD_TOUCH_V_RES           (ESP_PANEL_LCD_V_RES)
#define ESP_PANEL_LCD_TOUCH_BUS_TYPE        (0)
#define ESP_PANEL_LCD_TOUCH_BUS_SKIP_INIT_HOST  (0)

#if !ESP_PANEL_LCD_TOUCH_BUS_SKIP_INIT_HOST
    #define ESP_PANEL_LCD_TOUCH_BUS_HOST_ID     (0)
    #define ESP_PANEL_LCD_TOUCH_I2C_CLK_HZ      (400 * 1000)
    #define ESP_PANEL_LCD_TOUCH_I2C_SCL_PULLUP  (0)
    #define ESP_PANEL_LCD_TOUCH_I2C_SDA_PULLUP  (0)
    #define ESP_PANEL_LCD_TOUCH_I2C_IO_SCL      (BRONCO_TOUCH_SCL_IO)
    #define ESP_PANEL_LCD_TOUCH_I2C_IO_SDA      (BRONCO_TOUCH_SDA_IO)
#endif

#define ESP_PANEL_LCD_TOUCH_SWAP_XY         (0)
#define ESP_PANEL_LCD_TOUCH_MIRROR_X        (0)
#define ESP_PANEL_LCD_TOUCH_MIRROR_Y        (0)
#define ESP_PANEL_LCD_TOUCH_IO_RST          (BRONCO_TOUCH_RST_IO)
#define ESP_PANEL_LCD_TOUCH_IO_INT          (BRONCO_TOUCH_INT_IO)
#define ESP_PANEL_LCD_TOUCH_RST_LEVEL       (0)
#define ESP_PANEL_LCD_TOUCH_INT_LEVEL       (0)

/*-------------------------------- Backlight Related --------------------------------*/
#define ESP_PANEL_USE_BL                    (1)
#define ESP_PANEL_LCD_IO_BL                 (BRONCO_BL_PWM_PIN)
#define ESP_PANEL_LCD_BL_LEVEL              (1)
#define ESP_PANEL_LCD_BL_ON_LEVEL           (ESP_PANEL_LCD_BL_LEVEL)

#if ESP_PANEL_USE_BL
    #define ESP_PANEL_LCD_BL_USE_PWM        (BRONCO_BL_USES_PWM)
    #if ESP_PANEL_LCD_BL_USE_PWM
        #define ESP_PANEL_LCD_BL_PWM_FREQ_HZ    (23000)
        #define ESP_PANEL_LCD_BL_PWM_DUTY_RES   (8)
    #endif
#endif

/*-------------------------------- Others --------------------------------*/
#define ESP_PANEL_CHECK_RESULT_ASSERT       (0)

#endif /* ESP_PANEL_CONF_H */
