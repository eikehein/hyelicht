/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

 #include <Arduino.h>

unsigned long previous = 0;
byte incoming;

void setup() {
    // Open serial connection to receive new duty cycle values.
    Serial.begin(115200);

    // Set up output on pin 3.
    pinMode(3, OUTPUT);

    // Set up phase-correct PWM mode by triangular waveform
    // using timer 2.
    TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM20);

    // Set the clock divider pre-scaler to 128 for a PWM
    // frequency of about 245 Hz. Calculated thusly:
    //    16 MHz base clock 
    //    / 128 (prescaler)
    //    / 256 (timer wrap-around)
    //    / 2 (phase-correct PWM by triangular waveform)
    TCCR2B |=  (1 << CS22) | (0 << CS21) | (1 << CS20);

    // The output compare register for pin 3.
    // 0 = 100% PWM duty cycle = display on at boot.
    OCR2B = 0;

    // The output compare register for pin 11 on the same
    // timer. We've not actually enabled pin 11's output,
    // but lets init it to a known value so we know what
    // the count-compare action involving it is doing.
    // 255 = 0% PWM duty cycle.
    OCR2A = 255;
}

void loop() {
    const unsigned long now = millis();

    // Cap our PWM parameters update rate at 250 Hz. We want
    // to make sure the display gets a certain number of pulses
    // at the same duty cycle so it can catch up.
    // Popping bytes off the serial input buffer at max this
    // rate introduces delay and jitter vs. the animation clock
    // source of the host. Our cycle time is much speedier, so
    // it's not a big deal. 
    if (now - previous >= 4) {
        previous = now;

        // New duty cycle commands are coming in via serial.
        if (Serial.available() > 0) {
            // Skip to the newest command in case we drifted so
            // far from the host clock as to have accumulated
            // more than one.
            while (Serial.available()) {
                incoming = Serial.read();
            }

            // Update the output compare register for pin 3 if
            // needed.
            if (OCR2B != incoming) {
                OCR2B = incoming;
            }
        }      
    }
}
