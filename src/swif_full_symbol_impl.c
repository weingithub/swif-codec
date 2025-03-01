
/**
 * @brief       For now this is an hackish adaptation on top of liblc,
 *              will be replaced by a proper implementation later.
 */


#include "swif_full_symbol_impl.h" 
#define PACKET_INDEX_NONE 0xffffffff
/*---------------------------------------------------------------------------*/


/**
 * @brief Create a full_symbol set, that will be used to do gaussian elimination
 */

uint32_t max_size = 2; 
swif_full_symbol_set_t *full_symbol_set_alloc()
{

    
    //allocate the struct 
    swif_full_symbol_set_t *result
        = (swif_full_symbol_set_t *)calloc(1, sizeof(swif_full_symbol_set_t));

    if (result == NULL) {
        return NULL;
    }

    result->size = max_size ; 
    result->nmbr_packets = 0 ;
    // allocate the table of pointers to full_symbol
    swif_full_symbol_t **full_symbol_tab = calloc(result->size, sizeof(swif_full_symbol_t *)); 
    if (full_symbol_tab == NULL) {
        free(full_symbol_tab);
        return NULL;
    }
    result->full_symbol_tab = full_symbol_tab;
    ////result->full_symbol_tab[0] = PACKET_INDEX_NONE ;
    /*
    // allocate the table of pivots
    symbol_id_t *full_symbol_pivot = calloc(result->size, sizeof(symbol_id_t)); 
    if (full_symbol_pivot == NULL) {
        free(result->full_symbol_tab);
        free(full_symbol_pivot);
        return NULL;
    }
    
    result->full_symbol_pivot = full_symbol_pivot;
    result->full_symbol_pivot[0] = PACKET_INDEX_NONE ;
    */
    result->first_symbol_id = SYMBOL_ID_NONE ; 
    return result; 
}

/**
 * @brief Free a full_symbol set
 */
void full_symbol_set_free(swif_full_symbol_set_t *set)
{
    assert(set != NULL);
    assert(set->full_symbol_tab != NULL);
    for(uint32_t i = 0 ; i<  set->size; i++){
        if (set->full_symbol_tab[i]  != NULL )
            full_symbol_free(set->full_symbol_tab[i]);   
    } 
    free(set->full_symbol_tab);
    set->full_symbol_tab = NULL;
    free(set);
}


void full_symbol_set_dump(swif_full_symbol_set_t *full_symbol_set, FILE *out)
{
    fprintf(out, "'full_symbol_set size':%u", full_symbol_set->size);
    fprintf(out,"\n"); 

    fprintf(out, "'full_symbol_set first_symbol_id':%u", full_symbol_set->first_symbol_id);
    fprintf(out,"\n");
    fprintf(out, "'full_symbol_tab':[");
    fprintf(out,"\n");

    for (uint32_t i=0 ; i<full_symbol_set->size ; i++ ){
        if(full_symbol_set->full_symbol_tab[i] != NULL){
            printf("full_symbol_set->full_symbol_tab[%u|%u]: ", i, full_symbol_set->full_symbol_tab[i]->first_nonzero_id-full_symbol_set->first_symbol_id);
            full_symbol_dump(full_symbol_set->full_symbol_tab[i], out);
        }
        else 
            printf("full_symbol_set->full_symbol_tab[%u] is NULL \n ", i);
        
    }

    fprintf(out, "]"); 
    fprintf(out,"\n");
    

}

/**
 * @brief Add a full_symbol to a packet set.
 * 
 * Gaussian elimination can occur.
 * Return the pivot associated to the new full_symbol 
 * or SYMBOL_ID_NONE if dependent (e.g. redundant) packet
 * 
 * The full_symbol is not freed and also reference is not captured.
 */
