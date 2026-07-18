#include <string.h>
#include "mmp_sensor.h"
#include "adv7180.h"
#include "adv7280a.h"
#include "adv7181c.h"
#include "TVP5150AM1.h"
#include "userdefine.h"
#include "NT99141.h"
#include "RN6854M.h"
#include "RN6752.h"
#include "TP9950.h"
#include "TP2825B.h"
#include "TP2860.h"
#include "PR2000K.h"
#include "MAX9276.h"
#include "DS90UB926Q.h"

pthread_mutex_t internal_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Creates and returns a sensor driver instance based on the specified module name.
 * 
 * @param moduleName A pointer to a null-terminated string representing the name of the sensor module.
 *                   This name must match one of the predefined sensor driver names.
 * 
 * @return SensorDriver A handle to the sensor driver instance corresponding to the specified module name.
 *                      Returns NULL if the module name does not match any predefined sensor driver.
 */

SensorDriver ithSensorCreate(uint8_t *moduleName)
{

#ifdef CFG_SENSOR_ENABLE
    if (strcmp(moduleName, "NT99141.c") == 0)
        return (SensorDriver)NT99141SensorDriver_Create();

    if (strcmp(moduleName, "RN6854M.c") == 0)
        return (SensorDriver)RN6854SensorDriver_Create();
    
    if (strcmp(moduleName, "RN6752.c") == 0)
        return (SensorDriver)RN6752SensorDriver_Create();

    if (strcmp(moduleName, "adv7180.c") == 0)
        return (SensorDriver)ADV7180SensorDriver_Create();

    if (strcmp(moduleName, "adv7280a.c") == 0)
        return (SensorDriver)ADV7280SensorDriver_Create();

    if (strcmp(moduleName, "adv7181c.c") == 0)
        return (SensorDriver)ADV7181SensorDriver_Create();
    
    if (strcmp(moduleName, "TVP5150AM1.c") == 0)
        return (SensorDriver)TVP5150SensorDriver_Create();
    
    if (strcmp(moduleName, "TP9950.c") == 0)
        return (SensorDriver)TP9950SensorDriver_Create();

    if (strcmp(moduleName, "TP2825B.c") == 0)
        return (SensorDriver)TP2825BSensorDriver_Create();
    
    if (strcmp(moduleName, "TP2860.c") == 0)
        return (SensorDriver)TP2860SensorDriver_Create();

    if (strcmp(moduleName, "PR2000K.c") == 0)
        return (SensorDriver)PR2000SensorDriver_Create();
    
    if (strcmp(moduleName, "MAX9276.c") == 0) //Serdes
        return (SensorDriver)Max9276SensorDriver_Create();

    if (strcmp(moduleName, "DS90UB926Q.c") == 0) //Serdes
        return (SensorDriver)DS90UB926QSersorDriver_Create();
    
    if (strcmp(moduleName, "userdefine.c") == 0)
        return (SensorDriver)UserSensorDriver_Create();
#endif
    return NULL;
}

void ithSensorInit(SensorDriver self, uint16_t Mode)
{
    pthread_mutex_lock(&internal_mutex);
    if (self)
        self->vtable->Init(Mode);
    pthread_mutex_unlock(&internal_mutex);
}

/**
 * @brief Destroys the sensor driver instance and releases all associated resources.
 *
 * This function acquires a mutex to ensure thread safety, verifies that the provided
 * sensor driver instance is valid, and invokes the `Destroy` method from the driver's
 * virtual table to perform cleanup. The mutex is released after the operation completes.
 *
 * @param self A pointer to the SensorDriver instance to be destroyed. If NULL,
 *             the function performs no action.
 */

void ithSensorDestroy(SensorDriver self)
{
    pthread_mutex_lock(&internal_mutex);
    if (self)
        self->vtable->Destroy(self);
    pthread_mutex_unlock(&internal_mutex);
}

void ithSensorDeInit(SensorDriver self)
{
    pthread_mutex_lock(&internal_mutex);
    if (self)
        self->vtable->Terminate();
    pthread_mutex_unlock(&internal_mutex);
}

/**
 * @brief Checks whether the sensor signal is stable for the specified mode.
 *
 * This function checks the stability of the sensor signal by calling the
 * `IsSignalStable` method from the sensor driver's virtual table. Thread safety
 * is ensured by locking a mutex before the operation and unlocking it afterward.
 *
 * @param self A pointer to the SensorDriver instance.
 * @param Mode The sensor mode for which signal stability is being checked.
 * @return Returns 1 if the signal is stable; otherwise, returns 0.
 */

uint8_t ithSensorIsSignalStable(SensorDriver self, uint16_t Mode)
{
    uint8_t Isstable = 0;
    pthread_mutex_lock(&internal_mutex);

    if (self)
    {
        Isstable = self->vtable->IsSignalStable(Mode);
    }

    pthread_mutex_unlock(&internal_mutex);

    return Isstable;
}

/**
 * @brief Retrieves the specified property value from the sensor driver.
 *
 * This function acquires a mutex to ensure thread safety, invokes the corresponding
 * `GetProperty` method from the driver's virtual table to retrieve the property value,
 * and then releases the mutex before returning the result.
 *
 * @param self A pointer to the SensorDriver instance. Must not be NULL.
 * @param Property The property to retrieve, specified as a value from the MODULE_GETPROPERTY enum.
 * @return The retrieved property value as a 16-bit unsigned integer.
 *
 * @note Make sure the sensor driver instance (`self`) is properly initialized before calling this function.
 */

uint16_t ithSensorGetProperty(SensorDriver self, MODULE_GETPROPERTY Property)
{
    uint16_t value = 0;
    pthread_mutex_lock(&internal_mutex);

    if (self)
    {
        value = self->vtable->GetProperty(Property);
    }

    pthread_mutex_unlock(&internal_mutex);

    return value;       
}

uint8_t ithSensorGetStatus(SensorDriver self, MODULE_GETSTATUS Status)
{
    uint8_t status = 0;
    pthread_mutex_lock(&internal_mutex);

    if (self)
    {
        status = self->vtable->GetStatus(Status);
    }

    pthread_mutex_unlock(&internal_mutex); 
    
    return status;
}

void ithSensorSetProperty(SensorDriver self, MODULE_SETPROPERTY Property, uint16_t Value)
{
    pthread_mutex_lock(&internal_mutex);
    if (self)
        self->vtable->SetProperty(Property, Value);
    pthread_mutex_unlock(&internal_mutex);
}

void ithSensorPowerDown(SensorDriver self, uint8_t Enable)
{
    pthread_mutex_lock(&internal_mutex);
    if (self)
        self->vtable->PowerDown(Enable);
    pthread_mutex_unlock(&internal_mutex);
}

void ithSensorOutputPinTriState(SensorDriver self, uint8_t Flag)
{
    pthread_mutex_lock(&internal_mutex);
    if (self)
        self->vtable->OutputPinTriState(Flag);
    pthread_mutex_unlock(&internal_mutex);
}