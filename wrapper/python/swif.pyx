#---------------------------------------------------------------------------
# swif-codec - 2019
#---------------------------------------------------------------------------

cimport libc.stdio as stdio
from libc.stdlib cimport malloc, free

cimport cswif
from cswif cimport *
from libc.stdint cimport uint8_t, uint32_t, int64_t, bool
from libc.stdlib cimport malloc, free
from libc.string cimport memcpy, memset

#---------------------------------------------------------------------------

cdef check_swif_status(swif_status, swif_errno):
    if swif_status == SWIF_STATUS_ERROR:
        raise RuntimeError("SWIF Error", swif_errno) #XXX

cdef uint8_t* memclone(uint8_t* data, int data_size):
    cdef uint8_t* result = <uint8_t*> malloc(data_size)
    memcpy(result, data, data_size)
    return result

VERBOSITY = 2

cdef class RlcEncoder:
    cdef cswif.swif_encoder_t *encoder
    cdef symbol_size # XXX: replicated for convenience

    def __cinit__(self, symbol_size, max_coding_window_size):
        self.encoder = swif_encoder_create(
            SWIF_CODEPOINT_RLC_GF_256_FULL_DENSITY_CODEC,
            VERBOSITY, symbol_size, max_coding_window_size)
        self.symbol_size = symbol_size
        if self.encoder is NULL:
            raise RuntimeError("SWIF Error", "encoder_create returned NULL")#XXX
	#if (swif_encoder_set_callback_functions(ses, source_symbol_removed_from_coding_window_callback, NULL) != SWIF_STATUS_OK) {

    def add_source_symbol_to_coding_window(self, uint8_t *symbol, symbol_id):
        assert self.encoder is not NULL
        cdef uint8_t* cloned_symbol = memclone(symbol, self.symbol_size)
        status = swif_encoder_add_source_symbol_to_coding_window(
            self.encoder, cloned_symbol, symbol_id)
        check_swif_status(status, self.encoder.swif_errno)        

    def build_repair_symbol(self):
        assert self.encoder is not NULL
        result = bytes(self.symbol_size)
        cdef uint8_t *data = result
        status = swif_build_repair_symbol(self.encoder, data)
        check_swif_status(status, self.encoder.swif_errno)
        return result

    def generate_coding_coefs(self, key, add_param):
        status = swif_encoder_generate_coding_coefs(
            self.encoder, key, add_param)
        check_swif_status(status, self.encoder.swif_errno)        

    cpdef release(self):
        if self.encoder is NULL:
            return
        status = swif_encoder_release(self.encoder)
        check_swif_status(status, self.encoder.swif_errno)
        self.encoder = NULL

    def __dealloc__(self):
        if self.encoder is not NULL:
            status = swif_encoder_release(self.encoder)
            self.encoder = NULL

#---------------------------------------------------------------------------

cdef class RlcDecoder:
    cdef cswif.swif_decoder_t *decoder
    cdef symbol_size # XXX: replicated for convenience

    def __cinit__(self, symbol_size, max_coding_window_size):
        self.decoder = swif_decoder_create(
            SWIF_CODEPOINT_RLC_GF_256_FULL_DENSITY_CODEC,
            0, symbol_size, max_coding_window_size, 2*max_coding_window_size)
        self.symbol_size = symbol_size
        if self.decoder is NULL:
            raise RuntimeError("SWIF Error", "encoder_create returned NULL")#XXX

#---------------------------------------------------------------------------

cdef class Symbol:
    cdef public uint8_t *data
    cdef public uint32_t size

    def __cinit__(self, init_data=None):
        self.data = NULL
        self.size = 0
        if init_data is not None:
            self.set_data(init_data)

    def get_data(self):
        if self.size == 0:
            return b""
        result = bytes(self.size)
        cdef uint8_t *data = result        
        memcpy(data, self.data, self.size)
        return result
        
    cpdef alloc(self, new_size):
        if self.data is not NULL:
            self.dealloc()
        assert new_size>=0
        self.data = <uint8_t*>malloc(new_size)
        memset(self.data, 0, new_size)
        if self.data is NULL and new_size>0:
            self.size = 0
            raise RuntimeError("cannot malloc", new_size)
        self.size = new_size

    cpdef dealloc(self):
        if self.data is NULL:
            return
        free(self.data)
        self.data = NULL
        self.size = 0

    def set_data(self, data):
        self.dealloc()
        self.alloc(len(data))
        assert len(data) == self.size        
        cdef uint8_t *u8_data = data
        memcpy(self.data, u8_data, len(data))

    def __dealloc__(self):
        if self.data is not NULL:
            free(self.data)
            self.data = NULL
            self.size = 0

    def add(self, other):
        assert self.size == other.size
        result = Symbol()
        result.alloc(self.size)
        cdef uint8_t *other_data = other.data
        symbol_add(self.data, other_data, self.size, result.data)
        return result

    def mul(self, coef):
        result = Symbol()
        result.alloc(self.size)
        symbol_mul(self.data, coef, self.size, result.data)
        return result

    def div(self, coef):
        result = Symbol()
        result.alloc(self.size)
        symbol_div(self.data, self.size, coef, result.data)
        return result
    
    def sub(self, other):
        assert self.size == other.size
        result = Symbol()
        result.alloc(self.size)
        cdef uint8_t *other_data = other.data
        symbol_sub(self.data, other_data, self.size, result.data)
        return result

    def __add__(self, other):
        return self.add(other)

    def __sub__(self, other):
        return self.sub(other)

    def __mul__(v1, v2):
        if isinstance(v1, Symbol):
            return v1.mul(v2)
        else:
            return v2.mul(v1)

    def __truediv__(self, coef):
        return self.div(coef)
    
    def __repr__(self):
        #return "Symbol("+repr(self.get_data())+")"
        return "Symbol("+repr([x for x in self.get_data()])+")"

    def copy(self):
        return Symbol(self.get_data())