symbol_id_t calculate_min(symbol_id_t index1, symbol_id_t index2)
{
    return (index1 <= index2 ? index1 : index2) ;
}
uint32_t full_symbol_set_add(swif_full_symbol_set_t* set, swif_full_symbol_t* full_symbol)
{
    // clone 
    swif_full_symbol_t * full_symbol_cloned = full_symbol_clone(full_symbol);
    assert(set != NULL);
    if(full_symbol_cloned->first_nonzero_id == SYMBOL_ID_NONE ){
        return SYMBOL_ID_NONE ; 
    }
    if(set->first_symbol_id == SYMBOL_ID_NONE){// it was NULL
        set->first_symbol_id = full_symbol_cloned->first_nonzero_id; 
    }
    
    uint32_t old_size = set->size;
    /////  case : full_symbol_cloned->first_nonzero_id < set->first_symbol_id
     if ( full_symbol_cloned->first_nonzero_id < set->first_symbol_id)
    {   
        DEBUG_PRINT("Debugging is enabled. Case: full_symbol_cloned->first_nonzero_id < set->first_symbol_id \n");
        if (set->first_symbol_id-full_symbol_cloned->first_nonzero_id < set->size  )
        {
            memmove(set->full_symbol_tab+set->first_symbol_id-full_symbol_cloned->first_nonzero_id,set->full_symbol_tab, sizeof(swif_full_symbol_t*) *set->size-set->first_symbol_id-full_symbol_cloned->first_nonzero_id);
        }
        else if (set->first_symbol_id - full_symbol_cloned->first_nonzero_id < (set->size *2) ){
            set->size *= 2;
            set->full_symbol_tab =realloc(set->full_symbol_tab , set->size * sizeof(swif_full_symbol_t *)); 
            memmove(set->full_symbol_tab+set->first_symbol_id-full_symbol_cloned->first_nonzero_id,set->full_symbol_tab, sizeof(swif_full_symbol_t*) * old_size);
        }else{
            set->size = set->first_symbol_id-full_symbol_cloned->last_nonzero_id +1;
            set->full_symbol_tab =realloc(set->full_symbol_tab , set->size * sizeof(swif_full_symbol_t *)); 
          
            memmove(set->full_symbol_tab+set->first_symbol_id-full_symbol_cloned->first_nonzero_id,set->full_symbol_tab, sizeof(swif_full_symbol_t*) * old_size);
        }
        full_symbol_set_dump(set, stdout);
        full_symbol_set_dump(set, stdout);
        if (set->full_symbol_tab == NULL) {
            free(set->full_symbol_tab);
            return SYMBOL_ID_NONE;
        }
        set->full_symbol_tab[0] = full_symbol_cloned ; //set->first_symbol_id-full_symbol_cloned->first_nonzero_id
        set->nmbr_packets++;
        set->first_symbol_id = calculate_min(set->first_symbol_id, full_symbol_cloned->first_nonzero_id) ;
        return set->first_symbol_id-full_symbol_cloned->first_nonzero_id ; 
    }

    DEBUG_PRINT("Debugging is enabled. Case: full_symbol_cloned->first_nonzero_id > set->first_symbol_id \n");
    if ( full_symbol_cloned->first_nonzero_id-set->first_symbol_id < set->size  ){
        set->full_symbol_tab[full_symbol_cloned->first_nonzero_id-set->first_symbol_id] = full_symbol_cloned ;
        set->nmbr_packets++ ;
        return full_symbol_cloned->first_nonzero_id-set->first_symbol_id; 
    }else if (full_symbol_cloned->first_nonzero_id-set->first_symbol_id < (set->size *2) ){
        set->size *= 2;
    }else{
        set->size = full_symbol_cloned->last_nonzero_id-set->first_symbol_id +1;
    }
    // allocate the table of pointers to full_symbol
    set->full_symbol_tab =realloc(set->full_symbol_tab , set->size * sizeof(swif_full_symbol_t *)); 
    memset(set->full_symbol_tab+old_size,0, sizeof(swif_full_symbol_t*)*(set->size-old_size)  );

    if (set->full_symbol_tab == NULL) {
        free(set->full_symbol_tab);
        return SYMBOL_ID_NONE;
    }

    set->full_symbol_tab[(full_symbol_cloned->first_nonzero_id-set->first_symbol_id)] = full_symbol_cloned ; 
    set->nmbr_packets++;
    return (full_symbol_cloned->first_nonzero_id-set->first_symbol_id) ; 
}

