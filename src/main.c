/**
 * Copyright (c) 2022 murilo
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#define PYW_SSIZE_T_CLEAN
#include <Python.h>

#include "include/common.h"
#include "include/pydef.h"
#include "include/rdrand.h"


/*
 Direct access methods 
*/
static PyObject* rdrand32(PyObject* self) {
    /* Return a uint32_t random. Zero return indicates a underflow */
    uint32_t randf;
    int status = rdrand_get_uint32_retry(10, &randf);
    
    if (status == 1)
        return PyLong_FromLong(randf);
    return PyLong_FromLong(0);
}

/*
 Indirect access methods 
*/
static PyObject* rdrand64(PyObject* self) {
    uint64_t randf;
    generate_rdrand64(&randf);
    return PyLong_FromUnsignedLongLong(randf);
}

static PyObject* randbits(PyObject* self, PyObject* args) {
    
    int k, i, words;
    uint32_t randf;
    uint32_t *wordarray;
    PyObject *result;


    if (!PyArg_ParseTuple(args, "i", &k))
        return NULL;

    if (k < 0) {
        PyGILState_STATE gstate = PyGILState_Ensure();
        PyErr_SetString(PyExc_ValueError, "Number of bits must be non-negative.\n");
        PyGILState_Release(gstate);

        return NULL;
    }

    if (k == 0)
        return PyLong_FromLong(0);

    if (k <= 32) {
        rdrand_get_uint32_retry(10, &randf);
        return PyLong_FromUnsignedLong(randf >> (32 - k));
    }

    words = (k - 1) / 32 + 1;
    wordarray = (uint32_t *) PyMem_Malloc(words * 4);
    
    if (wordarray == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

#if PY_LITTLE_ENDIAN
    for (i = 0; i < words; i++, k -= 32)
#else
    for (i = words -1; i >= 0; i--, k-= 32)
#endif
    {
        rdrand_get_uint32_retry(10, &randf);
        if (randf < 32)
            randf >>= (32 - k);

        wordarray[i] = randf;
    }
    result = _PyLong_FromByteArray((unsigned char*)wordarray, words * 4, PY_LITTLE_ENDIAN, 0);
    PyMem_Free(wordarray);
    return result;
}


static PyListObject* n_range_below(PyObject* self, PyObject* args) {

    int length, below, amount;

    if (!PyArg_ParseTuple(args, "iii", &length, &below, &amount)) {
        return NULL;
    }

    if (amount < 0) {
        PyGILState_STATE gstate = PyGILState_Ensure();
        PyErr_SetString(PyExc_ValueError, "Amount must be positive");
        PyGILState_Release(gstate);

        return NULL;
    }

    if (length > below) {
        PyGILState_STATE gstate = PyGILState_Ensure();
        PyErr_SetString(PyExc_ValueError, "Length must be smaller than 'below', because this method will return a non-repeated values.");
        PyGILState_Release(gstate);

        return NULL;
    }

    if (length < 0) {

        PyGILState_STATE gstate = PyGILState_Ensure();
        PyErr_SetString(PyExc_ValueError, "Length must be positive.");
        PyGILState_Release(gstate);

        return NULL;
    }

    int randf;
    int* array = malloc(length * sizeof(int));

    PyListObject* output = (PyListObject*) PyList_New((Py_ssize_t)amount);

    for (int i = 0; i < amount; i++) {
        memset(array, length+1, length * sizeof(int));
        PyListObject* sublist = (PyListObject*) PyList_New((Py_ssize_t)length);

        int pos = 0;

        while (1) {
            
            int insert = 1;
            
            if (!generate_rdrand64_below(&randf, below+1)) {
                for (int j = 0; j < length; j++) {
                    if (array[j] == (int) randf) {
                        insert = 0;
                        break;
                    }
                }

                if (pos == length) {
                    break;
                }

                if (insert) {
                    array[pos++] = (int) randf;
                }
            } else {
                perror("Failed to get random value.");
                exit(2);
            }
        }

        for (int j = 0; j < length; j++) {
            PyList_SetItem((PyObject*) sublist, (Py_ssize_t) j, PyLong_FromLong(array[j]));
        }

        PyList_SetItem((PyObject*) output, (Py_ssize_t) i, (PyObject*) sublist);
    }

    free(array);
    return output;
}

static PyListObject* range_below(PyObject* self, PyObject* args) {

    int length, below;

    if (!PyArg_ParseTuple(args, "ii", &length, &below)) {
        return NULL;
    }

    if (length > below) {
        PyGILState_STATE gstate = PyGILState_Ensure();
        PyErr_SetString(PyExc_ValueError, "Length must be smaller than 'below', because this method will return a non-repeated values.");
        PyGILState_Release(gstate);

        return NULL;
    }

    if (length < 0) {

        PyGILState_STATE gstate = PyGILState_Ensure();
        PyErr_SetString(PyExc_ValueError, "Length must be positive.");
        PyGILState_Release(gstate);

        return NULL;
    }

    int randf;
    PyListObject* output = (PyListObject*) PyList_New((Py_ssize_t)length);

    int* array = malloc(length * sizeof(int));
    memset(array, length+1, length * sizeof(int));

    int pos = 0;

    while (1) {
        
        int insert = 1;
        
        if (!generate_rdrand64_below(&randf, below+1)) {
            for (int i = 0; i < length; i++) {
                if (array[i] == (int) randf) {
                    insert = 0;
                    break;
                }
            }

            if (pos == length) {
                break;
            }

            if (insert) {
                array[pos++] = (int) randf;
            }
        } else {
            perror("Failed to get random value.");
            exit(2);
        }
    }

    for (int i = 0; i < length; i++) {
        PyList_SetItem((PyObject*) output, (Py_ssize_t) i, PyLong_FromLong(array[i]));
    }

    free(array);
    return output;
}

static PyListObject* range(PyObject* self, PyObject* args, PyObject* kwargs) {

    int length;
    int boundary = 90;

    char* keywords[] = {"length", "boundary", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|i", keywords, &length, &boundary)) {
        return NULL;
    }

    if (length > boundary) {

        char exception_text[256];
        sprintf(exception_text, "Lenght must be in the following range 0-%d.\n", abs(boundary));

        PyGILState_STATE gstate = PyGILState_Ensure();
        PyErr_SetString(PyExc_ValueError, exception_text);
        PyGILState_Release(gstate);

        return NULL;
    }

    if (length < 0) {
        PyGILState_STATE gstate = PyGILState_Ensure();
        PyErr_SetString(PyExc_ValueError, "length must be positive");
        PyGILState_Release(gstate);

        return NULL;
    }

    int randf;
    PyListObject* output = (PyListObject*) PyList_New((Py_ssize_t)length);
    
    int* array = (int*) malloc(boundary * sizeof(int));
    memset(array, 0, boundary * sizeof(int));
    
    int pos = 0;

    while (1) {
        
        int insert = 1;
        
        if (!generate_rdrand64_boundary(&randf, boundary)) {
            for (int i = 0; i < length; i++) {
                if (array[i] == (int) randf) {
                    insert = 0;
                    break;
                }
            }

            if (pos == length) {
                break;
            }

            if (insert) {
                array[pos] = (int) randf;
                pos++;
            }
        } else {
            perror("Failed to get random value.");
            exit(2);
        }
    }

    for (int i = 0; i < length; i++) {
        PyList_SetItem((PyObject*) output, (Py_ssize_t) i, PyLong_FromLong(array[i]));
    }

    free(array);
    return output;
}

static PyObject* is_rdrand_supported(PyObject* self) {
    if (rdrand_check_support() == 1)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* is_rdseed_supported(PyObject* self) {
    if (rdseed_check_support() == 1)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* rdseed(PyObject* self) {

    uint64_t retn = 0;
   // generate_rdseed(&retn);
    rdseed64_step(&retn);

    return (PyObject*)PyLong_FromUnsignedLongLong(retn);
}

static PyMethodDef methods[] = {
    {"range", (PyCFunction)range, METH_VARARGS | METH_KEYWORDS, range__doc__},
    {"randbits", (PyCFunction)randbits, METH_VARARGS, randbits__doc__},
    {"range_below", (PyCFunction)range_below, METH_VARARGS, range_below__doc__},
    {"n_range_below", (PyCFunction)n_range_below, METH_VARARGS, n_range_below__doc__},
    {"is_rdrand_supported", (PyCFunction)is_rdrand_supported, METH_NOARGS, is_rdrand_supported__doc__},
    {"is_rdseed_supported", (PyCFunction)is_rdseed_supported, METH_NOARGS, is_rdseed_supported__doc__},
    {"rdseed", (PyCFunction)rdseed, METH_NOARGS, rdseed__doc__},
    {"rdrand64", (PyCFunction)rdrand64, METH_NOARGS, rdrand64__doc__},
    {"rdrand32", (PyCFunction)rdrand32, METH_NOARGS, rdrand32__doc__},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef Module = {
    PyModuleDef_HEAD_INIT,
    "_rdrand",
    rdrand__doc__,
    -1,
    methods
};

PyMODINIT_FUNC PyInit__rdrand(void) {
    
    /* Check rdrand/rdseed compatibiltiy before initialize the module */
    if (rdrand_check_support() != 1 || rdseed_check_support() != 1) {

        PyGILState_STATE gstate = PyGILState_Ensure();
        PyErr_SetString(PyExc_ImportError, "This CPU does not support the rdrand/rdseed instruction.\n");
        PyGILState_Release(gstate);

        return NULL;
    }

    return PyModule_Create(&Module);
}