#include <stdlib.h>
#include <string.h>
#include <nvml.h>
#include <Rinternals.h>


#define STRLEN 64
static char str[STRLEN];

static inline void str_reset()
{
  for (int i=0; i<STRLEN; i++)
    str[i] = '\0';
}


#define newRptr(ptr,Rptr,fin) PROTECT(Rptr = R_MakeExternalPtr(ptr, R_NilValue, R_NilValue));R_RegisterCFinalizerEx(Rptr, fin, TRUE)
#define getRptr(ptr) R_ExternalPtrAddr(ptr);

static void device_finalize(SEXP ptr)
{
  nvmlDevice_t* device;
  if (NULL == R_ExternalPtrAddr(ptr))
    return;
  
  device = (nvmlDevice_t*) R_ExternalPtrAddr(ptr);
  free(device);
  R_ClearExternalPtr(ptr);
}



// ----------------------------------------------------------------------------
// nvml interface
// ----------------------------------------------------------------------------
#define CHECK_NVML(call) {nvmlReturn_t check = call;check_nvml_ret(check);}

static inline void check_nvml_ret(nvmlReturn_t check)
{
  if (check != NVML_SUCCESS)
  {
    if (check == NVML_ERROR_UNINITIALIZED)
      error("NVML was not successfully initialized");  
    else if (check == NVML_ERROR_INVALID_ARGUMENT)
      error("invalid argument");
    else if (check == NVML_ERROR_NOT_SUPPORTED)
      error("device does not support requested feature");
    else if (check == NVML_ERROR_NO_PERMISSION)
      error("NVML does not have permission to talk to the driver");
    // else if (check == NVML_ERROR_ALREADY_INITIALIZED) // deprecated
    //   error("already initialized")
    else if (check == NVML_ERROR_NOT_FOUND)
      error("process not found");
    else if (check == NVML_ERROR_INSUFFICIENT_SIZE)
      error("internal string buffer too small");
    else if (check == NVML_ERROR_INSUFFICIENT_POWER)
      error("device has improperly attached external power cable");
    else if (check == NVML_ERROR_DRIVER_NOT_LOADED)
      error("NVIDIA driver is not running");
    else if (check == NVML_ERROR_TIMEOUT)
      error("provided timeout has passed");
    else if (check == NVML_ERROR_IRQ_ISSUE)
      error("NVIDIA kernel detected an interrupt issue with the attached GPUs");
    else if (check == NVML_ERROR_LIBRARY_NOT_FOUND)
      error("NVML shared library could not be loaded");
    else if (check == NVML_ERROR_FUNCTION_NOT_FOUND)
      error("local NVML version does not support requested function");
    else if (check == NVML_ERROR_CORRUPTED_INFOROM)
      error("infoROM is corrupted");
    else if (check == NVML_ERROR_GPU_IS_LOST)
      error("GPU is inaccessible");
    else if (check == NVML_ERROR_RESET_REQUIRED)
      error("GPU needs to be reset before it can be used again");
    else if (check == NVML_ERROR_OPERATING_SYSTEM)
      error("GPU control device was blocked by the OS");
    else if (check == NVML_ERROR_LIB_RM_VERSION_MISMATCH)
      error("driver/library version mismatch");
    else if (check == NVML_ERROR_IN_USE)
      error("GPU currently in use");
    else if (check == NVML_ERROR_MEMORY)
      error("insufficient memory");
    else if (check == NVML_ERROR_MEMORY)
      error("no data");
    else if (check == NVML_ERROR_VGPU_ECC_NOT_SUPPORTED)
      error("operation is not available because ECC is enabled");
    else if (check == NVML_ERROR_UNKNOWN)
      error("unknown NVML error");
    else
      error(nvmlErrorString(check));
  }
}



// initialization and cleanup
static inline void nvml_init()
{
  CHECK_NVML( nvmlInit() );
}

static inline void nvml_init_with_flags(unsigned int flags)
{
  CHECK_NVML( nvmlInitWithFlags(flags) );
}

static inline void nvml_shutdown()
{
  CHECK_NVML( nvmlShutdown() );
}



// system queries
static inline int system_get_cuda_driver_version()
{
  int version;
  CHECK_NVML( nvmlSystemGetCudaDriverVersion(&version) );
  return version;
}