/*---------------------------------------------------------------------------*/
swif_full_symbol_t *full_symbol_set_get_pivot(swif_full_symbol_set_t *full_symbol_set, symbol_id_t symbol_id)
{ 
    full_symbol_set_dump(full_symbol_set,stdout);
    if (symbol_id >= full_symbol_set->first_symbol_id && symbol_id < (full_symbol_set->size + full_symbol_set->first_symbol_id) && full_symbol_set->full_symbol_tab[symbol_id-full_symbol_set->first_symbol_id] ){
        DEBUG_PRINT("Coef of pivot is equal to : %u \n", full_symbol_get_coef(full_symbol_set->full_symbol_tab[symbol_id-full_symbol_set->first_symbol_id], symbol_id));
        return full_symbol_set->full_symbol_tab[symbol_id-full_symbol_set->first_symbol_id];
    }  
    DEBUG_PRINT("Pivot of symbol id %u is not found \n", symbol_id);
    return NULL; 
}
/*---------------------------------------------------------------------------*/
swif_full_symbol_t *full_symbol_set_remove_each_pivot(swif_full_symbol_set_t *full_symbol_set, swif_full_symbol_t *new_symbol1)
{   // to not have a free pb because of full_symbol_free(new_symbol);     
    swif_full_symbol_t *new_symbol = full_symbol_clone(new_symbol1);

    bool isNull = true ; 
    full_symbol_adjust_min_max_coef(new_symbol);
    if(new_symbol->first_nonzero_id == SYMBOL_ID_NONE)
        return SYMBOL_ID_NONE; 
    
    for (uint32_t i = new_symbol->first_nonzero_id ; i<= new_symbol->last_nonzero_id; i++)
    {
        uint8_t coef = full_symbol_get_coef(new_symbol,i); 
        if (coef != 0)
        {
            if (full_symbol_set_get_pivot(full_symbol_set,i)){ // i was coef
                swif_full_symbol_t * symbol1 = full_symbol_set_get_pivot(full_symbol_set,i); // i was coef
                swif_full_symbol_t *symbol1_cloned = full_symbol_clone(symbol1);
                full_symbol_scale(symbol1_cloned,coef);
                swif_full_symbol_t * symbol2 = full_symbol_add(new_symbol,symbol1_cloned);
                /* added to exclude symbol null */
                if(!full_symbol_is_zero(symbol2)){
                    full_symbol_free(new_symbol);
                    new_symbol =  symbol2;
                }
                if (new_symbol->coef[i] != NULL){
                    isNull= false;
                }
            }else{
                isNull= false; 
            }
        }
        
    }
    if (isNull){
        full_symbol_free(new_symbol);
        return NULL; 
    }
    return  new_symbol ; 
}
  
/*---------------------------------------------------------------------------*/
void full_symbol_set_add_as_pivot(swif_full_symbol_set_t *full_symbol_set, swif_full_symbol_t *new_symbol){
    full_symbol_adjust_min_max_coef(new_symbol);
    if (new_symbol->first_nonzero_id == SYMBOL_ID_NONE){
        return ;
    }
    full_symbol_scale(new_symbol, gf256_inv(full_symbol_get_coef(new_symbol,new_symbol->first_nonzero_id))); // full_symbol_get_coef(new_symbol,new_symbol->first_nonzero_id) au lieu de new_symbol->first_nonzero_id
    symbol_id_t first_index = new_symbol->first_nonzero_id; 
    
    for(uint32_t i = 0 ; i< full_symbol_set->size; i++){
        
        if (full_symbol_set->full_symbol_tab[i]){
            swif_full_symbol_t *symbol_cloned=full_symbol_clone(new_symbol);
            //uint8_t coef = full_symbol_get_coef(full_symbol_set->full_symbol_tab[i], full_symbol_set->full_symbol_tab[i]->first_nonzero_id);
            uint8_t coef = full_symbol_get_coef(full_symbol_set->full_symbol_tab[i], first_index);
            full_symbol_scale(symbol_cloned, coef);
            swif_full_symbol_t * symbol2 = full_symbol_add(full_symbol_set->full_symbol_tab[i],symbol_cloned);                  
            full_symbol_free(full_symbol_set->full_symbol_tab[i]);
            full_symbol_set->full_symbol_tab[i] = symbol2;
            full_symbol_free(symbol_cloned);
        }
    }
    full_symbol_dump(new_symbol, stdout);
    uint32_t position = full_symbol_set_add(full_symbol_set, new_symbol );
}


