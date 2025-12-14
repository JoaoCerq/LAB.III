/*
 * aic.h
 *
 * Created on: Nov 23, 2025
 * Author: joao0
 *
 * Description: Header file for AIC3204 audio codec control functions.
 */

#ifndef AIC_H_
#define AIC_H_

#include "ezdsp5502.h" // Necessário para os tipos Int16, Uint16, etc.

/*
 * Function Prototypes
 */

/**
 * @brief  Inicializa o Codec AIC3204 e configura o McBSP1 e GPIOs necessários.
 * Esta função chama internamente a aic3204_init() (loop setup).
 * @return 0 em caso de sucesso.
 */
Int16 aic3204_INIT(void);

/**
 * @brief  Escreve um valor em um registrador do AIC3204 via I2C.
 * @param  regnum  Número do registrador (0-127).
 * @param  regval  Valor a ser escrito (8-bit).
 * @return 0 em caso de sucesso, código de erro I2C caso contrário.
 */
Int16 AIC3204_rset(Uint16 regnum, Uint16 regval);

/**
 * @brief  Lê o valor de um registrador do AIC3204 via I2C.
 * @param  regnum  Número do registrador (0-127).
 * @param  regval  Ponteiro onde o valor lido será armazenado.
 * @return 0 em caso de sucesso, código de erro I2C caso contrário.
 */
Int16 AIC3204_rget(Uint16 regnum, Uint16* regval);

#endif /* AIC_H_ */