static inline void system_get_driver_version()
{
  CHECK_NVML( nvmlSystemGetDriverVersion(str, STRLEN) );
}

static inline void system_get_nvml_version()
{
  CHECK_NVML( nvmlSystemGetNVMLVersion(str, STRLEN) );
}

static inline void system_get_process_name(unsigned int pid)
{
  CHECK_NVML( nvmlSystemGetProcessName(pid, str, STRLEN) );
}



// device queries
static inline void device_get_board_part_number(nvmlDevice_t device)
{
  CHECK_NVML( nvmlDeviceGetBoardPartNumber(device, str, STRLEN) );
}

static inline void device_get_brand(nvmlDevice_t device)
{
  nvmlBrandType_t type;
  CHECK_NVML( nvmlDeviceGetBrand(device, &type) );
  if (type == NVML_BRAND_UNKNOWN)
    strcpy(str, "unknown");
  else if (type == NVML_BRAND_QUADRO)
    strcpy(str, "quadro");
  else if (type == NVML_BRAND_TESLA)
    strcpy(str, "tesla");
  else if (type == NVML_BRAND_NVS)
    strcpy(str, "nvs");
  else if (type == NVML_BRAND_GRID)
    strcpy(str, "grid");
  else if (type == NVML_BRAND_GEFORCE)
    strcpy(str, "geforce");
#ifdef NVML_BRAND_TITAN
  else if (type == NVML_BRAND_TITAN)
    strcpy(str, "titan");
#endif
  else
    strcpy(str, "missing from list; contact nvsmi devs");
}

static inline void device_get_compute_mode(nvmlDevice_t device)
{
  nvmlComputeMode_t mode;
  CHECK_NVML( nvmlDeviceGetComputeMode(device, &mode) );
  if (mode == NVML_COMPUTEMODE_DEFAULT)
    strcpy(str, "Default");
  else if (mode == NVML_COMPUTEMODE_EXCLUSIVE_THREAD)
    strcpy(str, "E. Thread");
  else if (mode == NVML_COMPUTEMODE_PROHIBITED)
    strcpy(str, "Prohibited");
  else if (mode == NVML_COMPUTEMODE_EXCLUSIVE_PROCESS)
    strcpy(str, "E. Process");
}

static inline int device_get_count()
{
  unsigned int num_gpus;
  CHECK_NVML( nvmlDeviceGetCount(&num_gpus) );
  return (int) num_gpus;
}

static inline void device_get_cuda_compute_capability(nvmlDevice_t device, int *major, int *minor)
{
  CHECK_NVML( nvmlDeviceGetCudaComputeCapability(device, major, minor) );
}

static inline int device_get_curr_pcie_link_generation(nvmlDevice_t device)
{
  unsigned int currLinkGen;
  CHECK_NVML( nvmlDeviceGetCurrPcieLinkGeneration(device, &currLinkGen) );
  return (int) currLinkGen;
}

static inline int device_get_curr_pcie_link_width(nvmlDevice_t device)
{
  unsigned int currLinkWidth;
  CHECK_NVML( nvmlDeviceGetCurrPcieLinkWidth(device, &currLinkWidth) );
  return (int) currLinkWidth;
}

static inline int device_get_display_active(nvmlDevice_t device)
{
  nvmlEnableState_t disp;
  CHECK_NVML( nvmlDeviceGetDisplayActive(device, &disp) );
  return (int) disp;
}

static inline int device_get_fan_speed(nvmlDevice_t device)
{
  unsigned int speed;
  nvmlReturn_t check = nvmlDeviceGetFanSpeed(device, &speed);
  if (check == NVML_ERROR_NOT_SUPPORTED)
    return INT_MIN;
  else
    CHECK_NVML(check);
  
  return (int) speed;
}

static inline nvmlDevice_t device_get_handle_by_index(int index)
{
  nvmlDevice_t device;
  CHECK_NVML( nvmlDeviceGetHandleByIndex(index, &device) );
  return device;
}

static inline int device_get_index(nvmlDevice_t device)
{
  unsigned int index;
  CHECK_NVML( nvmlDeviceGetIndex(device, &index) );
  return (int) index;
}