/*---------------------------------------------------------------------------*/
void full_symbol_add_with_elimination(swif_full_symbol_set_t *full_symbol_set, swif_full_symbol_t *new_symbol){
    //swif_full_symbol_set_t *myset = full_symbol_set_alloc();
    ///////full_symbol_set_dump(full_symbol_set, stdout);
    //uint32_t fss_added = full_symbol_set_add(full_symbol_set, new_symbol)
    swif_full_symbol_t *fss_remove_pivot= full_symbol_set_remove_each_pivot(full_symbol_set, new_symbol);
    if(fss_remove_pivot)
        full_symbol_set_add_as_pivot(full_symbol_set, fss_remove_pivot);
}
/*---------------------------------------------------------------------------*/
/**
 * @brief Create a full_symbol from a raw packet (a set of bytes)
 *        and initialize it with content '0'
 */
swif_full_symbol_t *full_symbol_alloc(symbol_id_t first_symbol_id, symbol_id_t last_symbol_id, uint32_t symbol_size) // data_size == symbol_size
{ 
    symbol_id_t symbol_id_size;
    if (first_symbol_id == SYMBOL_ID_NONE) {
        assert(last_symbol_id == SYMBOL_ID_NONE);
        symbol_id_size = 1; /* Actually 0, but we never want a NULL pointer, 
                               and malloc(0) might be NULL or not */
    } else {
        assert(first_symbol_id <= last_symbol_id);
        symbol_id_size = last_symbol_id-first_symbol_id+1;
    }

    swif_full_symbol_t *result
        = (swif_full_symbol_t *)calloc(1, sizeof(swif_full_symbol_t));
    if (result == NULL) {
        free(result);
        return NULL;
    }
    /* allocate coef and data */
     uint8_t *coef
        = (swif_full_symbol_t *)calloc(symbol_id_size , sizeof(uint8_t));
    if (coef == NULL) {
        /* deallocate in case of problem */
        free(result);
        return NULL; 
    }
    result->coef = coef ;

     uint8_t *data
        = (swif_full_symbol_t *)calloc(symbol_size, sizeof(uint8_t));
    if (data == NULL) {
        // deallouer en cas de pb
        free(result->coef);
        free(result);
        return NULL;

    }
    result->data = data ;
    result->first_id = first_symbol_id;
    result->last_id = last_symbol_id;
    result->data_size = symbol_size;
    full_symbol_adjust_min_max_coef(result);

    return result;

}


/**
 * @brief get the coefficient corresponding to the specified symbol identifier
 */
uint8_t full_symbol_get_coef
(swif_full_symbol_t *full_symbol, uint32_t symbol_id)
{
    if (full_symbol_includes_id(full_symbol , symbol_id)){
        return full_symbol->coef[symbol_id-full_symbol->first_id];
    }
    return 0; 
    
}


symbol_id_t full_symbol_get_coef_index
(swif_full_symbol_t *full_symbol, uint32_t symbol_id)
{                              

    assert(full_symbol_includes_id(full_symbol , symbol_id));
    return symbol_id-full_symbol->first_id;
    
}

/**
 * @brief Create a full_symbol from a raw packet (a set of bytes)
 *        and initialize it with content '0'
 */
swif_full_symbol_t *full_symbol_create_from_source
(uint32_t symbol_id, uint8_t *symbol_data, uint32_t symbol_size)
{
    swif_full_symbol_t *full_symbol = full_symbol_alloc( symbol_id,  symbol_id, symbol_size);
    symbol_id_t coef_index = full_symbol_get_coef_index( full_symbol, symbol_id);
    full_symbol->coef[coef_index]=1;
    full_symbol_adjust_min_max_coef(full_symbol);
    memcpy(full_symbol->data, symbol_data, symbol_size);  
    full_symbol->data_size = symbol_size;   
    return full_symbol;
}