#---------------------------------------------------------------------------

cdef class FullSymbol:
    cdef swif_full_symbol_t* symbol

    def __init__(self):
        self.symbol = NULL

    cpdef from_source_symbol(self, symbol_id, content):
        self.release()
        self.symbol = full_symbol_create_from_source(
            symbol_id, content, len(content))
        return self

    cpdef from_coefs_and_symbol(self, first_symbol_id,
                                symbol_id_table, content):
        self.release()
        self.symbol = full_symbol_create(
            symbol_id_table, first_symbol_id, len(symbol_id_table),
            content, len(content))
        return self

    cpdef from_other(self, FullSymbol other):
        self.release()
        self.symbol = full_symbol_clone(other.symbol)
        return self

    cpdef is_zero(self):
        assert self.symbol is not NULL    
        return full_symbol_is_zero(self.symbol)
    
    cpdef get_size(self):
        assert self.symbol is not NULL
        return full_symbol_get_size(self.symbol)
    
    cpdef get_min_symbol_id(self):
        assert self.symbol is not NULL
        return full_symbol_get_min_symbol_id(self.symbol)

    cpdef get_max_symbol_id(self):
        assert self.symbol is not NULL
        return full_symbol_get_max_symbol_id(self.symbol)

    cpdef count_coefs(self):
        assert self.symbol is not NULL    
        return full_symbol_count_coef(self.symbol)

    cpdef get_coef(self, symbol_id):
        assert self.symbol is not NULL
        return full_symbol_get_coef(self.symbol, symbol_id)

    def get_coefs(self):
        if self.is_zero():
            return (0, b"")
        min_id = self.get_min_symbol_id()
        nb_coefs = self.count_coefs()
        content = bytes([self.get_coef(min_id+i) for i in range(nb_coefs)])
        return min_id, content

    cpdef get_data(self):
        assert self.symbol is not NULL
        symbol_size = self.get_size()
        result = bytes(symbol_size)
        full_symbol_get_data(self.symbol, result)
        return result

    cpdef clone(self):
        assert self.symbol is not NULL    
        return FullSymbol().from_other(self)

    cpdef release(self):
        if self.symbol is NULL:
            return
        full_symbol_free(self.symbol)
        self.symbol = NULL

    def __dealloc__(self):
        if self.symbol is not NULL:
            full_symbol_free(self.symbol)
            self.symbol = NULL

    cpdef get_info(self):
        assert self.symbol is not NULL
        #if not self.is_zero():
        return self.get_coefs()+(self.get_data(),)
        #else:
        #    return (0, b"", self.get_data())

    cpdef dump(self):
        assert self.symbol is not NULL
        return full_symbol_dump(self.symbol, stdio.stdout) 
    
    cpdef _add_base(self, FullSymbol other1, FullSymbol other2):
        return full_symbol_add_base(other1.symbol, other2.symbol, self.symbol)

    cpdef add(self, FullSymbol other):
        result_symbol = full_symbol_add(self.symbol, other.symbol)
        result = FullSymbol()
        assert result.symbol is NULL
        result.symbol = result_symbol
        return result
        
    cpdef _scale(self, coef):
        full_symbol_scale(self.symbol, coef)
        return self
    
    cpdef _scale_inv(self, coef):
        full_symbol_scale(self.symbol, gf256_inv(coef))
        return self

#---------------------------------------------------------------------------


cdef class FullSymbolSet:

    cdef swif_full_symbol_set_t* symbol_set

    def __init__(self):
        self.symbol_set = NULL
    
    cpdef release_set(self):
        if self.symbol_set is NULL:
            return
        full_symbol_set_free(self.symbol_set)
        self.symbol_set = NULL

    def __dealloc__(self):
        if self.symbol_set is not NULL:
            full_symbol_set_free(self.symbol_set)
            self.symbol_set = NULL

    cpdef alloc_set(self):
        #self.release_set()
        #self.symbol_set = full_symbol_set_alloc()
        #return self

        result_symbol = full_symbol_set_alloc()
        result = FullSymbolSet()
        assert result.symbol_set is NULL
        result.symbol_set = result_symbol
        return result

    cpdef set_add(self, FullSymbol other):
        return full_symbol_set_add(self.symbol_set, other.symbol)
    
    cpdef dump(self):
        assert self.symbol_set is not NULL
        return full_symbol_set_dump(self.symbol_set, stdio.stdout) 
    
    def get_pivot(self, symbol_id):
        assert self.symbol_set is not NULL
        full_symbol_set_get_pivot(self.symbol_set, symbol_id) 
        return self
    
    def remove_each_pivot(self,  FullSymbol new_symbol):
        assert self.symbol_set is not NULL
        assert new_symbol.symbol is not NULL
        full_symbol_set_remove_each_pivot(self.symbol_set, new_symbol.symbol)
        return self

    def add_as_pivot(self,  FullSymbol new_symbol):
        assert self.symbol_set is not NULL
        assert new_symbol.symbol is not NULL
        full_symbol_set_add_as_pivot(self.symbol_set, new_symbol.symbol)
        return self

    def add_with_elimination(self,  FullSymbol new_symbol):
        assert self.symbol_set is not NULL
        assert new_symbol.symbol is not NULL
        full_symbol_add_with_elimination(self.symbol_set, new_symbol.symbol)
        return self
    
#---------------------------------------------------------------------------
