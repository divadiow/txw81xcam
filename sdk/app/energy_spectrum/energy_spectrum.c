#include "basic_include.h"
#include "dev/audio/components/vad/auvad.h"
#include "dev/audio/components/vad/hg_auvad_v0.h"

#define WINDOW_SIZE 5 // 用于计算平均值的窗口大小
#define SCALE_FACTOR 100 

struct auvad_device *vad_dev;
int32_t sp_num = 0;
int32_t sd_num = 1;
uint32_t vad_tem_buff[5];
uint32_t vad_buff_num;
uint32_t num;
volatile uint32 vad_r;
float average_energy_tem;
float smoothed_energy;          
float smoothed_map_energy;

float calculate_average_energy(uint32_t *energy_window, int size, float alpha) {
    float sum = 0;
    float new_energy = 0;
    for (int i = 0; i < size; ++i) {
        sum += energy_window[i];
    }
    new_energy = sum / size;
    smoothed_energy = alpha * new_energy + (1 - alpha) * smoothed_energy; 
    return smoothed_energy;
}

int map_energy_to_range(uint32_t current_energy, float average_energy, float alpha) {
    if (average_energy == 0) {
        // 避免除以零的情况
        return 0;
    }
    float normalized_energy = (float)current_energy / average_energy * SCALE_FACTOR ;
    smoothed_map_energy = alpha * normalized_energy + (1 - alpha) * smoothed_map_energy ;
    return (uint32_t)(smoothed_map_energy);
}

static uint32_t mp3_audio_vad_calc(int16_t *data, uint32_t samples)
{
    uint32_t ret = 0;
	uint32_t current_energy = 0;
	uint32_t mapped_energy = 0;
	uint32_t vad_result[2] = {0, 0};
    float average_energy = 0;
    
    ret = auvad_calc(vad_dev, data, samples, AUVAD_CALC_MODE_ENERGY, vad_result);   
    current_energy = vad_result[0];
	num++;

    // 计算平均能量值（如果窗口未满，则只计算已有帧的平均值）
    if (num >= WINDOW_SIZE - 1) {
        vad_tem_buff[vad_buff_num] = current_energy;
        average_energy = calculate_average_energy(vad_tem_buff, WINDOW_SIZE, 0.2);
    } else {
        //if(current_energy >= average_energy)
        vad_tem_buff[vad_buff_num] = current_energy;
        average_energy = calculate_average_energy(vad_tem_buff, vad_buff_num + 1, 0.2);
    }
    vad_buff_num = (vad_buff_num + 1) % WINDOW_SIZE;
    average_energy_tem = average_energy;
    mapped_energy = map_energy_to_range(current_energy, average_energy, 0.3);

    //os_printf("Frame %d:ret:%d data:%x sample = %d Energy = %d average= %f Mapped Energy = %d\n",num + 1,ret,(int16_t *)get_stream_real_data(s_data),sample,current_energy,average_energy, mapped_energy);
    if(num % 4 == 0 || num % 4 == 3){
        mapped_energy = mapped_energy/3;
    } else {
        mapped_energy = mapped_energy/3;
    }
	return mapped_energy;
}