swif_full_symbol_t *full_symbol_create
(uint8_t* symbol_coef_table, uint32_t min_symbol_id, uint32_t nb_symbol_id,
 uint8_t* symbol_data, uint32_t symbol_size)
{
    swif_full_symbol_t *full_symbol = full_symbol_alloc(min_symbol_id, min_symbol_id+nb_symbol_id-1 , symbol_size);
    //full_symbol->coef = symbol_coef_table;
    memcpy(full_symbol->coef, symbol_coef_table, nb_symbol_id * sizeof(uint8_t));
    full_symbol->first_id = min_symbol_id;
    full_symbol->last_id = min_symbol_id+nb_symbol_id-1;
    full_symbol_adjust_min_max_coef(full_symbol);
    //full_symbol->data = symbol_data;
    memcpy(full_symbol->data, symbol_data,  symbol_size);   
    full_symbol->data_size = symbol_size;  

    return full_symbol;
}

/**
 * @brief Release a full_symbol
 */
void full_symbol_free(swif_full_symbol_t* full_symbol)
{    
    assert(full_symbol != NULL);
    assert(full_symbol->coef != NULL);
    free(full_symbol->coef);
    full_symbol->coef = NULL;
    assert(full_symbol->data != NULL);
    free(full_symbol->data);
    full_symbol->data = NULL;
    free(full_symbol);
    
}

/**
 * @brief Create a new (unlinked) copy of a full_symbol
 */
swif_full_symbol_t *full_symbol_clone(swif_full_symbol_t* full_symbol)
{
    //allouer et copier
    swif_full_symbol_t *result = full_symbol_alloc( full_symbol->first_nonzero_id,  full_symbol->last_nonzero_id , full_symbol->data_size);

    memcpy(result->coef, full_symbol->coef, (full_symbol->last_id - full_symbol->first_id +1 ) * sizeof(uint8_t));
    result->first_id = full_symbol->first_id;
    result->last_id = full_symbol->last_id;
    result->first_nonzero_id = full_symbol->first_nonzero_id;
    result->last_nonzero_id = full_symbol->last_nonzero_id;
    result->data_size = full_symbol->data_size;
    memcpy(result->data, full_symbol->data, full_symbol->data_size * sizeof(uint8_t));
    return result;
}

/**
 * @brief get the size of the data
 */
uint32_t full_symbol_get_size(swif_full_symbol_t *full_symbol)
{
    return full_symbol->data_size; 
}




static inline bool full_symbol_has_sufficient_size(swif_full_symbol_t* symbol,
                                           symbol_id_t id1, symbol_id_t id2)
{ 
    assert(id1 <= id2);
    symbol_id_t symbol_id_size = full_symbol_count_coef(symbol);
    symbol_id_t requested_size = id2 - id1 + 1;
    return (symbol_id_size >= requested_size); 
}

static inline bool full_symbol_includes_id(swif_full_symbol_t* symbol,
                                           symbol_id_t symbol_id)
{
    return (symbol_id >= symbol->first_id && symbol_id <= symbol->last_id );
}

// adjust full symbol min coef


static bool full_symbol_adjust_min_coef(swif_full_symbol_t* symbol)
{
    assert( symbol->first_id != SYMBOL_ID_NONE
	    && symbol->last_id != SYMBOL_ID_NONE );

    bool result = false;
    symbol->first_nonzero_id = SYMBOL_ID_NONE;

    for (symbol_id_t  i=symbol->first_id ;i<=symbol->last_id; i++) {
        if (full_symbol_get_coef(symbol, i) != 0){
            symbol->first_nonzero_id = i;
            result = true;
            break;
        }
    }
    return result;
}


static inline uint32_t full_symbol_count_allocated_coef(swif_full_symbol_t *full_symbol)
{
    if (full_symbol->first_nonzero_id == SYMBOL_ID_NONE) { //  first_id
        assert(full_symbol->last_nonzero_id == SYMBOL_ID_NONE); // last_id
        return 0;
    } else {
        assert(full_symbol->first_id <= full_symbol->last_id);
        return full_symbol->last_id-full_symbol->first_id+1;
    }
}


