#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stddef.h>
#include <stdint.h>


/*
 *  Calculate CRC16
 *
 *  @param p_data       Pointer to data
 *  @param len          Data len
 *  @param init_val     CRC init val
 *  @return             Calculated CRC
 */
uint16_t Checksum_CalcCRC16(uint8_t *p_data, size_t len, uint16_t init_val);

/*
 *  Calculate CRC32
 *
 *  @param p_data       Pointer to data
 *  @param len          Data len
 *  @param init_val     CRC init val
 *  @return             Calculated CRC
 */
uint32_t Checksum_CalcCRC32(uint8_t *p_data, size_t len, uint32_t init_val);

/*
 *  Calculate SHA256
 *
 *  @param p_data       Pointer to data
 *  @param len          Data len
 *  @param p_sha256     [out] calculated SHA256
 */
void Checksum_CalcSHA256(uint8_t *p_data, size_t len, uint8_t *p_sha256);

#endif
