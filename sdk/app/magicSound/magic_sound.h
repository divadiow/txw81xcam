#ifndef _MAGIC_SOUND_H_
#define _MAGIC_SOUND_H_

typedef struct {
    uint32_t samplerate;
    sonicStream sonic_s;
    int16_t *buf;
    uint8_t channel;
    uint8_t new_type;
    uint8_t current_type;
}magicSound;

typedef enum {
    original_sound,
    alien_sound,
    robot_sound,
    hight_sound,
    deep_sound,
    etourdi_sound
}magicSound_type;

extern magicSound *magicSound_init(uint32_t samplerate, uint8_t channel, uint32_t size);
extern void magicSound_set_type(magicSound *magic_sound, magicSound_type type);
extern void magicSound_process(magicSound *magic_sound, int16_t *buf, uint32_t samples);

#endif       