// adjust full symbol max coef

static bool full_symbol_adjust_max_coef(swif_full_symbol_t* symbol)
{
    assert( symbol->first_id != SYMBOL_ID_NONE
	    && symbol->last_id != SYMBOL_ID_NONE );

    bool result = false;
    symbol->last_nonzero_id = SYMBOL_ID_NONE;

    //for (symbol_id_t  i=symbol->last_id ;i>=symbol->first_id; i--) {
    for (symbol_id_t  j=0; j<full_symbol_count_allocated_coef(symbol); j++) {    
        symbol_id_t  i = symbol->last_id-j;    
         if (full_symbol_get_coef(symbol, i) != 0){
            symbol->last_nonzero_id = i;
            result = true;
            break;
        }
    }
    return result;
}




// adjust full symbol min and max coef
// returns whether there exists non-zero coefs
bool full_symbol_adjust_min_max_coef(swif_full_symbol_t* symbol)
{
    if (symbol->first_id == SYMBOL_ID_NONE) {
        assert( symbol->last_id == SYMBOL_ID_NONE );
        symbol->first_nonzero_id == SYMBOL_ID_NONE;
        symbol->last_nonzero_id == SYMBOL_ID_NONE;
        return false;
    }
    
    bool result1 = full_symbol_adjust_min_coef(symbol);
    bool result2 = full_symbol_adjust_max_coef(symbol);
    assert(result1 == result2);

    return result1;
}



/**
 * @brief get the minimum source index that appears in the symbol
 *        SYMBOL_ID_NONE if there is none (e.g. symbol is 0)
 */
uint32_t full_symbol_get_min_symbol_id(swif_full_symbol_t *full_symbol)
{
    //full_symbol_adjust_min_max_coef(full_symbol);
    return full_symbol->first_nonzero_id; 
}

/**
 * @brief get the maximum source index that appears in the symbol
 *        returns SYMBOL_ID_NONE if there is none (e.g. symbol is 0)
 */
uint32_t full_symbol_get_max_symbol_id(swif_full_symbol_t *full_symbol)
{
    //full_symbol_adjust_min_max_coef(full_symbol);
    return full_symbol->last_nonzero_id; 
}


/**
 * @brief get the symbol 'data'. result_data should be a pointer to a
 *        a block of memory of full_symbol_get_size(full_symbol)
 */
void full_symbol_get_data(swif_full_symbol_t *full_symbol, uint8_t *result_data)
{
    assert( result_data != NULL);
    memcpy(result_data, full_symbol->data, full_symbol->data_size * sizeof(uint8_t));
}

void full_symbol_dump_id(symbol_id_t symbol_id, FILE *out)
{
    if (symbol_id == SYMBOL_ID_NONE) {
        fprintf(out, "null");
    } else {
        fprintf(out, "%u", symbol_id);
    }
}

void full_symbol_dump(swif_full_symbol_t *full_symbol, FILE *out)
{ 
    fprintf(out, "{'size':%u, 'internal':", full_symbol->data_size);
    fprintf(out, "{ 'type':'full_symbol'");
    fprintf(out, ", 'nb_coef':%u", full_symbol_count_allocated_coef(full_symbol));
    fprintf(out, ", 'data_size': %u", full_symbol->data_size);

    fprintf(out, ", 'first_id':");
    full_symbol_dump_id(full_symbol->first_id, out);
    fprintf(out, ", 'last_id':");
    full_symbol_dump_id(full_symbol->last_id, out);
    fprintf(out, ", 'first_nonzero_id':");
    full_symbol_dump_id(full_symbol->first_nonzero_id, out);
    fprintf(out, ", 'last_nonzero_id':");
    full_symbol_dump_id(full_symbol->last_nonzero_id, out);

    fprintf(out, ", 'coef_value':[");
    uint32_t i;
    if ( full_symbol->first_id != SYMBOL_ID_NONE) {
        for (i=full_symbol->first_id; i<=full_symbol->last_id; i++) {
            if (i > full_symbol->first_id)
                fprintf(out, ", ");
        fprintf(out, "%u", full_symbol_get_coef(full_symbol,i));
        }
    }
    fprintf(out, "]");
    fprintf(out, ", 'data':");
    fprintf(out, "'");
    for (i=0; i<full_symbol->data_size; i++)
        fprintf(out, "\\x%02x", full_symbol->data[i]);
    fprintf(out, "'");
    fprintf(out, " }");
    fprintf(out, "}");  
    fprintf(out,"\n");
}

