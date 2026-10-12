/**
 * @file Shared GPIO definitions.
 */
#ifndef GPIO_H_
#define GPIO_H_

/**
 * @brief Enumeration of GPIO pins.
 *
 * @note Every pin can be used as a digital input or output. Only the pins marked below can be
 *       used with the analog driver. I/O port A is reserved for the display.
 */
typedef enum
{
    GPIO_PIN_PC0, ///< Pin 0 on I/O port C.
    GPIO_PIN_PC1, ///< Pin 1 on I/O port C.
    GPIO_PIN_PC2, ///< Pin 2 on I/O port C.
    GPIO_PIN_PC3, ///< Pin 3 on I/O port C.
    GPIO_PIN_PD1, ///< Pin 1 on I/O port D (analog input).
    GPIO_PIN_PD2, ///< Pin 2 on I/O port D (analog input).
    GPIO_PIN_PD3, ///< Pin 3 on I/O port D (analog input).
    GPIO_PIN_PD4, ///< Pin 4 on I/O port D (analog input).
    GPIO_PIN_PD5, ///< Pin 5 on I/O port D (analog input).
    GPIO_PIN_PD6, ///< Pin 6 on I/O port D (analog input or output).
    GPIO_PIN_PD7, ///< Pin 7 on I/O port D (analog input).
    GPIO_PIN_PF0, ///< Pin 0 on I/O port F (analog input).
    GPIO_PIN_PF1, ///< Pin 1 on I/O port F (analog input).
} gpio_pin_t;

/**
 * @brief Enumeration of GPIO modes.
 */
typedef enum
{
    GPIO_MODE_IN,  ///< Input.
    GPIO_MODE_OUT, ///< Output.
} gpio_mode_t;

#endif /** GPIO_H_ */
