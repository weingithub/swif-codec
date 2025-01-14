#ifndef __SWIF_FULL_SYMBOL_IMPL_H__
#define __SWIF_FULL_SYMBOL_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s: %d: %s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)

/*---------------------------------------------------------------------------*/
/**
 * @brief       For now this is an hackish adaptation on top of liblc,
 *              will be replaced by a proper implementation later.
 */

#include <stdio.h>
#include <stdlib.h>

#include "swif_full_symbol.h" 
#include "swif_symbol.h"

#include <assert.h>

/*---------------------------------------------------------------------------*/


struct s_swif_full_symbol_t {
    //coded_packet_t* coded_packet;
    //uint32_t symbol_size;

    // first_id, last_id  relate to the memory allocation
    // can be SYMBOL_ID_NONE (when no coefficient available)
    uint8_t* coef; // never NULL
    symbol_id_t first_id; // coef[0] is the coefficient of first symbol_id
    symbol_id_t last_id; // (last included)

    // first_nonzero_id, last_nonzero_id relate to the coef array
    // they can be SYMBOL_ID_NONE if no coef is different of zero
    // otherwise full_symbol_get_coef(i) is 0 outside of this range is always zzero o
    // and the coefficients at these, are non-zero
    symbol_id_t first_nonzero_id;
    symbol_id_t last_nonzero_id;    
    
    uint8_t* data; // never NULL
    uint32_t data_size;
};

/*---------------------------------------------------------------------------*/

struct s_swif_full_symbol_set_t {
    // size of the table containing pointers to full symbol
    uint32_t size; 
    uint32_t first_symbol_id;
    uint32_t nmbr_packets;
    swif_full_symbol_t **full_symbol_tab;
    //symbol_id_t *full_symbol_pivot;
    //uint8_t *full_symbol_index;
};

/*---------------------------------------------------------------------------*/


typedef struct s_swif_pivot_coefs_t swif_pivot_coefs_t;

/*---------------------------------------------------------------------------*/


/**
 * @brief Create a full_symbol set, that will be used to do gaussian elimination
 */
swif_full_symbol_set_t *full_symbol_set_alloc();

/**
 * @brief Free a full_symbol set
 */
void full_symbol_set_free(swif_full_symbol_set_t *set);


/**
 * @brief Add a full_symbol to a packet set.
 * 
 * Gaussian elimination can occur.
 * Return the pivot associated to the new full_symbol 
 * or SYMBOL_ID_NONE if dependent (e.g. redundant) packet
 * 
 * The full_symbol is not freed and also reference is not captured.
 */
uint32_t swif_full_symbol_set_add
(swif_full_symbol_set_t* set, swif_full_symbol_t* full_symbol);

/*---------------------------------------------------------------------------*/

/**
 * @brief Create a full_symbol from a raw packet (a set of bytes)
 *        and initialize it with content '0'
 */
swif_full_symbol_t *full_symbol_alloc
(symbol_id_t first_symbol_id, symbol_id_t last_symbol_id, uint32_t symbol_size) ;



/* XXX:doc */
void full_symbol_set_dump(swif_full_symbol_set_t *full_symbol_set, FILE *out);


/**
 * @brief get the size of the data
 */



static inline bool full_symbol_has_sufficient_size(swif_full_symbol_t* symbol,
                                           symbol_id_t id1, symbol_id_t id2);
static inline bool full_symbol_includes_id(swif_full_symbol_t* symbol,
                                           symbol_id_t symbol_id);
// adjust full symbol min coef


static bool full_symbol_adjust_min_coef(swif_full_symbol_t* symbol);




// adjust full symbol max coef

static bool full_symbol_adjust_max_coef(swif_full_symbol_t* symbol);


// adjust full symbol min and max coef
// Recompute the first_nonzero_id and last_nonzero_id 
bool full_symbol_adjust_min_max_coef(swif_full_symbol_t* symbol);


/*---------------------------------------------------------------------------*/

/**
 * @brief Take a symbol and add another symbol multiplied by a 
 *        coefficient, e.g. performs the equivalent of: p1 += coef * p1
 * @param[in,out] p1     First symbol (to which coef*p1 will be added)
 * @param[in]     coef2  Coefficient by which the second packet is multiplied
 */
void full_symbol_scale
( swif_full_symbol_t *symbol1, uint8_t coereef);


/**
 * @brief Take a symbol and add another symbol to it, e.g. performs the equivalent of: p3 = p1 + p2
 * @param[in] p1     First symbol (to which p2 will be added)
 * @param[in] p2     Second symbol
 * @param[in] p3     result XXX
 */
void full_symbol_add_base(swif_full_symbol_t *symbol1, swif_full_symbol_t *symbol2, swif_full_symbol_t *symbol_result);



swif_full_symbol_t* full_symbol_add
(swif_full_symbol_t *symbol1, swif_full_symbol_t *symbol2);



/* Substration */
//void full_symbol_sub(void *symbol1, void *symbol2, uint32_t symbol_size, uint8_t* result);


/**
 * @brief Take a symbol and multiply it by another symbol, e.g. performs the equivalent of: result = p1 * p2
 * @param[in] p1     First symbol (to which coeff will be multiplied)
 * @param[in]     coeff      Coefficient by which the second packet is multiplied
 */
//void full_symbol_mul(void *symbol1, uint8_t coeff, uint32_t symbol_size, uint8_t* result);


/**
 * @brief Take a symbol and divide it by another symbol, e.g. performs the equivalent of: result = p1 / p2
 * @param[in] p1     First symbol (to which coeff will be divided)
 * @param[in]     coeff      Coefficient by which the second packet is divided
 */
//void full_symbol_div(void *symbol1, uint32_t symbol_size, uint8_t coeff, uint8_t* result);

/*---------------------------------------------------------------------------*/




void full_symbol_dump(swif_full_symbol_t *full_symbol, FILE *out);









/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __CODED_PACKET_H__ */
/*---------------------------------------------------------------------------*/
/** @} */