/*---------------------------------------------------------------------------*/

/**
 * @brief Take a symbol and add another symbol multiplied by a 
 *        coefficient, e.g. performs the equivalent of: p1 += coef * p1
 * @param[in,out] p1     First symbol (to which coef*p2 will be added)
 * @param[in]     coef  Coefficient by which the second packet is multiplied
 */
void full_symbol_scale(swif_full_symbol_t *symbol1, uint8_t coef)
{
    uint8_t *result;
    assert(symbol1->coef != NULL);
    assert(symbol1->data != NULL);
    if (symbol1->first_id == SYMBOL_ID_NONE || symbol1->first_nonzero_id == SYMBOL_ID_NONE) {
        return; /* empty/zero packet */
    }
    assert(symbol1->first_nonzero_id != SYMBOL_ID_NONE && symbol1->last_nonzero_id != SYMBOL_ID_NONE);
    symbol_mul(symbol1->data, coef, symbol1->data_size, symbol1->data);
    symbol_mul(symbol1->coef, coef, full_symbol_count_allocated_coef(symbol1), symbol1->coef);
    full_symbol_adjust_min_max_coef(symbol1); // because can be 0
    //swif_full_symbol_t *symbol_result = full_symbol_create(symbol1->coef, symbol1->first_nonzero_id, full_symbol_count_coef(symbol1) , result, symbol1->data_size);
    //swif_full_symbol_t *symbol2 = full_symbol_clone(symbol1);
    //full_symbol_add_base(symbol2, symbol_result ,symbol1);
}


/**
 * @brief Take a symbol and add another symbol to it, e.g. performs the equivalent of: p3 = p1 + p2
 * @param[in] p1     First symbol (to which p2 will be added)
 * @param[in] p2     Second symbol
 * @param[in] p3     result XXX
 * 
 * symbol1, symbol2 should not be zero
 * symbol_result has pre-set first_id, last_id
 */