static inline void device_get_memory_info(nvmlDevice_t device, double *memory_used, double *memory_total)
{
  nvmlMemory_t memory;
  CHECK_NVML( nvmlDeviceGetMemoryInfo(device, &memory) );
  *memory_used = (double) memory.used;
  *memory_total = (double) memory.total;
}

static inline void device_get_name(nvmlDevice_t device)
{
  CHECK_NVML( nvmlDeviceGetName(device, str, STRLEN) );
}

static inline int device_get_performance_state(nvmlDevice_t device)
{
  nvmlPstates_t pState;
  CHECK_NVML( nvmlDeviceGetPerformanceState(device, &pState) );
  return (int) pState;
}

static inline int device_get_persistence_mode(nvmlDevice_t device)
{
  nvmlEnableState_t mode;
  CHECK_NVML( nvmlDeviceGetPersistenceMode(device, &mode) );
  return (int) mode;
}

static inline int device_get_power_max(nvmlDevice_t device)
{
  unsigned int power_min, power_max;
  CHECK_NVML( nvmlDeviceGetPowerManagementLimitConstraints(device, &power_min, &power_max) );
  return (int) power_max;
}

static inline int device_get_power_usage(nvmlDevice_t device)
{
  unsigned int power;
  CHECK_NVML( nvmlDeviceGetPowerUsage(device, &power) );
  return (int) power;
}

static inline void device_get_serial(nvmlDevice_t device)
{
  CHECK_NVML( nvmlDeviceGetSerial(device, str, STRLEN) );
}

static inline int device_get_temperature(nvmlDevice_t device)
{
  nvmlTemperatureSensors_t sensor = NVML_TEMPERATURE_GPU;
  unsigned int temp;
  CHECK_NVML( nvmlDeviceGetTemperature(device, sensor, &temp) );
  return (int) temp;
}

static inline int device_get_utilization(nvmlDevice_t device)
{
  nvmlUtilization_t utilization;
  CHECK_NVML( nvmlDeviceGetUtilizationRates(device, &utilization) );
  return (int) utilization.gpu;
}

static inline void device_get_uuid(nvmlDevice_t device)
{
  CHECK_NVML( nvmlDeviceGetUUID(device, str, STRLEN) );
}



// ----------------------------------------------------------------------------
// R interface
// ----------------------------------------------------------------------------

// initialization and cleanup
SEXP R_nvsmi_init()
{
  nvml_init();
  return R_NilValue;
}

SEXP R_nvsmi_shutdown()
{
  nvml_shutdown();
  return R_NilValue;
}



// system queries
SEXP R_system_get_cuda_driver_version()
{
  SEXP ret;
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = system_get_cuda_driver_version();
  
  UNPROTECT(1);
  return ret;
}

SEXP R_system_get_driver_version()
{
  SEXP ret;
  
  str_reset();
  
  system_get_driver_version();
  PROTECT(ret = allocVector(STRSXP, 1));
  SET_STRING_ELT(ret, 0, mkChar(str));
  
  str_reset();
  
  UNPROTECT(1);
  return ret;
}

SEXP R_system_get_nvml_version()
{
  SEXP ret;
  
  str_reset();
  
  system_get_nvml_version();
  PROTECT(ret = allocVector(STRSXP, 1));
  SET_STRING_ELT(ret, 0, mkChar(str));
  
  str_reset();
  
  UNPROTECT(1);
  return ret;
}

SEXP R_system_get_process_name(SEXP pid)
{
  SEXP ret;
  
  str_reset();
  
  system_get_process_name(INTEGER(pid)[0]);
  PROTECT(ret = allocVector(STRSXP, 1));
  SET_STRING_ELT(ret, 0, mkChar(str));
  
  str_reset();
  
  UNPROTECT(1);
  return ret;
}



