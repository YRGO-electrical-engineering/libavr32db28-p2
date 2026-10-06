/**
 * @brief Mocked hardware platform for AVR32DB28.
 */
#ifdef TESTSUITE

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** I/O port. */
typedef struct
{
    volatile uint8_t DIR;      /* Data direction. */
    volatile uint8_t OUT;      /* Output value. */
    volatile uint8_t OUTSET;   /* Output value set. */
    volatile uint8_t OUTCLR;   /* Output value clear. */
    volatile uint8_t OUTTGL;   /* Output value toggle. */
    volatile uint8_t IN;       /* Input value. */
    volatile uint8_t PIN0CTRL; /* Pin 0 control. */
    volatile uint8_t PIN1CTRL; /* Pin 1 control. */
    volatile uint8_t PIN2CTRL; /* Pin 2 control. */
    volatile uint8_t PIN3CTRL; /* Pin 3 control. */
    volatile uint8_t PIN4CTRL; /* Pin 4 control. */
    volatile uint8_t PIN5CTRL; /* Pin 5 control. */
    volatile uint8_t PIN6CTRL; /* Pin 6 control. */
    volatile uint8_t PIN7CTRL; /* Pin 7 control. */
} PORT_t;

/** Analog to digital converter. */
typedef struct
{
    volatile uint8_t CTRLA;    /* Control A. */
    volatile uint8_t CTRLB;    /* Control B. */
    volatile uint8_t CTRLC;    /* Control C. */
    volatile uint8_t CTRLD;    /* Control D. */
    volatile uint8_t CTRLE;    /* Control E. */
    volatile uint8_t SAMPCTRL; /* Sample control. */
    volatile uint8_t MUXPOS;   /* Positive mux input. */
    volatile uint8_t MUXNEG;   /* Negative mux input. */
    volatile uint8_t COMMAND;  /* Command. */
    volatile uint8_t EVCTRL;   /* Event control. */
    volatile uint8_t INTCTRL;  /* Interrupt control. */
    volatile uint8_t INTFLAGS; /* Interrupt flags. */
    volatile uint8_t DBGCTRL;  /* Debug control. */
    volatile uint16_t RES;     /* Result. */
} ADC_t;

/** Digital to analog converter. */
typedef struct
{
    volatile uint8_t CTRLA; /* Control A. */
    volatile uint16_t DATA; /* Data, held in the ten most significant bits. */
} DAC_t;

/** Voltage reference. */
typedef struct
{
    volatile uint8_t ADC0REF; /* ADC0 reference. */
    volatile uint8_t DAC0REF; /* DAC0 reference. */
    volatile uint8_t ACREF;   /* AC reference. */
} VREF_t;

/** Mocked peripheral instances. Only the ports bonded on the 28-pin package are provided. */
extern PORT_t PORTA;
extern PORT_t PORTC;
extern PORT_t PORTD;
extern PORT_t PORTF;
extern ADC_t ADC0;
extern DAC_t DAC0;
extern VREF_t VREF;

/* Pin bit masks. */
#define PIN0_bm 0x01
#define PIN1_bm 0x02
#define PIN2_bm 0x04
#define PIN3_bm 0x08
#define PIN4_bm 0x10
#define PIN5_bm 0x20
#define PIN6_bm 0x40
#define PIN7_bm 0x80

/* Pin bit positions. */
#define PIN0_bp 0
#define PIN1_bp 1
#define PIN2_bp 2
#define PIN3_bp 3
#define PIN4_bp 4
#define PIN5_bp 5
#define PIN6_bp 6
#define PIN7_bp 7

/* PORT bit masks and group configurations. */
#define PORT_PULLUPEN_bm 0x08
#define PORT_ISC_gm 0x07
#define PORT_ISC_INTDISABLE_gc (0x00 << 0)
#define PORT_ISC_INPUT_DISABLE_gc (0x04 << 0)

/* ADC bit masks and group configurations. */
#define ADC_ENABLE_bm 0x01
#define ADC_FREERUN_bm 0x02
#define ADC_STCONV_bm 0x01
#define ADC_RESRDY_bm 0x01
#define ADC_WCMP_bm 0x02
#define ADC_RESSEL_12BIT_gc (0x00 << 2)
#define ADC_RESSEL_10BIT_gc (0x01 << 2)
#define ADC_PRESC_DIV2_gc (0x00 << 0)
#define ADC_PRESC_DIV4_gc (0x01 << 0)
#define ADC_PRESC_DIV8_gc (0x02 << 0)
#define ADC_PRESC_DIV16_gc (0x04 << 0)
#define ADC_MUXPOS_AIN0_gc (0x00 << 0)
#define ADC_MUXPOS_AIN1_gc (0x01 << 0)
#define ADC_MUXPOS_AIN2_gc (0x02 << 0)
#define ADC_MUXPOS_AIN3_gc (0x03 << 0)
#define ADC_MUXPOS_AIN4_gc (0x04 << 0)
#define ADC_MUXPOS_AIN5_gc (0x05 << 0)
#define ADC_MUXPOS_AIN6_gc (0x06 << 0)
#define ADC_MUXPOS_AIN7_gc (0x07 << 0)
#define ADC_MUXPOS_AIN16_gc (0x10 << 0)
#define ADC_MUXPOS_AIN17_gc (0x11 << 0)

/* DAC bit masks and group positions. */
#define DAC_ENABLE_bm 0x01
#define DAC_OUTEN_bm 0x40
#define DAC_DATA_gp 6

/* VREF bit masks and group configurations. */
#define VREF_ALWAYSON_bm 0x80
#define VREF_REFSEL_1V024_gc (0x00 << 0)
#define VREF_REFSEL_2V048_gc (0x01 << 0)
#define VREF_REFSEL_4V096_gc (0x02 << 0)
#define VREF_REFSEL_2V500_gc (0x03 << 0)
#define VREF_REFSEL_VDD_gc (0x05 << 0)

/**
 * @brief Reset every mocked register to zero and discard the recorded delays.
 *
 *        Call this at the start of each test case, so that state does not leak between tests.
 */
void testHwPlatformReset(void);

/**
 * @brief Get the number of delay_ms calls made since the last reset.
 *
 *        The mocked delay_ms records the duration it was asked for rather than sleeping, so a
 *        test can check how long a driver intended to wait without waiting itself.
 *
 * @return Number of calls.
 */
uint16_t testDelayCount(void);

/**
 * @brief Get the duration passed to one of the recorded delay_ms calls.
 *
 * @param[in] index Zero based index of the call.
 *
 * @return Duration in milliseconds, or zero if no such call was recorded.
 */
uint16_t testDelayAt(uint16_t index);

#ifdef __cplusplus
}
#endif

#endif // TESTSUITE