void full_symbol_add_base(swif_full_symbol_t *symbol1, swif_full_symbol_t *symbol2, swif_full_symbol_t *symbol_result)
{
    // assert (symbol1->data != NULL && symbol2->data != NULL .... )
    assert (symbol1->data != NULL && symbol2->data != NULL && symbol_result->data != NULL);
    // assert (symbol_result->data_size >= symbol2->data_size);
    assert (symbol_result->data_size >= symbol1->data_size && symbol_result->data_size >= symbol2->data_size);

    // assert (symbol2->first_nonzero_id -> is included in encoding header symbol_result);
    // assert (symbol2->last_nonzero_id -> is included in encoding header symbol_result));
    assert (full_symbol_includes_id(symbol_result, symbol1->first_nonzero_id));
    assert (full_symbol_includes_id(symbol_result, symbol1->last_nonzero_id));
    assert (full_symbol_includes_id(symbol_result, symbol2->first_nonzero_id));
    assert (full_symbol_includes_id(symbol_result, symbol2->last_nonzero_id));

    uint32_t first_coef_index;
    uint32_t last_coef_index;

    // XXX: should not be NONE
    if (symbol1->first_nonzero_id == SYMBOL_ID_NONE && symbol2->first_nonzero_id == SYMBOL_ID_NONE ){
        symbol_result->first_nonzero_id = SYMBOL_ID_NONE;
        symbol_result->last_nonzero_id = SYMBOL_ID_NONE;
    }
    if ( symbol1->first_nonzero_id <= symbol2->first_nonzero_id){
         first_coef_index = symbol1->first_nonzero_id ;
    }
    else {
         first_coef_index = symbol2->first_nonzero_id ;
    }

    if ( symbol1->last_nonzero_id >= symbol2->last_nonzero_id){
         last_coef_index = symbol1->last_nonzero_id ;
    }
    else {
         last_coef_index = symbol2->last_nonzero_id ;
    }

    memset(symbol_result->coef , 0, sizeof(uint8_t)*(first_coef_index-symbol_result->first_id));
    memset(symbol_result->coef+(last_coef_index+1-symbol_result->first_id) , 0 , 
        sizeof(uint8_t)*(symbol_result->last_id-last_coef_index));

    for (uint32_t i = first_coef_index; i <= last_coef_index; i++){
        //memset(symbol_result->coef+(i-symbol_result->first_id) , full_symbol_get_coef(symbol1, i) ^ full_symbol_get_coef(symbol2, i) , sizeof(uint8_t));
        symbol_result->coef[i-symbol_result->first_id] = full_symbol_get_coef(symbol1, i) ^ full_symbol_get_coef(symbol2, i);
        //symbol_result->coef[i] = full_symbol_get_coef(symbol1, i) ^ full_symbol_get_coef(symbol2, i);
    }

    full_symbol_adjust_min_max_coef(symbol_result);

    // get data
    //symbol_add((void *)symbol1->data, (void *)symbol2->data, full_symbol_get_size(symbol_result), (uint8_t * )symbol_result->data); 

    if(symbol1->data_size >= symbol2->data_size ){
        symbol_add((void *)symbol1->data, (void *)symbol2->data, symbol2->data_size, (uint8_t * )symbol_result->data); 
        memcpy(symbol_result->data+(symbol2->data_size), symbol1->data+(symbol2->data_size) , symbol1->data_size - (symbol2->data_size));
        memset(symbol_result->data+(symbol1->data_size), 0, symbol_result->data_size - (symbol1->data_size));
    }
    else {
        symbol_add((void *)symbol1->data, (void *)symbol2->data, symbol1->data_size, (uint8_t * )symbol_result->data); 
        memcpy(symbol_result->data+symbol1->data_size, symbol2->data+(symbol1->data_size) , symbol2->data_size - (symbol1->data_size));
        memset(symbol_result->data+symbol2->data_size, 0, symbol_result->data_size - (symbol2->data_size));
    }
}



swif_full_symbol_t* full_symbol_add(swif_full_symbol_t *symbol1, swif_full_symbol_t *symbol2)
{
    uint32_t first_coef_index;
    uint32_t last_coef_index;
    if (full_symbol_is_zero(symbol1) && full_symbol_is_zero(symbol2)) {
        /* return 0 */
        return full_symbol_alloc(SYMBOL_ID_NONE, SYMBOL_ID_NONE, 0);
    }
    if (full_symbol_is_zero(symbol1)) {
        return full_symbol_clone(symbol2);
    }
    if (full_symbol_is_zero(symbol2)) {
        return full_symbol_clone(symbol1);
    }
    if ( symbol1->first_nonzero_id <= symbol2->first_nonzero_id){
         first_coef_index = symbol1->first_nonzero_id ;
    }
    else {
         first_coef_index = symbol2->first_nonzero_id ;
    }

    if ( symbol1->last_nonzero_id >= symbol2->last_nonzero_id){
         last_coef_index = symbol1->last_nonzero_id ;
    }
    else {
         last_coef_index = symbol2->last_nonzero_id ;
    }

    uint32_t data_size = (symbol1->data_size >= symbol2->data_size) 
                        ? symbol1->data_size : symbol2->data_size;
    //uint8_t content[data_size]; 
    //memset( content, 0, dataSize); 
    //uint8_t coef[last_coef_index-first_coef_index+1]; 
    //memset( coef, 0, last_coef_index-first_coef_index+1); 

    //swif_full_symbol_t *symbol_result = full_symbol_create(coef, first_coef_index, last_coef_index-first_coef_index+1, content, dataSize);

    swif_full_symbol_t *symbol_result = full_symbol_alloc(first_coef_index, last_coef_index,
        data_size);

    full_symbol_add_base(symbol1, symbol2, symbol_result);
    return symbol_result; 
}



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