// device queries
SEXP R_device_get_board_part_number(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  str_reset();
  
  device_get_board_part_number(*device);
  PROTECT(ret = allocVector(STRSXP, 1));
  SET_STRING_ELT(ret, 0, mkChar(str));
  
  str_reset();
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_brand(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  str_reset();
  
  device_get_brand(*device);
  PROTECT(ret = allocVector(STRSXP, 1));
  SET_STRING_ELT(ret, 0, mkChar(str));
  
  str_reset();
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_compute_mode(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  str_reset();
  
  device_get_compute_mode(*device);
  PROTECT(ret = allocVector(STRSXP, 1));
  SET_STRING_ELT(ret, 0, mkChar(str));
  
  str_reset();
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_compute_running_processes(SEXP device_ptr)
{
  SEXP ret, ret_names;
  SEXP ret_pid, ret_memory_used;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  unsigned int size = 0;
  nvmlProcessInfo_t *infos = NULL;
  nvmlDeviceGetComputeRunningProcesses(*device, &size, infos); // NOTE: do not check error return; it will be NVML_ERROR_INSUFFICIENT_SIZE
  infos = malloc(size*sizeof(*infos));
  CHECK_NVML( nvmlDeviceGetComputeRunningProcesses(*device, &size, infos) );
  
  PROTECT(ret_pid = allocVector(INTSXP, size));
  PROTECT(ret_memory_used = allocVector(REALSXP, size));
  
  for (unsigned int i=0; i<size; i++)
  {
    INTEGER(ret_pid)[i] = (int) infos[i].pid;
    REAL(ret_memory_used)[i] = (double) infos[i].usedGpuMemory;
  }
  
  if (infos != NULL)
    free(infos);
  
  PROTECT(ret = allocVector(VECSXP, 2));
  PROTECT(ret_names = allocVector(STRSXP, 2));
  setAttrib(ret, R_NamesSymbol, ret_names);
  
  SET_VECTOR_ELT(ret, 0, ret_pid);
  SET_STRING_ELT(ret_names, 0, mkChar("pid"));
  SET_VECTOR_ELT(ret, 1, ret_memory_used);
  SET_STRING_ELT(ret_names, 1, mkChar("memory_used"));
  
  UNPROTECT(4);
  return ret;
}

SEXP R_device_get_count()
{
  SEXP ret;
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = device_get_count();
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_cuda_compute_capability(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(INTSXP, 2));
  device_get_cuda_compute_capability(*device, INTEGER(ret), INTEGER(ret)+1);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_curr_pcie_link_generation(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = device_get_curr_pcie_link_generation(*device);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_curr_pcie_link_width(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = device_get_curr_pcie_link_width(*device);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_display_active(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(LGLSXP, 1));
  LOGICAL(ret)[0] = device_get_display_active(*device);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_fan_speed(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = device_get_fan_speed(*device);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_graphics_running_processes(SEXP device_ptr)
{
  SEXP ret, ret_names;
  SEXP ret_pid, ret_memory_used;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  unsigned int size = 0;
  nvmlProcessInfo_t *infos = NULL;
  nvmlDeviceGetGraphicsRunningProcesses(*device, &size, infos); // NOTE: do not check error return; it will be NVML_ERROR_INSUFFICIENT_SIZE
  infos = malloc(size*sizeof(*infos));
  CHECK_NVML( nvmlDeviceGetGraphicsRunningProcesses(*device, &size, infos) );
  
  PROTECT(ret_pid = allocVector(INTSXP, size));
  PROTECT(ret_memory_used = allocVector(REALSXP, size));
  
  for (unsigned int i=0; i<size; i++)
  {
    INTEGER(ret_pid)[i] = (int) infos[i].pid;
    REAL(ret_memory_used)[i] = (double) infos[i].usedGpuMemory;
  }
  
  if (infos != NULL)
    free(infos);
  
  PROTECT(ret = allocVector(VECSXP, 2));
  PROTECT(ret_names = allocVector(STRSXP, 2));
  setAttrib(ret, R_NamesSymbol, ret_names);
  
  SET_VECTOR_ELT(ret, 0, ret_pid);
  SET_STRING_ELT(ret_names, 0, mkChar("pid"));
  SET_VECTOR_ELT(ret, 1, ret_memory_used);
  SET_STRING_ELT(ret_names, 1, mkChar("memory_used"));
  
  UNPROTECT(4);
  return ret;
}

SEXP R_device_get_handle_by_index(SEXP index)
{
  SEXP ret;
  nvmlDevice_t *device = malloc(sizeof(*device));
  
  *device = device_get_handle_by_index(INTEGER(index)[0]);
  newRptr(device, ret, device_finalize);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_index(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = device_get_index(*device);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_memory_info(SEXP device_ptr)
{
  SEXP ret, ret_names;
  SEXP ret_used, ret_total;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret_used = allocVector(REALSXP, 1));
  PROTECT(ret_total = allocVector(REALSXP, 1));
  
  PROTECT(ret = allocVector(VECSXP, 2));
  PROTECT(ret_names = allocVector(STRSXP, 2));
  setAttrib(ret, R_NamesSymbol, ret_names);
  
  SET_VECTOR_ELT(ret, 0, ret_used);
  SET_STRING_ELT(ret_names, 0, mkChar("used"));
  SET_VECTOR_ELT(ret, 1, ret_total);
  SET_STRING_ELT(ret_names, 1, mkChar("total"));
  
  device_get_memory_info(*device, REAL(ret_used), REAL(ret_total));
  
  UNPROTECT(4);
  return ret;
}

SEXP R_device_get_name(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  str_reset();
  
  device_get_name(*device);
  PROTECT(ret = allocVector(STRSXP, 1));
  SET_STRING_ELT(ret, 0, mkChar(str));
  
  str_reset();
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_performance_state(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = device_get_performance_state(*device);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_persistence_mode(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = device_get_persistence_mode(*device);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_power_max(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = device_get_power_max(*device);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_power_usage(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = device_get_power_usage(*device);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_serial(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  str_reset();
  
  device_get_serial(*device);
  PROTECT(ret = allocVector(STRSXP, 1));
  SET_STRING_ELT(ret, 0, mkChar(str));
  
  str_reset();
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_temperature(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = device_get_temperature(*device);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_utilization(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  PROTECT(ret = allocVector(INTSXP, 1));
  INTEGER(ret)[0] = device_get_utilization(*device);
  
  UNPROTECT(1);
  return ret;
}

SEXP R_device_get_uuid(SEXP device_ptr)
{
  SEXP ret;
  nvmlDevice_t *device = (nvmlDevice_t*) getRptr(device_ptr);
  
  str_reset();
  
  device_get_uuid(*device);
  PROTECT(ret = allocVector(STRSXP, 1));
  SET_STRING_ELT(ret, 0, mkChar(str));
  
  str_reset();
  
  UNPROTECT(1);
  return ret;
}



// smi
SEXP R_smi()
{
  SEXP ret, ret_names;
  SEXP ret_df, ret_df_names, ret_df_rownames;
  SEXP ret_version;
  SEXP ret_name, ret_busid, ret_persistence_mode, ret_disp;
  SEXP ret_speed, ret_temp, ret_perf, ret_power, ret_power_max, ret_memory_used,
    ret_memory_total, ret_utilization, ret_compute_mode;
  
  str_reset();
  nvml_init();
  
  system_get_driver_version();
  PROTECT(ret_version = allocVector(STRSXP, 1));
  SET_STRING_ELT(ret_version, 0, mkChar(str));
  
  unsigned int num_gpus = device_get_count();
  PROTECT(ret_name = allocVector(STRSXP, num_gpus));
  PROTECT(ret_busid = allocVector(STRSXP, num_gpus));
  PROTECT(ret_persistence_mode = allocVector(LGLSXP, num_gpus));
  PROTECT(ret_disp = allocVector(LGLSXP, num_gpus));
  PROTECT(ret_speed = allocVector(INTSXP, num_gpus));
  PROTECT(ret_temp = allocVector(INTSXP, num_gpus));
  PROTECT(ret_perf = allocVector(INTSXP, num_gpus));
  PROTECT(ret_power = allocVector(INTSXP, num_gpus));
  PROTECT(ret_power_max = allocVector(INTSXP, num_gpus));
  PROTECT(ret_memory_used = allocVector(REALSXP, num_gpus));
  PROTECT(ret_memory_total = allocVector(REALSXP, num_gpus));
  PROTECT(ret_utilization = allocVector(INTSXP, num_gpus));
  PROTECT(ret_compute_mode = allocVector(STRSXP, num_gpus));
  
  for (unsigned int i=0; i<num_gpus; i++)
  {
    nvmlDevice_t device = device_get_handle_by_index(i);
    
    device_get_name(device);
    SET_STRING_ELT(ret_name, i, mkChar(str));
    
    nvmlPciInfo_t pci;
    CHECK_NVML( nvmlDeviceGetPciInfo (device, &pci) );
    SET_STRING_ELT(ret_busid, i, mkChar(pci.busId));
    
    LOGICAL(ret_persistence_mode)[i] = device_get_persistence_mode(device);
    
    LOGICAL(ret_disp)[i] = device_get_display_active(device);
    
    INTEGER(ret_speed)[i] = device_get_fan_speed(device);
    
    INTEGER(ret_temp)[i] = device_get_temperature(device);
    
    INTEGER(ret_perf)[i] = device_get_performance_state(device);
    
    INTEGER(ret_power)[i] = device_get_power_usage(device);
    
    INTEGER(ret_power_max)[i] = device_get_power_max(device);
    
    device_get_memory_info(device, REAL(ret_memory_used)+i, REAL(ret_memory_total)+i);
    
    INTEGER(ret_utilization)[i] = device_get_utilization(device);
    
    device_get_compute_mode(device);
    SET_STRING_ELT(ret_compute_mode, i, mkChar(str));
  }
  
  nvml_shutdown();
  str_reset();
  
  
  PROTECT(ret = allocVector(VECSXP, 3));
  PROTECT(ret_names = allocVector(STRSXP, 3));
  setAttrib(ret, R_NamesSymbol, ret_names);
  
  PROTECT(ret_df = allocVector(VECSXP, 13));
  PROTECT(ret_df_names = allocVector(STRSXP, 13));
  PROTECT(ret_df_rownames = allocVector(INTSXP, num_gpus));
  for (unsigned int i=0; i<num_gpus; i++)
    INTEGER(ret_df_rownames)[i] = i+1;
  
  setAttrib(ret_df, R_NamesSymbol, ret_df_names);
  setAttrib(ret_df, R_RowNamesSymbol, ret_df_rownames);
  setAttrib(ret_df, R_ClassSymbol, mkString("data.frame"));
  
  SET_VECTOR_ELT(ret, 0, ret_version);
  SET_STRING_ELT(ret_names, 0, mkChar("version"));
  
  SET_VECTOR_ELT(ret, 1, R_NilValue);
  SET_STRING_ELT(ret_names, 1, mkChar("date"));
  
  SET_VECTOR_ELT(ret, 2, ret_df);
  SET_STRING_ELT(ret_names, 2, mkChar("gpus"));
  
  int n = 0;
  SET_VECTOR_ELT(ret_df, n, ret_name);
  SET_STRING_ELT(ret_df_names, n++, mkChar("name"));
  SET_VECTOR_ELT(ret_df, n, ret_busid);
  SET_STRING_ELT(ret_df_names, n++, mkChar("busid"));
  SET_VECTOR_ELT(ret_df, n, ret_persistence_mode);
  SET_STRING_ELT(ret_df_names, n++, mkChar("persistence_mode"));
  SET_VECTOR_ELT(ret_df, n, ret_disp);
  SET_STRING_ELT(ret_df_names, n++, mkChar("disp"));
  SET_VECTOR_ELT(ret_df, n, ret_speed);
  SET_STRING_ELT(ret_df_names, n++, mkChar("speed"));
  SET_VECTOR_ELT(ret_df, n, ret_temp);
  SET_STRING_ELT(ret_df_names, n++, mkChar("temp"));
  SET_VECTOR_ELT(ret_df, n, ret_perf);
  SET_STRING_ELT(ret_df_names, n++, mkChar("perf"));
  SET_VECTOR_ELT(ret_df, n, ret_power);
  SET_STRING_ELT(ret_df_names, n++, mkChar("power"));
  SET_VECTOR_ELT(ret_df, n, ret_power_max);
  SET_STRING_ELT(ret_df_names, n++, mkChar("power_max"));
  SET_VECTOR_ELT(ret_df, n, ret_memory_used);
  SET_STRING_ELT(ret_df_names, n++, mkChar("memory_used"));
  SET_VECTOR_ELT(ret_df, n, ret_memory_total);
  SET_STRING_ELT(ret_df_names, n++, mkChar("memory_total"));
  SET_VECTOR_ELT(ret_df, n, ret_utilization);
  SET_STRING_ELT(ret_df_names, n++, mkChar("utilization"));
  SET_VECTOR_ELT(ret_df, n, ret_compute_mode);
  SET_STRING_ELT(ret_df_names, n++, mkChar("compute_mode"));
  
  UNPROTECT(19);
  return ret;
}
