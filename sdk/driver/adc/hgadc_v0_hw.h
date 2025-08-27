#ifndef _HGADC_V0_HW_H
#define _HGADC_V0_HW_H

#ifdef __cplusplus
    extern "C" {
#endif


#if TXW80X || TXW8000


#define LL_ADKEY_SOTF_KICK(hw)                      (hw->ADKEY_CON |= BIT(19))

#define LL_ADKEY_GET_IRQ_EN_SAMPLE_DONE(hw)         (hw->ADKEY_CON & BIT(20))

#define LL_ADKEY_GET_DONE_PENDING(hw)               (hw->ADKEY_DATA & BIT(31))

#define LL_ADKEY_CLEAR_DONE_PENDING(hw)             (hw->ADKEY_DATA |= BIT(12))

#define LL_ADKEY_GET_DATA(hw)                       (hw->ADKEY_DATA & 0xFFF)


/** @brief ADC register structure
  * @{
  */
struct hgadc_v0_hw {
    __IO uint32_t ADKEY_CON;
    __IO uint32_t RESERVED;
    __IO uint32_t ADKEY_DATA;
};
#endif



#if TXW81X

#define LL_ADKEY_SOTF_KICK(hw)                      (hw->ADKEY_CON |= BIT(19))

#define LL_ADKEY_GET_IRQ_EN_SAMPLE_DONE(hw)         (hw->ADKEY_CON & BIT(20))

#define LL_ADKEY_GET_DONE_PENDING(hw)               (hw->ADKEY_STA & BIT(0))

#define LL_ADKEY_CLEAR_DONE_PENDING(hw)             (hw->ADKEY_STA = BIT(0))

#define LL_ADKEY_GET_DATA(hw)                       (hw->ADKEY_DATA & 0xFFF)



/** @brief ADC register structure
  * @{
  */
struct hgadc_v0_hw {
    __IO uint32 ADKEY_CON;
    __IO uint32 ADKEY_BAUD;
    __IO uint32 ADKEY_DATA;
    __IO uint32 ADKEY_STA;
    __IO uint32 ADKEY_DMACON;
    __IO uint32 ADKEY_DMAADR;
    __IO uint32 ADKEY_DMALEN;
    __IO uint32 ADKEY_DMACNT;
};
#endif






#ifdef __cplusplus
}
#endif

#endif /* _HGADC_V0_HW_H */